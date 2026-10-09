
#include "breezy/bz_xdg_shell.h"

#include <stdlib.h>

#include <xdg-shell-server-protocol.h>

#include "breezy/bz_list.h"
#include "breezy/bz_wayland.h"
#include "breezy/bz_logger.h"
#include "breezy/bz_math.h"
#include "breezy/bz_wl_devices.h"
#include "breezy/bz_wl_display.h"
#include "breezy/bz_xdg_shell.h"


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
// Helpers
static struct bz_xdg_positioner *bz_positioner_clone(struct bz_xdg_positioner *pos);
static void bz_free_xdg_positioner(struct bz_xdg_positioner *positioner);

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
static struct bz_rect *bz_calculate_popup_position(struct bz_xdg_positioner *positioner);
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
// Helpers
struct bz_surface *bz_popup_get_associated_toplevel(struct bz_surface *surface);


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
	bz_free_xdg_positioner(wl_resource_get_user_data(data));
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
	// Parameter validation
	if (width <= 0 || height <= 0) {
		bz_error(BZ_LOG_WL_XDG_SHELL, "Provided positioner size cannot be non-positive.");
		wl_resource_post_error(resource, XDG_POSITIONER_ERROR_INVALID_INPUT,
			"Provided size (%d, %d) was non-positive.", width, height);
		return;
	}

	struct bz_xdg_positioner *positioner = wl_resource_get_user_data(resource);

	if (positioner->size == nullptr) {
		positioner->size = calloc(1, sizeof(*positioner->size));
	}
	positioner->size->w = width;
	positioner->size->h = height;
}

static void bz_xdg_positioner_set_anchor_rect(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t x,
	int32_t y,
	int32_t width,
	int32_t height
) {
	// Parameter validation
	if (width < 0 || height < 0) {
		bz_error(BZ_LOG_WL_XDG_SHELL, "Provided positioner anchor rectangle cannot have negative size.");
		wl_resource_post_error(resource, XDG_POSITIONER_ERROR_INVALID_INPUT,
			"Provided anchor rect size (%d, %d) was negative.", width, height);
		return;
	}

	struct bz_xdg_positioner *positioner = wl_resource_get_user_data(resource);

	if (positioner->anchor_rect == nullptr) {
		positioner->anchor_rect = calloc(1, sizeof(*positioner->anchor_rect));
	}
	positioner->anchor_rect->x = x;
	positioner->anchor_rect->y = y;
	positioner->anchor_rect->w = width;
	positioner->anchor_rect->h = height;
}

static void bz_xdg_positioner_set_anchor(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t anchor
) {
	// Parameter validation
	if (anchor > 8) {
		bz_error(BZ_LOG_WL_XDG_SHELL, "Provided anchor is not a valid value.");
		wl_resource_post_error(resource, XDG_POSITIONER_ERROR_INVALID_INPUT,
			"Provided anchor (%d) is not valid.", anchor);
		return;
	}

	struct bz_xdg_positioner *positioner = wl_resource_get_user_data(resource);
	positioner->anchor = anchor;
}

