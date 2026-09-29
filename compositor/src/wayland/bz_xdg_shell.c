
#include "breezy/bz_xdg_shell.h"

#include <stdlib.h>

#include <xdg-shell-server-protocol.h>

#include "breezy/bz_list.h"
#include "breezy/bz_wayland.h"
#include "breezy/bz_logger.h"
#include "breezy/bz_math.h"
#include "breezy/bz_wl_devices.h"
#include "breezy/bz_wl_display.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

// -- xdg_wm_base --

const struct xdg_wm_base_interface bz_xdg_wm_base_implementation;
static void bz_xdg_wm_base_destroy(struct wl_client *client, struct wl_resource *resource);
static void bz_xdg_wm_base_create_positioner(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void bz_xdg_wm_base_get_xdg_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface);
static void bz_xdg_wm_base_pong(struct wl_client *client, struct wl_resource *resource, uint32_t serial);

// -- xdg_positioner --

void bz_xdg_positioner_dtor(struct wl_resource *data);
const struct xdg_positioner_interface bz_xdg_positioner_implementation;
static void bz_xdg_positioner_destroy(struct wl_client *client, struct wl_resource *resource);
static void bz_xdg_positioner_set_size(struct wl_client *client, struct wl_resource *resource, int32_t width, int32_t height);
static void bz_xdg_positioner_set_anchor_rect(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height);
static void bz_xdg_positioner_set_anchor(struct wl_client *client, struct wl_resource *resource, uint32_t anchor);
static void bz_xdg_positioner_set_gravity(struct wl_client *client, struct wl_resource *resource, uint32_t gravity);
static void bz_xdg_positioner_set_constraint_adjustment(struct wl_client *client, struct wl_resource *resource, uint32_t constraint_adjustment);
static void bz_xdg_positioner_set_offset(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y);
static void bz_xdg_positioner_set_reactive(struct wl_client *client, struct wl_resource *resource);
static void bz_xdg_positioner_set_parent_size(struct wl_client *client, struct wl_resource *resource, int32_t parent_width, int32_t parent_height);
static void bz_xdg_positioner_set_parent_configure(struct wl_client *client, struct wl_resource *resource, uint32_t serial);

// -- xdg_surface --

void bz_xdg_surface_dtor(struct wl_resource *data);
const struct xdg_surface_interface bz_xdg_surface_implementation;
static void bz_xdg_surface_destroy(struct wl_client *client, struct wl_resource *resource);
static void bz_xdg_surface_get_toplevel(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void bz_xdg_surface_get_popup(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *parent, struct wl_resource *positioner);
static void bz_xdg_surface_set_window_geometry(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height);
static void bz_xdg_surface_ack_configure(struct wl_client *client, struct wl_resource *resource, uint32_t serial);
// Helpers
void bz_xdg_surface_initial_configure(struct wl_client *client, struct bz_surface *bzsurf);
static bool bz_serial_matches(void *item, void *serial);
static bool bz_serial_is_newer(void *item, void *serial);
static void bz_free_xdg_surface_configure(void *xdg_surface_configure);

// -- xdg_toplevel --

void bz_xdg_toplevel_dtor(struct wl_resource *data);
const struct xdg_toplevel_interface bz_xdg_toplevel_implementation;
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

// -- xdg_popup --

void bz_xdg_popup_dtor(struct wl_resource *data);
const struct xdg_popup_interface bz_xdg_popup_implementation;
static void bz_xdg_popup_destroy(struct wl_client *client, struct wl_resource *resource);
static void bz_xdg_popup_grab(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial);
static void bz_xdg_popup_reposition(struct wl_client *client, struct wl_resource *resource, struct wl_resource *positioner, uint32_t token);


// =================================================================================================
//  xdg_wm_base
// -------------------------------------------------------------------------------------------------

/** Gets executed whenever a client binds to xdg_wm_base. */
void bz_xdg_wm_base_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	bz_debug(BZ_LOG_WL_XDG_SHELL, "Binding a client to xdg_wm_base with version %d.", version);

	struct wl_resource *res = wl_resource_create(client, &xdg_wm_base_interface, version, id);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(res, &bz_xdg_wm_base_implementation, nullptr, nullptr);
}

