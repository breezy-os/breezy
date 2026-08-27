#ifndef BZ_BREEZY_H
#define BZ_BREEZY_H
// #################################################################################################


#include <stdint.h>

#include <xf86drmMode.h>
#include <gbm.h>
#include <EGL/egl.h>
#include <libseat.h>
#include <wayland-server.h>

#include "glad/gles2.h"
#include "breezy/bz_math.h"

struct bz_drm {
	int fd;
	int device_id;
	uint32_t connector_id;
	uint32_t crtc_id;
	drmModeModeInfo mode_info;
	uint32_t mode_blob_id;
	uint32_t plane_id;
	bz_mat3 output_projection;
	bool retry_render_on_page_flip;
};

struct bz_gbm {
	struct gbm_device *device;
	struct gbm_surface *surface;
	struct gbm_bo *prev_bo; // The currently displayed buffer.
	struct gbm_bo *new_bo;  // The recently committed, but not yet displayed, buffer.
};

struct bz_renderable {
	struct bz_position position;
	/** How far to shift the position before rendering. A positive number means the buffer should
	 * be shifted to the right and down. For cursors, this is the "negative hotspot". */
	struct bz_position offset;
	struct bz_dimension size;
	GLuint texture;
};

struct bz_gl {
	EGLDisplay display;
	EGLConfig config;
	EGLContext context;
	EGLSurface surface;
	bool is_dirty;
	GLuint client_shader_program;
	GLuint vbo;
	struct bz_renderable cursor;
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

	struct bz_list *device_lookup; // Each item is of type "struct bz_input_device *"
	_Atomic int keyboard_count;
	_Atomic int pointer_count;
	bool ever_had_keyboard;
	bool ever_had_pointer;
};

struct bz_wayland {
	struct wl_display *display;
	const char *socket_name;
	struct wl_listener new_client_listener;
	struct bz_list *event_sources; // Each item is of type "struct wl_event_source *"

	struct bz_list *clients; // Each item is of type "struct wl_client *"
};

struct bz_window_mgmt {
	/**
	 * These are keyboard-focusable surfaces, and include xdg_toplevel, xdg_popup, and
	 * sometimes wlr_layer_surfaces. The final item in the list has "keyboard focus".
	 */
	struct bz_list *activable_surfaces; // Each item is of type "struct bz_surface *"

	struct bz_surface *pointer_focus;  // nullptr if cursor isn't above a surface
	struct bz_position last_cursor_loc;
};


struct bz_breezy {
	struct bz_drm drm;
	struct bz_gbm gbm;
	struct bz_gl gl;
	struct bz_seat seat;
	struct bz_input input;
	struct bz_wayland wayland;
	struct bz_window_mgmt window_mgmt;
	bool is_terminating;
};


// #################################################################################################
#endif
