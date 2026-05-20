
#include "breezy/bz_wayland.h"

#include <wayland-server-core.h>

#include "breezy/bz_list.h"
#include "breezy/bz_logger.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

static void bz_wayland_destroy_event_source(void *source);


// =================================================================================================
//  Internal API
// -------------------------------------------------------------------------------------------------

/** Utility method for destroying an event source within Wayland via our bz_list_free() method. */
static void bz_wayland_destroy_event_source(void *source)
{
	struct wl_event_source *s = source;
	wl_event_source_remove(s);
}


// =================================================================================================
//  Exposed API
// -------------------------------------------------------------------------------------------------

int bz_wayland_initialize(struct bz_breezy *breezy)
{
	// Create the Wayland display
	breezy->wayland.display = wl_display_create();
	if (!breezy->wayland.display) {
		bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Failed to create Wayland display.");
		return -1;
	}

	// Create our globals
	// TODO: Upcoming video (wl_compositor, wl_shm, xwm_base)

	// Prepare the socket for client connection
	const char *socket = wl_display_add_socket_auto(breezy->wayland.display);
	if (!socket) {
		bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Failed to add socket to Wayland display.");
		return -2;
	}

	bz_info(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Successfully initialized our Wayland system.");
	return 0;
}

void bz_wayland_cleanup(struct bz_breezy *breezy)
{
	if (breezy->wayland.event_sources != nullptr) {
		bz_list_free(breezy->wayland.event_sources, bz_wayland_destroy_event_source);
	}
	if (breezy->wayland.display != nullptr) {
		wl_display_destroy(breezy->wayland.display);
	}
}
