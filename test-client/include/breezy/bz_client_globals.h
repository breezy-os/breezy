#ifndef BZ_CLIENT_GLOBALS_H
#define BZ_CLIENT_GLOBALS_H
// #################################################################################################


struct bz_client_globals {
	int is_quitting;

	struct wl_display *display;
	struct wl_registry *registry;

	struct wl_compositor *compositor;
	uint32_t compositor_name;
};

// #################################################################################################
#endif
