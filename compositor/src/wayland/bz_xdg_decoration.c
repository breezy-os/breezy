
#include "breezy/bz_xdg_decoration.h"

#include <stdlib.h>

#include <xdg-decoration-unstable-v1-server-protocol.h>

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

// -- zxdg_decoration_manager_v1 --

const struct zxdg_decoration_manager_v1_interface bz_zxdg_decoration_manager_v1_implementation;
static void bz_zxdg_decoration_manager_v1_destroy(struct wl_client *client, struct wl_resource *resource);
static void bz_zxdg_decoration_manager_v1_get_toplevel_decoration(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *toplevel);

// -- zxdg_toplevel_decoration_v1 --

void bz_xdg_toplevel_decoration_dtor(struct wl_resource *data);
const struct zxdg_toplevel_decoration_v1_interface bz_zxdg_toplevel_decoration_v1_implementation;
static void bz_zxdg_toplevel_decoration_v1_destroy(struct wl_client *client, struct wl_resource *resource);
static void bz_zxdg_toplevel_decoration_v1_set_mode(struct wl_client *client, struct wl_resource *resource, uint32_t mode);
static void bz_zxdg_toplevel_decoration_v1_unset_mode(struct wl_client *client, struct wl_resource *resource);


// =================================================================================================
//  zxdg_decoration_manager_v1
// -------------------------------------------------------------------------------------------------

/** Gets executed whenever a client binds to xdg_decoration_manager. */
void bz_zxdg_decoration_manager_v1_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	bz_debug(BZ_LOG_WL_XDG_DECORATION, "Binding a client to xdg_decoration with version %d.", version);

	struct wl_resource *res = wl_resource_create(client, &zxdg_decoration_manager_v1_interface, version, id);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(res, &bz_zxdg_decoration_manager_v1_implementation, nullptr, nullptr);
}

const struct zxdg_decoration_manager_v1_interface bz_zxdg_decoration_manager_v1_implementation = {
	.destroy = bz_zxdg_decoration_manager_v1_destroy,
	.get_toplevel_decoration = bz_zxdg_decoration_manager_v1_get_toplevel_decoration,
};

static void bz_zxdg_decoration_manager_v1_destroy(
	struct wl_client *client,
	struct wl_resource *resource
) {
	wl_resource_destroy(resource);
}

static void bz_zxdg_decoration_manager_v1_get_toplevel_decoration(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id,
	struct wl_resource *toplevel
) {
	struct bz_xdg_toplevel *toplevel_data = wl_resource_get_user_data(toplevel);

	if (toplevel_data->decoration != nullptr) {
		wl_resource_post_error(
			resource,
			ZXDG_TOPLEVEL_DECORATION_V1_ERROR_ALREADY_CONSTRUCTED,
			"XDG toplevel already has a decoration object."
		);
		bz_warn(BZ_LOG_WL_XDG_SHELL, "XDG toplevel already has a decoration object.");
		goto initial_checks_failed;
	}

	// "For objects of version 1, creating an xdg_toplevel_decoration from an xdg_toplevel which has
	//   a buffer attached or committed is a client error."
	if (wl_resource_get_version(resource) == 1) {
		struct bz_surface *surf = toplevel_data->wlsurface;
		if (surf->pending_state->buffer != nullptr || surf->active_state->buffer != nullptr) {
			wl_resource_post_error(
				resource,
				ZXDG_TOPLEVEL_DECORATION_V1_ERROR_UNCONFIGURED_BUFFER,
				"Decoration cannot be added to a toplevel with an existing buffer."
			);
			bz_warn(BZ_LOG_WL_XDG_SHELL, "Decoration cannot be added to a toplevel with an existing buffer.");
			goto initial_checks_failed;
		}
	}

	// Allocate our user data
	struct bz_xdg_toplevel_decoration *decoration = calloc(1, sizeof(*decoration));
	if (decoration == nullptr) {
		wl_client_post_no_memory(client);
		goto user_data_failed;
	}

	// Create the resource
	struct wl_resource *res = wl_resource_create(
		client,
		&zxdg_toplevel_decoration_v1_interface,
		wl_resource_get_version(resource),
		id
	);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		goto resource_failed;
	}
	wl_resource_set_implementation(
		res,
		&bz_zxdg_toplevel_decoration_v1_implementation,
		decoration,
		bz_xdg_toplevel_decoration_dtor
	);

	// Assign our user data
	decoration->resource = res;
	decoration->toplevel = toplevel_data;
	decoration->requested_mode = ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE; // Default to server.
	toplevel_data->decoration = decoration;

	return;

	resource_failed:
		free(decoration);
	user_data_failed:
	initial_checks_failed:
		bz_error(BZ_LOG_WL_XDG_DECORATION, "Failed to create an XDG toplevel decoration.");
}


// =================================================================================================
//  zxdg_toplevel_decoration_v1
// -------------------------------------------------------------------------------------------------

void bz_xdg_toplevel_decoration_dtor(struct wl_resource *resource)
{
	struct bz_xdg_toplevel_decoration *data = wl_resource_get_user_data(resource);
	data->toplevel->decoration = nullptr;
	free(data);
}

const struct zxdg_toplevel_decoration_v1_interface bz_zxdg_toplevel_decoration_v1_implementation = {
	.destroy = bz_zxdg_toplevel_decoration_v1_destroy,
	.set_mode = bz_zxdg_toplevel_decoration_v1_set_mode,
	.unset_mode = bz_zxdg_toplevel_decoration_v1_unset_mode,
};

static void bz_zxdg_toplevel_decoration_v1_destroy(
	struct wl_client *client,
	struct wl_resource *resource
) {
	wl_resource_destroy(resource);
}

static void bz_zxdg_toplevel_decoration_v1_set_mode(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t mode
) {
	if (!zxdg_toplevel_decoration_v1_mode_is_valid(mode, wl_resource_get_version(resource))) {
		wl_resource_post_error(resource, ZXDG_TOPLEVEL_DECORATION_V1_ERROR_INVALID_MODE,
			"Provided decoration mode was invalid.");
		bz_warn(BZ_LOG_WL_XDG_SHELL, "Provided decoration mode was invalid.");
		return;
	}

	struct bz_xdg_toplevel_decoration *decoration = wl_resource_get_user_data(resource);
	decoration->requested_mode = mode;
}

static void bz_zxdg_toplevel_decoration_v1_unset_mode(
	struct wl_client *client,
	struct wl_resource *resource
) {
	struct bz_xdg_toplevel_decoration *decoration = wl_resource_get_user_data(resource);
	decoration->requested_mode = ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE; // Default to server.
}