const struct xdg_wm_base_interface bz_xdg_wm_base_implementation = {
	.destroy = bz_xdg_wm_base_destroy,
	.create_positioner = bz_xdg_wm_base_create_positioner,
	.get_xdg_surface = bz_xdg_wm_base_get_xdg_surface,
	.pong = bz_xdg_wm_base_pong,
};

static void bz_xdg_wm_base_destroy(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_wm_base.destroy not implemented");
	// TODO
}

static void bz_xdg_wm_base_create_positioner(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id
) {
	// Allocate our user data
	struct bz_xdg_positioner *positioner = calloc(1, sizeof(*positioner));
	if (positioner == nullptr) {
		wl_client_post_no_memory(client);
		goto positioner_alloc_failed;
	}

	// Create the resource, bound to the data
	struct wl_resource *res = wl_resource_create(
		client,
		&xdg_positioner_interface,
		wl_resource_get_version(resource),
		id
	);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		goto resource_failed;
	}
	wl_resource_set_implementation(
		res,
		&bz_xdg_positioner_implementation,
		positioner,
		bz_xdg_positioner_dtor
	);

	// Populate the surface's user data
	positioner->resource = res;

	// Everything succeeded!
	return;

	// Error cleanup
	resource_failed:
		free(positioner);
	positioner_alloc_failed:
		bz_error(BZ_LOG_WL_XDG_SHELL, "Failed to construct a new xdg_positioner.");
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
		wl_resource_get_version(resource),
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
	xdgsurface->wlsurface = bzsurf;
	xdgsurface->pending_configures = bz_list_create();
	// ...and add our back-references.
	bzsurf->xdgsurface = xdgsurface;

	// Everything succeeded!
	return;

	resource_failed:
		free(xdgsurface);
	surface_alloc_failed:
	initial_checks_failed:
		bz_error(BZ_LOG_WL_XDG_SHELL, "Failed to construct a new xdg_surface.");
}

static void bz_xdg_wm_base_pong(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t serial
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_wm_base.pong not implemented");
	// TODO
}


// =================================================================================================
//  xdg_positioner
// -------------------------------------------------------------------------------------------------

void bz_xdg_positioner_dtor(struct wl_resource *data)
{
	struct bz_xdg_positioner *positioner = wl_resource_get_user_data(data);
	free(positioner);
}

const struct xdg_positioner_interface bz_xdg_positioner_implementation = {
	.destroy = bz_xdg_positioner_destroy,
	.set_size = bz_xdg_positioner_set_size,
	.set_anchor_rect = bz_xdg_positioner_set_anchor_rect,
	.set_anchor = bz_xdg_positioner_set_anchor,
	.set_gravity = bz_xdg_positioner_set_gravity,
	.set_constraint_adjustment = bz_xdg_positioner_set_constraint_adjustment,
	.set_offset = bz_xdg_positioner_set_offset,
	.set_reactive = bz_xdg_positioner_set_reactive,
	.set_parent_size = bz_xdg_positioner_set_parent_size,
	.set_parent_configure = bz_xdg_positioner_set_parent_configure
};

static void bz_xdg_positioner_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void bz_xdg_positioner_set_size(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t width,
	int32_t height
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_positioner.set_size() not implemented.");
	// TODO
}

static void bz_xdg_positioner_set_anchor_rect(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t x,
	int32_t y,
	int32_t width,
	int32_t height
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_positioner.set_anchor_rect() not implemented.");
	// TODO
}

static void bz_xdg_positioner_set_anchor(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t anchor
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_positioner.set_anchor() not implemented.");
	// TODO
}

static void bz_xdg_positioner_set_gravity(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t gravity
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_positioner.set_gravity() not implemented.");
	// TODO
}

static void bz_xdg_positioner_set_constraint_adjustment(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t constraint_adjustment
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_positioner.set_constraint_adjustment() not implemented.");
	// TODO
}

