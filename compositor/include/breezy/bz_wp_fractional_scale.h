#ifndef BZ_WP_FRACTIONAL_SCALE_H
#define BZ_WP_FRACTIONAL_SCALE_H
// #################################################################################################

#include <stdint.h>

#include <wayland-server.h>

// -- wp_fractional_scale_manager --

#define BZ_WP_FRACTIONAL_SCALE_MANAGER_VERSION 1

void bz_wp_fractional_scale_manager_v1_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id);


// -- wp_fractional_scale --

struct bz_wp_fractional_scale {
	struct wl_resource *resource; // wp_fractional_scale
	struct bz_surface *surface;
};

// #################################################################################################
#endif
