#ifndef BZ_WL_DEVICES_H
#define BZ_WL_DEVICES_H
// #################################################################################################

#include <stdint.h>

#include <wayland-server.h>

#include "breezy/bz_breezy.h"

// -- wl_seat --
#define BZ_SEAT_VERSION 10
void bz_seat_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id);
void bz_seat_dtor(struct wl_resource *data);
struct bz_wl_seat {
	struct wl_resource *resource; // The "wl_seat"
	struct bz_list *keyboards;    // List of "struct wl_resource *"
	struct bz_list *pointers;     // List of "struct wl_resource *"
};

// -- wl_pointer --
#define BZ_POINTER_VERSION 10
void bz_pointer_dtor(struct wl_resource *data);

// -- wl_keyboard --
#define BZ_KEYBOARD_VERSION 10
void bz_keyboard_dtor(struct wl_resource *data);

// -- wl_output --
#define BZ_OUTPUT_VERSION 4
void bz_output_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id);

// -- wl_data_device_manager --
#define BZ_DATA_DEVICE_MANAGER_VERSION 3 // TODO: version 4 exists - upgrade?
void bz_data_device_manager_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id);


// #################################################################################################
#endif