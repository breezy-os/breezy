
#include "breezy/bz_xdg_shell.h"

#include "xdg-shell-server-protocol.h"

#include "breezy/bz_logger.h"


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
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_wm_base.get_xdg_surface not implemented");
	// TODO
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

static void bz_xdg_surface_destroy(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_surface.destroy not implemented");
	// TODO
}

static void bz_xdg_surface_get_toplevel(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id
) {
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_surface.get_toplevel not implemented");
	// TODO
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
	bz_error(BZ_LOG_WL_XDG_SHELL, __FILE__, __LINE__, "xdg_surface.ack_configure not implemented");
	// TODO
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