static void bz_xdg_positioner_set_offset(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t x,
	int32_t y
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_positioner.set_offset() not implemented.");
	// TODO
}

static void bz_xdg_positioner_set_reactive(
	struct wl_client *client,
	struct wl_resource *resource
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_positioner.set_reactive() not implemented.");
	// TODO
}

static void bz_xdg_positioner_set_parent_size(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t parent_width,
	int32_t parent_height
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_positioner.set_parent_size() not implemented.");
	// TODO
}

static void bz_xdg_positioner_set_parent_configure(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t serial
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_positioner.set_parent_configure() not implemented.");
	// TODO
}



// =================================================================================================
//  xdg_surface
// -------------------------------------------------------------------------------------------------

void bz_xdg_surface_dtor(struct wl_resource *data)
{
	struct bz_xdg_surface *xdgsurf = wl_resource_get_user_data(data);

	// If the surface dtor was called first, then the wl_surface's reference to this was already severed.
	if (xdgsurf->wlsurface != nullptr) {
		xdgsurf->wlsurface->xdgsurface = nullptr;
	}

	bz_list_free(xdgsurf->pending_configures, bz_free_xdg_surface_configure);
	bz_free_xdg_surface_configure(xdgsurf->last_acked_configure);

	free(xdgsurf);
}

const struct xdg_surface_interface bz_xdg_surface_implementation = {
	.destroy = bz_xdg_surface_destroy,
	.get_toplevel = bz_xdg_surface_get_toplevel,
	.get_popup = bz_xdg_surface_get_popup,
	.set_window_geometry = bz_xdg_surface_set_window_geometry,
	.ack_configure = bz_xdg_surface_ack_configure,
};

static void bz_xdg_surface_destroy(struct wl_client *client, struct wl_resource *resource)
{
	struct bz_xdg_surface *xdg_surface_data = wl_resource_get_user_data(resource);
	struct bz_surface *surface_data = xdg_surface_data->wlsurface;

	// Role must be destroyed first. Otherwise, "defunct_role_object" error
	if (
		(surface_data->role == BZ_SURF_ROLE_XDG_TOPLEVEL && surface_data->xdgtoplevel != nullptr) ||
		(surface_data->role == BZ_SURF_ROLE_XDG_POPUP && surface_data->xdgpopup != nullptr)
	) {
		bz_error(BZ_LOG_WL_DISPLAY, "Surface role must be destroyed before the XDG surface.");
		wl_resource_post_error(resource, XDG_SURFACE_ERROR_DEFUNCT_ROLE_OBJECT,
			"Surface role must be destroyed before the XDG surface.");
		return;
	}

	wl_resource_destroy(resource);
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
		wl_resource_get_version(resource),
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
		bz_error(BZ_LOG_WL_XDG_SHELL, "Failed to construct a new xdg_toplevel.");
}

static void bz_xdg_surface_get_popup(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id,
	struct wl_resource *parent,
	struct wl_resource *positioner
) {
	// Cannot assign a different role if the surface already has one.
	struct bz_xdg_surface *xdgsurf = wl_resource_get_user_data(resource);
	struct bz_surface *bzsurf = xdgsurf->wlsurface;
	if (bzsurf->role != BZ_SURF_ROLE_NONE && bzsurf->role != BZ_SURF_ROLE_XDG_POPUP) {
		wl_resource_post_error(resource, XDG_WM_BASE_ERROR_ROLE, "Surface role cannot be changed.");
		goto initial_checks_failed;
	}

	// Allocate our user data
	struct bz_xdg_popup *popup = calloc(1, sizeof(*popup));
	if (popup == nullptr) {
		wl_client_post_no_memory(client);
		goto popup_alloc_failed;
	}

	// Create the resource, bound to the data
	struct wl_resource *res = wl_resource_create(
		client,
		&xdg_popup_interface,
		wl_resource_get_version(resource),
		id
	);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		goto resource_failed;
	}
	wl_resource_set_implementation(
		res,
		&bz_xdg_popup_implementation,
		popup,
		bz_xdg_popup_dtor
	);

	// Populate the surface's user data
	struct bz_xdg_surface *xdgsurface = wl_resource_get_user_data(resource);
	popup->resource = res;
	popup->xdgsurface = xdgsurface;
	popup->wlsurface = xdgsurface->wlsurface;
	// TODO-dl12: store the "parent" and "positioner" parameters

	// Populate the surface's role.
	xdgsurface->wlsurface->role = BZ_SURF_ROLE_XDG_POPUP;
	xdgsurface->wlsurface->xdgpopup = popup;

	// Everything succeeded!
	return;

	// Error cleanup
	resource_failed:
		free(popup);
	popup_alloc_failed:
	initial_checks_failed:
		bz_error(BZ_LOG_WL_XDG_SHELL, "Failed to construct a new xdg_popup.");
}

