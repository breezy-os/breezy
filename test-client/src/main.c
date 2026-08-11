// This gives us access to signal handling functionality
#define _POSIX_C_SOURCE 200809L // NOLINT

#include <signal.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <time.h>
#include <unistd.h>

#include <wayland-client.h>
#include <sys/mman.h>
#include <xkbcommon/xkbcommon.h>

#include "breezy/bz_logger.h"
#include "breezy/bz_application.h"
#include "breezy/bz_wl_protocol.h"
#include "breezy/bz_client_globals.h"
#include "breezy/bz_client_utils.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

static struct bz_client_globals client_globals = {0};

static void bz_termint_handler(int signum);
static void bz_add_termint_handler(int signum);


// =================================================================================================
//  Function Definitions
// -------------------------------------------------------------------------------------------------

static void bz_termint_handler(int signum)
{
	bz_info(BZ_LOG_MAIN, __FILE__, __LINE__, "SIGTERM/SIGINT (%d) signal received", signum);
	constexpr uint64_t val = 1;
	write(client_globals.is_quitting, &val, sizeof(val));
}

static void bz_add_termint_handler(int signum)
{
	struct sigaction sa;
	sa.sa_handler = &bz_termint_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(signum, &sa, nullptr);
}


// =================================================================================================
//  Main Program
// -------------------------------------------------------------------------------------------------

int main(void)
{
	// Set up our logger
	bz_log_initialize(BZ_LOG_INFO);
	bz_log_set_level(BZ_LOG_WAYLAND, BZ_LOG_DEBUG);

	client_globals.is_quitting = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

	bz_add_termint_handler(SIGTERM);
	bz_add_termint_handler(SIGINT);

	srand(time(nullptr));

	// Establish the connection
	client_globals.display = wl_display_connect(nullptr);
	if (!client_globals.display) {
		bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Failed to connect to Wayland display.");
		close(client_globals.is_quitting);
		return 1;
	}

	// Set up our globals
	bz_registry_constructor(&client_globals);
	wl_display_roundtrip(client_globals.display);

	// Make our main application window and cursor
	client_globals.window = bz_create_app_window(&client_globals);
	client_globals.cursor = bz_create_cursor_surface(&client_globals);

	// Loop!
	bz_run_event_loop(&client_globals);

	// Cleanup
	bz_info(BZ_LOG_MAIN, __FILE__, __LINE__, "Cleaning up and disconnecting.");
	// TODO-dl10: Call *surface.destroy methods
	if (client_globals.window != nullptr) {
		for (uint8_t i = 0; i < 2; i++) {
			if (client_globals.window->buffers[i].buffer != nullptr) {
				wl_buffer_destroy(client_globals.window->buffers[i].buffer);
			}
		}
		if (client_globals.window->shm_pool != nullptr) {
			wl_shm_pool_destroy(client_globals.window->shm_pool);
		}
		if (client_globals.window->pool_data != nullptr) {
			munmap(client_globals.window->pool_data, client_globals.window->pool_size);
		}
		if (client_globals.window->frame_callback != nullptr) {
			wl_callback_destroy(client_globals.window->frame_callback);
			client_globals.window->frame_callback = nullptr;
		}
	}
	if (client_globals.seat != nullptr) {
		struct bz_seat *seat_data = wl_seat_get_user_data(client_globals.seat);
		if (seat_data != nullptr) {
			if (seat_data->keyboard   != nullptr) { wl_keyboard_release(seat_data->keyboard); }
			if (seat_data->pointer    != nullptr) { wl_pointer_release(seat_data->pointer); }
			if (seat_data->xkbstate   != nullptr) { xkb_state_unref(seat_data->xkbstate); }
			if (seat_data->xkbkeymap  != nullptr) { xkb_keymap_unref(seat_data->xkbkeymap); }
			if (seat_data->xkbcontext != nullptr) { xkb_context_unref(seat_data->xkbcontext); }
			free(seat_data);
		}
	}

	wl_display_disconnect(client_globals.display);
	close(client_globals.is_quitting);

	bz_info(BZ_LOG_MAIN, __FILE__, __LINE__, "Clean exit.");
	return 0;
}
