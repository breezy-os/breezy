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

struct bz_buffer {
	bool is_released;
	struct wl_buffer *buffer;
	uint32_t *pixel_data;
	struct bz_dimension size;
};

struct bz_application_window {
	struct wl_surface *wlsurface;
	struct xdg_surface *xdgsurface;
	struct xdg_toplevel *xdgtoplevel;

	size_t pool_size;
	uint8_t *pool_data;           // nullptr prior to mmap
	struct wl_shm_pool *shm_pool; // nullptr prior to wl_shm_create_pool
	uint8_t active_buffer;        // The index of the buffer we should write to
	struct bz_buffer buffers[2];

	struct bz_configure_sequence *pending;
	struct bz_configure_sequence *finalized;

	struct bz_dimension size;
	uint32_t bg_color;
	uint32_t fg_color;
	uint32_t prev_time; // Used for circle animation.
	struct bz_position circle_center;
	float circle_speed_x;
	float circle_speed_y;

	struct wl_callback *frame_callback; // nullptr when not active
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
	struct bz_application_window *window;

};

// #################################################################################################
#endif
