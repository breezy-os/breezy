#define _POSIX_C_SOURCE 200809L // NOLINT

#include "breezy/bz_breezy.h"

#include <signal.h>
#include <stdlib.h>
#include <time.h>

#include <wayland-server.h>

#include "breezy/bz_graphics.h"
#include "breezy/bz_input.h"
#include "breezy/bz_list.h"
#include "breezy/bz_logger.h"
#include "breezy/bz_seat.h"
#include "breezy/bz_wayland.h"


int main(void)
{
	int retval = 0;

	srand(time(NULL));

	// Set up our logger
	bz_log_initialize(BZ_LOG_INFO);
	bz_log_set_level(BZ_LOG_INPUT, BZ_LOG_DEBUG);
	bz_log_set_level(BZ_LOG_WINDOW_MGMT, BZ_LOG_DEBUG);
	bz_log_set_level(BZ_LOG_WL_DEVICES, BZ_LOG_DEBUG);
	// bz_log_set_level(BZ_LOG_WAYLAND, BZ_LOG_DEBUG);
	// bz_log_set_level(BZ_LOG_WL_DISPLAY, BZ_LOG_DEBUG);
	// bz_log_set_level(BZ_LOG_WL_XDG_SHELL, BZ_LOG_DEBUG);

	// Initialize our main "breezy" struct, explicitly setting non-zero/nullptr values as needed.
	struct bz_breezy breezy = { 0 };
	breezy.drm.fd = -1;
	breezy.drm.device_id = -1;
	breezy.seat.fd = -1;
	breezy.input.device_lookup = bz_list_create();
	breezy.wayland.event_sources = bz_list_create();
	breezy.wayland.clients = bz_list_create();
	if (breezy.input.device_lookup == nullptr) {
		bz_error(BZ_LOG_MAIN, __FILE__, __LINE__, "Failed to initialize device lookup list.");
		return -1; // If we're already failing to malloc this early, let's just exit.
	}

	// Window management initialization
	retval = bz_mgmt_initialize(&breezy.window_mgmt);
	if (retval != 0) {
		bz_error(BZ_LOG_MAIN, __FILE__, __LINE__,
			"Failed to initialize window management. Error: %d", retval);
		goto window_management_cleanup;
	}

	// Seat initialization
	retval = bz_seat_initialize(&breezy);
	if (retval != 0) {
		bz_error(BZ_LOG_MAIN, __FILE__, __LINE__,
			"Failed to initialize seat code. Error: %d", retval);
		goto seat_cleanup;
	}

	// Graphics initialization
	retval = bz_graphics_initialize(&breezy);
	if (retval != 0) {
		bz_error(BZ_LOG_MAIN, __FILE__, __LINE__,
			"Failed to initialize graphics code. Error: %d", retval);
		goto graphics_cleanup;
	}

	// Input initialization
	retval = bz_input_initialize(&breezy);
	if (retval != 0) {
		bz_error(BZ_LOG_MAIN, __FILE__, __LINE__,
			"Failed to initialize input code. Error: %d", retval);
		goto input_cleanup;
	}

	// Wayland initialization
	retval = bz_wayland_initialize(&breezy);
	if (retval != 0) {
		bz_error(BZ_LOG_MAIN, __FILE__, __LINE__,
			"Failed to initialize Wayland code. Error: %d", retval);
		goto wayland_cleanup;
	}

	// Configure our Wayland event loop
	struct wl_event_loop *evt_loop = wl_display_get_event_loop(breezy.wayland.display);
	struct wl_event_source *seat_source = wl_event_loop_add_fd(
		evt_loop,
		breezy.seat.fd,
		WL_EVENT_READABLE,
		bz_seat_handle_libseat_event,
		&breezy
	);
	bz_list_append(breezy.wayland.event_sources, seat_source);
	struct wl_event_source *drm_source = wl_event_loop_add_fd(
		evt_loop,
		breezy.drm.fd,
		WL_EVENT_READABLE,
		bz_graphics_handle_drm_event,
		&breezy
	);
	bz_list_append(breezy.wayland.event_sources, drm_source);
	struct wl_event_source *input_source = wl_event_loop_add_fd(
		evt_loop,
		breezy.input.fd,
		WL_EVENT_READABLE,
		bz_input_process_events,
		&breezy
	);
	bz_list_append(breezy.wayland.event_sources, input_source);

	// Event loop!
	bz_graphics_schedule_render(&breezy);
	wl_display_run(breezy.wayland.display);
	bz_info(BZ_LOG_MAIN, __FILE__, __LINE__, "Shutting down...");

	// Cleanup (backwards from initialization)
wayland_cleanup:
	bz_wayland_cleanup(&breezy);
input_cleanup:
	bz_input_cleanup(&breezy);
graphics_cleanup:
	bz_graphics_cleanup(&breezy);
seat_cleanup:
	bz_seat_cleanup(&breezy);
window_management_cleanup:
	bz_mgmt_cleanup(&breezy.window_mgmt);

	return retval;
}
