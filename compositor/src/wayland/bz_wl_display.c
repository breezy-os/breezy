
#include "breezy/bz_wl_display.h"

#include <stdint.h>
#include <stdlib.h>

#include <wayland-server.h>

#include "breezy/bz_graphics.h"
#include "breezy/bz_list.h"
#include "breezy/bz_logger.h"
#include "breezy/bz_math.h"
#include "breezy/bz_wayland.h"
#include "breezy/bz_window_management.h"
#include "breezy/bz_xdg_shell.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

// -- wl_compositor --

const struct wl_compositor_interface bz_compositor_implementation;
static void bz_compositor_create_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void bz_compositor_create_region(struct wl_client *client, struct wl_resource *resource, uint32_t id);
// Helpers
static struct bz_surface_state *bz_init_surface_state(void);
static void bz_free_surface_state(struct bz_surface_state *state);

// -- wl_subcompositor --

const struct wl_subcompositor_interface bz_subcompositor_implementation;
static void bz_subcompositor_destroy(struct wl_client *client, struct wl_resource *resource);
static void bz_subcompositor_get_subsurface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface, struct wl_resource *parent);
// Helpers
static bool bz_surface_stack_contains_surface(struct bz_surface *source, struct bz_surface *target);

// -- wl_region --

void bz_region_dtor(struct wl_resource *data);
const struct wl_region_interface bz_region_implementation;
static void bz_region_destroy(struct wl_client *client, struct wl_resource *resource);
static void bz_region_add(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height);
static void bz_region_subtract(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height);
// Helpers
static void bz_region_append_mutation(struct wl_client *client, struct bz_region *region_data, enum bz_region_op operation, int32_t x, int32_t y, int32_t width, int32_t height);

// -- wl_surface --

void bz_surface_dtor(struct wl_resource *data);
const struct wl_surface_interface bz_surface_implementation;
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
// Helpers
static void bz_write_surface_texture(struct bz_surface *surface_data);
static bool bz_is_effectively_sync(struct bz_surface *surface_data);
static void bz_walk_content_update_queue(struct bz_content_update *cu);
static bool bz_is_content_update_satisfied(struct bz_content_update *cu);
static struct bz_list *bz_apply_content_update(struct bz_content_update *cu);
static void bz_apply_subsurface_cu_state(struct bz_list *subsurface_states, struct bz_surface *surface);
static void bz_free_content_update(struct bz_content_update *content_update);

// -- wl_subsurface --

void bz_subsurface_dtor(struct wl_resource *subsurface);
const struct wl_subsurface_interface bz_subsurface_implementation;
static void bz_subsurface_destroy(struct wl_client *client, struct wl_resource *resource);
static void bz_subsurface_set_position(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y);
static void bz_subsurface_place_above(struct wl_client *client, struct wl_resource *resource, struct wl_resource *sibling);
static void bz_subsurface_place_below(struct wl_client *client, struct wl_resource *resource, struct wl_resource *sibling);
static void bz_subsurface_set_sync(struct wl_client *client, struct wl_resource *resource);
static void bz_subsurface_set_desync(struct wl_client *client, struct wl_resource *resource);
// Helpers
static struct bz_subsurface_state *bz_subsurface_get_pending_state(struct bz_subsurface *for_child);
static bool bz_subsurface_state_matches(void *list_item, void *match_data);


// =================================================================================================
//  wl_compositor
// -------------------------------------------------------------------------------------------------

/** Gets executed whenever a client binds to wl_compositor. */
void bz_compositor_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	bz_debug(BZ_LOG_WL_DISPLAY, "Binding a client to wl_compositor.");

	struct wl_resource *res = wl_resource_create(client, &wl_compositor_interface, version, id);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(res, &bz_compositor_implementation, nullptr, nullptr);
}

const struct wl_compositor_interface bz_compositor_implementation = {
	.create_surface = bz_compositor_create_surface,
	.create_region = bz_compositor_create_region,
};

