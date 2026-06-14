// This gives us access to signal handling functionality
#define _POSIX_C_SOURCE 200809L // NOLINT

#include <signal.h>
#include <stdio.h>
#include <time.h>

#include <wayland-client.h>


// =================================================================================================
//  Structs
// -------------------------------------------------------------------------------------------------

struct bz_test_client {
	bool terminating;
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
	globals.terminating = true;
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
	bz_add_termint_handler(SIGTERM);
	bz_add_termint_handler(SIGINT);

	// Establish the connection
	struct wl_display *display = wl_display_connect(nullptr);
	if (!display) {
		fprintf(stderr, "Client: Failed to connect to Wayland display.\n");
		return 1;
	}

	// Run our "event loop"
	while (!globals.terminating) {
		printf("Client: Sleeping.\n");
		bz_sleep_ms(1000);
	}

	printf("Client: Disconnecting.\n");
	wl_display_disconnect(display);

	printf("Client: Clean exit.\n");
	return 0;
}
