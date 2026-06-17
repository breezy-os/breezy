
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

// -- wl_surface --

static const struct wl_surface_interface bz_surface_implementation;
static void bz_surface_destroy(struct wl_client *client, struct wl_resource *resource);
static void bz_surface_attach(struct wl_client *client, struct wl_resource *resource, struct wl_resource *buffer, int32_t x, int32_t y);
static void bz_surface_damage(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height);
static void bz_surface_frame(struct wl_client *client, struct wl_resource *resource, uint32_t callback);
static void bz_surface_set_opaque_region(struct wl_client *client, struct wl_resource *resource, struct wl_resource *region);
static void bz_surface_set_input_region(struct wl_client *client, struct wl_resource *resource, struct wl_resource *region);
static void bz_surface_commit(struct wl_client *client, struct wl_resource *resource);
static void bz_surface_set_buffer_transform(struct wl_client *client, struct wl_resource *resource, int32_t transform);
static void bz_surface_set_buffer_scale(struct wl_client *client, struct wl_resource *resource, int32_t scale);
static void bz_surface_damage_buffer(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height);
static void bz_surface_offset(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y);


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


// =================================================================================================
//  wl_surface
// -------------------------------------------------------------------------------------------------

static const struct wl_surface_interface bz_surface_implementation = {
	.destroy = bz_surface_destroy,
	.attach = bz_surface_attach,
	.damage = bz_surface_damage,
	.frame = bz_surface_frame,
	.set_opaque_region = bz_surface_set_opaque_region,
	.set_input_region = bz_surface_set_input_region,
	.commit = bz_surface_commit,
	.set_buffer_transform = bz_surface_set_buffer_transform,
	.set_buffer_scale = bz_surface_set_buffer_scale,
	.damage_buffer = bz_surface_damage_buffer,
	.offset = bz_surface_offset,
};

static void bz_surface_destroy(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_surface.destroy not implemented");
	// TODO
}

static void bz_surface_attach(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *buffer,
	int32_t x,
	int32_t y
) {
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_surface.attach not implemented");
	// TODO
}

static void bz_surface_damage(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t x,
	int32_t y,
	int32_t width,
	int32_t height
) {
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_surface.damage not implemented");
	// TODO
}

static void bz_surface_frame(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t callback
) {
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_surface.frame not implemented");
	// TODO
}

static void bz_surface_set_opaque_region(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *region
) {
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_surface.set_opaque_region not implemented");
	// TODO
}

static void bz_surface_set_input_region(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *region
) {
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_surface.set_input_region not implemented");
	// TODO
}

static void bz_surface_commit(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_surface.commit not implemented");
	// TODO
}

static void bz_surface_set_buffer_transform(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t transform
) {
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_surface.set_buffer_transform not implemented");
	// TODO
}

static void bz_surface_set_buffer_scale(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t scale
) {
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_surface.set_buffer_scale not implemented");
	// TODO
}

static void bz_surface_damage_buffer(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t x,
	int32_t y,
	int32_t width,
	int32_t height
) {
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_surface.damage_buffer not implemented");
	// TODO
}

static void bz_surface_offset(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t x,
	int32_t y
) {
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_surface.offset not implemented");
	// TODO
}

