#ifndef BZ_XDG_SHELL_H
#define BZ_XDG_SHELL_H
// #################################################################################################

#include <stdint.h>

#include <wayland-server.h>

// -- xdg_wm_base --
#define BZ_XDG_WM_BASE_VERSION 7
void bz_xdg_wm_base_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id);


// #################################################################################################
#endif