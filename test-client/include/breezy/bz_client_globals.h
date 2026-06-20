#ifndef BZ_CLIENT_GLOBALS_H
#define BZ_CLIENT_GLOBALS_H
// #################################################################################################

#include <stdint.h>

#include <wayland-client-protocol.h>

#include "breezy/bz_math.h"

struct bz_configure_sequence {
	uint32_t serial;
	struct bz_dimension max_size;
	struct bz_dimension recommended_size;
	struct wl_array *states;
};

struct bz_application_window {
	struct wl_surface *wlsurface;
	struct xdg_surface *xdgsurface;
	struct xdg_toplevel *xdgtoplevel;

	struct bz_configure_sequence *pending;
	struct bz_configure_sequence *finalized;

	struct bz_dimension size;
};

struct bz_client_globals {

	// -- Globals --

	struct wl_display *display;
	struct wl_registry *registry;

	struct wl_compositor *compositor;
	uint32_t compositor_name;

	struct wl_shm *shm;
	uint32_t shm_name;

	struct xdg_wm_base *xdg_wm_base;
	uint32_t xdg_wm_base_name;

	// -- Application State --

	int is_quitting;
	uint32_t bg_color;
	uint32_t fg_color;
	struct bz_application_window *window;

};

// #################################################################################################
#endif