static void bz_compositor_create_surface(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id
) {
	// Allocate our user data
	struct bz_surface *surface = calloc(1, sizeof(*surface));
	if (surface == nullptr) {
		wl_client_post_no_memory(client);
		goto surface_alloc_failed;
	}
	struct bz_surface_state *pending = bz_init_surface_state();
	if (pending == nullptr) {
		wl_client_post_no_memory(client);
		goto pending_state_alloc_failed;
	}
	struct bz_surface_state *active = bz_init_surface_state();
	if (active == nullptr) {
		wl_client_post_no_memory(client);
		goto active_state_alloc_failed;
	}
	struct bz_list *content_updates = bz_list_create();
	if (content_updates == nullptr) {
		wl_client_post_no_memory(client);
		goto content_updates_alloc_failed;
	}
	struct bz_list *surface_stack = bz_list_create();
	if (surface_stack == nullptr) {
		wl_client_post_no_memory(client);
		goto surface_stack_alloc_failed;
	}

	// Create the resource, bound to the data
	struct wl_resource *res = wl_resource_create(
		client,
		&wl_surface_interface,
		BZ_SURFACE_VERSION,
		id
	);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		goto resource_failed;
	}
	wl_resource_set_implementation(
		res,
		&bz_surface_implementation,
		surface,
		bz_surface_dtor
	);

	// Populate the surface's user data
	// const struct bz_client *client_data = wl_client_get_user_data(client);
	surface->resource = res;
	surface->role = BZ_SURF_ROLE_NONE;
	surface->pending_state = pending;
	surface->active_state = active;
	surface->content_updates = content_updates;
	surface->surface_stack = surface_stack;
	surface->renderable.position.x = 0;
	surface->renderable.position.y = 0;

	// Starts with only itself in its stack of surfaces.
	bz_list_append(surface->surface_stack, surface);

	// Everything succeeded!
	return;

	// Error cleanups
	resource_failed:
		bz_list_free(surface_stack, nullptr);
	surface_stack_alloc_failed:
		bz_list_free(content_updates, nullptr);
	content_updates_alloc_failed:
		bz_free_surface_state(active);
	active_state_alloc_failed:
		bz_free_surface_state(pending);
	pending_state_alloc_failed:
		free(surface);
	surface_alloc_failed:
		bz_error(BZ_LOG_WL_DISPLAY, "Failed to construct a new surface.");
}

static void bz_compositor_create_region(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id
) {
	// Allocate our user data
	struct bz_region *region_data = calloc(1, sizeof(*region_data));
	if (region_data == nullptr) {
		wl_client_post_no_memory(client);
		goto region_alloc_failed;
	}
	region_data->mutations = bz_list_create();
	if (region_data->mutations == nullptr) {
		wl_client_post_no_memory(client);
		goto mutations_alloc_failed;
	}

	// Create the resource, bound to the data
	struct wl_resource *res = wl_resource_create(
		client,
		&wl_region_interface,
		BZ_REGION_VERSION,
		id
	);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		goto resource_failed;
	}
	wl_resource_set_implementation(
		res,
		&bz_region_implementation,
		region_data,
		bz_region_dtor
	);

	// Populate the region's user data
	region_data->resource = res;

	// Everything succeeded!
	return;

	// Error cleanups
	resource_failed:
		bz_list_free(region_data->mutations, nullptr);
	mutations_alloc_failed:
		free(region_data);
	region_alloc_failed:
		bz_error(BZ_LOG_WL_DISPLAY, "Failed to construct a new region.");
}

// ---  Helpers  -----------------------------------------------------------------------------------

static struct bz_surface_state *bz_init_surface_state(void)
{
	struct bz_surface_state *state = calloc(1, sizeof(*state));
	if (state == nullptr) {
		goto state_alloc_failed;
	}

	state->frame_callbacks = bz_list_create();
	if (state->frame_callbacks == nullptr) {
		goto callback_list_failed;
	}

	state->subsurface_states = bz_list_create();
	if (state->subsurface_states == nullptr) {
		goto subsurface_list_failed;
	}

	// All is good!
	return state;

	// Error cleanups
	subsurface_list_failed:
		bz_list_free(state->frame_callbacks, nullptr);
	callback_list_failed:
		free(state);
	state_alloc_failed:
		bz_error(BZ_LOG_WL_DISPLAY, "Failed to create empty bz_surface_state.");
		return nullptr;
}

static void bz_free_surface_state(struct bz_surface_state *state)
{
	if (state == nullptr) { return; }

	if (state->frame_callbacks != nullptr) {
		bz_list_free(state->frame_callbacks, nullptr);
	}
	if (state->subsurface_states != nullptr) {
		bz_list_free(state->subsurface_states, free);
	}
	free(state);
}


// =================================================================================================
//  wl_subcompositor
// -------------------------------------------------------------------------------------------------

/** Gets executed whenever a client binds to wl_subcompositor. */
void bz_subcompositor_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	bz_debug(BZ_LOG_WL_DISPLAY, "Binding a client to wl_subcompositor.");

	struct wl_resource *res = wl_resource_create(client, &wl_subcompositor_interface, version, id);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(res, &bz_subcompositor_implementation, nullptr, nullptr);
}

const struct wl_subcompositor_interface bz_subcompositor_implementation = {
	.destroy = bz_subcompositor_destroy,
	.get_subsurface = bz_subcompositor_get_subsurface,
};

