
#include "breezy/bz_wayland.h"

#include <signal.h>
#include <sys/wait.h>

#include <wayland-server.h>
#include <wayland/xdg-shell-server-protocol.h>

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
static void bz_wayland_handle_client_connection(struct wl_listener * /*listener*/, void *data);

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
		bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Error creating compositor global.");
		return -1;
	}

	return 0;
}

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
		bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Error creating subcompositor global.");
		return -1;
	}

	return 0;
}

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
		bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Error creating xdg_wm_base global.");
		return -1;
	}

	return 0;
}

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
		bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Error creating data_device_manager global.");
		return -1;
	}

	return 0;
}

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
		bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Error creating wl_seat global.");
		return -1;
	}

	return 0;
}

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
		bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Error creating wl_output global.");
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
		bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Failed to create Wayland display.");
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
	struct wl_listener client_conn_listener;
	client_conn_listener.notify = bz_wayland_handle_client_connection;
	wl_display_add_client_created_listener(breezy->wayland.display, &client_conn_listener);

	// Prepare the socket for client connection
	breezy->wayland.socket_name = wl_display_add_socket_auto(breezy->wayland.display);
	if (!breezy->wayland.socket_name) {
		bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Failed to add socket to Wayland display.");
		return -9;
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
