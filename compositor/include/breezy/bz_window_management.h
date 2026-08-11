#ifndef BZ_WINDOW_MANAGEMENT_H
#define BZ_WINDOW_MANAGEMENT_H
// #################################################################################################

#include <xkbcommon/xkbcommon.h>

#include "breezy/bz_wl_display.h"
#include "breezy/bz_breezy.h"

// -- Initialization / Teardown --
int bz_mgmt_initialize(struct bz_window_mgmt *mgmt);
void bz_mgmt_cleanup(struct bz_window_mgmt *mgmt);

// -- Window Lifecycle --
void bz_mgmt_open_window(struct bz_window_mgmt *mgmt, struct bz_surface *surface);
int bz_mgmt_close_active_window(struct bz_window_mgmt *mgmt);
void bz_mgmt_notify_kb_enter(uint32_t enter_serial, uint32_t modifiers_serial, struct xkb_state *xkbstate, struct wl_resource *keyboard, struct wl_resource *surface);

// -- Window Focus --
struct bz_surface *bz_mgmt_get_active_surface(struct bz_window_mgmt *mgmt);
struct bz_surface *bz_mgmt_update_pointer_position(struct bz_window_mgmt *mgmt, struct bz_renderable *cursor);

// #################################################################################################
#endif
