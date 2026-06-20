
#include "breezy/bz_xdg_shell.h"

#include <stdlib.h>

#include <xdg-shell-server-protocol.h>

#include "breezy/bz_list.h"
#include "breezy/bz_wayland.h"
#include "breezy/bz_logger.h"
#include "breezy/bz_wl_display.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

// -- xdg_wm_base --

static const struct xdg_wm_base_interface bz_xdg_wm_base_implementation;
static void bz_xdg_wm_base_destroy(struct wl_client *client, struct wl_resource *resource);
static void bz_xdg_wm_base_create_positioner(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void bz_xdg_wm_base_get_xdg_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface);
static void bz_xdg_wm_base_pong(struct wl_client *client, struct wl_resource *resource, uint32_t serial);

// -- xdg_surface --

static const struct xdg_surface_interface bz_xdg_surface_implementation;
static void bz_xdg_surface_destroy(struct wl_client *client, struct wl_resource *resource);
static void bz_xdg_surface_get_toplevel(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void bz_xdg_surface_get_popup(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *parent, struct wl_resource *positioner);
static void bz_xdg_surface_set_window_geometry(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height);
static void bz_xdg_surface_ack_configure(struct wl_client *client, struct wl_resource *resource, uint32_t serial);
// Helpers
static bool bz_serial_matches(void *item, void *serial);
static bool bz_serial_is_newer(void *item, void *serial);

// -- xdg_toplevel --

static const struct xdg_toplevel_interface bz_xdg_toplevel_implementation;
static void bz_xdg_toplevel_destroy(struct wl_client *client, struct wl_resource *resource);
static void bz_xdg_toplevel_set_parent(struct wl_client *client, struct wl_resource *resource, struct wl_resource *parent);
static void bz_xdg_toplevel_set_title(struct wl_client *client, struct wl_resource *resource, const char *title);
static void bz_xdg_toplevel_set_app_id(struct wl_client *client, struct wl_resource *resource, const char *app_id);
static void bz_xdg_toplevel_show_window_menu(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial, int32_t x, int32_t y);
static void bz_xdg_toplevel_move(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial);
static void bz_xdg_toplevel_resize(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial, uint32_t edges);
static void bz_xdg_toplevel_set_max_size(struct wl_client *client, struct wl_resource *resource, int32_t width, int32_t height);
static void bz_xdg_toplevel_set_min_size(struct wl_client *client, struct wl_resource *resource, int32_t width, int32_t height);
static void bz_xdg_toplevel_set_maximized(struct wl_client *client, struct wl_resource *resource);
static void bz_xdg_toplevel_unset_maximized(struct wl_client *client, struct wl_resource *resource);
static void bz_xdg_toplevel_set_fullscreen(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output);
static void bz_xdg_toplevel_unset_fullscreen(struct wl_client *client, struct wl_resource *resource);
static void bz_xdg_toplevel_set_minimized(struct wl_client *client, struct wl_resource *resource);


// =================================================================================================
//  xdg_wm_base
// -------------------------------------------------------------------------------------------------

/** Gets executed whenever a client binds to xdg_wm_base. */
void bz_xdg_wm_base_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	bz_debug(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "Binding a client to xdg_wm_base.");

	struct wl_resource *res = wl_resource_create(client, &xdg_wm_base_interface, version, id);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(res, &bz_xdg_wm_base_implementation, nullptr, nullptr);
}

static const struct xdg_wm_base_interface bz_xdg_wm_base_implementation = {
	.destroy = bz_xdg_wm_base_destroy,
	.create_positioner = bz_xdg_wm_base_create_positioner,
	.get_xdg_surface = bz_xdg_wm_base_get_xdg_surface,
	.pong = bz_xdg_wm_base_pong,
};

static void bz_xdg_wm_base_destroy(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_wm_base.destroy not implemented");
	// TODO
}

static void bz_xdg_wm_base_create_positioner(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_wm_base.create_positioner not implemented");
	// TODO
}

