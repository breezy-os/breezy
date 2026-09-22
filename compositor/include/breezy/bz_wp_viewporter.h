#ifndef BZ_WP_VIEWPORTER_H
#define BZ_WP_VIEWPORTER_H
// #################################################################################################

#include <stdint.h>

#include <wayland-server.h>

// -- wp_viewporter --

#define BZ_WP_VIEWPORTER_VERSION 1

void bz_wp_viewporter_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id);


// -- wp_viewport --

#define BZ_WP_VIEWPORT_VERSION 1

struct bz_wp_viewport {
	struct wl_resource *resource; // wl_viewport
	struct bz_surface *surface;
};

// #################################################################################################
#endif
