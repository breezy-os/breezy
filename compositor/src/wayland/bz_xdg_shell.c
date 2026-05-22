
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


// =================================================================================================
//  wl_compositor
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
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "xdg_wm_base.destroy not implemented");
	// TODO
}

static void bz_xdg_wm_base_create_positioner(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id
) {
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "xdg_wm_base.create_positioner not implemented");
	// TODO
}

static void bz_xdg_wm_base_get_xdg_surface(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id,
	struct wl_resource *surface
) {
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "xdg_wm_base.get_xdg_surface not implemented");
	// TODO
}

static void bz_xdg_wm_base_pong(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t serial
) {
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "xdg_wm_base.pong not implemented");
	// TODO
}