static void bz_xdg_wm_base_get_xdg_surface(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id,
	struct wl_resource *surface
) {
	// It is illegal to create an xdg_surface for a wl_surface which already has an assigned role.
	struct bz_surface *bzsurf = wl_resource_get_user_data(surface);
	if (bzsurf->role != BZ_SURF_ROLE_NONE) {
		wl_resource_post_error(resource, XDG_WM_BASE_ERROR_ROLE, "Surface role cannot be changed.");
		goto initial_checks_failed;
	}

	// Allocate our user data
	struct bz_xdg_surface *xdgsurface = calloc(1, sizeof(*xdgsurface));
	if (xdgsurface == nullptr) {
		wl_client_post_no_memory(client);
		goto surface_alloc_failed;
	}

	// Create the resource, bound to the data
	struct wl_resource *res = wl_resource_create(
		client,
		&xdg_surface_interface,
		BZ_XDG_SURFACE_VERSION,
		id
	);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		goto resource_failed;
	}
	wl_resource_set_implementation(
		res,
		&bz_xdg_surface_implementation,
		xdgsurface,
		bz_xdg_surface_dtor
	);

	// Populate the surface's user data
	xdgsurface->resource = res;
	xdgsurface->wlsurface = wl_resource_get_user_data(surface);
	xdgsurface->pending_configures = bz_list_create();
	// ...and add our back-references.
	bzsurf->xdgsurface = xdgsurface;

	// Everything succeeded!
	return;

	resource_failed:
		free(xdgsurface);
	surface_alloc_failed:
	initial_checks_failed:
		bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "Failed to construct a new xdg_surface.");
}

static void bz_xdg_wm_base_pong(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t serial
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_wm_base.pong not implemented");
	// TODO
}


// =================================================================================================
//  xdg_surface
// -------------------------------------------------------------------------------------------------

static const struct xdg_surface_interface bz_xdg_surface_implementation = {
	.destroy = bz_xdg_surface_destroy,
	.get_toplevel = bz_xdg_surface_get_toplevel,
	.get_popup = bz_xdg_surface_get_popup,
	.set_window_geometry = bz_xdg_surface_set_window_geometry,
	.ack_configure = bz_xdg_surface_ack_configure,
};

void bz_xdg_surface_dtor(struct wl_resource *data)
{
	struct bz_xdg_surface *xdgsurf = wl_resource_get_user_data(data);

	bz_list_free(xdgsurf->pending_configures, free);
	free(xdgsurf->last_acked_configure);

	free(xdgsurf);
}

static void bz_xdg_surface_destroy(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_surface.destroy not implemented");
	// TODO
	// Role must be destroyed first. Otherwise, "defunct_role_object" error
}

static void bz_xdg_surface_get_toplevel(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id
) {
	// Cannot assign a different role if the surface already has one.
	struct bz_xdg_surface *xdgsurf = wl_resource_get_user_data(resource);
	struct bz_surface *bzsurf = xdgsurf->wlsurface;
	if (bzsurf->role != BZ_SURF_ROLE_NONE && bzsurf->role != BZ_SURF_ROLE_XDG_TOPLEVEL) {
		wl_resource_post_error(resource, XDG_WM_BASE_ERROR_ROLE, "Surface role cannot be changed.");
		goto initial_checks_failed;
	}

	// Allocate our user data
	struct bz_xdg_toplevel *xdgtoplevel = calloc(1, sizeof(*xdgtoplevel));
	if (xdgtoplevel == nullptr) {
		wl_client_post_no_memory(client);
		goto surface_alloc_failed;
	}

	// Create the resource, bound to the data
	struct wl_resource *res = wl_resource_create(
		client,
		&xdg_toplevel_interface,
		BZ_XDG_TOPLEVEL_VERSION,
		id
	);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		goto resource_failed;
	}
	wl_resource_set_implementation(
		res,
		&bz_xdg_toplevel_implementation,
		xdgtoplevel,
		bz_xdg_toplevel_dtor
	);

	// Populate the surface's user data
	xdgtoplevel->resource = res;
	xdgtoplevel->xdgsurface = xdgsurf;
	xdgtoplevel->wlsurface = bzsurf;

	// Populate the surface's role.
	bzsurf->role = BZ_SURF_ROLE_XDG_TOPLEVEL;
	bzsurf->xdgtoplevel = xdgtoplevel;

	// Everything succeeded!
	return;

	// Error cleanup
	resource_failed:
		free(xdgtoplevel);
	surface_alloc_failed:
	initial_checks_failed:
		bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "Failed to construct a new xdg_toplevel.");
}

