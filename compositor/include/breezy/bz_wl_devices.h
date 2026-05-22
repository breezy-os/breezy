#ifndef BZ_WL_DEVICES_H
#define BZ_WL_DEVICES_H
// #################################################################################################

#include <stdint.h>

#include <wayland-server.h>

// -- wl_seat --
#define BZ_SEAT_VERSION 10
void bz_seat_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id);

// -- wl_output --
#define BZ_OUTPUT_VERSION 4
void bz_output_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id);

// -- wl_data_device_manager --
#define BZ_DATA_DEVICE_MANAGER_VERSION 3 // TODO: version 4 exists - upgrade?
void bz_data_device_manager_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id);


// #################################################################################################
#endif