// This gives us access to signal handling functionality
#define _POSIX_C_SOURCE 200809L // NOLINT

#include <signal.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <time.h>

#include <wayland-client.h>
#include <xdg-shell-client-protocol.h>

#include "breezy/bz_logger.h"
#include "breezy/bz_client_globals.h"
#include "breezy/bz_wl_protocol.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

static void bz_sleep_ms(uint32_t ms);


// =================================================================================================
//  Utilities
// -------------------------------------------------------------------------------------------------

static void bz_sleep_ms(uint32_t ms)
{
	struct timespec ts = {
		.tv_sec = ms / 1000,
		.tv_nsec = (ms % 1000) * 1000000L,
	};
	nanosleep(&ts, nullptr);
}


// =================================================================================================
//  Application Setup
// -------------------------------------------------------------------------------------------------

void bz_run_event_loop(const struct bz_client_globals *globals)
{
	const int wayland_fd = wl_display_get_fd(globals->display);

	// Run our event loop
	while (true) {
		// "wl_display_prepare_read" must be paired with either "wl_display_read_events" or, if
		//   unable to read events for some reason, "wl_display_cancel_read".
		while (wl_display_prepare_read(globals->display) != 0) {
			wl_display_dispatch_pending(globals->display);
		}

		// Flush pending requests to the server
		if (wl_display_flush(globals->display) < 0) {
			// If errno is EAGAIN, then we'd ideally be polling the display fd to wait for it to
			//   become writable before trying again ... but this client doesn't really matter, so
			//   let's just exit to keep things simple.
			wl_display_cancel_read(globals->display);
			break;
		}

		// Check for changes to our FDs
		struct pollfd fds[2] = {
			{ .fd = globals->is_quitting, .events = POLLIN },
			{ .fd = wayland_fd,           .events = POLLIN },
		};
		const int ret = poll(fds, 2, 1000);
		if (ret == 0) bz_warn(BZ_LOG_MAIN, __FILE__, __LINE__, "Timeout waiting for FD.");
		if (ret < 0) {
			wl_display_cancel_read(globals->display);
			break; // Failure
		}

		if (fds[0].revents & POLLIN) {
			wl_display_cancel_read(globals->display);
			break; // is_quitting got toggled
		}

		if (fds[1].revents & POLLIN) {
			wl_display_read_events(globals->display);
		} else {
			wl_display_cancel_read(globals->display);
		}

		wl_display_dispatch_pending(globals->display);
	}
}

struct bz_application_window *bz_create_app_window(struct bz_client_globals *globals)
{
	bz_info(BZ_LOG_MAIN, __FILE__, __LINE__, "Creating application window.");
	struct bz_application_window *window = calloc(1, sizeof(*window));
	window->wlsurface = wl_compositor_create_surface(globals->compositor);
	window->xdgsurface = bz_xdg_surface_constructor(globals, window->wlsurface);
	window->xdgtoplevel = bz_xdg_toplevel_constructor(globals, window->xdgsurface);
	window->pending = calloc(1, sizeof(*window->pending));
	window->finalized = calloc(1, sizeof(*window->finalized));

	wl_surface_commit(window->wlsurface);

	return window;
}
