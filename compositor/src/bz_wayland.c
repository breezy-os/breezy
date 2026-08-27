
#include "breezy/bz_wayland.h"

#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>

#include <wayland-server.h>
#include <xdg-shell-server-protocol.h>

#include "breezy/bz_graphics.h"
#include "breezy/bz_list.h"
#include "breezy/bz_logger.h"
#include "breezy/bz_wl_devices.h"
#include "breezy/bz_wl_display.h"
#include "breezy/bz_xdg_shell.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

static void bz_wayland_destroy_event_source(void *source);
static int bz_wayland_sigchld_handler(int /*signal_number*/, void * /*data*/);
static void bz_wayland_client_dtor(void *data);
static void bz_wayland_handle_client_connection(struct wl_listener *listener, void *data);
static void bz_wayland_handle_client_disconnect(struct wl_listener * /*listener*/, void *data);

// Wayland "Global" constructors
static int bz_wayland_create_compositor(struct bz_breezy *breezy);
static int bz_wayland_create_subcompositor(struct bz_breezy *breezy);
static int bz_wayland_create_xdg_wm_base(struct bz_breezy *breezy);
static int bz_wayland_create_data_device_manager(struct bz_breezy *breezy);
static int bz_wayland_create_seat(struct bz_breezy *breezy);
static int bz_wayland_create_output(struct bz_breezy *breezy);


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
		bz_debug(BZ_LOG_WAYLAND, "Reaped PID with id %d", pid);
	}
	return 0;
}

static void bz_wayland_client_dtor(void *data)
{
	struct bz_client *client_data = data;

	bz_list_free(client_data->seats, nullptr);

	free(client_data);
}

/** Registers new clients, and sets up proper tracking structures for them. */
static void bz_wayland_handle_client_connection(struct wl_listener *listener, void *data)
{
	bz_debug(BZ_LOG_WAYLAND, "Handling client connection");
	struct wl_client *client = data;
	struct bz_wayland *wayland_data = wl_container_of(listener, wayland_data, new_client_listener);
	struct bz_breezy *breezy = wl_container_of(wayland_data, breezy, wayland);

	// Figure out the PID.
	pid_t pid;
	uid_t uid;
	gid_t gid;
	wl_client_get_credentials(client, &pid, &uid, &gid);
	bz_info(BZ_LOG_WAYLAND, "Client with pid %d connected!", pid);

	// Set up our custom data for the client
	struct bz_client *client_data = calloc(1, sizeof(*client_data));
	if (client_data == nullptr) {
		wl_client_post_no_memory(client);
		goto client_alloc_failed;
	}
	client_data->breezy = breezy;
	client_data->pid = pid;
	client_data->seats = bz_list_create();
	if (client_data->seats == nullptr) {
		wl_client_post_no_memory(client);
		goto seat_alloc_failed;
	}
	wl_client_set_user_data(client, client_data, bz_wayland_client_dtor);

	// Insert the client at the beginning of our "clients" list.
	bz_list_insert(wayland_data->clients, client, nullptr);

	// Set up a listener to clean up the client.
	client_data->client_disconnect_listener.notify = bz_wayland_handle_client_disconnect;
	wl_client_add_destroy_listener(client, &client_data->client_disconnect_listener);

	// Everything succeeded!
	return;

	seat_alloc_failed:
		free(client_data);
	client_alloc_failed:
		bz_error(BZ_LOG_WAYLAND, "Failed to initialize a new client.");
}

/** Handles client disconnects by removing the client from our global tracking list. */
static void bz_wayland_handle_client_disconnect(struct wl_listener * /*listener*/, void *data)
{
	bz_debug(BZ_LOG_WAYLAND, "Handling client disconnect");
	// Remove the wl_client from our globally-tracked list of wayland clients
	struct wl_client *client = data;
	struct bz_client *client_data = wl_client_get_user_data(client);
	bz_list_remove(client_data->breezy->wayland.clients, client, nullptr);
	bz_info(BZ_LOG_WAYLAND, "Client with pid %d disconnected!", client_data->pid);
}

/** Creates the wl_compositor global. */
static int bz_wayland_create_compositor(struct bz_breezy *breezy)
{
	struct wl_global *glob = wl_global_create(
		breezy->wayland.display,
		&wl_compositor_interface,
		BZ_COMPOSITOR_VERSION,
		nullptr,
		bz_compositor_constructor
	);
	if (glob == nullptr) {
		bz_error(BZ_LOG_WAYLAND, "Error creating compositor global.");
		return -1;
	}

	return 0;
}

