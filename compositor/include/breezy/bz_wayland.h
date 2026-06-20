#ifndef BZ_WAYLAND_H
#define BZ_WAYLAND_H
// #################################################################################################

#include "glad/gles2.h"

#include "breezy/bz_breezy.h"

int bz_wayland_initialize(struct bz_breezy *breezy);
void bz_wayland_cleanup(struct bz_breezy *breezy);

struct bz_client {
	struct bz_breezy *breezy;
	struct wl_listener client_disconnect_listener;
	pid_t pid;

	struct bz_list *surfaces; // Stores a list of "struct wl_resource *"

	// TODO: Temporary OpenGL things. Will be moved to surfaces. (Also move cleanup code from dtor)
	GLuint vao;
	GLuint vbo;
	GLuint ebo;
};


// #################################################################################################
#endif
