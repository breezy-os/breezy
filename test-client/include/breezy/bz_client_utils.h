#ifndef BZ_CLIENT_UTILS_H
#define BZ_CLIENT_UTILS_H
// #################################################################################################

#include <stddef.h>
#include <stdint.h>
#include <bits/types.h>

#include <wayland-client-protocol.h>

int bz_allocate_shm_file(__off64_t size);

struct bz_buff_alloc {
	size_t buffer_size;
	size_t pool_size;
	uint8_t *pool_data;           // nullptr prior to mmap
	struct wl_shm_pool *shm_pool; // nullptr prior to wl_shm_create_pool
	struct bz_buffer *buffers;    // Array of buffers
};
struct bz_buff_alloc *bz_allocate_shm_buffers(int32_t width, int32_t height, uint32_t num_buffers, struct wl_shm *shm_global, const struct wl_buffer_listener *buffer_listener);

uint32_t bz_random_color(void);

// #################################################################################################
#endif