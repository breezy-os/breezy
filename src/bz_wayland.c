
#include "breezy/bz_wayland.h"

#include <signal.h>
#include <sys/wait.h>

#include <wayland-server.h>

#include "breezy/bz_list.h"
#include "breezy/bz_logger.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

static void bz_wayland_destroy_event_source(void *source);
static int bz_wayland_sigchld_handler(int /*signal_number*/, void * /*data*/);
static void bz_wayland_handle_client_connection(struct wl_listener * /*listener*/, void *data);


// =================================================================================================
//  Internal API
// -------------------------------------------------------------------------------------------------

/** Utility method for destroying an event source within Wayland via our bz_list_free() method. */
static void bz_wayland_destroy_event_source(void *source)
{
	struct wl_event_source *s = source;
	wl_event_source_remove(s);
}

/** Removes/reaps the exited process IDs from the OS's process-tracking tables to avoid zombies. */
static int bz_wayland_sigchld_handler(int /*signal_number*/, void * /*data*/)
{
	pid_t pid;
	while ((pid = waitpid(-1, nullptr, WNOHANG)) > 0) {
		// No-op. Just reaping some zoombies.
		bz_debug(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Reaped PID with id %d", pid);
	}
	return 0;
}

/** Registers new clients, and sets up proper tracking structures for them. */
static void bz_wayland_handle_client_connection(struct wl_listener * /*listener*/, void *data)
{
	struct wl_client *client = data;

	pid_t pid;
	uid_t uid;
	gid_t gid;
	wl_client_get_credentials(client, &pid, &uid, &gid);

	bz_info(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Client with pid %d connected!", pid);

	// TODO-now: Save off wl_client in a list somewhere..?
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

	// Set up signal handling for cleaning up exited children.
	struct wl_event_loop *evt_loop = wl_display_get_event_loop(breezy->wayland.display);
	struct wl_event_source *sigchld_handler = wl_event_loop_add_signal(
		evt_loop, SIGCHLD, bz_wayland_sigchld_handler, nullptr);
	bz_list_append(breezy->wayland.event_sources, sigchld_handler);

	// Create our globals
	// TODO: Upcoming video (wl_compositor, wl_shm, xwm_base)

	// Set up a listener for new client connections
	struct wl_listener client_conn_listener;
	client_conn_listener.notify = bz_wayland_handle_client_connection;
	wl_display_add_client_created_listener(breezy->wayland.display, &client_conn_listener);

	// Prepare the socket for client connection
	breezy->wayland.socket_name = wl_display_add_socket_auto(breezy->wayland.display);
	if (!breezy->wayland.socket_name) {
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
		wl_display_destroy_clients(breezy->wayland.display);
		wl_display_destroy(breezy->wayland.display);
	}
}
