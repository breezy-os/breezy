#ifndef BZ_XDG_DECORATION_H
#define BZ_XDG_DECORATION_H
// #################################################################################################

#include <stdint.h>

#include <wayland-server.h>
#include <xdg-decoration-unstable-v1-server-protocol.h>


// -- zxdg_decoration_manager_v1 --

#define BZ_ZXDG_DECORATION_MANAGER_V1_VERSION 2

void bz_zxdg_decoration_manager_v1_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id);


// -- zxdg_toplevel_decoration_v1 --

struct bz_xdg_toplevel_decoration {
	struct wl_resource *resource;
	struct bz_xdg_toplevel *toplevel;

	enum zxdg_toplevel_decoration_v1_mode requested_mode;
};


// #################################################################################################
#endif