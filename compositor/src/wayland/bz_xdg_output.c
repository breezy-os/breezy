
#include "breezy/bz_xdg_output.h"

#include "breezy/bz_xdg_shell.h"

#include <stdlib.h>

#include <xdg-output-unstable-v1-server-protocol.h>

#include "breezy/bz_list.h"
#include "breezy/bz_wayland.h"
#include "breezy/bz_logger.h"
#include "breezy/bz_math.h"
#include "breezy/bz_wl_devices.h"
#include "breezy/bz_wl_display.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

// -- zxdg_output_manager_v1 --

const struct zxdg_output_manager_v1_interface bz_zxdg_output_manager_v1_implementation;
static void bz_zxdg_output_manager_v1_destroy(struct wl_client *client, struct wl_resource *resource);
static void bz_zxdg_output_manager_v1_get_xdg_output(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *output);

// -- zxdg_output_v1 --

const struct zxdg_output_v1_interface bz_zxdg_output_v1_implementation;
static void bz_zxdg_output_v1_destroy(struct wl_client *client, struct wl_resource *resource);


// =================================================================================================
//  zxdg_output_manager_v1
// -------------------------------------------------------------------------------------------------

/** Gets executed whenever a client binds to xdg_output. */
void bz_zxdg_output_manager_v1_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	bz_debug(BZ_LOG_WL_XDG_OUTPUT, "Binding a client to xdg_output with version %d.", version);

	struct wl_resource *res = wl_resource_create(client, &zxdg_output_manager_v1_interface, version, id);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(res, &bz_zxdg_output_manager_v1_implementation, nullptr, nullptr);
}

const struct zxdg_output_manager_v1_interface bz_zxdg_output_manager_v1_implementation = {
	.destroy = bz_zxdg_output_manager_v1_destroy,
	.get_xdg_output = bz_zxdg_output_manager_v1_get_xdg_output,
};

static void bz_zxdg_output_manager_v1_destroy(
	struct wl_client *client,
	struct wl_resource *resource
) {
	bz_error(BZ_LOG_WL_XDG_OUTPUT, "xdg_output_manager.destroy not implemented");
	// TODO
}

static void bz_zxdg_output_manager_v1_get_xdg_output(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id,
	struct wl_resource *output
) {
	// Create the resource
	struct wl_resource *res = wl_resource_create(
		client,
		&zxdg_output_v1_interface,
		wl_resource_get_version(resource),
		id
	);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		goto resource_failed;
	}
	wl_resource_set_implementation(
		res,
		&bz_zxdg_output_v1_implementation,
		nullptr,
		nullptr
	);

	// Emit the things
	struct bz_output *output_data = wl_resource_get_user_data(output);
	zxdg_output_v1_send_logical_position(res, output_data->position.x, output_data->position.y);
	zxdg_output_v1_send_logical_size(res, output_data->size.w, output_data->size.h);
	if (wl_resource_get_version(res) >= ZXDG_OUTPUT_V1_NAME_SINCE_VERSION) {
		zxdg_output_v1_send_name(res, output_data->name);
	}
	if (wl_resource_get_version(res) >= ZXDG_OUTPUT_V1_DESCRIPTION_SINCE_VERSION) {
		zxdg_output_v1_send_description(res, output_data->description);
	}
	if (wl_resource_get_version(res) >= 3) {
		wl_output_send_done(output);
	} else {
		zxdg_output_v1_send_done(res);
	}

	return;

	resource_failed:
		bz_error(BZ_LOG_WL_XDG_OUTPUT, "Failed to create an XDG output.");
}


// =================================================================================================
//  zxdg_output_v1
// -------------------------------------------------------------------------------------------------

const struct zxdg_output_v1_interface bz_zxdg_output_v1_implementation = {
	.destroy = bz_zxdg_output_v1_destroy,
};

static void bz_zxdg_output_v1_destroy(
	struct wl_client *client,
	struct wl_resource *resource
) {
	wl_resource_destroy(resource);
}
