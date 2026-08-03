
#include "breezy/bz_wl_display.h"

#include <stdint.h>
#include <stdlib.h>

#include <wayland-server.h>

#include "breezy/bz_graphics.h"
#include "breezy/bz_list.h"
#include "breezy/bz_logger.h"
#include "breezy/bz_math.h"
#include "breezy/bz_wayland.h"
#include "breezy/bz_xdg_shell.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

// -- wl_compositor --

static const struct wl_compositor_interface bz_compositor_implementation;
static void bz_compositor_create_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void bz_compositor_create_region(struct wl_client *client, struct wl_resource *resource, uint32_t id);
// Helpers
static struct bz_surface_state *bz_surface_state_init(void);
static void bz_surface_state_free(struct bz_surface_state *state);

// -- wl_subcompositor --

static const struct wl_subcompositor_interface bz_subcompositor_implementation;
static void bz_subcompositor_destroy(struct wl_client *client, struct wl_resource *resource);
static void bz_subcompositor_get_subsurface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface, struct wl_resource *parent);

// -- wl_surface --

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
static void bz_initialize_gl_texture(struct bz_surface *surface);
static void bz_apply_damage(struct bz_surface *bzsurf);


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
	// Allocate our user data
	struct bz_surface *surface = calloc(1, sizeof(*surface));
	if (surface == nullptr) {
		wl_client_post_no_memory(client);
		goto surface_alloc_failed;
	}
	struct bz_surface_state *pending = bz_surface_state_init();
	if (pending == nullptr) {
		wl_client_post_no_memory(client);
		goto pending_state_alloc_failed;
	}
	struct bz_surface_state *active = bz_surface_state_init();
	if (active == nullptr) {
		wl_client_post_no_memory(client);
		goto active_state_alloc_failed;
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
	const struct bz_client *client_data = wl_client_get_user_data(client);
	surface->resource = res;
	surface->role = BZ_SURF_ROLE_NONE;
	surface->pending_state = pending;
	surface->active_state = active;
	surface->position.x = bz_rand_int(0, 3.0f/4*client_data->breezy->drm.mode_info.hdisplay);
	surface->position.y = bz_rand_int(0, 3.0f/4*client_data->breezy->drm.mode_info.vdisplay);

	// Everything succeeded!
	return;

	// Error cleanups
	resource_failed:
		bz_surface_state_free(active);
	active_state_alloc_failed:
		bz_surface_state_free(pending);
	pending_state_alloc_failed:
		free(surface);
	surface_alloc_failed:
		bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "Failed to construct a new surface.");
}

static void bz_compositor_create_region(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id
) {
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_compositor.create_region not implemented");
	// TODO
}

// ---  Helpers  -----------------------------------------------------------------------------------

static struct bz_surface_state *bz_surface_state_init(void)
{
	struct bz_surface_state *state = calloc(1, sizeof(*state));
	if (state == nullptr) {
		goto state_alloc_failed;
	}

	state->frame_callbacks = bz_list_create();
	if (state->frame_callbacks == nullptr) {
		goto callback_list_failed;
	}
	return state;

	// Error cleanups
	callback_list_failed:
		free(state);
	state_alloc_failed:
		bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "Failed to create empty bz_surface_state.");
		return nullptr;
}

static void bz_surface_state_free(struct bz_surface_state *state)
{
	if (state == nullptr) { return; }

	if (state->frame_callbacks != nullptr) {
		bz_list_free(state->frame_callbacks, nullptr);
	}
	free(state);
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

void bz_surface_dtor(struct wl_resource *data)
{
	struct bz_surface *bzsurf = wl_resource_get_user_data(data);

	bz_surface_state_free(bzsurf->pending_state);
	bz_surface_state_free(bzsurf->active_state);

	glDeleteTextures(1, &bzsurf->texture);

	free(bzsurf);

	// Not sure I really like this here... keep an eye out for a better place to unrender clients.
	struct wl_client *client = wl_resource_get_client(data);
	struct bz_client *bzclient = wl_client_get_user_data(client);
	bz_graphics_schedule_render(bzclient->breezy);
}

static void bz_surface_destroy(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_surface.destroy not implemented");
	// TODO
	// wl_resource_destroy(resource);
	// Role must be destroyed first. Otherwise, "defunct_role_object" error
}

static void bz_surface_attach(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *buffer,
	int32_t x,
	int32_t y
) {
	struct bz_surface *bzsurf = wl_resource_get_user_data(resource);
	bzsurf->pending_state->buffer = buffer;
	// TODO: accommodate x and y
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
		bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "Failed to construct a new frame callback.");
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
	struct bz_client *bzclient = wl_client_get_user_data(client);
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
			bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__,
				"XDG surface object was null on wl_surface after being assigned. "
				"THIS SHOULD NEVER HAPPEN.");
			wl_client_post_implementation_error(client,
				"XDG surface object does not exist on the wl_surface.");
			return;
		}
		bz_xdg_surface_initial_configure(client, bzsurf);
		return;
	}

	// Copy over our other pending state into active state.
	bzsurf->active_state->buffer = bzsurf->pending_state->buffer;
	bz_list_move_to_end(
		bzsurf->active_state->frame_callbacks,
		bzsurf->pending_state->frame_callbacks
	);

	if (bzsurf->active_state->buffer != nullptr) {
		// Update our OpenGL texture
		if (bzsurf->texture == 0) {
			bz_initialize_gl_texture(bzsurf);
		}
		bz_apply_damage(bzsurf);
		// Update our displayed window size
		struct wl_shm_buffer *shmbuf = wl_shm_buffer_get(bzsurf->active_state->buffer);
		bzsurf->size.w = wl_shm_buffer_get_width(shmbuf);
		bzsurf->size.h = wl_shm_buffer_get_height(shmbuf);
	}

	// Since the buffer is on our OpenGL texture, release the buffer.
	// TODO: This might need to be deferred for the DMA-BUF protocol..?
	if (bzsurf->active_state->buffer != nullptr) {
		wl_buffer_send_release(bzsurf->active_state->buffer);
	}

	// Schedule a repaint
	bz_graphics_schedule_render(bzclient->breezy);
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

// ---  Helpers  -----------------------------------------------------------------------------------

static void bz_initialize_gl_texture(struct bz_surface *surface)
{
	bz_info(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "Initializing OpenGL texture for surface.");

	glGenTextures(1, &surface->texture);
	glBindTexture(GL_TEXTURE_2D, surface->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

static void bz_apply_damage(struct bz_surface *bzsurf)
{
	if (bzsurf->active_state->buffer == nullptr) {
		return;
	}

	struct wl_shm_buffer *shmbuf = wl_shm_buffer_get(bzsurf->active_state->buffer);
	wl_shm_buffer_begin_access(shmbuf);
	// --- Buffer Access Begin ---------------------------------------------------------------------

	uint32_t *data = wl_shm_buffer_get_data(shmbuf);
	int32_t width = wl_shm_buffer_get_width(shmbuf);
	int32_t height = wl_shm_buffer_get_height(shmbuf);
	glBindTexture(GL_TEXTURE_2D, bzsurf->texture);
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
