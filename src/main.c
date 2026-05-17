
#include "breezy/bz_breezy.h"

#include <sys/poll.h>

#include "breezy/bz_graphics.h"
#include "breezy/bz_input.h"
#include "breezy/bz_list.h"
#include "breezy/bz_logger.h"
#include "breezy/bz_seat.h"

static int bz_loop_iteration(struct bz_breezy *breezy)
{
	bz_debug(BZ_LOG_MAIN, __FILE__, __LINE__, "Executing event loop iteration.");

	// Check for changes to our FDs
	struct pollfd fds[3] = {
		{ .fd = breezy->seat.fd,  .events = POLLIN },
		{ .fd = breezy->drm.fd,   .events = POLLIN },
		{ .fd = breezy->input.fd, .events = POLLIN },
	};
	const int ret = poll(fds, 3, 1000); // 0 indicates a timeout, -1 indicates failure
	if (ret < 0) return -1;
	if (ret == 0) bz_info(BZ_LOG_MAIN, __FILE__, __LINE__, "Timed out polling FDs.");

	// Handle any potential updates from our FDs
	if (fds[0].revents & POLLIN) { bz_seat_handle_libseat_event(breezy); }
	if (fds[1].revents & POLLIN) { bz_graphics_handle_drm_event(breezy); }
	if (fds[2].revents & POLLIN) { bz_input_process_events(breezy); }

	// Do other processing as appropriate
	if (breezy->seat.active) {
		bz_graphics_loop_iteration(breezy);
	}

	return 0;
}

bool is_shutting_down = false;
void bz_shutdown()
{
	bz_info(BZ_LOG_MAIN, __FILE__, __LINE__, "Shutting down...");
	is_shutting_down = true;
}

int main(void)
{
	int retval = 0;

	// Set up our logger
	bz_log_initialize(BZ_LOG_WARN);
	bz_log_set_level(BZ_LOG_MAIN, BZ_LOG_INFO);
	bz_log_set_level(BZ_LOG_GRAPHICS, BZ_LOG_INFO);
	bz_log_set_level(BZ_LOG_INPUT, BZ_LOG_DEBUG);

	// Initialize our main "breezy" struct, explicitly setting non-zero/nullptr values as needed.
	struct bz_breezy breezy = { 0 };
	breezy.drm.fd = -1;
	breezy.drm.device_id = -1;
	breezy.gl.is_dirty = true;
	breezy.seat.fd = -1;
	breezy.input.device_lookup = bz_list_create();
	if (breezy.input.device_lookup == nullptr) {
		bz_error(BZ_LOG_MAIN, __FILE__, __LINE__, "Failed to initialize device lookup list.");
		return -1; // If we're already failing to malloc this early, let's just exit.
	}

	// Seat initialization
	retval = bz_seat_initialize(&breezy);
	if (retval != 0) {
		bz_error(BZ_LOG_SEAT, __FILE__, __LINE__,
			"Failed to initialize seat code. Code: %d", retval);
		goto seat_cleanup;
	}

	// Graphics initialization
	retval = bz_graphics_initialize(&breezy);
	if (retval != 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__,
			"Failed to initialize graphics code. Code: %d", retval);
		goto graphics_cleanup;
	}

	// Input initialization
	retval = bz_input_initialize(&breezy);
	if (retval != 0) {
		bz_error(BZ_LOG_INPUT, __FILE__, __LINE__,
			"Failed to initialize input code. Code: %d", retval);
		goto input_cleanup;
	}

	// Event loop!
	while (!is_shutting_down) {
		bz_loop_iteration(&breezy);
	}

	// Cleanup (backwards from initialization)
input_cleanup:
	bz_input_cleanup(&breezy);
graphics_cleanup:
	bz_graphics_cleanup(&breezy);
seat_cleanup:
	bz_seat_cleanup(&breezy);

	return retval;
}