/** Creates the wl_subcompositor global. */
static int bz_wayland_create_subcompositor(struct bz_breezy *breezy)
{
	struct wl_global *glob = wl_global_create(
		breezy->wayland.display,
		&wl_subcompositor_interface,
		BZ_SUBCOMPOSITOR_VERSION,
		nullptr,
		bz_subcompositor_constructor
	);
	if (glob == nullptr) {
		bz_error(BZ_LOG_WAYLAND, "Error creating subcompositor global.");
		return -1;
	}

	return 0;
}

/** Creates the xdg_wm_base global. */
static int bz_wayland_create_xdg_wm_base(struct bz_breezy *breezy)
{
	struct wl_global *glob = wl_global_create(
		breezy->wayland.display,
		&xdg_wm_base_interface,
		BZ_XDG_WM_BASE_VERSION,
		nullptr,
		bz_xdg_wm_base_constructor
	);
	if (glob == nullptr) {
		bz_error(BZ_LOG_WAYLAND, "Error creating xdg_wm_base global.");
		return -1;
	}

	return 0;
}

/** Creates the wl_data_device_manager global. */
static int bz_wayland_create_data_device_manager(struct bz_breezy *breezy)
{
	struct wl_global *glob = wl_global_create(
		breezy->wayland.display,
		&wl_data_device_manager_interface,
		BZ_DATA_DEVICE_MANAGER_VERSION,
		nullptr,
		bz_data_device_manager_constructor
	);
	if (glob == nullptr) {
		bz_error(BZ_LOG_WAYLAND, "Error creating data_device_manager global.");
		return -1;
	}

	return 0;
}

/** Creates the wl_seat global. */
static int bz_wayland_create_seat(struct bz_breezy *breezy)
{
	struct wl_global *glob = wl_global_create(
		breezy->wayland.display,
		&wl_seat_interface,
		BZ_SEAT_VERSION,
		nullptr,
		bz_seat_constructor
	);
	if (glob == nullptr) {
		bz_error(BZ_LOG_WAYLAND, "Error creating wl_seat global.");
		return -1;
	}

	return 0;
}

/** Creates the wl_output global. */
static int bz_wayland_create_output(struct bz_breezy *breezy)
{
	struct wl_global *glob = wl_global_create(
		breezy->wayland.display,
		&wl_output_interface,
		BZ_OUTPUT_VERSION,
		nullptr,
		bz_output_constructor
	);
	if (glob == nullptr) {
		bz_error(BZ_LOG_WAYLAND, "Error creating wl_output global.");
		return -1;
	}

	return 0;
}


// =================================================================================================
//  Exposed API
// -------------------------------------------------------------------------------------------------

int bz_wayland_initialize(struct bz_breezy *breezy)
{
	// Create the Wayland display
	breezy->wayland.display = wl_display_create();
	if (!breezy->wayland.display) {
		bz_error(BZ_LOG_WAYLAND, "Failed to create Wayland display.");
		return -1;
	}

	// Set up signal handling for cleaning up exited children.
	struct wl_event_loop *evt_loop = wl_display_get_event_loop(breezy->wayland.display);
	struct wl_event_source *sigchld_handler = wl_event_loop_add_signal(
		evt_loop, SIGCHLD, bz_wayland_sigchld_handler, nullptr);
	bz_list_append(breezy->wayland.event_sources, sigchld_handler);

	// Create our globals
	if (bz_wayland_create_compositor(breezy) < 0) {
		return -2;
	}
	if (bz_wayland_create_subcompositor(breezy) < 0) {
		return -3;
	}
	if (wl_display_init_shm(breezy->wayland.display) != 0) {
		return -4;
	}
	if (bz_wayland_create_xdg_wm_base(breezy) < 0) {
		return -5;
	}
	if (bz_wayland_create_data_device_manager(breezy) < 0) {
		return -6;
	}
	if (bz_wayland_create_seat(breezy) < 0) {
		return -7;
	}
	if (bz_wayland_create_output(breezy) < 0) {
		return -8;
	}


	// Set up a listener for new client connections
	breezy->wayland.new_client_listener.notify = bz_wayland_handle_client_connection;
	wl_display_add_client_created_listener(
		breezy->wayland.display,
		&breezy->wayland.new_client_listener
	);

	// Prepare the socket for client connection
	breezy->wayland.socket_name = wl_display_add_socket_auto(breezy->wayland.display);
	if (!breezy->wayland.socket_name) {
		bz_error(BZ_LOG_WAYLAND, "Failed to add socket to Wayland display.");
		return -9;
	}

	bz_info(BZ_LOG_WAYLAND, "Successfully initialized our Wayland system.");
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
	if (breezy->wayland.clients != nullptr) {
		// The actual wl_clients (and resources) are destroyed through wl_display_destroy_clients()
		bz_list_free(breezy->wayland.clients, nullptr);
	}
}
