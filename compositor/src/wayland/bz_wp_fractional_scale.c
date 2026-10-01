
#include "breezy/bz_wp_fractional_scale.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <wayland-server.h>
#include <fractional-scale-v1-server-protocol.h>

#include "breezy/bz_logger.h"
#include "breezy/bz_wl_display.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

// -- wp_fractional_scale_manager_v1 --

const struct wp_fractional_scale_manager_v1_interface bz_wp_fractional_scale_manager_v1_implementation;
static void bz_wp_fractional_scale_manager_v1_destroy(struct wl_client *client, struct wl_resource *resource);
static void bz_wp_fractional_scale_manager_v1_get_fractional_scale(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface);

// -- wp_fractional_scale_v1 --

void bz_wp_fractional_scale_v1_dtor(struct wl_resource *fractional_scale);
const struct wp_fractional_scale_v1_interface bz_wp_fractional_scale_v1_implementation;
static void bz_wp_fractional_scale_v1_destroy(struct wl_client *client, struct wl_resource *resource);


// =================================================================================================
//  wp_fractional_scale_manager_v1
// -------------------------------------------------------------------------------------------------

/** Gets executed whenever a client binds to wp_fractional_scale_manager. */
void bz_wp_fractional_scale_manager_v1_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	bz_debug(BZ_LOG_WL_WP_FRACTIONAL_SCALE,
		"Binding a client to wp_fractional_scale_manager_v1 with version %d.",
		version);

	struct wl_resource *res = wl_resource_create(client, &wp_fractional_scale_manager_v1_interface, version, id);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(res, &bz_wp_fractional_scale_manager_v1_implementation, nullptr, nullptr);
}

const struct wp_fractional_scale_manager_v1_interface bz_wp_fractional_scale_manager_v1_implementation = {
	.destroy = bz_wp_fractional_scale_manager_v1_destroy,
	.get_fractional_scale = bz_wp_fractional_scale_manager_v1_get_fractional_scale,
};

static void bz_wp_fractional_scale_manager_v1_destroy(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_WP_FRACTIONAL_SCALE, "wp_fractional_scale_manager_v1.destroy not implemented");
	// TODO
}

static void bz_wp_fractional_scale_manager_v1_get_fractional_scale(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id,
	struct wl_resource *surface
) {
	// Make sure the given surface doesn't already have a fractional scale.
	struct bz_surface *surface_data = wl_resource_get_user_data(surface);
	if (surface_data->fractional_scale != nullptr) {
		wl_resource_post_error(resource, WP_FRACTIONAL_SCALE_MANAGER_V1_ERROR_FRACTIONAL_SCALE_EXISTS,
			"Fractional scale already exists on provided surface.");
		bz_warn(BZ_LOG_WL_WP_FRACTIONAL_SCALE,
			"Attempted to add multiple fractional scales to a surface");
		return; // Returning instead of goto for a better log message. (and nothing to cleanup)
	}

	// Allocate our user data
	struct bz_wp_fractional_scale *frac_scale = calloc(1, sizeof(*frac_scale));
	if (frac_scale == nullptr) {
		wl_client_post_no_memory(client);
		goto frac_scale_alloc_failed;
	}

	// Create the resource, bound to the data
	struct wl_resource *res = wl_resource_create(
		client,
		&wp_fractional_scale_v1_interface,
		wl_resource_get_version(resource),
		id
	);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		goto resource_failed;
	}
	wl_resource_set_implementation(
		res,
		&bz_wp_fractional_scale_v1_implementation,
		frac_scale,
		bz_wp_fractional_scale_v1_dtor
	);

	// Populate the region's user data
	frac_scale->resource = res;
	frac_scale->surface = surface_data;
	// And add it to the provided surface
	surface_data->fractional_scale = frac_scale;

	// Everything succeeded!
	return;

	// Error cleanups
	resource_failed:
		free(frac_scale);
	frac_scale_alloc_failed:
		bz_error(BZ_LOG_WL_WP_FRACTIONAL_SCALE, "Failed to construct a new fractional scale.");
}


// =================================================================================================
//  wp_fractional_scale_v1
// -------------------------------------------------------------------------------------------------

void bz_wp_fractional_scale_v1_dtor(struct wl_resource *fractional_scale)
{
	struct bz_wp_fractional_scale *data = wl_resource_get_user_data(fractional_scale);

	if (data->surface != nullptr) {
		data->surface->fractional_scale = nullptr;
	}

	free(data);
}

const struct wp_fractional_scale_v1_interface bz_wp_fractional_scale_v1_implementation = {
	.destroy = bz_wp_fractional_scale_v1_destroy,
};

static void bz_wp_fractional_scale_v1_destroy(
	struct wl_client *client,
	struct wl_resource *resource
) {
	wl_resource_destroy(resource);
}
