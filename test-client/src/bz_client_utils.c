
#define _POSIX_C_SOURCE 200112L

#include "breezy/bz_client_utils.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "breezy/bz_logger.h"
#include "breezy/bz_client_globals.h"


// =================================================================================================
//  SHM file allocation
// -------------------------------------------------------------------------------------------------

// Taken (with much appreciation!) from: https://wayland-book.com/surfaces/shared-memory.html

static void randname(char *buf)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	long r = ts.tv_nsec;
	for (int i = 0; i < 6; ++i) {
		buf[i] = 'A'+(r&15)+(r&16)*2;
		r >>= 5;
	}
}

static int create_shm_file(void)
{
	int retries = 100;
	do {
		char name[] = "/wl_shm-XXXXXX";
		randname(name + sizeof(name) - 7);
		--retries;
		int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
		if (fd >= 0) {
			shm_unlink(name);
			return fd;
		}
	} while (retries > 0 && errno == EEXIST);
	return -1;
}

int bz_allocate_shm_file(const __off64_t size)
{
	const int fd = create_shm_file();
	if (fd < 0)
		return -1;
	int ret;
	do {
		ret = ftruncate(fd, size);
	} while (ret < 0 && errno == EINTR);
	if (ret < 0) {
		close(fd);
		return -1;
	}
	return fd;
}


// =================================================================================================
//  Buffer Allocation
// -------------------------------------------------------------------------------------------------

/**
 * Callers are expected to free the returned bz_buff_data when they are finished copying data
 * out of it.
 */
struct bz_buff_alloc *bz_allocate_shm_buffers(
	int32_t width,
	int32_t height,
	uint32_t num_buffers,
	struct wl_shm *shm_global,
	const struct wl_buffer_listener *buffer_listener
) {
	struct bz_buff_alloc *allocation = calloc(1, sizeof(*allocation));
	allocation->buffers = calloc(num_buffers, sizeof(*allocation->buffers));

	// Create a pool for our buffers
	allocation->buffer_size = width * height * 4; // 4 bytes per px (XRGB8888)
	allocation->pool_size = allocation->buffer_size * num_buffers; // Two buffers per pool (double-buffered)
	int fd = bz_allocate_shm_file(allocation->pool_size);
	if (fd == -1) {
		bz_error(BZ_LOG_WAYLAND, "Failed to allocate shared memory.");
		goto shm_alloc_failure;
	}

	// Map the pool's file descriptor to memory
	allocation->pool_data = mmap(NULL, allocation->pool_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (allocation->pool_data == MAP_FAILED) {
		bz_error(BZ_LOG_WAYLAND, "Failed to mmap pool data.");
		goto pool_mmap_failure;
	}

	// Send the pool to the server
	allocation->shm_pool = wl_shm_create_pool(shm_global, fd, allocation->pool_size);
	close(fd);

	// Create our two buffers
	for (uint8_t i = 0; i < num_buffers; i++) {
		size_t offset = i * allocation->buffer_size;
		allocation->buffers[i].is_released = true;
		allocation->buffers[i].size = (struct bz_dimension){ .w = width, .h = height };
		allocation->buffers[i].pixel_data = (uint32_t *)(&allocation->pool_data[offset]);
		allocation->buffers[i].buffer = wl_shm_pool_create_buffer(
			allocation->shm_pool,
			offset,
			width,
			height,
			width * 4, // Stride
			WL_SHM_FORMAT_XRGB8888
		);
		wl_buffer_add_listener(
			allocation->buffers[i].buffer,
			buffer_listener,
			&allocation->buffers[i]
		);
	}

	return allocation;

	pool_mmap_failure:
		close(fd);
	shm_alloc_failure:
		free(allocation->buffers);
		free(allocation);
		return nullptr;
}

// =================================================================================================
//  Color Utilities
// -------------------------------------------------------------------------------------------------

uint32_t bz_random_color(void)
{
	return ((uint32_t)(rand() & 0xFF) << 16) |  // R
		   ((uint32_t)(rand() & 0xFF) << 8)  |  // G
		   ((uint32_t)(rand() & 0xFF));         // B
}