static void bz_xdg_surface_get_popup(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id,
	struct wl_resource *parent,
	struct wl_resource *positioner
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_surface.get_popup not implemented");
	// TODO

	// // Cannot assign a different role if the surface already has one.
	// struct bz_surface *bzsurf = wl_resource_get_user_data(resource);
	// if (bzsurf->role != BZ_SURF_ROLE_NONE && bzsurf->role != BZ_SURF_ROLE_XDG_POPUP) {
	// 	wl_resource_post_error(resource, XDG_WM_BASE_ERROR_ROLE, "Surface role cannot be changed.");
	// 	goto initial_checks_failed;
	// }
	//
	// // Error cleanup
	// initial_checks_failed:
	// 	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "Failed to construct a new xdg_popup.");
}

static void bz_xdg_surface_set_window_geometry(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t x,
	int32_t y,
	int32_t width,
	int32_t height
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_surface.set_window_geometry not implemented");
	// TODO
}

static void bz_xdg_surface_ack_configure(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t serial
) {
	struct bz_xdg_surface *xdgsurf = wl_resource_get_user_data(resource);
	struct bz_list *pending_configs = xdgsurf->pending_configures;

	// Pull off the matching configure event
	struct bz_xdg_surface_configure *configevt = bz_list_find(
		pending_configs,
		&serial,
		bz_serial_matches
	);
	if (configevt == nullptr) {
		bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__,
			"Configure event not found for serial %d", serial);
		wl_resource_post_error(resource, XDG_SURFACE_ERROR_INVALID_SERIAL,
			"Configure event not found for serial %d.", serial);
		return;
	}

	// Update our last ack'd configure
	if (xdgsurf->last_acked_configure != nullptr) {
		free(xdgsurf->last_acked_configure);
	}
	xdgsurf->last_acked_configure = configevt;

	// Remove any pending configure events that are older than the provided one.
	bz_list_remove(pending_configs, configevt, nullptr); // Remove current one first to avoid "free"
	const int removed_items = bz_list_filter(pending_configs, &serial, bz_serial_is_newer, free);
	if (removed_items > 0) {
		bz_info(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__,
			"Removed %d outdated configure events.", removed_items);
	}
}

// ---  Helpers  -----------------------------------------------------------------------------------

void bz_xdg_surface_initial_configure(struct wl_client *client, struct bz_surface *bzsurf)
{
	struct bz_client *bzclient = wl_client_get_user_data(client);
	struct bz_breezy *globals = bzclient->breezy;
	struct bz_xdg_surface *xdgsurface = bzsurf->xdgsurface;

	// Allocate our configure event
	struct bz_xdg_surface_configure *configevt = calloc(1, sizeof(*configevt));
	if (configevt == nullptr) {
		wl_client_post_no_memory(client);
		goto configevt_alloc_failed;
	}

	if (bzsurf->role == BZ_SURF_ROLE_XDG_TOPLEVEL) {
		// Safety checks
		if (bzsurf->xdgtoplevel == nullptr) {
			bz_warn(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__,
				"XDG toplevel object does not exist on the surface.");
			wl_resource_post_error(bzsurf->xdgsurface->resource, XDG_SURFACE_ERROR_NOT_CONSTRUCTED,
				"XDG toplevel object does not exist on the surface.");
			goto null_toplevel;
		}

		// Assign our initial state
		configevt->serial = xdgsurface->serial;
		configevt->type = BZ_XDG_SURF_TOPLEVEL;
		wl_array_init(&configevt->toplevel.states);
		configevt->toplevel.max_size.w = globals->drm.mode_info.hdisplay;
		configevt->toplevel.max_size.h = globals->drm.mode_info.vdisplay;
		configevt->toplevel.recommended_size.w = globals->drm.mode_info.hdisplay;
		configevt->toplevel.recommended_size.h = globals->drm.mode_info.vdisplay;

		// Send initial state values
		//   wl_surface_send_preferred_buffer_scale(resource, 1);
		//   wl_surface_send_preferred_buffer_transform(resource, 0);
		xdg_toplevel_send_configure_bounds(
			bzsurf->xdgtoplevel->resource,
			configevt->toplevel.max_size.w,
			configevt->toplevel.max_size.h
		);
		xdg_toplevel_send_configure(
			bzsurf->xdgtoplevel->resource,
			configevt->toplevel.recommended_size.w,
			configevt->toplevel.recommended_size.h,
			&configevt->toplevel.states
		);
	} else if (bzsurf->role == BZ_SURF_ROLE_XDG_POPUP) {
		// TODO
		bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__,
			"Configuring roles for XDG popups not yet supported.");
	}

	bz_list_append(xdgsurface->pending_configures, configevt);
	xdg_surface_send_configure(xdgsurface->resource, xdgsurface->serial++);
	return;

	null_toplevel:
		free(configevt);
	configevt_alloc_failed:
		bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "Failed to send initial configure sequence.");
}

