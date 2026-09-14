#ifndef BZ_XDG_OUTPUT_H
#define BZ_XDG_OUTPUT_H
// #################################################################################################

#include <stdint.h>

#include <wayland-server.h>


// -- zxdg_output_manager_v1 --

#define BZ_ZXDG_OUTPUT_MANAGER_V1_VERSION 2 // TODO: 3 is available

void bz_zxdg_output_manager_v1_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id);


// -- zxdg_output_v1 --

#define BZ_ZXDG_OUTPUT_V1_VERSION 2 // TODO: 3 is available


// #################################################################################################
#endif