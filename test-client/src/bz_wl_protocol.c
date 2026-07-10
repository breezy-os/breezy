
#include "breezy/bz_wl_protocol.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include <xdg-shell-client-protocol.h>

#include "breezy/bz_client_utils.h"
#include "breezy/bz_logger.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

// -- wl_registry --

static const struct wl_registry_listener bz_registry_implementation;
static void bz_registry_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
static void bz_registry_global_remove(void *data, struct wl_registry *registry, uint32_t name);

// -- wl_shm --

static const struct wl_shm_listener bz_shm_implementation;
static void bz_shm_format(void *data, struct wl_shm *shm, uint32_t format);

// -- wl_buffer --

static const struct wl_buffer_listener bz_buffer_implementation;
static void bz_buffer_release(void *data, struct wl_buffer *buffer);

// -- xdg_wm_base --

static const struct xdg_wm_base_listener bz_xdg_wm_base_implementation;
static void bz_xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial);

// -- xdg_surface --

static const struct xdg_surface_listener bz_xdg_surface_implementation;
static void bz_xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial);
// Helpers
static void bz_initialize_surface_buffers(struct bz_client_globals *globals, struct bz_application_window* window);
static void bz_draw_frame(struct bz_application_window *window);
static void bz_submit_frame(struct bz_application_window* window);
static void bz_render(void *data, struct wl_callback *wl_callback, uint32_t callback_data);

// -- xdg_toplevel --

static const struct xdg_toplevel_listener bz_xdg_toplevel_implementation;
static void bz_xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states);
static void bz_xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel);
static void bz_xdg_toplevel_configure_bounds(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height);
static void bz_xdg_toplevel_wm_capabilities(void *data, struct xdg_toplevel *xdg_toplevel, struct wl_array *capabilities);


// =================================================================================================
//  wl_registry
// -------------------------------------------------------------------------------------------------

void bz_registry_constructor(struct bz_client_globals *globals)
{
	bz_debug(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Setting up registry listener.");
	globals->registry = wl_display_get_registry(globals->display);
	wl_registry_add_listener(globals->registry, &bz_registry_implementation, globals);
}

static const struct wl_registry_listener bz_registry_implementation = {
	.global = bz_registry_global,
	.global_remove = bz_registry_global_remove,
};

static void bz_registry_global(
	void *data,
	struct wl_registry *registry,
	uint32_t name,
	const char *interface,
	uint32_t version
) {
	bz_info(BZ_LOG_WAYLAND, __FILE__, __LINE__,
		"Registering interface: '%s', version: %d, name: %d", interface, version, name);

	struct bz_client_globals *globals = data;

	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		globals->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 6);
		globals->compositor_name = name;
	}
	else if (strcmp(interface, wl_subcompositor_interface.name) == 0) {
		// Version 2
	}
	else if (strcmp(interface, wl_shm_interface.name) == 0) {
		globals->shm = wl_registry_bind(registry, name, &wl_shm_interface, 2);
		globals->shm_name = name;
		wl_shm_add_listener(globals->shm, &bz_shm_implementation, data);
	}
	else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		globals->xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 7);
		globals->xdg_wm_base_name = name;
		xdg_wm_base_add_listener(globals->xdg_wm_base, &bz_xdg_wm_base_implementation, data);
	}
	else if (strcmp(interface, wl_data_device_manager_interface.name) == 0) {
		// Version 3
	}
	else if (strcmp(interface, wl_seat_interface.name) == 0) {
		// Version 10
	}
	else if (strcmp(interface, wl_output_interface.name) == 0) {
		// Version 4
	}
}

