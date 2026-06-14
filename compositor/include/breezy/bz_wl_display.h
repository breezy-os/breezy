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


// #################################################################################################
#endif