static void bz_subcompositor_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void bz_subcompositor_get_subsurface(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id,
	struct wl_resource *surface,
	struct wl_resource *parent
) {
	// Validate the provided surface
	struct bz_surface *surface_data = wl_resource_get_user_data(surface);
	if (surface_data->role != BZ_SURF_ROLE_NONE && surface_data->role != BZ_SURF_ROLE_WL_SUBSURFACE) {
		wl_resource_post_error(resource, WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE,
			"Surface role cannot be changed.");
		goto validation_error;
	}
	if (surface_data->subsurface != nullptr) {
		wl_resource_post_error(resource, WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE,
			"Surface already has an assigned subsurface.");
		goto validation_error;
	}

	// Validate the provided parent
	struct bz_surface *parent_data = wl_resource_get_user_data(parent);
	if (parent_data == surface_data) {
		wl_resource_post_error(resource, WL_SUBCOMPOSITOR_ERROR_BAD_PARENT,
			"Parent and child surfaces cannot be the same surface.");
		goto validation_error;
	}
	if (bz_surface_stack_contains_surface(surface_data, parent_data)) {
		wl_resource_post_error(resource, WL_SUBCOMPOSITOR_ERROR_BAD_PARENT,
			"Parent surface cannot descend from one of its children.");
		goto validation_error;
	}

	// Allocate our user data
	struct bz_subsurface *subsurface_data = calloc(1, sizeof(*subsurface_data));
	if (subsurface_data == nullptr) {
		wl_client_post_no_memory(client);
		goto subsurface_alloc_failed;
	}

	// Create the resource, bound to the data
	struct wl_resource *res = wl_resource_create(
		client,
		&wl_subsurface_interface,
		BZ_SUBSURFACE_VERSION,
		id
	);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		goto resource_failed;
	}
	wl_resource_set_implementation(
		res,
		&bz_subsurface_implementation,
		subsurface_data,
		bz_subsurface_dtor
	);

	// Populate the surface's user data
	subsurface_data->resource = res;
	subsurface_data->surface = surface_data;
	subsurface_data->parent = parent_data;
	subsurface_data->is_sync = true;

	// And update other, related data
	surface_data->role = BZ_SURF_ROLE_WL_SUBSURFACE;
	surface_data->subsurface = subsurface_data;
	bz_list_append(parent_data->surface_stack, surface_data);

	// Everything succeeded!
	return;

	// Error cleanups
	resource_failed:
		free(subsurface_data);
	subsurface_alloc_failed:
	validation_error:
		bz_error(BZ_LOG_WL_DISPLAY, "Failed to construct a new subsurface.");
}

static bool bz_surface_stack_contains_surface(
	struct bz_surface *source,
	struct bz_surface *target
) {
	struct bz_surface *current; bz_list_foreach(current, source->surface_stack) {
		if (current == source) { continue; }
		if (current == target || bz_surface_stack_contains_surface(current, target)) {
			return true;
		}
	}
	return false;
}


// =================================================================================================
//  wl_region
// -------------------------------------------------------------------------------------------------

void bz_region_dtor(struct wl_resource *data)
{
	struct bz_region *region_data = wl_resource_get_user_data(data);
	bz_list_free(region_data->mutations, free);
	free(region_data);
}

const struct wl_region_interface bz_region_implementation = {
	.destroy = bz_region_destroy,
	.add = bz_region_add,
	.subtract = bz_region_subtract,
};

static void bz_region_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void bz_region_add(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t x,
	int32_t y,
	int32_t width,
	int32_t height
) {
	struct bz_region *region_data = wl_resource_get_user_data(resource);
	bz_region_append_mutation(client, region_data, OP_ADD, x, y, width, height);
}

static void bz_region_subtract(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t x,
	int32_t y,
	int32_t width,
	int32_t height
) {
	struct bz_region *region_data = wl_resource_get_user_data(resource);
	bz_region_append_mutation(client, region_data, OP_SUBTRACT, x, y, width, height);
}

static void bz_region_append_mutation(
	struct wl_client *client,
	struct bz_region *region_data,
	enum bz_region_op operation,
	int32_t x,
	int32_t y,
	int32_t width,
	int32_t height
) {
	// Create the mutation
	struct bz_region_mutation *mutation = calloc(1, sizeof(*mutation));
	if (mutation == nullptr) {
		wl_client_post_no_memory(client);
		goto calloc_failed;
	}

	// Populate the mutation
	mutation->op = operation;
	mutation->x = x;
	mutation->y = y;
	mutation->w = width;
	mutation->h = height;

	// Add it to our region's list
	if (bz_list_append(region_data->mutations, mutation) < 0) {
		wl_client_post_no_memory(client);
		goto append_failed;
	}

	return;

	append_failed:
		free(mutation);
	calloc_failed:
		bz_error(BZ_LOG_WL_DISPLAY, "Failed to add or subtract rectangle to wl_region.");
}


// =================================================================================================
//  wl_surface
// -------------------------------------------------------------------------------------------------

