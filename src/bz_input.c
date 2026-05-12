
#include "breezy/bz_input.h"

#include <libinput.h>
#include <xkbcommon/xkbcommon.h>

#include "breezy/bz_breezy.h"
#include "breezy/bz_logger.h"
#include "breezy/bz_seat.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

static int bz_open_restricted(const char *path, int flags, void *data);
static void bz_close_restricted(int fd, void *data);


// =================================================================================================
//  Internal API
// -------------------------------------------------------------------------------------------------

static const struct libinput_interface bz_libinput_interface = {
	.open_restricted = bz_open_restricted,
	.close_restricted = bz_close_restricted,
};

static int bz_open_restricted(const char *path, int flags, void *data) {
	// TODO: Needs to return file descriptor.
	return 0;
}

static void bz_close_restricted(int fd, void *data) {
	// TODO
}


// =================================================================================================
//  Exposed API
// -------------------------------------------------------------------------------------------------

int bz_input_initialize(struct bz_breezy *breezy) {
	// // Initialize udev
	// breezy.input.udev = udev_new();
	// if (!breezy.input.udev) {
	// 	bz_error(BZ_LOG_INPUT, __FILE__, __LINE__, "Failed to create udev context.");
	// 	return -1;
	// }
	//
	// // Create libinput context
	// breezy.input.libinput = libinput_udev_create_context(&bz_libinput_interface, nullptr, breezy.input.udev);
	// if (!breezy.input.libinput) {
	// 	bz_error(BZ_LOG_INPUT, __FILE__, __LINE__, "Failed to create libinput context.");
	// 	return -2;
	// }


	const char *seat_name = bz_seat_name(breezy);
	bz_info(BZ_LOG_INPUT, __FILE__, __LINE__, "Seat name: %s", seat_name);

	bz_info(BZ_LOG_INPUT, __FILE__, __LINE__, "Successfully initialized our input system.");
	return 0;
}

void bz_input_cleanup(struct bz_breezy *breezy) {
}
