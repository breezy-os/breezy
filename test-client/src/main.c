// This gives us access to signal handling functionality
#define _POSIX_C_SOURCE 200809L // NOLINT

#include <signal.h>
#include <stdio.h>
#include <sys/poll.h>
#include <sys/eventfd.h>
#include <time.h>
#include <unistd.h>

#include <wayland-client.h>


// =================================================================================================
//  Structs
// -------------------------------------------------------------------------------------------------

struct bz_test_client {
	int is_quitting;
};


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

static struct bz_test_client globals = {0};

static void bz_termint_handler(int signum);
static void bz_add_termint_handler(int signum);

// =================================================================================================
//  Function Definitions
// -------------------------------------------------------------------------------------------------

static void bz_termint_handler(int signum)
{
	printf("Client: Terminated\n");
	constexpr uint64_t val = 1;
	write(globals.is_quitting, &val, sizeof(val));
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


// =================================================================================================
//  Main Program
// -------------------------------------------------------------------------------------------------

int main(void)
{
	globals.is_quitting = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

	bz_add_termint_handler(SIGTERM);
	bz_add_termint_handler(SIGINT);

	// Establish the connection
	struct wl_display *display = wl_display_connect(nullptr);
	if (!display) {
		fprintf(stderr, "Client: Failed to connect to Wayland display.\n");
		return 1;
	}

	int wayland_fd = wl_display_get_fd(display);

	// Run our event loop
	while (true) {
		// "wl_display_prepare_read" must be paired with either "wl_display_read_events" or, if
		//   unable to read events for some reason, "wl_display_cancel_read".
		while (wl_display_prepare_read(display) != 0) {
			wl_display_dispatch_pending(display);
		}

		// Flush pending requests to the server
		if (wl_display_flush(display) < 0) {
			// If errno is EAGAIN, then we'd ideally be polling the display fd to wait for it to
			//   become writable before trying again ... but this client doesn't really matter, so
			//   let's just exit to keep things simple.
			wl_display_cancel_read(display);
			break;
		}

		// Check for changes to our FDs
		struct pollfd fds[2] = {
			{ .fd = globals.is_quitting, .events = POLLIN },
			{ .fd = wayland_fd,          .events = POLLIN },
		};
		const int ret = poll(fds, 2, 1000);
		if (ret == 0) fprintf(stdout, "Client: Timeout waiting for FD.\n");
		if (ret < 0) {
			wl_display_cancel_read(display);
			break; // Failure
		}

		if (fds[0].revents & POLLIN) {
			wl_display_cancel_read(display);
			break; // is_quitting got toggled
		}

		if (fds[1].revents & POLLIN) {
			wl_display_read_events(display);
		} else {
			wl_display_cancel_read(display);
		}

		wl_display_dispatch_pending(display);
	}

	printf("Client: Disconnecting.\n");
	wl_display_disconnect(display);
	close(globals.is_quitting);

	printf("Client: Clean exit.\n");
	return 0;
}
