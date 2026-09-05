#ifndef BZ_WL_DISPLAY_H
#define BZ_WL_DISPLAY_H
// #################################################################################################

#include <stdint.h>

#include <wayland-server.h>

#include "breezy/bz_breezy.h"
#include "breezy/bz_math.h"
#include "glad/gles2.h"


// -- wl_compositor --

#define BZ_COMPOSITOR_VERSION 6

void bz_compositor_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id);


// -- wl_subcompositor --

#define BZ_SUBCOMPOSITOR_VERSION 1

void bz_subcompositor_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id);


// -- wl_region --

#define BZ_REGION_VERSION 7

struct bz_region {
	struct wl_resource *resource;
	struct bz_list *mutations; // List of "struct bz_region_mutation *"
};

enum bz_region_op { OP_ADD, OP_SUBTRACT };
struct bz_region_mutation {
	enum bz_region_op op;
	int32_t x;
	int32_t y;
	int32_t w;
	int32_t h;
};

void bz_region_dtor(struct wl_resource *data);


// -- wl_surface --

#define BZ_SURFACE_VERSION 6

enum bz_surface_role {
	BZ_SURF_ROLE_NONE,
	BZ_SURF_ROLE_XDG_TOPLEVEL,  // xdg_surface      :: get_toplevel()
	BZ_SURF_ROLE_XDG_POPUP,     // xdg_surface      :: get_popup()
	BZ_SURF_ROLE_WL_CURSOR,     // wl_pointer       :: set_cursor()
	BZ_SURF_ROLE_WL_SUBSURFACE, // wl_subcompositor :: get_subsurface()
	BZ_SURF_ROLE_WL_DRAG_ICON,  // wl_data_device   :: start_drag()
};

struct bz_surface_state {
	struct wl_resource *buffer;
	struct bz_list *frame_callbacks; // List of "struct wl_resource *" (wl_callback objects)
	// TODO-dl12: damage, opaque region, input region, etc.
	// TODO-dl12: set_position, place_above, place_below
};

struct bz_surface {
	struct wl_resource *resource;
	struct bz_xdg_surface *xdgsurface; // nullptr for non-xdg surfaces.

	struct wl_listener disable_on_destroy; // Removes surface from breezy.wayland.activable_surfaces

	// Role tracking
	enum bz_surface_role role;
	union {
		// We use these pointers to know if the role object was destroyed. (nullptr == destroyed)
		struct bz_xdg_toplevel *xdgtoplevel;
		struct bz_xdg_popup *xdgpopup;
		struct bz_subsurface *subsurface;
		// (No role object for cursors)
		// ...etc...
	};

	// Double-buffered state management
	struct bz_surface_state *pending_state;
	struct bz_list *content_updates; // List of "struct bz_content_update *"
	struct bz_surface_state *active_state;

	/** First item is "on top". Includes the current surface and all child subsurfaces. */
	struct bz_list *surface_stack; // List of "struct bz_surface *".

	// Display data
	struct bz_renderable renderable;
};

struct bz_content_update {
	struct bz_surface *surface;
	struct bz_surface_state *state;

	bool is_sync;

	struct bz_content_update *depended_on_by; // Nullptr when not claimed.
	struct bz_list *dependencies; // List of "struct bz_content_update *".
};

void bz_surface_dtor(struct wl_resource *data);


// -- wl_subsurface --

#define BZ_SUBSURFACE_VERSION 1

struct bz_subsurface {
	struct wl_resource *resource;
	struct bz_surface *surface;
	struct bz_surface *parent;

	bool is_sync;
};

void bz_subsurface_dtor(struct wl_resource *subsurface);


// #################################################################################################
#endif