static void bz_xdg_surface_set_window_geometry(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t x,
	int32_t y,
	int32_t width,
	int32_t height
) {
	bz_debug(BZ_LOG_WL_XDG_SHELL, "Setting window geometry: %dx%d, %dx%d", x, y, width, height);
	// Parameter validation
	if (width <= 0 || height <= 0) {
		wl_resource_post_error(resource, XDG_SURFACE_ERROR_INVALID_SIZE,
			"Width and height must both be positive.");
		bz_warn(BZ_LOG_WL_XDG_SHELL,
			"Non-positive width/height provided to set_window_geometry: %d, %d", width, height);
		return;
	}

	struct bz_xdg_surface *xdg_surf = wl_resource_get_user_data(resource);
	struct bz_surface_state *state = xdg_surf->wlsurface->pending_state;

	// Store our new geometry
	if (state->window_geometry == nullptr) {
		state->window_geometry = calloc(1, sizeof(*state->window_geometry));
	}
	*state->window_geometry = (struct bz_rect){ .x = x, .y = y, .w = width, .h = height };
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
		bz_error(BZ_LOG_WL_XDG_SHELL, "Configure event not found for serial %d", serial);
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
		bz_info(BZ_LOG_WL_XDG_SHELL, "Removed %d outdated configure events.", removed_items);
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
			bz_warn(BZ_LOG_WL_XDG_SHELL, "XDG toplevel object does not exist on the surface.");
			wl_resource_post_error(bzsurf->xdgsurface->resource, XDG_SURFACE_ERROR_NOT_CONSTRUCTED,
				"XDG toplevel object does not exist on the surface.");
			goto null_toplevel;
		}

		// Assign our initial state
		configevt->serial = wl_display_next_serial(wl_client_get_display(client));
		configevt->type = BZ_XDG_SURF_TOPLEVEL;
		wl_array_init(&configevt->toplevel.states);
		configevt->toplevel.max_size.w = globals->drm.current_output->compositor_bounds.w;
		configevt->toplevel.max_size.h = globals->drm.current_output->compositor_bounds.h;
		// Set recommended size based on min/max size.
		struct bz_size rec_size = { .w = 1920, .h = 1080 };
		if (bzsurf->active_state->min_size) {
			rec_size.w = bz_max(rec_size.w, bzsurf->active_state->min_size->w);
			rec_size.h = bz_max(rec_size.h, bzsurf->active_state->min_size->h);
		}
		if (bzsurf->active_state->max_size) {
			rec_size.w = bz_min(rec_size.w, bzsurf->active_state->max_size->w);
			rec_size.h = bz_min(rec_size.h, bzsurf->active_state->max_size->h);
		}
		configevt->toplevel.recommended_size = rec_size;

		if (wl_resource_get_version(bzsurf->xdgtoplevel->resource) >= XDG_TOPLEVEL_CONFIGURE_BOUNDS_SINCE_VERSION) {
			xdg_toplevel_send_configure_bounds(
				bzsurf->xdgtoplevel->resource,
				configevt->toplevel.max_size.w,
				configevt->toplevel.max_size.h
			);
		}

		// TODO-dl??: Set window capabilities..? (can be tested with foot)
		// struct wl_array caps;
		// wl_array_init(&caps);
		// uint32_t *c1 = wl_array_add(&caps, sizeof(uint32_t));
		// *c1 = XDG_TOPLEVEL_WM_CAPABILITIES_FULLSCREEN;
		// uint32_t *c2 = wl_array_add(&caps, sizeof(uint32_t));
		// *c2 = XDG_TOPLEVEL_WM_CAPABILITIES_MAXIMIZE;
		// xdg_toplevel_send_wm_capabilities(bzsurf->xdgtoplevel->resource, &caps);
		// wl_array_release(&caps);

		xdg_toplevel_send_configure(
			bzsurf->xdgtoplevel->resource,
			// configevt->toplevel.recommended_size.w,
			// configevt->toplevel.recommended_size.h,
			0,
			0,
			&configevt->toplevel.states
		);
	} else if (bzsurf->role == BZ_SURF_ROLE_XDG_POPUP) {
		// Safety checks
		if (bzsurf->xdgpopup == nullptr) {
			bz_warn(BZ_LOG_WL_XDG_SHELL, "XDG popup object does not exist on the surface.");
			wl_resource_post_error(bzsurf->xdgsurface->resource, XDG_SURFACE_ERROR_NOT_CONSTRUCTED,
				"XDG popup object does not exist on the surface.");
			goto null_popup;
		}

		// Assign our initial state
		configevt->serial = wl_display_next_serial(wl_client_get_display(client));
		configevt->type = BZ_XDG_SURF_POPUP;
		// TODO-dl13: Fill these in from the attached positioner
		configevt->popup.position.x = 0;
		configevt->popup.position.y = 0;
		configevt->popup.size.w = 600;
		configevt->popup.size.h = 600;

		xdg_popup_send_configure(
			bzsurf->xdgpopup->resource,
			configevt->popup.position.x,
			configevt->popup.position.y,
			configevt->popup.size.w,
			configevt->popup.size.h
		);
	}

	bz_list_append(xdgsurface->pending_configures, configevt);
	xdg_surface_send_configure(xdgsurface->resource, configevt->serial);
	return;

	null_toplevel:
	null_popup:
		free(configevt);
	configevt_alloc_failed:
		bz_error(BZ_LOG_WL_XDG_SHELL, "Failed to send initial configure sequence.");
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

