#ifndef BZ_WAYLAND_H
#define BZ_WAYLAND_H
// #################################################################################################

#include "breezy/bz_breezy.h"

int bz_wayland_initialize(struct bz_breezy *breezy);
void bz_wayland_cleanup(struct bz_breezy *breezy);

struct bz_client {
	struct bz_breezy *breezy;
	struct wl_listener client_disconnect_listener;
	pid_t pid;

	struct bz_list *surfaces; // Stores a list of "struct bz_surface *"
	struct wl_resource *seat;
};


// -- wl_callback --

#define BZ_CALLBACK_VERSION 1


// #################################################################################################
#endif