void bz_surface_dtor(struct wl_resource *data)
{
	struct bz_surface *bzsurf = wl_resource_get_user_data(data);

	// If a wl_surface was destroyed before its (subsurface) role, sever ties with our parent.
	if (bzsurf->role == BZ_SURF_ROLE_WL_SUBSURFACE && bzsurf->subsurface != nullptr) {
		bz_list_remove(bzsurf->subsurface->parent->surface_stack, bzsurf, nullptr);
	}

	// If it's an XDG surface, sever the link between the two.
	if (bzsurf->xdgsurface != nullptr) {
		bzsurf->xdgsurface->wlsurface = nullptr;
	}
	if (bzsurf->role == BZ_SURF_ROLE_XDG_TOPLEVEL && bzsurf->xdgtoplevel != nullptr) {
		bzsurf->xdgtoplevel->wlsurface = nullptr;
	} else if (bzsurf->role == BZ_SURF_ROLE_XDG_POPUP && bzsurf->xdgpopup != nullptr) {
		bzsurf->xdgpopup->wlsurface = nullptr;
	}

	// Clear the surface_stack, but first unlink all the children from this surface.
	struct bz_surface *subsurface; bz_list_foreach(subsurface, bzsurf->surface_stack) {
		if (subsurface == bzsurf) { continue; }
		subsurface->subsurface->parent = nullptr;
	}
	bz_list_free(bzsurf->surface_stack, nullptr);

	// Free our allocated state
	bz_free_surface_state(bzsurf->pending_state);
	bz_free_surface_state(bzsurf->active_state);
	struct bz_content_update *cu;
	while ((cu = bz_list_shift(bzsurf->content_updates)) != nullptr) {
		bz_free_content_update(cu);
	}
	bz_list_free(bzsurf->content_updates, nullptr);

	glDeleteTextures(1, &bzsurf->renderable.texture);

	free(bzsurf);

	// Not sure I really like this here... keep an eye out for a better place to unrender clients.
	struct wl_client *client = wl_resource_get_client(data);
	struct bz_client *client_data = wl_client_get_user_data(client);
	bz_graphics_schedule_render(client_data->breezy);
}

const struct wl_surface_interface bz_surface_implementation = {
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
	struct bz_surface *surface_data = wl_resource_get_user_data(resource);

	// Role must be destroyed first. Otherwise, "defunct_role_object" error
	if (
		(surface_data->role == BZ_SURF_ROLE_XDG_TOPLEVEL && surface_data->xdgtoplevel != nullptr) ||
		(surface_data->role == BZ_SURF_ROLE_XDG_POPUP && surface_data->xdgpopup != nullptr) ||
		(surface_data->role == BZ_SURF_ROLE_WL_SUBSURFACE && surface_data->subsurface != nullptr)
	) {
		bz_error(BZ_LOG_WL_DISPLAY, "Surface role must be destroyed before the surface.");
		wl_resource_post_error(resource, WL_SURFACE_ERROR_DEFUNCT_ROLE_OBJECT,
			"Surface role must be destroyed before the surface.");
		return;
	}

	wl_resource_destroy(resource);
}

static void bz_surface_attach(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *buffer,
	int32_t x,
	int32_t y
) {
	struct bz_surface *surf_data = wl_resource_get_user_data(resource);
	surf_data->pending_state->buffer = buffer;
	// TODO: accommodate x and y. Consider how this would apply for both XDG surfaces and cursors.
}

static void bz_surface_damage(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t x,
	int32_t y,
	int32_t width,
	int32_t height
) {
	bz_error(BZ_LOG_WL_DISPLAY, "wl_surface.damage not implemented");
	// TODO
}

static void bz_surface_frame(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t callback
) {
	// Create the resource, bound to the data
	struct wl_resource *res = wl_resource_create(
		client,
		&wl_callback_interface,
		BZ_CALLBACK_VERSION,
		callback
	);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		goto resource_failed;
	}

	// Add this callback to our surface for tracking.
	const struct bz_surface *surf_data = wl_resource_get_user_data(resource);
	bz_list_append(surf_data->pending_state->frame_callbacks, res);

	// Everything succeeded!
	return;

	resource_failed:
		bz_error(BZ_LOG_WL_DISPLAY, "Failed to construct a new frame callback.");
}

static void bz_surface_set_opaque_region(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *region
) {
	bz_error(BZ_LOG_WL_DISPLAY, "wl_surface.set_opaque_region not implemented");
	// TODO
}

static void bz_surface_set_input_region(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *region
) {
	bz_error(BZ_LOG_WL_DISPLAY, "wl_surface.set_input_region not implemented");
	// TODO
}

