#ifndef BZ_GRAPHICS_H
#define BZ_GRAPHICS_H
// #################################################################################################

#include <stdint.h>

#include "breezy/bz_breezy.h"

// -- Public API --

int bz_graphics_initialize(struct bz_breezy *breezy);
int bz_graphics_loop_iteration(struct bz_breezy *breezy);
void bz_graphics_cleanup(struct bz_breezy *breezy);

void bz_graphics_handle_drm_event(struct bz_breezy *breezy);
int bz_graphics_activate(struct bz_breezy *breezy);
int bz_graphics_deactivate(struct bz_breezy *breezy);

// TODO: temp fun
void bz_graphics_set_color_index(int i);
void bz_graphics_change_color(struct bz_breezy *breezy, float amount);

// -- Data Structures --

struct bz_gbm_bo_data {
	struct bz_breezy *breezy;
	uint32_t fb_id;
};

struct bz_drm_prop_ids {
	uint32_t crtc_active;
	uint32_t crtc_mode_id;
	uint32_t connector_crtc_id;
	uint32_t plane_fb_id;
	uint32_t plane_crtc_id;
	uint32_t plane_src_x;
	uint32_t plane_src_y;
	uint32_t plane_src_w;
	uint32_t plane_src_h;
	uint32_t plane_crtc_x;
	uint32_t plane_crtc_y;
	uint32_t plane_crtc_w;
	uint32_t plane_crtc_h;
};

// #################################################################################################
#endif
