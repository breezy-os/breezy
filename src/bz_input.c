
#include "breezy/bz_input.h"

#include <libinput.h>
#include <xkbcommon/xkbcommon.h>

#include "breezy/bz_breezy.h"
#include "breezy/bz_list.h"
#include "breezy/bz_logger.h"
#include "breezy/bz_seat.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

static int bz_input_open_restricted(const char *path, int flags, void *data);
static void bz_input_close_restricted(int fd, void *data);
static bool bz_input_device_fd_matches(void *fd, void *device);


// =================================================================================================
//  Internal API
// -------------------------------------------------------------------------------------------------

static const struct libinput_interface bz_libinput_interface = {
	.open_restricted = bz_input_open_restricted,
	.close_restricted = bz_input_close_restricted,
};

static int bz_input_open_restricted(const char *path, int /*flags*/, void *data) {
	struct bz_breezy *breezy = data;

	// Open the device
	int fd = 0;
	int device_id = bz_seat_open_device(breezy, path, &fd);

	// Track the new device (needed for closing)
	struct bz_input_device *device = malloc(sizeof(*device));
	device->id = device_id;
	device->fd = fd;
	bz_list_append(breezy->input.device_lookup, device);

	return fd;
}

static void bz_input_close_restricted(int fd, void *data) {
	struct bz_breezy *breezy = data;

	// Look up and close the device
	struct bz_input_device *device = bz_list_find(breezy->input.device_lookup, &fd, bz_input_device_fd_matches);
	bz_seat_close_device(breezy, device->id);

	// Clean up the (no longer used) device
	bz_list_remove(breezy->input.device_lookup, device, free);
}

/**
 * Compares the given fd (int *) to the given device (struct bz_input_device *), and if the file
 * descriptors are equal, returns true. Used to search for a specific item inside a bz_list.
 */
static bool bz_input_device_fd_matches(void *fd, void *device)
{
	const int *_fd = fd;
	const struct bz_input_device *_device = device;
	return _device->fd == *_fd;
}


// =================================================================================================
//  Exposed API
// -------------------------------------------------------------------------------------------------

int bz_input_initialize(struct bz_breezy *breezy) {
	// Initialize udev
	breezy->input.udev = udev_new();
	if (!breezy->input.udev) {
		bz_error(BZ_LOG_INPUT, __FILE__, __LINE__, "Failed to create udev context.");
		return -1;
	}

	// Create libinput context
	breezy->input.libinput = libinput_udev_create_context(
		&bz_libinput_interface,
		breezy,
		breezy->input.udev
	);
	if (!breezy->input.libinput) {
		bz_error(BZ_LOG_INPUT, __FILE__, __LINE__, "Failed to create libinput context.");
		return -2;
	}

	// Assign the libinput seat
	const char *seat_name = bz_seat_name(breezy);
	if (libinput_udev_assign_seat(breezy->input.libinput, seat_name) != 0) {
		bz_error(BZ_LOG_INPUT, __FILE__, __LINE__, "Failed to assign libinput seat.");
		return -3;
	}

	// Initialize the xkbcommon context
	breezy->input.xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!breezy->input.xkb_context) {
		bz_error(BZ_LOG_INPUT, __FILE__, __LINE__, "Failed to initialize xkbcommon context.");
		return -4;
	}

	// Create the xkbcommon keymap
	breezy->input.xkb_keymap = xkb_keymap_new_from_names(
		breezy->input.xkb_context,
		nullptr,
		XKB_KEYMAP_COMPILE_NO_FLAGS
	);
	if (!breezy->input.xkb_keymap) {
		bz_error(BZ_LOG_INPUT, __FILE__, __LINE__, "Failed to create xkbcommon keymap.");
		return -5;
	}

	// Create the xkbcommon state
	breezy->input.xkb_state = xkb_state_new(breezy->input.xkb_keymap);
	if (!breezy->input.xkb_state) {
		bz_error(BZ_LOG_INPUT, __FILE__, __LINE__, "Failed to create xkbcommon state.");
		return -6;
	}

	breezy->input.fd = libinput_get_fd(breezy->input.libinput);

	bz_info(BZ_LOG_INPUT, __FILE__, __LINE__, "Successfully initialized our input system.");
	return 0;
}

void bz_input_cleanup(struct bz_breezy *breezy) {
	if (breezy->input.xkb_state) {
		xkb_state_unref(breezy->input.xkb_state);
		breezy->input.xkb_state = nullptr;
	}
	if (breezy->input.xkb_keymap) {
		xkb_keymap_unref(breezy->input.xkb_keymap);
		breezy->input.xkb_keymap = nullptr;
	}
	if (breezy->input.xkb_context) {
		xkb_context_unref(breezy->input.xkb_context);
		breezy->input.xkb_context = nullptr;
	}
	if (breezy->input.libinput) {
		libinput_unref(breezy->input.libinput);
		breezy->input.libinput = nullptr;
	}
	if (breezy->input.udev) {
		udev_unref(breezy->input.udev);
		breezy->input.udev = nullptr;
	}
	if (breezy->input.device_lookup != nullptr) {
		bz_list_free(breezy->input.device_lookup, free);
		breezy->input.device_lookup = nullptr;
	}
}