static void bz_surface_commit(struct wl_client *client, struct wl_resource *resource)
{
	struct bz_surface *bzsurf = wl_resource_get_user_data(resource);

	// After creating an XDG role, the client must perform an initial commit w/o a buffer. The
	//   compositor will reply with initial wl_surface state, followed by xdg_surface.configure.
	//   The client must acknowledge it, and can then proceed to attach a buffer / map the surface.
	if ((bzsurf->role == BZ_SURF_ROLE_XDG_TOPLEVEL || bzsurf->role == BZ_SURF_ROLE_XDG_POPUP)
			&& bzsurf->pending_state->buffer == nullptr
			// Also checking active because user might commit a NULL buffer to clear the surface.
			&& bzsurf->active_state->buffer == nullptr
	) {
		if (bzsurf->xdgsurface == nullptr) {
			bz_error(BZ_LOG_WL_DISPLAY,
				"XDG surface object was null on wl_surface after being assigned. "
				"THIS SHOULD NEVER HAPPEN.");
			wl_client_post_implementation_error(client,
				"XDG surface object does not exist on the wl_surface.");
			return;
		}
		bz_xdg_surface_initial_configure(client, bzsurf);
		return;
	}

	// Copy over our pending state into a content update
	struct bz_surface_state *cu_state = bz_init_surface_state();
	if (cu_state == nullptr) {
		wl_client_post_no_memory(client);
		goto cu_state_alloc_failed;
	}
	cu_state->buffer = bzsurf->pending_state->buffer;
	bz_list_move_to_end(bzsurf->pending_state->frame_callbacks, cu_state->frame_callbacks);
	bz_list_move_to_end(bzsurf->pending_state->subsurface_states, cu_state->subsurface_states);

	// Create that content update
	struct bz_content_update *cu = calloc(1, sizeof(*cu));
	if (cu == nullptr) {
		wl_client_post_no_memory(client);
		goto cu_alloc_failed;
	}
	cu->surface = bzsurf;
	cu->state = cu_state;
	cu->is_sync = bz_is_effectively_sync(bzsurf);
	cu->dependencies = bz_list_create();
	if (cu->dependencies == nullptr) {
		wl_client_post_no_memory(client);
		goto cu_claims_alloc_failed;
	}

	// Add it to our list of content updates, taking note of the CU that comes before it.
	struct bz_content_update *preceding_cu = bzsurf->content_updates->tail
		? bzsurf->content_updates->tail->data
		: nullptr;
	bz_list_append(bzsurf->content_updates, cu);

	// This CU depends on the item before it in the queue
	if (preceding_cu != nullptr) {
		preceding_cu->depended_on_by = cu;
		bz_list_append(cu->dependencies, preceding_cu);
	}
	// This CU claims on all SCU's at the end of each direct child's queue.
	struct bz_surface *child; bz_list_foreach(child, bzsurf->surface_stack) {
		if (child == bzsurf) { continue; } // Ignore self.
		if (child->content_updates->tail != nullptr) {
			struct bz_content_update *child_cu = child->content_updates->tail->data;
			if (child_cu->is_sync && child_cu->claimed_by == nullptr) {
				child_cu->claimed_by = cu;
				bz_list_append(cu->dependencies, child_cu);
			}
		}
	}

	// Walk the content update queue to try and resolve any candidates. If the current commit
	//   created an SCU, then there's no reason to walk because it didn't create a candidate.
	if (bzsurf->role != BZ_SURF_ROLE_WL_SUBSURFACE || !bzsurf->subsurface->is_sync) {
		bz_walk_content_update_queue(cu);
	}

	return;

	cu_claims_alloc_failed:
		free(cu);
	cu_alloc_failed:
		bz_free_surface_state(cu_state);
	cu_state_alloc_failed:
		bz_error(BZ_LOG_WL_DISPLAY, "Failed to commit the wl_surface.");
}

static void bz_surface_set_buffer_transform(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t transform
) {
	bz_error(BZ_LOG_WL_DISPLAY, "wl_surface.set_buffer_transform not implemented");
	// TODO
}

static void bz_surface_set_buffer_scale(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t scale
) {
	bz_error(BZ_LOG_WL_DISPLAY, "wl_surface.set_buffer_scale not implemented");
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
	bz_error(BZ_LOG_WL_DISPLAY, "wl_surface.damage_buffer not implemented");
	// TODO
}

static void bz_surface_offset(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t x,
	int32_t y
) {
	bz_error(BZ_LOG_WL_DISPLAY, "wl_surface.offset not implemented");
	// TODO
	// TODO: Make sure to ignore this request for subsurfaces.
}

// ---  Helpers  -----------------------------------------------------------------------------------