/** "void *state" is of type "struct bz_xdg_surface_configure *". */
static void bz_free_xdg_surface_configure(void *xdg_surface_configure)
{
	if (xdg_surface_configure == nullptr) {
		return;
	}

	struct bz_xdg_surface_configure *configure = xdg_surface_configure;

	if (configure->type == BZ_XDG_SURF_TOPLEVEL) {
		wl_array_release(&configure->toplevel.states);
	} else if (configure->type == BZ_XDG_SURF_POPUP) {
		// Nothing to free (...at this point)
	}

	free(configure);
}


// =================================================================================================
//  xdg_toplevel
// -------------------------------------------------------------------------------------------------

void bz_xdg_toplevel_dtor(struct wl_resource *data)
{
	struct bz_xdg_toplevel *toplevel_data = wl_resource_get_user_data(data);

	if (toplevel_data->wlsurface != nullptr) {
		toplevel_data->wlsurface->xdgtoplevel = nullptr;
	}

	free(toplevel_data);
}

const struct xdg_toplevel_interface bz_xdg_toplevel_implementation = {
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

static void bz_xdg_toplevel_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void bz_xdg_toplevel_set_parent(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *parent
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_toplevel.set_parent not implemented");
	// TODO
}

static void bz_xdg_toplevel_set_title(
	struct wl_client *client,
	struct wl_resource *resource,
	const char *title
) {
	struct bz_xdg_toplevel *toplevel_data = wl_resource_get_user_data(resource);
	toplevel_data->title = title;
}

static void bz_xdg_toplevel_set_app_id(
	struct wl_client *client,
	struct wl_resource *resource,
	const char *app_id
) {
	struct bz_xdg_toplevel *toplevel_data = wl_resource_get_user_data(resource);
	toplevel_data->app_id = app_id;
}

static void bz_xdg_toplevel_show_window_menu(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *seat,
	uint32_t serial,
	int32_t x,
	int32_t y
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_toplevel.show_window_menu not implemented");
	// TODO
}

static void bz_xdg_toplevel_move(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *seat,
	uint32_t serial
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_toplevel.move not implemented");
	// TODO
}

static void bz_xdg_toplevel_resize(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *seat,
	uint32_t serial,
	uint32_t edges
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_toplevel.resize not implemented");
	// TODO
}

static void bz_xdg_toplevel_set_max_size(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t width,
	int32_t height
) {
	// Parameter validation
	if (width < 0 || height < 0) {
		wl_resource_post_error(resource, XDG_TOPLEVEL_ERROR_INVALID_SIZE,
			"Neither width nor height can be negative.");
		bz_warn(BZ_LOG_WL_XDG_SHELL,
			"Negative width/height provided to set_max_size: %d, %d", width, height);
		return;
	}

	// Store our new max size
	struct bz_xdg_toplevel *xdg_toplevel = wl_resource_get_user_data(resource);
	if (xdg_toplevel->wlsurface->pending_state->max_size == nullptr) {
		struct bz_size *max_size = calloc(1, sizeof(*max_size));
		xdg_toplevel->wlsurface->pending_state->max_size = max_size;
	}
	*xdg_toplevel->wlsurface->pending_state->max_size = (struct bz_size){ .w = width, .h = height };
}

static void bz_xdg_toplevel_set_min_size(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t width,
	int32_t height
) {
	// Parameter validation
	if (width < 0 || height < 0) {
		wl_resource_post_error(resource, XDG_TOPLEVEL_ERROR_INVALID_SIZE,
			"Neither width nor height can be negative.");
		bz_warn(BZ_LOG_WL_XDG_SHELL,
			"Negative width/height provided to set_min_size: %d, %d", width, height);
		return;
	}

	// Store our new min size
	struct bz_xdg_toplevel *xdg_toplevel = wl_resource_get_user_data(resource);
	if (xdg_toplevel->wlsurface->pending_state->min_size == nullptr) {
		struct bz_size *min_size = calloc(1, sizeof(*min_size));
		xdg_toplevel->wlsurface->pending_state->min_size = min_size;
	}
	*xdg_toplevel->wlsurface->pending_state->min_size = (struct bz_size){ .w = width, .h = height };
}

static void bz_xdg_toplevel_set_maximized(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_toplevel.set_maximized not implemented");
	// TODO
}

static void bz_xdg_toplevel_unset_maximized(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_toplevel.unset_maximized not implemented");
	// TODO
}

static void bz_xdg_toplevel_set_fullscreen(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *output
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_toplevel.set_fullscreen not implemented");
	// TODO
}

static void bz_xdg_toplevel_unset_fullscreen(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_toplevel.unset_fullscreen not implemented");
	// TODO
}

static void bz_xdg_toplevel_set_minimized(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_toplevel.set_minimized not implemented");
	// TODO
}


// =================================================================================================
//  xdg_popup
// -------------------------------------------------------------------------------------------------

void bz_xdg_popup_dtor(struct wl_resource *data)
{
	struct bz_xdg_popup *popup = wl_resource_get_user_data(data);

	if (popup->wlsurface != nullptr) {
		popup->wlsurface->xdgpopup = nullptr;
	}

	free(popup);
}

const struct xdg_popup_interface bz_xdg_popup_implementation = {
	.destroy = bz_xdg_popup_destroy,
	.grab = bz_xdg_popup_grab,
	.reposition = bz_xdg_popup_reposition,
};

static void bz_xdg_popup_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void bz_xdg_popup_grab(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *seat,
	uint32_t serial
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_popup.grab() not implemented");
	// TODO
}

static void bz_xdg_popup_reposition(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *positioner,
	uint32_t token
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, "xdg_popup.reposition() not implemented");
	// TODO
}


