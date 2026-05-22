
#include "breezy/bz_wl_display.h"

#include <stdint.h>

#include <wayland-server.h>

#include "breezy/bz_logger.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

// -- wl_compositor --

static const struct wl_compositor_interface bz_compositor_implementation;
static void bz_compositor_create_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void bz_compositor_create_region(struct wl_client *client, struct wl_resource *resource, uint32_t id);

// -- wl_subcompositor --

static const struct wl_subcompositor_interface bz_subcompositor_implementation;
static void bz_subcompositor_destroy(struct wl_client *client, struct wl_resource *resource);
static void bz_subcompositor_get_subsurface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface, struct wl_resource *parent);


// =================================================================================================
//  wl_compositor
// -------------------------------------------------------------------------------------------------

/** Gets executed whenever a client binds to wl_compositor. */
void bz_compositor_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	bz_debug(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "Binding a client to wl_compositor.");

	struct wl_resource *res = wl_resource_create(client, &wl_compositor_interface, version, id);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(res, &bz_compositor_implementation, nullptr, nullptr);
}

static const struct wl_compositor_interface bz_compositor_implementation = {
	.create_surface = bz_compositor_create_surface,
	.create_region = bz_compositor_create_region,
};

static void bz_compositor_create_surface(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id
) {
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_compositor.create_surface not implemented");
	// TODO
}

static void bz_compositor_create_region(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id
) {
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_compositor.create_region not implemented");
	// TODO
}


// =================================================================================================
//  wl_subcompositor
// -------------------------------------------------------------------------------------------------

/** Gets executed whenever a client binds to wl_subcompositor. */
void bz_subcompositor_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	bz_debug(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "Binding a client to wl_subcompositor.");

	struct wl_resource *res = wl_resource_create(client, &wl_subcompositor_interface, version, id);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(res, &bz_subcompositor_implementation, nullptr, nullptr);
}

static const struct wl_subcompositor_interface bz_subcompositor_implementation = {
	.destroy = bz_subcompositor_destroy,
	.get_subsurface = bz_subcompositor_get_subsurface,
};

static void bz_subcompositor_destroy(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_subcompositor.destroy not implemented");
	// TODO
}

static void bz_subcompositor_get_subsurface(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id,
	struct wl_resource *surface,
	struct wl_resource *parent
) {
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_subcompositor.get_subsurface not implemented");
	// TODO
}