static bool bz_serial_matches(void *item, void *serial)
{
	const struct bz_xdg_surface_configure *item_data = item;
	const uint32_t *serial_int = serial;
	return item_data->serial == *serial_int;
}

static bool bz_serial_is_newer(void *item, void *serial)
{
	const struct bz_xdg_surface_configure *item_data = item;
	const uint32_t *serial_int = serial;
	return item_data->serial > *serial_int;
}


// =================================================================================================
//  xdg_toplevel
// -------------------------------------------------------------------------------------------------

static const struct xdg_toplevel_interface bz_xdg_toplevel_implementation = {
	.destroy = bz_xdg_toplevel_destroy,
	.set_parent = bz_xdg_toplevel_set_parent,
	.set_title = bz_xdg_toplevel_set_title,
	.set_app_id = bz_xdg_toplevel_set_app_id,
	.show_window_menu = bz_xdg_toplevel_show_window_menu,
	.move = bz_xdg_toplevel_move,
	.resize = bz_xdg_toplevel_resize,
	.set_max_size = bz_xdg_toplevel_set_max_size,
	.set_min_size = bz_xdg_toplevel_set_min_size,
	.set_maximized = bz_xdg_toplevel_set_maximized,
	.unset_maximized = bz_xdg_toplevel_unset_maximized,
	.set_fullscreen = bz_xdg_toplevel_set_fullscreen,
	.unset_fullscreen = bz_xdg_toplevel_unset_fullscreen,
	.set_minimized = bz_xdg_toplevel_set_minimized,
};

void bz_xdg_toplevel_dtor(struct wl_resource *data)
{
	struct bz_xdg_toplevel *xdgtoplevel = wl_resource_get_user_data(data);

	free(xdgtoplevel);
}

static void bz_xdg_toplevel_destroy(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_toplevel.destroy not implemented");
	// TODO
}

static void bz_xdg_toplevel_set_parent(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *parent
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_toplevel.set_parent not implemented");
	// TODO
}

static void bz_xdg_toplevel_set_title(
	struct wl_client *client,
	struct wl_resource *resource,
	const char *title
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_toplevel.set_title not implemented");
	// TODO
}

static void bz_xdg_toplevel_set_app_id(
	struct wl_client *client,
	struct wl_resource *resource,
	const char *app_id
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_toplevel.set_app_id not implemented");
	// TODO
}

static void bz_xdg_toplevel_show_window_menu(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *seat,
	uint32_t serial,
	int32_t x,
	int32_t y
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_toplevel.show_window_menu not implemented");
	// TODO
}

static void bz_xdg_toplevel_move(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *seat,
	uint32_t serial
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_toplevel.move not implemented");
	// TODO
}

static void bz_xdg_toplevel_resize(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *seat,
	uint32_t serial,
	uint32_t edges
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_toplevel.resize not implemented");
	// TODO
}

static void bz_xdg_toplevel_set_max_size(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t width,
	int32_t height
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_toplevel.set_max_size not implemented");
	// TODO
}

static void bz_xdg_toplevel_set_min_size(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t width,
	int32_t height
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_toplevel.set_min_size not implemented");
	// TODO
}

static void bz_xdg_toplevel_set_maximized(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_toplevel.set_maximized not implemented");
	// TODO
}

static void bz_xdg_toplevel_unset_maximized(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_toplevel.unset_maximized not implemented");
	// TODO
}

static void bz_xdg_toplevel_set_fullscreen(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *output
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_toplevel.set_fullscreen not implemented");
	// TODO
}

static void bz_xdg_toplevel_unset_fullscreen(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_toplevel.unset_fullscreen not implemented");
	// TODO
}

static void bz_xdg_toplevel_set_minimized(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_toplevel.set_minimized not implemented");
	// TODO
}