static void bz_registry_global_remove(void *data, struct wl_registry * /*registry*/, uint32_t name)
{
	bz_info(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Removing global with name: %d", name);
	struct bz_client_globals *globals = data;

	if (name == globals->compositor_name) {
		wl_compositor_destroy(globals->compositor);
		globals->compositor = nullptr;
		globals->compositor_name = 0;
	} else if (name == globals->shm_name) {
		wl_shm_destroy(globals->shm);
		globals->shm = nullptr;
		globals->shm_name = 0;
	}
	// TODO: Remember to clean up other global types, such as outputs for hotplug events, etc.
}


// =================================================================================================
//  wl_shm
// -------------------------------------------------------------------------------------------------

static const struct wl_shm_listener bz_shm_implementation = {
	.format = bz_shm_format,
};

static void bz_shm_format(void * /*data*/, struct wl_shm * /*shm*/, uint32_t format)
{
	bz_debug(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Supported shm format: %d", format);
}


// =================================================================================================
//  wl_buffer
// -------------------------------------------------------------------------------------------------

static const struct wl_buffer_listener bz_buffer_implementation = {
	.release = bz_buffer_release,
};

static void bz_buffer_release(void *data, struct wl_buffer *buffer)
{
	struct bz_buffer *bzbuff = data;
	bzbuff->is_released = true;
}


// =================================================================================================
//  xdg_wm_base
// -------------------------------------------------------------------------------------------------

static const struct xdg_wm_base_listener bz_xdg_wm_base_implementation = {
	.ping = bz_xdg_wm_base_ping,
};

static void bz_xdg_wm_base_ping(void * /*data*/, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
	xdg_wm_base_pong(xdg_wm_base, serial);
}


// =================================================================================================
//  xdg_surface
// -------------------------------------------------------------------------------------------------

struct xdg_surface *bz_xdg_surface_constructor(
	struct bz_client_globals *globals,
	struct wl_surface *wlsurface
) {
	bz_debug(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Creating an XDG surface");
	struct xdg_surface *xdgsurf = xdg_wm_base_get_xdg_surface(globals->xdg_wm_base, wlsurface);
	xdg_surface_add_listener(xdgsurf, &bz_xdg_surface_implementation, globals);
	return xdgsurf;
}

static const struct xdg_surface_listener bz_xdg_surface_implementation = {
	.configure = bz_xdg_surface_configure,
};

static void bz_xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
	bz_debug(BZ_LOG_WAYLAND, __FILE__, __LINE__,
		"xdg_surface.configure(): Finalizing configure sequence for serial %d.", serial);
	struct bz_client_globals *client_globals = data;
	struct bz_application_window *window = client_globals->window;

	window->pending->serial = serial;

	// Promote our "pending" data to our "finalized" data.
	free(window->finalized);
	window->finalized = window->pending;
	window->pending = calloc(1, sizeof(*window->pending));

	window->size.w = window->finalized->recommended_size.w;
	window->size.h = window->finalized->recommended_size.h;

	// Build our buffer, ack our configure, and submit!
	xdg_surface_ack_configure(window->xdgsurface, window->finalized->serial);
	bz_initialize_surface_buffers(client_globals, window);
	bz_draw_frame(window);
	bz_submit_frame(window);
}

// ---  Helpers  -----------------------------------------------------------------------------------

static void bz_initialize_surface_buffers(
	struct bz_client_globals *globals,
	struct bz_application_window *window
) {
	// Create a pool for our buffers
	size_t buffer_size = window->size.w * window->size.h * 4; // 4 bytes per px (XRGB8888)
	window->pool_size = buffer_size * 2; // Two buffers per pool (double-buffered)
	int fd = bz_allocate_shm_file(window->pool_size);
	if (fd == -1) {
		bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Failed to allocate shared memory.");
		return;
	}

	// Map the pool's file descriptor to memory
	window->pool_data = mmap(NULL, window->pool_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (window->pool_data == MAP_FAILED) {
		bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Failed to mmap pool data.");
		window->pool_data = nullptr;
		close(fd);
		return;
	}

	// Send the pool to the server
	window->shm_pool = wl_shm_create_pool(globals->shm, fd, window->pool_size);
	close(fd);

	// Create our two buffers
	for (uint8_t i = 0; i < 2; i++) {
		size_t offset = i * buffer_size;
		window->buffers[i].is_released = true;
		window->buffers[i].size = window->size;
		window->buffers[i].pixel_data = (uint32_t *)(&window->pool_data[offset]);
		window->buffers[i].buffer = wl_shm_pool_create_buffer(
			window->shm_pool,
			offset,
			window->size.w,
			window->size.h,
			window->size.w * 4, // Stride
			WL_SHM_FORMAT_XRGB8888
		);
		wl_buffer_add_listener(
			window->buffers[i].buffer,
			&bz_buffer_implementation,
			&window->buffers[i]
		);
	}
	window->active_buffer = 0;
}

#define BZ_BORDER_WIDTH 4
#define BZ_TITLE_WIDTH 40
#define BZ_CIRCLE_RADIUS 50
static void bz_draw_frame(struct bz_application_window *window)
{
	const struct bz_buffer *buffer = &window->buffers[window->active_buffer];
	if (!buffer->is_released) {
		bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Cannot draw frame on an unreleased buffer.");
		return;
	}
	int circle_x = buffer->size.w/2;
	int circle_y = buffer->size.h/2;
	for (int y = 0; y < buffer->size.h; y++) {
		for (int x = 0; x < buffer->size.w; x++) {
			if (x < BZ_BORDER_WIDTH || x > (buffer->size.w - BZ_BORDER_WIDTH) || y > (buffer->size.h - BZ_BORDER_WIDTH)) {
				buffer->pixel_data[y * buffer->size.w + x] = 0xFFFFFFFF;
			} else if (y < BZ_TITLE_WIDTH) {
				buffer->pixel_data[y * buffer->size.w + x] = 0xFFFFFFFF;
			} else if (distance(circle_x, circle_y, x, y) < BZ_CIRCLE_RADIUS) {
				buffer->pixel_data[y * buffer->size.w + x] = window->fg_color;
			} else {
				buffer->pixel_data[y * buffer->size.w + x] = window->bg_color;
			}
		}
	}
}

static void bz_submit_frame(struct bz_application_window *window)
{
	struct bz_buffer *bzbuffer = &window->buffers[window->active_buffer];
	window->active_buffer = window->active_buffer == 0 ? 1 : 0;
	bzbuffer->is_released = false;

	wl_surface_attach(window->wlsurface, bzbuffer->buffer, 0, 0);
	// TODO-dl10: wl_surface_damage(window->wlsurface, 0, 0, INT32_MAX, INT32_MAX);

	// Set up our frame callback to get notified when our next frame should be drawn
	window->frame_callback = wl_surface_frame(window->wlsurface);
	const struct wl_callback_listener frame_listener = {
		.done = bz_render,
	};
	wl_callback_add_listener(window->frame_callback, &frame_listener, window);

	wl_surface_commit(window->wlsurface);
}

static void bz_render(void *data, struct wl_callback *wl_callback, uint32_t callback_data)
{
	// bz_info(BZ_LOG_WAYLAND, __FILE__, __LINE__, "RENDER CALLBACK TRIGGERED! Time: %d", callback_data); // TODO-dl9: delete

	struct bz_application_window *window = data;

	window->frame_callback = nullptr;
	wl_callback_destroy(wl_callback);

	bz_draw_frame(window);
	bz_submit_frame(window);
}


// =================================================================================================
//  xdg_toplevel
// -------------------------------------------------------------------------------------------------

struct xdg_toplevel *bz_xdg_toplevel_constructor(
	struct bz_client_globals *globals,
	struct xdg_surface *xdgsurface
) {
	bz_debug(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Creating an XDG toplevel");
	struct xdg_toplevel *xdgtoplevel = xdg_surface_get_toplevel(xdgsurface);
	xdg_toplevel_add_listener(xdgtoplevel, &bz_xdg_toplevel_implementation, globals);
	// xdg_toplevel_set_title(xdgtoplevel, "Greetings from test client!");
	return xdgtoplevel;
}

static const struct xdg_toplevel_listener bz_xdg_toplevel_implementation = {
	.configure = bz_xdg_toplevel_configure,
	.close = bz_xdg_toplevel_close,
	.configure_bounds = bz_xdg_toplevel_configure_bounds,
	.wm_capabilities = bz_xdg_toplevel_wm_capabilities,
};

static void bz_xdg_toplevel_configure(
	void *data,
	struct xdg_toplevel *xdg_toplevel,
	int32_t width,
	int32_t height,
	struct wl_array *states
) {
	bz_debug(BZ_LOG_WAYLAND, __FILE__, __LINE__,
		"xdg_toplevel.configure(): Setting recommended bounds (%dx%d) and states on xdg toplevel.",
		width, height);
	struct bz_client_globals *client_globals = data;
	struct bz_application_window *window = client_globals->window;
	window->pending->recommended_size.w = width;
	window->pending->recommended_size.h = height;
	window->pending->states = states;
}

static void bz_xdg_toplevel_close(void * /*data*/, struct xdg_toplevel *xdg_toplevel)
{
	bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "xdg_toplevel.close not implemented");
	// TODO
}

static void bz_xdg_toplevel_configure_bounds(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height)
{
	bz_debug(BZ_LOG_WAYLAND, __FILE__, __LINE__,
		"xdg_toplevel.configure_bounds(): Setting max bounds on xdg toplevel to %dx%d.",
		width, height);
	struct bz_client_globals *client_globals = data;
	struct bz_application_window *window = client_globals->window;
	window->pending->max_size.w = width;
	window->pending->max_size.h = height;
}

static void bz_xdg_toplevel_wm_capabilities(void * /*data*/, struct xdg_toplevel *xdg_toplevel, struct wl_array *capabilities)
{
	bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "xdg_toplevel.wm_capabilities not implemented");
	// TODO
}
