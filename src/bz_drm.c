
#include <libseat.h>
#include <poll.h>
#include <unistd.h>
#include <xf86drm.h>

#include "breezy/bz_logger.h"


// =================================================================================================
//  DRM Device
// -------------------------------------------------------------------------------------------------

static int drm_fd = -1;
static int drm_device_id = -1;


// =================================================================================================
//  Seat Management
// -------------------------------------------------------------------------------------------------

static struct libseat *seat = nullptr;
static int seat_fd = -1;
static bool seat_active = false;

static void handle_enable_seat(struct libseat * /*s*/, void * /*data*/) {
	bz_debug(BZ_LOG_DRM, __FILE__, __LINE__, "Enabling seat and claiming DRM Master.");

	seat_active = true;
}

static void handle_disable_seat(struct libseat *s, void * /*data*/) {
	bz_debug(BZ_LOG_DRM, __FILE__, __LINE__, "Disabling seat and releasing DRM Master.");
	seat_active = false;

	// TODO: Clean up DRM state. Ex:
	// // Synchronously disable all CRTCs so the display is clean
	// // Use the *saved* drm_fd — do NOT call libseat functions here
	// drmModeSetCrtc(drm_fd, crtc_id, 0, 0, 0, nullptr, 0, nullptr);

	// Acknowledge we're done, releasing DRM Master.
	if (libseat_disable_seat(s) != 0) {
		bz_error(BZ_LOG_DRM, __FILE__, __LINE__, "Failed to release DRM Master.");
	}
}

static const struct libseat_seat_listener seat_listener = {
	.enable_seat = handle_enable_seat,
	.disable_seat = handle_disable_seat,
};


// =================================================================================================
//  Main Functions
// -------------------------------------------------------------------------------------------------

int bz_drm_initialize(void) {
	// TODO: Improve error handling with gotos

	// Open a seat
	seat = libseat_open_seat(&seat_listener, nullptr);
	if (!seat) {
		bz_error(BZ_LOG_DRM, __FILE__, __LINE__, "Failed to open seat.");
		return 1;
	}

	// Initial dispatch to trigger "handle_enable_seat()"
	if (libseat_dispatch(seat, 1000) < 0) {
		bz_error(BZ_LOG_DRM, __FILE__, __LINE__, "libseat_dispatch failed.");
	}

	if (!seat_active) {
		bz_error(BZ_LOG_DRM, __FILE__, __LINE__, "Seat not active after open.");
		libseat_close_seat(seat);
		return 2;
	}

	seat_fd = libseat_get_fd(seat);

	// Open our DRM device / graphics card, and claim master
	drm_device_id = libseat_open_device(seat, "/dev/dri/card1", &drm_fd);
	if (drm_fd < 0) {
		bz_error(BZ_LOG_DRM, __FILE__, __LINE__, "Failed to open DRM device.");
		libseat_close_seat(seat);
		return 3;
	}
	bz_debug(BZ_LOG_DRM, __FILE__, __LINE__, "Successfully opened DRM device.");

	// (Make sure we're master)
	if (drmIsMaster(drm_fd)) {
		bz_info(BZ_LOG_DRM, __FILE__, __LINE__, "Successfully claimed DRM master.");
	} else {
		bz_error(BZ_LOG_DRM, __FILE__, __LINE__, "Failed to claim DRM master.");
		return 4;
	}

	return 0;
}

int bz_drm_loop_iteration(void) {
	bz_debug(BZ_LOG_DRM, __FILE__, __LINE__, "Executing DRM event loop iteration.");

	// Check/block for changes to our connection status
	struct pollfd fds[2] = {
		{ .fd = seat_fd, .events = POLLIN },
		{ .fd = drm_fd,  .events = POLLIN },
	};

	// 0 indicates a timeout, -1 indicates failure
	const int ret = poll(fds, 2, 1000); // TODO: We're waiting a full second on event loop, meaning this is never bueno.
	if (ret < 0) return -1;
	if (ret == 0) bz_debug(BZ_LOG_DRM, __FILE__, __LINE__, "Timed out polling FDs.");

	// Handle any potential updates from our seat
	if (fds[0].revents & POLLIN) {
		bz_debug(BZ_LOG_DRM, __FILE__, __LINE__, "Handling seat_fd event.");
		if (libseat_dispatch(seat, 0) < 0) {
			bz_error(BZ_LOG_DRM, __FILE__, __LINE__, "libseat_dispatch failed.");
		}
	}

	// TODO: Handle any potential updates from our DRM device..?
	if (fds[1].revents & POLLIN) {
		bz_debug(BZ_LOG_DRM, __FILE__, __LINE__, "Handling drm_fd event.");
		// TODO: drmHandleEvent(drm_fd, /* event context */);
	}

	return 0;
}

int bz_drm_cleanup(void) {
	// TODO: Verify things are initialized before closing.
	bz_debug(BZ_LOG_DRM, __FILE__, __LINE__, "Cleaning up bz_drm.");
	libseat_close_device(seat, drm_device_id);
	close(drm_fd);
	libseat_close_seat(seat);
	return 0;
}