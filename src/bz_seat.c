
#include "breezy/bz_seat.h"

#include <libseat.h>

#include "breezy/bz_graphics.h"
#include "breezy/bz_logger.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

static void handle_enable_seat(struct libseat *s, void *data);
static void handle_disable_seat(struct libseat *s, void *data);


// =================================================================================================
//  Internal API
// -------------------------------------------------------------------------------------------------

/**
 * Our libseat seat listener interface implementation. Enable and disable seat will be called
 * whenever our seat is activated or deactivated. Our handlers enable/disable the DRM/rendering
 * stuff as necessary.
 */
static const struct libseat_seat_listener seat_listener = {
	.enable_seat = handle_enable_seat,
	.disable_seat = handle_disable_seat,
};

/**
 * Enables our seat. If it was previously enabled, the proper DRM commands are run to re-enable
 * our rendering state/buffer.
 */
static void handle_enable_seat(struct libseat * /*s*/, void *data) {
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Enabling seat and claiming DRM Master.");
	struct bz_breezy *breezy = data;
	bz_graphics_activate(breezy);
	breezy->seat.active = true;
}

/** Disables our seat, and deactivates our DRM resources. */
static void handle_disable_seat(struct libseat *s, void *data) {
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Disabling seat and releasing DRM Master.");
	struct bz_breezy *breezy = data;
	bz_graphics_deactivate(breezy);
	breezy->seat.active = false;

	// Acknowledge we're done, releasing DRM Master.
	if (libseat_disable_seat(s) != 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to release DRM Master.");
	}
}


// =================================================================================================
//  Exposed API
// -------------------------------------------------------------------------------------------------

/**
 * Initializes our libseat implementation by opening a seat for use by our graphics and input
 * devices. Returns 0 on success, or a negative integer on failure.
 */
int bz_seat_initialize(struct bz_breezy *breezy) {
	// Open a seat
	breezy->seat.seat = libseat_open_seat(&seat_listener, breezy);
	if (!breezy->seat.seat) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to open seat.");
		return -1;
	}

	// Initial dispatch to trigger "handle_enable_seat()"
	if (libseat_dispatch(breezy->seat.seat, 1000) < 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Initial libseat_dispatch failed.");
		return -2;
	}

	if (!breezy->seat.active) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Seat not active after open.");
		return -3;
	}

	breezy->seat.fd = libseat_get_fd(breezy->seat.seat);

	return 0;
}

/** Dispatches a pending libseat event. This should only be called when there are events pending. */
void bz_seat_handle_libseat_event(struct bz_breezy *breezy) {
	bz_debug(BZ_LOG_SEAT, __FILE__, __LINE__, "Handling seat_fd event.");
	if (libseat_dispatch(breezy->seat.seat, 0) < 0) {
		bz_error(BZ_LOG_SEAT, __FILE__, __LINE__, "libseat_dispatch failed.");
	}
}

/**
 * Closes the current seat. Anything that's opened a device via bz_seat_open_device should first
 * call bz_seat_close_device() prior to this function being called.
 */
void bz_seat_cleanup(struct bz_breezy *breezy) {
	if (breezy->seat.seat != nullptr) {
		libseat_close_seat(breezy->seat.seat);
		breezy->seat.seat = nullptr;
	}
}

/** Returns the name of the current seat, or null if the current seat isn't initialized. */
const char *bz_seat_name(struct bz_breezy *breezy) {
	if (breezy->seat.seat == nullptr) {
		return nullptr;
	}
	return libseat_seat_name(breezy->seat.seat);
}

/**
 * Opens a device, returning the libseat device_id. When the device is no longer needed,
 * bz_seat_close_device() should be called.
 */
int bz_seat_open_device(struct bz_breezy *breezy, const char *path, int *fd) {
	return libseat_open_device(breezy->seat.seat, path, fd);
}

int bz_seat_close_device(struct bz_breezy *breezy, const int device_id) {
	return libseat_close_device(breezy->seat.seat, device_id);
}
