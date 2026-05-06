#ifndef BZ_DRM_H
#define BZ_DRM_H
// #################################################################################################

#include <stdint.h>

// -- Public API --

int bz_drm_initialize(void);
int bz_drm_loop_iteration(void);
int bz_drm_cleanup(void);

// -- Data Structures --

struct bz_gbm_bo_data {
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
