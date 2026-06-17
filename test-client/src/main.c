// This gives us access to signal handling functionality
#define _POSIX_C_SOURCE 200809L // NOLINT

#include <signal.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/eventfd.h>
#include <time.h>
#include <unistd.h>

#include <wayland-client.h>

#include "breezy/bz_logger.h"
#include "breezy/bz_wl_protocol.h"
#include "breezy/bz_client_globals.h"
#include "breezy/bz_client_utils.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

static struct bz_client_globals client_globals = {0};

static void bz_termint_handler(int signum);
static void bz_add_termint_handler(int signum);
static void bz_sleep_ms(uint32_t ms);
static void bz_run_event_loop(const struct bz_client_globals *globals);

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


static void bz_sleep_ms(uint32_t ms)
{
	struct timespec ts = {
		.tv_sec = ms / 1000,
		.tv_nsec = (ms % 1000) * 1000000L,
	};
	nanosleep(&ts, nullptr);
}

static void bz_run_event_loop(const struct bz_client_globals *globals)
{
	const int wayland_fd = wl_display_get_fd(globals->display);

	// Run our event loop
	while (true) {
		// "wl_display_prepare_read" must be paired with either "wl_display_read_events" or, if
		//   unable to read events for some reason, "wl_display_cancel_read".
		while (wl_display_prepare_read(globals->display) != 0) {
			wl_display_dispatch_pending(globals->display);
		}

		// Flush pending requests to the server
		if (wl_display_flush(globals->display) < 0) {
			// If errno is EAGAIN, then we'd ideally be polling the display fd to wait for it to
			//   become writable before trying again ... but this client doesn't really matter, so
			//   let's just exit to keep things simple.
			wl_display_cancel_read(globals->display);
			break;
		}

		// Check for changes to our FDs
		struct pollfd fds[2] = {
			{ .fd = globals->is_quitting, .events = POLLIN },
			{ .fd = wayland_fd,           .events = POLLIN },
		};
		const int ret = poll(fds, 2, 1000);
		if (ret == 0) bz_warn(BZ_LOG_MAIN, __FILE__, __LINE__, "Timeout waiting for FD.");
		if (ret < 0) {
			wl_display_cancel_read(globals->display);
			break; // Failure
		}

		if (fds[0].revents & POLLIN) {
			wl_display_cancel_read(globals->display);
			break; // is_quitting got toggled
		}

		if (fds[1].revents & POLLIN) {
			wl_display_read_events(globals->display);
		} else {
			wl_display_cancel_read(globals->display);
		}

		wl_display_dispatch_pending(globals->display);
	}
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

	// Pick some random colors for our app
	srand(time(nullptr));
	client_globals.bg_color = bz_random_color();
	client_globals.fg_color = bz_random_color();

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

	// Loop!
	bz_run_event_loop(&client_globals);

	bz_info(BZ_LOG_MAIN, __FILE__, __LINE__, "Disconnecting from compositor.");
	wl_display_disconnect(client_globals.display);
	close(client_globals.is_quitting);

	bz_info(BZ_LOG_MAIN, __FILE__, __LINE__, "Clean exit.");
	return 0;
}