static void bz_write_surface_texture(struct bz_surface *surface_data)
{
	// Initialize the texture
	if (surface_data->renderable.texture == 0) {
		bz_info(BZ_LOG_WL_DEVICES, "Initializing OpenGL texture for surface.");
		glGenTextures(1, &surface_data->renderable.texture);
		glBindTexture(GL_TEXTURE_2D, surface_data->renderable.texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	// Make sure we have a buffer ready to go
	if (surface_data->active_state->buffer == nullptr) {
		return;
	}

	// Copy in the surface
	struct wl_shm_buffer *shmbuf = wl_shm_buffer_get(surface_data->active_state->buffer);
	wl_shm_buffer_begin_access(shmbuf);
	// --- Buffer Access Begin ---------------------------------------------------------------------

	uint32_t *data = wl_shm_buffer_get_data(shmbuf);
	int32_t width = wl_shm_buffer_get_width(shmbuf);
	int32_t height = wl_shm_buffer_get_height(shmbuf);
	glBindTexture(GL_TEXTURE_2D, surface_data->renderable.texture);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,                // mipmap level
		GL_BGRA_EXT,      // format
		width,
		height,
		0,                // border
		GL_BGRA_EXT,      // format
		GL_UNSIGNED_BYTE, // type
		data              // pointer to new data
	);

	// --- Buffer Access End -----------------------------------------------------------------------
	wl_shm_buffer_end_access(shmbuf);
}

static bool bz_is_effectively_sync(struct bz_surface *surface_data)
{
	// Only subsurfaces can be sync.
	if (surface_data->role != BZ_SURF_ROLE_WL_SUBSURFACE) {
		return false;
	}
	// If it's sync ... then it's sync.
	if (surface_data->subsurface->is_sync) {
		return true;
	}
	// If any of its parents are sync, then it's sync.
	return bz_is_effectively_sync(surface_data->subsurface->parent);
}

static void bz_walk_content_update_queue(struct bz_content_update *cu)
{
	// Walk upwards to find the root-level DCU
	struct bz_content_update *root_cu = nullptr;
	struct bz_content_update *parent_cu = cu;
	while (parent_cu != nullptr) {
		root_cu = parent_cu;
		parent_cu = (parent_cu->claimed_by != nullptr)
			? parent_cu->claimed_by
			: parent_cu->depended_on_by;
	}
	if (root_cu->is_sync) {
		return; // We hit a dead end. There is no parent DCU, just some SCUs.
	}
	if (root_cu->surface->content_updates->head->data != root_cu) {
		return; // This graph depends on another graph to complete first.
	}

	// Walk all the branches claimed by the root DCU, checking if their dependencies are satisfied.
	if (bz_is_content_update_satisfied(root_cu)) {
		struct wl_client *client = wl_resource_get_client(cu->surface->resource);
		struct bz_client *client_data = wl_client_get_user_data(client);

		// Apply our content updates, and then prune them out of our treelists.
		struct bz_list *processed_CUs = bz_apply_content_update(root_cu);
		struct bz_content_update *curr_cu;
		while ((curr_cu = bz_list_shift(processed_CUs)) != nullptr) {
			bz_free_content_update(curr_cu);
		}
		bz_list_free(processed_CUs, nullptr);

		// Schedule a repaint
		bz_graphics_schedule_render(client_data->breezy);
	}
}

/**
 * A content update is satisfied if:
 *   (1) all of its dependencies are satisfied, and...
 *   (2) it has no blockers.
 */
static bool bz_is_content_update_satisfied(struct bz_content_update *cu)
{
	// First, check to make sure all its dependencies are satisfied.
	struct bz_content_update *child_cu; bz_list_foreach(child_cu, cu->dependencies) {
		if (!bz_is_content_update_satisfied(child_cu)) {
			return false;
		}
	}

	// Synchronized commits that haven't been claimed are blockers.
	if (cu->is_sync && cu->claimed_by == nullptr && cu->depended_on_by == nullptr) {
		return false;
	}

	// If we made it here, all constraints have been satisfied!
	return true;
}

/**
 * Applies the full tree of content updates, starting with the deepest CUs (leaves) first and
 * working backwards to the root. It's assumed that the full tree of the provided CU has all of
 * its constraints fully satisfied already.
 *
 * All CUs that were applied are returned in a single list in the order that they were applied.
 */
static struct bz_list *bz_apply_content_update(struct bz_content_update *cu)
{
	struct bz_list *applied_CUs = bz_list_create();

	// First, apply all the CUs this depends on, starting at the leaf nodes.
	struct bz_content_update *cu_node; bz_list_foreach(cu_node, cu->dependencies) {
		struct bz_list *dep_CUs = bz_apply_content_update(cu_node);
		bz_list_move_to_end(dep_CUs, applied_CUs);
		bz_list_free(dep_CUs, nullptr);
	}

	// Then, apply the current CU. (This is also the base case.)
	struct bz_surface *surface = cu->surface;
	surface->active_state->buffer = cu->state->buffer;
	bz_list_move_to_end(cu->state->frame_callbacks, surface->active_state->frame_callbacks);
	bz_apply_subsurface_cu_state(cu->state->subsurface_states, surface);

	// When a CU is applied, the buffer is applied first. This means all other coordinates are
	//   relative to the new buffer. If there is no new buffer, the coordinates are relative to
	//   the previous CU.
	if (surface->active_state->buffer != nullptr) {
		// Update our OpenGL texture
		bz_write_surface_texture(surface);

		// Update our displayed window size
		struct wl_shm_buffer *shmbuf = wl_shm_buffer_get(surface->active_state->buffer);
		surface->renderable.size.w = wl_shm_buffer_get_width(shmbuf);
		surface->renderable.size.h = wl_shm_buffer_get_height(shmbuf);

		// Since the buffer contents are saved on our OpenGL texture, release the buffer.
		// TODO-dl??: This might need to be deferred for the DMA-BUF protocol..?
		wl_buffer_send_release(surface->active_state->buffer);

		// Since the surface was just mapped (and therefore displayed), let's focus the surface
		// TODO-dl?? : Also consider wlr_layer_surfaces
		if (surface->role == BZ_SURF_ROLE_XDG_TOPLEVEL || surface->role == BZ_SURF_ROLE_XDG_POPUP) {
			struct wl_client *client = wl_resource_get_client(surface->resource);
			struct bz_client *client_data = wl_client_get_user_data(client);
			bz_mgmt_open_window(&client_data->breezy->window_mgmt, surface);
		}
	}

	bz_list_append(applied_CUs, cu);
	return applied_CUs;
}

static void bz_apply_subsurface_cu_state(
	struct bz_list *subsurface_states,
	struct bz_surface *surface
) {
	struct bz_subsurface_state *subsurf_state; bz_list_foreach(subsurf_state, subsurface_states) {
		// Update the relative render position
		subsurf_state->subsurface->surface->renderable.position.x = subsurf_state->position.x;
		subsurf_state->subsurface->surface->renderable.position.y = subsurf_state->position.y;
		// Update the z-index relative to its other siblings
		if (subsurf_state->sibling != nullptr) {
			if (subsurf_state->placement == BZ_SUBSURFACE_PLACE_BELOW) {
				// Subsurface should be placed BEFORE the sibling in parent's stack
				bz_list_remove(surface->surface_stack, subsurf_state->subsurface, nullptr);
				struct bz_surface *after_sibling = nullptr;
				struct bz_surface *data; bz_list_foreach(data, surface->surface_stack) {
					if (data == subsurf_state->sibling) {
						break;
					}
					after_sibling = data;
				}
				bz_list_insert(surface->surface_stack, subsurf_state->subsurface, after_sibling);
			} else if (subsurf_state->placement == BZ_SUBSURFACE_PLACE_ABOVE) {
				// Surface should be placed AFTER the sibling in parent's stack
				bz_list_remove(surface->surface_stack, subsurf_state->subsurface, nullptr);
				bz_list_insert(surface->surface_stack, subsurf_state->subsurface, subsurf_state->sibling);
			}
		}
	}
}

static void bz_free_content_update(struct bz_content_update *content_update)
{
	struct bz_content_update *cu = content_update;

	if (cu->claimed_by != nullptr) {
		bz_list_remove(cu->claimed_by->dependencies, cu, nullptr);
	}
	if (cu->depended_on_by != nullptr) {
		bz_list_remove(cu->depended_on_by->dependencies, cu, nullptr);
	}
	bz_list_remove(cu->surface->content_updates, cu, nullptr);
	bz_list_free(cu->dependencies, nullptr);

	bz_free_surface_state(cu->state);

	free(cu);
}


// =================================================================================================
//  wl_subsurface
// -------------------------------------------------------------------------------------------------

void bz_subsurface_dtor(struct wl_resource *subsurface)
{
	struct bz_subsurface *subsurface_data = wl_resource_get_user_data(subsurface);

	// Sever the ties to its parent (if not already done by the parent surface dtor).
	struct bz_surface *parent = subsurface_data->parent;
	if (parent != nullptr) {
		bz_list_remove(parent->surface_stack, subsurface_data->surface, nullptr);
		struct bz_list *subsurface_states = parent->pending_state->subsurface_states;
		bz_list_filter(subsurface_states, subsurface_data, bz_subsurface_state_matches, free);
		struct bz_content_update *cu; bz_list_foreach(cu, parent->content_updates) {
			struct bz_list *subsurface_states = cu->state->subsurface_states;
			bz_list_filter(subsurface_states, subsurface_data, bz_subsurface_state_matches, free);
		}
	}

	subsurface_data->surface->subsurface = nullptr;

	free(subsurface_data);
}

const struct wl_subsurface_interface bz_subsurface_implementation = {
	.destroy = bz_subsurface_destroy,
	.set_position = bz_subsurface_set_position,
	.place_above = bz_subsurface_place_above,
	.place_below = bz_subsurface_place_below,
	.set_sync = bz_subsurface_set_sync,
	.set_desync = bz_subsurface_set_desync,
};

const struct wl_subsurface_interface bz_subsurface_implementation;

static void bz_subsurface_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void bz_subsurface_set_position(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t x,
	int32_t y
) {
	struct bz_subsurface *subsurface_data = wl_resource_get_user_data(resource);
	struct bz_subsurface_state *state = bz_subsurface_get_pending_state(subsurface_data);

	if (state == nullptr) {
		wl_resource_post_no_memory(resource);
		bz_error(BZ_LOG_WL_DISPLAY, "Failed to look up state on parent surface.");
		return;
	}

	// (No restrictions on these. Negative values are allowed.)
	state->position.x = x;
	state->position.y = y;
}

static void bz_subsurface_place_above(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *sibling
) {
	struct bz_subsurface *subsurface_data = wl_resource_get_user_data(resource);
	struct bz_surface *sibling_data = wl_resource_get_user_data(sibling);

	// Make sure the sibling is not itself
	if (subsurface_data->surface == sibling_data) {
		wl_resource_post_error(resource, WL_SUBSURFACE_ERROR_BAD_SURFACE,
			"Sibling cannot equal self.");
		goto validation_failure;
	}

	// Make sure the given sibling is in the parent's surface stack.
	if (!bz_list_contains(subsurface_data->parent->surface_stack, sibling_data)) {
		wl_resource_post_error(resource, WL_SUBSURFACE_ERROR_BAD_SURFACE,
			"Sibling not found in parent surface.");
		goto validation_failure;
	}

	// Look up the subsurface's state on the parent
	struct bz_subsurface_state *state = bz_subsurface_get_pending_state(subsurface_data);
	if (state == nullptr) {
		wl_resource_post_no_memory(resource);
		return;
	}

	// Make the change
	state->placement = BZ_SUBSURFACE_PLACE_ABOVE;
	state->sibling = sibling_data;

	validation_failure:
		bz_error(BZ_LOG_WL_DISPLAY, "Failed to place subsurface above sibling.");
}

static void bz_subsurface_place_below(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *sibling
) {
	struct bz_subsurface *subsurface_data = wl_resource_get_user_data(resource);
	struct bz_surface *sibling_data = wl_resource_get_user_data(sibling);

	// Make sure the sibling is not itself
	if (subsurface_data->surface == sibling_data) {
		wl_resource_post_error(resource, WL_SUBSURFACE_ERROR_BAD_SURFACE,
			"Sibling cannot equal self.");
		goto validation_failure;
	}

	// Make sure the given sibling is in the parent's surface stack.
	if (!bz_list_contains(subsurface_data->parent->surface_stack, sibling_data)) {
		wl_resource_post_error(resource, WL_SUBSURFACE_ERROR_BAD_SURFACE,
			"Sibling not found in parent surface.");
		goto validation_failure;
	}

	// Look up the subsurface's state on the parent
	struct bz_subsurface_state *state = bz_subsurface_get_pending_state(subsurface_data);
	if (state == nullptr) {
		wl_resource_post_no_memory(resource);
		return;
	}

	// Make the change
	state->placement = BZ_SUBSURFACE_PLACE_BELOW;
	state->sibling = sibling_data;

	validation_failure:
		bz_error(BZ_LOG_WL_DISPLAY, "Failed to place subsurface below sibling.");
}

static void bz_subsurface_set_sync(struct wl_client *client, struct wl_resource *resource)
{
	struct bz_subsurface *subsurface_data = wl_resource_get_user_data(resource);

	// Already sync.
	if (subsurface_data->is_sync) {
		return;
	}

	subsurface_data->is_sync = true;
}

static void bz_subsurface_set_desync(struct wl_client *client, struct wl_resource *resource)
{
	struct bz_subsurface *subsurface_data = wl_resource_get_user_data(resource);

	// Already desync.
	if (!subsurface_data->is_sync) {
		return;
	}

	subsurface_data->is_sync = false;

	// When a surface changes to desync, all UNREACHABLE SCUs in its CU queue become desync, and
	//   any dependencies leading into these CUs are severed.
	struct bz_list *eval_candidates = nullptr;
	struct bz_content_update *cu; bz_list_foreach(cu, subsurface_data->surface->content_updates) {
		// First, check itself. If it's already desync, then it doesn't matter.
		if (!cu->is_sync) { continue; }

		// Next, walk up its content update tree, looking for a DCU.
		struct bz_content_update *parent_cu = cu;
		while (parent_cu != nullptr) {
			if (!parent_cu->is_sync) { break; }
			parent_cu = parent_cu->claimed_by != nullptr
				? parent_cu->claimed_by
				: parent_cu->depended_on_by;
		}
		if (parent_cu != nullptr && !parent_cu->is_sync) {
			continue; // Found a desync, so no need to change the current CU into a DCU.
		}

		// If we made it here, then it's unreachable. Change its type, and sever its dependencies.
		cu->is_sync = false;
		if (cu->claimed_by != nullptr) {
			bz_list_remove(cu->claimed_by->dependencies, cu, nullptr);
			cu->claimed_by = nullptr;
		}

		// Record all DCU candidates for possible evaluation once we exit this loop.
		if (eval_candidates == nullptr) {
			eval_candidates = bz_list_create();
			if (eval_candidates == nullptr) {
				wl_resource_post_no_memory(resource);
				bz_error(BZ_LOG_WL_DISPLAY, "Failed to allocate memory for eval list.");
				return;
			}
		}
		bz_list_append(eval_candidates, cu);
	}

	// Try evaluating all of our candidates. Since they're now desync, maybe they can activate.
	if (eval_candidates != nullptr) {
		struct bz_content_update *eval_cu; bz_list_foreach(eval_cu, eval_candidates) {
			bz_walk_content_update_queue(eval_cu);
		}
		bz_list_free(eval_candidates, nullptr);
	}
}

// ---  Helpers  -----------------------------------------------------------------------------------

static struct bz_subsurface_state *bz_subsurface_get_pending_state(struct bz_subsurface *for_child)
{
	// Look up the state object on the parent
	struct bz_surface *parent = for_child->parent;
	if (parent != nullptr) {
		struct bz_subsurface_state *state = bz_list_find(parent->pending_state->subsurface_states, for_child, bz_subsurface_state_matches);
		if (state == nullptr) {
			state = calloc(1, sizeof(*state));
			state->subsurface = for_child;
			bz_list_append(parent->pending_state->subsurface_states, state);
		}
		return state;
	}

	// Shouldn't be able to get here. Return null to indicate an error.
	return nullptr;
}

static bool bz_subsurface_state_matches(void *list_item, void *match_data)
{
	struct bz_subsurface *subsurface = match_data;
	struct bz_subsurface_state *state = list_item;

	return state->subsurface == subsurface;
}
