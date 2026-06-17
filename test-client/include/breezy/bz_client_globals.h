#ifndef BZ_CLIENT_GLOBALS_H
#define BZ_CLIENT_GLOBALS_H
// #################################################################################################


struct bz_client_globals {
	int is_quitting;
	uint32_t bg_color;
	uint32_t fg_color;

	struct wl_display *display;
	struct wl_registry *registry;

	struct wl_compositor *compositor;
	uint32_t compositor_name;

	struct wl_shm *shm;
	uint32_t shm_name;
};

// #################################################################################################
#endif