static void bz_xdg_positioner_set_gravity(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t gravity
) {
	// Parameter validation
	if (gravity > 8) {
		bz_error(BZ_LOG_WL_XDG_SHELL, "Provided gravity is not a valid value.");
		wl_resource_post_error(resource, XDG_POSITIONER_ERROR_INVALID_INPUT,
			"Provided gravity (%d) is not valid.", gravity);
		return;
	}

	struct bz_xdg_positioner *positioner = wl_resource_get_user_data(resource);
	positioner->gravity = gravity;
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
	struct bz_xdg_positioner *positioner = wl_resource_get_user_data(resource);
	if (positioner->offset == nullptr) {
		positioner->offset = calloc(1, sizeof(*positioner->offset));
	}
	positioner->offset->x = x;
	positioner->offset->y = y;
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

// ---  Helpers  -----------------------------------------------------------------------------------

static struct bz_xdg_positioner *bz_positioner_clone(struct bz_xdg_positioner *pos)
{
	if (pos == nullptr) { return nullptr; }

	struct bz_xdg_positioner *clone = calloc(1, sizeof(*clone));
	if (pos->size != nullptr) {
		clone->size = calloc(1, sizeof(*clone->size));
		*clone->size = *pos->size;
	}
	if (pos->anchor_rect != nullptr) {
		clone->anchor_rect = calloc(1, sizeof(*clone->anchor_rect));
		*clone->anchor_rect = *pos->anchor_rect;
	}
	clone->anchor = pos->anchor;
	clone->gravity = pos->gravity;
	if (pos->offset != nullptr) {
		clone->offset = calloc(1, sizeof(*clone->offset));
		*clone->offset = *pos->offset;
	}
	return clone;
}

static void bz_free_xdg_positioner(struct bz_xdg_positioner *positioner)
{
	if (positioner == nullptr) { return; }

	if (positioner->size != nullptr)        { free(positioner->size); }
	if (positioner->anchor_rect != nullptr) { free(positioner->anchor_rect); }
	if (positioner->offset != nullptr)      { free(positioner->offset); }

	free(positioner);
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

	// If the provided positioner isn't complete, raise an error
	struct bz_xdg_positioner *positioner_data = wl_resource_get_user_data(positioner);
	if (positioner_data->size == nullptr || positioner_data->anchor_rect == nullptr) {
		wl_resource_post_error(resource, XDG_WM_BASE_ERROR_INVALID_POSITIONER,
			"Positioner did not include required fields.");
		bz_error(BZ_LOG_WL_XDG_SHELL, "Positioner missing either size or anchor_rect.");
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
	struct bz_xdg_surface *xdg_parent = parent == nullptr ? nullptr : wl_resource_get_user_data(parent);
	struct bz_surface *parent_data = xdg_parent == nullptr ? nullptr : xdg_parent->wlsurface;
	popup->resource = res;
	popup->xdgsurface = xdgsurf;
	popup->wlsurface = xdgsurf->wlsurface;
	popup->parent = parent_data;
	popup->positioner = bz_positioner_clone(positioner_data);

	// Populate the surface's role.
	xdgsurf->wlsurface->role = BZ_SURF_ROLE_XDG_POPUP;
	xdgsurf->wlsurface->xdgpopup = popup;

	// Tell the parent about this popup
	// TODO-dl13-pu: Test this!
	if (parent_data != nullptr && parent_data->popups != nullptr) {
		bz_list_append(parent_data->popups, popup);
	}

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
		// struct bz_size rec_size = { .w = 1920, .h = 1080 };
		struct bz_size rec_size = { .w = 1280, .h = 720 };
		// struct bz_size rec_size = { .w = 800, .h = 600 };
		if (bzsurf->active_state->min_size) {
			if (bzsurf->active_state->min_size->w > 0) {
				rec_size.w = bz_max(rec_size.w, bzsurf->active_state->min_size->w);
			}
			if (bzsurf->active_state->min_size->h > 0) {
				rec_size.h = bz_max(rec_size.h, bzsurf->active_state->min_size->h);
			}
		}
		if (bzsurf->active_state->max_size) {
			if (bzsurf->active_state->max_size->w > 0) {
				rec_size.w = bz_min(rec_size.w, bzsurf->active_state->max_size->w);
			}
			if (bzsurf->active_state->max_size->h > 0) {
				rec_size.h = bz_min(rec_size.h, bzsurf->active_state->max_size->h);
			}
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
			configevt->toplevel.recommended_size.w,
			configevt->toplevel.recommended_size.h,
			// 0,
			// 0,
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
		configevt->popup.bounds = bz_calculate_popup_position(bzsurf->xdgpopup->positioner);
		// Popups treat these bounds as "relative to the parent".
		bzsurf->renderable.compositor_bounds.x = configevt->popup.bounds->x;
		bzsurf->renderable.compositor_bounds.y = configevt->popup.bounds->y;
		// (renderable's w/h are computed from window geometry or buff size when content update applied.)

		xdg_popup_send_configure(
			bzsurf->xdgpopup->resource,
			configevt->popup.bounds->x,
			configevt->popup.bounds->y,
			configevt->popup.bounds->w,
			configevt->popup.bounds->h
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

static struct bz_rect *bz_calculate_popup_position(struct bz_xdg_positioner *positioner)
{
	struct bz_rect *bounds = calloc(1, sizeof(*bounds));

	// We'll start by assuming it's the optimal/requested size
	bounds->w = positioner->size->w;
	bounds->h = positioner->size->h;

	// Next, we'll put its upper-left corner on its anchor point.
	// >> First, top-to-bottom placement
	switch (positioner->anchor) {
	case XDG_POSITIONER_ANCHOR_TOP_LEFT:    case XDG_POSITIONER_ANCHOR_TOP:    case XDG_POSITIONER_ANCHOR_TOP_RIGHT:
		bounds->y = positioner->anchor_rect->y;
		break;
	case XDG_POSITIONER_ANCHOR_LEFT:        case XDG_POSITIONER_ANCHOR_NONE:   case XDG_POSITIONER_ANCHOR_RIGHT:
		bounds->y = positioner->anchor_rect->y + positioner->anchor_rect->h/2;
		break;
	case XDG_POSITIONER_ANCHOR_BOTTOM_LEFT: case XDG_POSITIONER_ANCHOR_BOTTOM: case XDG_POSITIONER_ANCHOR_BOTTOM_RIGHT:
		bounds->y = positioner->anchor_rect->y + positioner->anchor_rect->h;
		break;
	}
	// >> Then, left-to-right placement
	switch (positioner->anchor) {
	case XDG_POSITIONER_ANCHOR_TOP_LEFT:  case XDG_POSITIONER_ANCHOR_LEFT:  case XDG_POSITIONER_ANCHOR_BOTTOM_LEFT:
		bounds->x = positioner->anchor_rect->x;
		break;
	case XDG_POSITIONER_ANCHOR_TOP:       case XDG_POSITIONER_ANCHOR_NONE:  case XDG_POSITIONER_ANCHOR_BOTTOM:
		bounds->x = positioner->anchor_rect->x + positioner->anchor_rect->w/2;
		break;
	case XDG_POSITIONER_ANCHOR_TOP_RIGHT: case XDG_POSITIONER_ANCHOR_RIGHT: case XDG_POSITIONER_ANCHOR_BOTTOM_RIGHT:
		bounds->x = positioner->anchor_rect->x + positioner->anchor_rect->w;
		break;
	}

	// Then shift it per its gravity
	switch (positioner->gravity) {
	case XDG_POSITIONER_GRAVITY_TOP_LEFT:    case XDG_POSITIONER_GRAVITY_TOP:    case XDG_POSITIONER_GRAVITY_TOP_RIGHT:
		bounds->y -= bounds->h;
		break;
	case XDG_POSITIONER_GRAVITY_LEFT:        case XDG_POSITIONER_GRAVITY_NONE:   case XDG_POSITIONER_GRAVITY_RIGHT:
		bounds->y -= bounds->h/2;
		break;
	}
	// Then shift it per its gravity
	switch (positioner->gravity) {
	case XDG_POSITIONER_GRAVITY_TOP_LEFT:  case XDG_POSITIONER_GRAVITY_LEFT:  case XDG_POSITIONER_GRAVITY_BOTTOM_LEFT:
		bounds->x -= bounds->w;
		break;
	case XDG_POSITIONER_GRAVITY_TOP:       case XDG_POSITIONER_GRAVITY_NONE:  case XDG_POSITIONER_GRAVITY_BOTTOM:
		bounds->x -= bounds->w/2;
		break;
	}

	// Lastly, add in any offset that might exist
	if (positioner->offset != nullptr) {
		bounds->x += positioner->offset->x;
		bounds->y += positioner->offset->y;
	}

	return bounds;
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
		free(configure->popup.bounds);
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

	// Sever the bidirectional connection between this popup and its parent
	if (popup->parent != nullptr) {
		bz_list_remove(popup->parent->popups, popup, nullptr);
		popup->parent = nullptr;
	}

	if (popup->positioner != nullptr) {
		bz_free_xdg_positioner(popup->positioner);
		popup->positioner = nullptr;
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

// ---  Helpers  -----------------------------------------------------------------------------------

struct bz_surface *bz_popup_get_associated_toplevel(struct bz_surface *surface)
{
	if (surface->role != BZ_SURF_ROLE_XDG_POPUP || surface->xdgpopup == nullptr) {
		return nullptr;
	}

	// Trace upwards to its toplevel to close
	struct bz_surface *parent = surface->xdgpopup->parent;
	while (
		parent->role == BZ_SURF_ROLE_XDG_POPUP &&
		parent->xdgpopup != nullptr &&
		parent->xdgpopup->parent != nullptr
	) {
		parent = parent->xdgpopup->parent;
	}

	if (parent != nullptr && parent->role == BZ_SURF_ROLE_XDG_TOPLEVEL && parent->xdgtoplevel) {
		return parent;
	}
	return nullptr;
}
