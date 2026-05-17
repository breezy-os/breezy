
#include "breezy/bz_input.h"

#include <errno.h>
#include <libinput.h>
#include <string.h>
#include <xkbcommon/xkbcommon.h>

#include "breezy/bz_breezy.h"
#include "breezy/bz_graphics.h"
#include "breezy/bz_list.h"
#include "breezy/bz_logger.h"
#include "breezy/bz_seat.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

static int bz_input_open_restricted(const char *path, int flags, void *data);
static void bz_input_close_restricted(int fd, void *data);
static bool bz_input_device_fd_matches(void *fd, void *device);
static void bz_input_process_kb_event(struct bz_breezy *breezy, struct libinput_event_keyboard *kb_event);
static int bz_input_check_vt_change(bool ctrl_held, bool alt_held, uint32_t keysym);


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

static void bz_input_process_kb_event(struct bz_breezy *breezy, struct libinput_event_keyboard *kb_event)
{
	// Parse the libinput event
	enum libinput_key_state keystate = libinput_event_keyboard_get_key_state(kb_event);
	const uint32_t keycode = libinput_event_keyboard_get_key(kb_event);

	// Feed into xkbcommon
	const uint32_t xkb_keycode = keycode + 8; // xkb keycode is offset by 8 from evdev
	enum xkb_key_direction press_state = keystate ? XKB_KEY_DOWN : XKB_KEY_UP;
	xkb_state_update_key(breezy->input.xkb_state, xkb_keycode, press_state);
	const uint32_t xkb_keysym = xkb_state_key_get_one_sym(breezy->input.xkb_state, xkb_keycode);

	// Figure out our modifier keys
	const bool super_held = xkb_state_mod_name_is_active(breezy->input.xkb_state, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE);
	const bool ctrl_held  = xkb_state_mod_name_is_active(breezy->input.xkb_state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE);
	const bool alt_held   = xkb_state_mod_name_is_active(breezy->input.xkb_state, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE);
	// const bool shift_held = xkb_state_mod_name_is_active(breezy->input.xkb_state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE);

	// First, check for a VT switch. (The only hotkey that doesn't use "super".)
	const int target_vt = bz_input_check_vt_change(ctrl_held, alt_held, xkb_keysym);
	if (target_vt != -1) {
		bz_seat_change_vt(breezy, target_vt);
		return;
	}

	// Now handle all Breezy keycombos (ie, those with "super")
	if (super_held && press_state == XKB_KEY_DOWN) {
		switch (xkb_keysym) {
		case XKB_KEY_Escape:
			bz_shutdown();
			break;
		case XKB_KEY_1:
			bz_graphics_set_color_index(0);
			break;
		case XKB_KEY_2:
			bz_graphics_set_color_index(1);
			break;
		case XKB_KEY_3:
			bz_graphics_set_color_index(2);
			break;
		case XKB_KEY_Up:
			bz_graphics_change_color(breezy, 20.0f/255);
			break;
		case XKB_KEY_Down:
			bz_graphics_change_color(breezy, -20.0f/255);
			break;
		default:
			break; // Does nothing, but shuts up clang-tidy
		}
	}
}

/**
 * Checks if the user is attempting to change VTs using Ctrl+Alt+F1-12. Returns the VT number the
 * user is trying to switch to, or -1 if an attempt is not being made.
 */
static int bz_input_check_vt_change(const bool ctrl_held, const bool alt_held, const uint32_t keysym)
{
	if (!ctrl_held || !alt_held) {
		return -1;
	}

	switch (keysym) {
	case XKB_KEY_F1:  case XKB_KEY_XF86Fn_F1:  case XKB_KEY_XF86Switch_VT_1:  return 1;
	case XKB_KEY_F2:  case XKB_KEY_XF86Fn_F2:  case XKB_KEY_XF86Switch_VT_2:  return 2;
	case XKB_KEY_F3:  case XKB_KEY_XF86Fn_F3:  case XKB_KEY_XF86Switch_VT_3:  return 3;
	case XKB_KEY_F4:  case XKB_KEY_XF86Fn_F4:  case XKB_KEY_XF86Switch_VT_4:  return 4;
	case XKB_KEY_F5:  case XKB_KEY_XF86Fn_F5:  case XKB_KEY_XF86Switch_VT_5:  return 5;
	case XKB_KEY_F6:  case XKB_KEY_XF86Fn_F6:  case XKB_KEY_XF86Switch_VT_6:  return 6;
	case XKB_KEY_F7:  case XKB_KEY_XF86Fn_F7:  case XKB_KEY_XF86Switch_VT_7:  return 7;
	case XKB_KEY_F8:  case XKB_KEY_XF86Fn_F8:  case XKB_KEY_XF86Switch_VT_8:  return 8;
	case XKB_KEY_F9:  case XKB_KEY_XF86Fn_F9:  case XKB_KEY_XF86Switch_VT_9:  return 9;
	case XKB_KEY_F10: case XKB_KEY_XF86Fn_F10: case XKB_KEY_XF86Switch_VT_10: return 10;
	case XKB_KEY_F11: case XKB_KEY_XF86Fn_F11: case XKB_KEY_XF86Switch_VT_11: return 11;
	case XKB_KEY_F12: case XKB_KEY_XF86Fn_F12: case XKB_KEY_XF86Switch_VT_12: return 12;
	default: return -1;
	}
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

int bz_input_activate(struct bz_breezy *breezy)
{
	if (breezy->input.libinput != nullptr) {
		return libinput_resume(breezy->input.libinput);
	}
	return 0;
}

void bz_input_deactivate(struct bz_breezy *breezy)
{
	if (breezy->input.libinput != nullptr) {
		libinput_suspend(breezy->input.libinput);
	}
}

int bz_input_process_events(struct bz_breezy *breezy)
{
	if (libinput_dispatch(breezy->input.libinput) != 0) {
		bz_error(BZ_LOG_INPUT, __FILE__, __LINE__,
			"Failed to dispatch libinput: %s.", strerror(errno));
		return 1;
	}

	struct libinput_event *event;
	while ((event = libinput_get_event(breezy->input.libinput)) != nullptr) {
		if (libinput_event_get_type(event) == LIBINPUT_EVENT_KEYBOARD_KEY) {
			bz_input_process_kb_event(breezy, libinput_event_get_keyboard_event(event));
		}
		libinput_event_destroy(event);
	}

	return 0;
}
