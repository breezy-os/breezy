#ifndef BZ_XDG_SHELL_H
#define BZ_XDG_SHELL_H
// #################################################################################################

#include <stdint.h>

#include <wayland-server.h>

#include "breezy/bz_breezy.h"
#include "breezy/bz_math.h"


// -- xdg_wm_base --

#define BZ_XDG_WM_BASE_VERSION 7

void bz_xdg_wm_base_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id);


// -- xdg_surface --

#define BZ_XDG_SURFACE_VERSION 7

enum bz_xdg_surface_type {
	BZ_XDG_SURF_TOPLEVEL,
	BZ_XDG_SURF_POPUP,
};
struct bz_toplevel_configure {
	struct wl_array states; // TODO: Remember to call wl_array_release(&states)
	struct bz_dimension max_size;
	struct bz_dimension recommended_size;
};
struct bz_popup_configure {
	struct bz_position position;
	struct bz_dimension size;
};
struct bz_xdg_surface_configure {
	uint32_t serial;
	enum bz_xdg_surface_type type;
	union {
		struct bz_toplevel_configure toplevel;
		struct bz_popup_configure popup;
	};
};

struct bz_xdg_surface {
	struct wl_resource *resource;
	struct bz_surface *wlsurface;

	// Configure event tracking
	uint32_t serial;
	struct bz_list *pending_configures; // List of "struct bz_xdg_surface_configure *"
	struct bz_xdg_surface_configure *last_acked_configure; // nullptr before first ack
};

void bz_xdg_surface_dtor(struct wl_resource *data);
void bz_xdg_surface_initial_configure(struct wl_client *client, struct bz_surface *bzsurf);


// -- xdg_toplevel --

#define BZ_XDG_TOPLEVEL_VERSION 7

struct bz_xdg_toplevel {
	struct wl_resource *resource;
	struct bz_xdg_surface *xdgsurface;
	struct bz_surface *wlsurface;
};

void bz_xdg_toplevel_dtor(struct wl_resource *data);


// -- xdg_popup --

#define BZ_XDG_POPUP_VERSION 7

struct bz_xdg_popup {
	struct wl_resource *resource;
	struct bz_xdg_surface *xdgsurface;
	struct bz_surface *wlsurface;
};

// void bz_xdg_popup_dtor(struct wl_resource *data);



// #################################################################################################
#endif