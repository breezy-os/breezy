#ifndef BZ_WINDOW_MANAGEMENT_H
#define BZ_WINDOW_MANAGEMENT_H
// #################################################################################################

#include <xkbcommon/xkbcommon.h>

#include "breezy/bz_list.h"
#include "breezy/bz_wl_display.h"

struct bz_window_mgmt {
	/**
	 * These are keyboard-focusable surfaces, and include xdg_toplevel, xdg_popup, and
	 * sometimes wlr_layer_surfaces
	 */
	struct bz_list *activable_surfaces; // Each item is of type "struct bz_surface *"
};

// -- Initialization / Teardown --
int bz_mgmt_initialize(struct bz_window_mgmt *mgmt);
void bz_mgmt_cleanup(struct bz_window_mgmt *mgmt);

// -- Window Lifecycle --
int bz_mgmt_open_window(struct bz_window_mgmt *mgmt, struct bz_surface *surface);
int bz_mgmt_close_active_window(struct bz_window_mgmt *mgmt);
struct bz_surface *bz_mgmt_get_active_surface(struct bz_window_mgmt *mgmt);
void bz_mgmt_notify_enter(struct xkb_state *xkbstate, struct wl_resource *keyboard, struct wl_resource *surface);

// #################################################################################################
#endif
