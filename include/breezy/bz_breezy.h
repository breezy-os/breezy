#ifndef BZ_BREEZY_H
#define BZ_BREEZY_H
// #################################################################################################


#include <stdint.h>

#include <xf86drmMode.h>
#include <gbm.h>
#include <EGL/egl.h>

struct bz_drm {
	int fd;
	int device_id;
	uint32_t connector_id;
	uint32_t crtc_id;
	drmModeModeInfo mode_info;
	uint32_t mode_blob_id;
	uint32_t plane_id;
};

struct bz_gbm {
	struct gbm_device *device;
	struct gbm_surface *surface;
	struct gbm_bo *prev_bo;
	struct gbm_bo *new_bo; // Only set temporarily during page flip events.
};

struct bz_gl {
	EGLDisplay display;
	EGLConfig config;
	EGLContext context;
	EGLSurface surface;
};

struct bz_seat {
	struct libseat *seat;
	int fd;
	int active;
};

struct bz_input_device {
	int fd; // Device file descriptor
	int id; // Libinput device ID
};

struct bz_input {
	int fd;
	struct udev *udev;
	struct libinput *libinput;
	struct xkb_context *xkb_context;
	struct xkb_keymap *xkb_keymap;
	struct xkb_state *xkb_state;
	struct bz_list *device_lookup; // Each item is of type "struct bz_input_device"
};


struct bz_breezy {
	struct bz_drm drm;
	struct bz_gbm gbm;
	struct bz_gl gl;
	struct bz_seat seat;
	struct bz_input input;
};


// #################################################################################################
#endif
