
#include "breezy/bz_wp_viewporter.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <wayland-server.h>
#include <viewporter-server-protocol.h>

#include "breezy/bz_logger.h"
#include "breezy/bz_wl_display.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

// -- wp_viewporter --

const struct wp_viewporter_interface bz_wp_viewporter_implementation;
static void bz_wp_viewporter_destroy(struct wl_client *client, struct wl_resource *resource);
static void bz_wp_viewporter_get_viewport(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface);

// -- wp_viewport --

void bz_wp_viewport_dtor(struct wl_resource *viewport);
const struct wp_viewport_interface bz_wp_viewport_implementation;
static void bz_wp_viewport_destroy(struct wl_client *client, struct wl_resource *resource);
static void bz_wp_viewport_set_source(struct wl_client *client, struct wl_resource *resource, wl_fixed_t x, wl_fixed_t y, wl_fixed_t width, wl_fixed_t height);
static void bz_wp_viewport_set_destination(struct wl_client *client, struct wl_resource *resource, int32_t width, int32_t height);


// =================================================================================================
//  wp_viewporter
// -------------------------------------------------------------------------------------------------

/** Gets executed whenever a client binds to wp_viewporter. */
void bz_wp_viewporter_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	bz_debug(BZ_LOG_WL_WP_VIEWPORTER, "Binding a client to wp_viewporter.");

	struct wl_resource *res = wl_resource_create(client, &wp_viewporter_interface, version, id);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(res, &bz_wp_viewporter_implementation, nullptr, nullptr);
}

const struct wp_viewporter_interface bz_wp_viewporter_implementation = {
	.destroy = bz_wp_viewporter_destroy,
	.get_viewport = bz_wp_viewporter_get_viewport,
};

static void bz_wp_viewporter_destroy(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_WP_VIEWPORTER, "wp_viewporter.destroy not implemented");
	// TODO
}

static void bz_wp_viewporter_get_viewport(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id,
	struct wl_resource *surface
) {
	// Make sure the given surface doesn't already have a viewport.
	struct bz_surface *surface_data = wl_resource_get_user_data(surface);
	if (surface_data->viewport != nullptr) {
		wl_resource_post_error(resource, WP_VIEWPORTER_ERROR_VIEWPORT_EXISTS,
			"Viewport already exists on provided surface.");
		bz_warn(BZ_LOG_WL_WP_VIEWPORTER, "Attempted to add multiple viewports to a surface");
		return; // Returning instead of goto for a better log message. (and nothing to cleanup)
	}

	// Allocate our user data
	struct bz_wp_viewport *viewport_data = calloc(1, sizeof(*viewport_data));
	if (viewport_data == nullptr) {
		wl_client_post_no_memory(client);
		goto viewport_alloc_failed;
	}

	// Create the resource, bound to the data
	struct wl_resource *res = wl_resource_create(
		client,
		&wp_viewport_interface,
		BZ_WP_VIEWPORT_VERSION,
		id
	);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		goto resource_failed;
	}
	wl_resource_set_implementation(
		res,
		&bz_wp_viewport_implementation,
		viewport_data,
		bz_wp_viewport_dtor
	);

	// Populate the region's user data
	viewport_data->resource = res;
	viewport_data->surface = surface_data;
	// And add it to the provided surface
	surface_data->viewport = viewport_data;

	// Everything succeeded!
	return;

	// Error cleanups
	resource_failed:
		free(viewport_data);
	viewport_alloc_failed:
		bz_error(BZ_LOG_WL_WP_VIEWPORTER, "Failed to construct a new viewport.");
}


// =================================================================================================
//  wp_viewport
// -------------------------------------------------------------------------------------------------

void bz_wp_viewport_dtor(struct wl_resource *viewport) {
	struct bz_wp_viewport *data = wl_resource_get_user_data(viewport);

	if (data->surface != nullptr) {
		data->surface->viewport = nullptr;

		// "The associated wl_surface's crop and scale state is removed. The change is
		//   applied on the next wl_surface.commit."
		if (data->surface->pending_state->vp_source != nullptr) {
			free(data->surface->pending_state->vp_source);
			data->surface->pending_state->vp_source = nullptr;
		}
		if (data->surface->pending_state->vp_dest != nullptr) {
			free(data->surface->pending_state->vp_dest);
			data->surface->pending_state->vp_dest = nullptr;
		}
	}

	free(data);
}

