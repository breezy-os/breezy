// This gives us access to signal handling functionality
#define _POSIX_C_SOURCE 200809L // NOLINT

#include <signal.h>
#include <stdlib.h>
#include <sys/poll.h>

#include <wayland-client.h>
#include <xdg-shell-client-protocol.h>

#include "breezy/bz_logger.h"
#include "breezy/bz_client_globals.h"
#include "breezy/bz_client_utils.h"
#include "breezy/bz_wl_protocol.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------


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
	// Visual / application state
	window->radius = 50;
	window->bg_color = bz_random_color();
	window->fg_color = bz_random_color();
	window->circle_speed_x = bz_rand_int(2, 20) * 0.1f;
	window->circle_speed_y = bz_rand_int(2, 20) * 0.1f;
	// Wayland state
	window->wlsurface = wl_compositor_create_surface(globals->compositor);
	window->xdgsurface = bz_xdg_surface_constructor(globals, window->wlsurface);
	window->xdgtoplevel = bz_xdg_toplevel_constructor(globals, window->xdgsurface);
	window->pending = calloc(1, sizeof(*window->pending));
	window->finalized = calloc(1, sizeof(*window->finalized));

	wl_surface_commit(window->wlsurface);

	return window;
}

extern const struct wl_buffer_listener bz_buffer_implementation;
struct bz_cursor *bz_create_cursor_surface(struct bz_client_globals *globals)
{
	bz_info(BZ_LOG_MAIN, __FILE__, __LINE__, "Creating cursor surface.");
	struct bz_cursor *cursor = calloc(1, sizeof(*cursor));

	const int32_t width = 24;
	const int32_t height = 24;

	// Create the Wayland surface and buffer
	struct wl_surface *surface = wl_compositor_create_surface(globals->compositor);
	struct bz_buff_alloc *allocation = bz_allocate_shm_buffers(
		width,
		height,
		1,
		globals->shm,
		&bz_buffer_implementation
	);
	cursor->wlsurface = surface;
	cursor->pool_size = allocation->pool_size;
	cursor->pool_data = allocation->pool_data;
	cursor->shm_pool = allocation->shm_pool;
	cursor->buffer = allocation->buffers;
	free(allocation);

	// Create/populate our cursor
	uint32_t color = globals->window->bg_color;
	for (int32_t row = 0; row < height; row++) {
		for (int32_t col = 0; col < width; col++) {
			cursor->buffer->pixel_data[row * width + col] = color;
		}
	}

	// Attach and commit
	wl_surface_attach(surface, cursor->buffer->buffer, 0, 0);
	wl_surface_commit(surface);

	return cursor;
}
