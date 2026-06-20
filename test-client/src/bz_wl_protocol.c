
#include "breezy/bz_wl_protocol.h"

#include <stdlib.h>
#include <string.h>

#include <xdg-shell-client-protocol.h>

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

// -- xdg_wm_base --

static const struct xdg_wm_base_listener bz_xdg_wm_base_implementation;
static void bz_xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial);

// -- xdg_surface --

static const struct xdg_surface_listener bz_xdg_surface_implementation;
static void bz_xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial);

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

	// Build our buffer, ack our configure, and submit!
	xdg_surface_ack_configure(window->xdgsurface, window->finalized->serial);
	// TODO
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

static void bz_xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states)
{
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