const struct wp_viewport_interface bz_wp_viewport_implementation = {
	.destroy = bz_wp_viewport_destroy,
	.set_source = bz_wp_viewport_set_source,
	.set_destination = bz_wp_viewport_set_destination,
};

static void bz_wp_viewport_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void bz_wp_viewport_set_source(
	struct wl_client *client,
	struct wl_resource *resource,
	wl_fixed_t x,
	wl_fixed_t y,
	wl_fixed_t width,
	wl_fixed_t height
) {
	struct bz_wp_viewport *vp_data = wl_resource_get_user_data(resource);
	struct bz_surface *surface_data = vp_data->surface;

	if (surface_data == nullptr) {
		bz_warn(BZ_LOG_WL_WP_VIEWPORTER, "Surface associated with this viewporter was already destroyed.");
		wl_resource_post_error(resource, WP_VIEWPORT_ERROR_NO_SURFACE,
			"Surface associated with this viewporter was already destroyed.");
		return;
	}

	// Special case to clear the source rectangle
	wl_fixed_t NEG_1 = wl_fixed_from_double(-1.0);
	if (x == NEG_1 && y == NEG_1 && width == NEG_1 && height == NEG_1) {
		if (surface_data->pending_state->vp_source != nullptr) {
			free(surface_data->pending_state->vp_source);
			surface_data->pending_state->vp_source = nullptr;
		}
		return;
	}

	double x_real = wl_fixed_to_double(x);
	double y_real = wl_fixed_to_double(y);
	double w_real = wl_fixed_to_double(width);
	double h_real = wl_fixed_to_double(height);

	// Parameter validation
	if (x_real < 0 || y_real < 0) {
		bz_warn(BZ_LOG_WL_WP_VIEWPORTER, "Provided x and y values were negative.");
		wl_resource_post_error(resource, WP_VIEWPORT_ERROR_BAD_VALUE,
			"Source x or y contained negative values.");
		return;
	}
	if (w_real <= 0 || h_real <= 0) {
		bz_warn(BZ_LOG_WL_WP_VIEWPORTER, "Provided width and height were not greater than 0.");
		wl_resource_post_error(resource, WP_VIEWPORT_ERROR_BAD_VALUE,
			"Source w or h must be greater than 0.");
		return;
	}

	// Save the values
	if (surface_data->pending_state->vp_source == nullptr) {
		surface_data->pending_state->vp_source = calloc(1, sizeof(*surface_data->pending_state->vp_source));
	}
	surface_data->pending_state->vp_source->x = x_real;
	surface_data->pending_state->vp_source->y = y_real;
	surface_data->pending_state->vp_source->w = w_real;
	surface_data->pending_state->vp_source->h = h_real;
}

static void bz_wp_viewport_set_destination(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t width,
	int32_t height
) {
	struct bz_wp_viewport *vp_data = wl_resource_get_user_data(resource);
	struct bz_surface *surface_data = vp_data->surface;

	if (surface_data == nullptr) {
		bz_warn(BZ_LOG_WL_WP_VIEWPORTER, "Surface associated with this viewporter was already destroyed.");
		wl_resource_post_error(resource, WP_VIEWPORT_ERROR_NO_SURFACE,
			"Surface associated with this viewporter was already destroyed.");
		return;
	}

	// Special case to clear the destination
	if (width == -1 && height == -1) {
		if (surface_data->pending_state->vp_dest != nullptr) {
			free(surface_data->pending_state->vp_dest);
			surface_data->pending_state->vp_dest = nullptr;
		}
		return;
	}

	// Parameter validation
	if (width <= 0 || height <= 0) {
		bz_warn(BZ_LOG_WL_WP_VIEWPORTER, "Provided width and height were not greater than 0.");
		wl_resource_post_error(resource, WP_VIEWPORT_ERROR_BAD_VALUE,
			"Width and height must both be greater than 0.");
		return;
	}

	// Save the values
	if (surface_data->pending_state->vp_dest == nullptr) {
		surface_data->pending_state->vp_dest = calloc(1, sizeof(*surface_data->pending_state->vp_dest));
	}
	surface_data->pending_state->vp_dest->w = width;
	surface_data->pending_state->vp_dest->h = height;
}
