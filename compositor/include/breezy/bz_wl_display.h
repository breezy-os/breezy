#ifndef BZ_WL_DISPLAY_H
#define BZ_WL_DISPLAY_H
// #################################################################################################

#include <stdint.h>

#include <wayland-server.h>


// -- wl_compositor --

#define BZ_COMPOSITOR_VERSION 6

void bz_compositor_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id);


// -- wl_subcompositor --

#define BZ_SUBCOMPOSITOR_VERSION 1

void bz_subcompositor_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id);


// -- wl_surface --

#define BZ_SURFACE_VERSION 6

enum bz_surface_role {
	BZ_SURF_ROLE_NONE,
	BZ_SURF_ROLE_XDG_TOPLEVEL,
	BZ_SURF_ROLE_XDG_POPUP,
	BZ_SURF_ROLE_WL_CURSOR,
	// ...etc...
};

struct bz_surface_state {
	struct wl_resource *buffer;
};

struct bz_surface {
	struct wl_resource *resource;
	struct bz_xdg_surface *xdgsurface; // nullptr for non-xdg surfaces.

	// Role tracking
	enum bz_surface_role role;
	union {
		struct bz_xdg_toplevel *xdgtoplevel;
		struct bz_xdg_popup *xdgpopup;
		// ...etc...
	};

	// Double-buffered state management
	struct bz_surface_state *pending_state;
	struct bz_surface_state *active_state;
};

void bz_surface_dtor(struct wl_resource *data);

// #################################################################################################
#endif