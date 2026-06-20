#ifndef BZ_WL_PROTOCOL_H
#define BZ_WL_PROTOCOL_H
// #################################################################################################

#include "breezy/bz_client_globals.h"

void bz_registry_constructor(struct bz_client_globals *globals);

struct xdg_surface *bz_xdg_surface_constructor(struct bz_client_globals *globals, struct wl_surface *wlsurface);
struct xdg_toplevel *bz_xdg_toplevel_constructor(struct bz_client_globals *globals, struct xdg_surface *xdgsurface);

// #################################################################################################
#endif