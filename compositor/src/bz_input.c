// This gives us access to setenv() for setting WAYLAND_DISPLAY on the forked child process
#define _POSIX_C_SOURCE 200809L // NOLINT

#include "breezy/bz_input.h"

#include <errno.h>
#include <libinput.h>
#include <libgen.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wayland-server.h>
#include <xkbcommon/xkbcommon.h>

#include "breezy/bz_breezy.h"
#include "breezy/bz_graphics.h"
#include "breezy/bz_list.h"
#include "breezy/bz_logger.h"
#include "breezy/bz_seat.h"
#include "breezy/bz_wayland.h"
#include "breezy/bz_wl_devices.h"
#include "breezy/bz_wl_display.h"
#include "breezy/bz_xdg_shell.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

static int bz_input_open_restricted(const char *path, int flags, void *data);
static void bz_input_close_restricted(int fd, void *data);
static bool bz_input_device_fd_matches(void *fd, void *device);
static void bz_input_process_hotplug_event(struct bz_breezy *breezy, struct libinput_device *device, bool new_plugged_status);
static void bz_input_process_kb_event(struct bz_breezy *breezy, struct libinput_event_keyboard *kb_event);
static int bz_input_check_vt_change(bool ctrl_held, bool alt_held, uint32_t keysym);
static void bz_input_spawn_child(const char *socket_name, const char *program_path);


// =================================================================================================
//  Internal API
// -------------------------------------------------------------------------------------------------

static const struct libinput_interface bz_libinput_interface = {
	.open_restricted = bz_input_open_restricted,
	.close_restricted = bz_input_close_restricted,
};

static int bz_input_open_restricted(const char *path, int /*flags*/, void *data)
{
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

static void bz_input_close_restricted(int fd, void *data)
{
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

static void bz_input_process_hotplug_event(struct bz_breezy *breezy, struct libinput_device *device, bool new_plugged_status)
{
	// Record our initial capabilities so we know if they've changed.
	bool had_keyboard = breezy->input.keyboard_count > 0;
	bool had_pointer  = breezy->input.pointer_count  > 0;

	// Update our plugged in keyboard/pointer counts
	if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_KEYBOARD)) {
		breezy->input.keyboard_count += new_plugged_status ? 1 : -1;
	}
	if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_POINTER)) {
		breezy->input.pointer_count += new_plugged_status ? 1 : -1;
	}

	// If the capabilities have changed, broadcast the new capabilities to each client.
	bool has_keyboard = breezy->input.keyboard_count > 0;
	bool has_pointer  = breezy->input.pointer_count  > 0;
	if (had_keyboard != has_keyboard || had_pointer != has_pointer) {
		uint32_t capabilities =
			(has_keyboard ? WL_SEAT_CAPABILITY_KEYBOARD : 0) |
			(has_pointer ? WL_SEAT_CAPABILITY_POINTER : 0);
		struct bz_node *node = breezy->wayland.clients->head;
		while (node != nullptr) {
			struct wl_client *client = node->data;
			struct bz_client *client_data = wl_client_get_user_data(client);
			if (client_data->seat != nullptr) {
				wl_seat_send_capabilities(client_data->seat->resource, capabilities);
			}
			node = node->next;
		}
	}

	// We need to track if our seat *ever* had these things, so toggle them to true if able.
	if (has_keyboard) { breezy->input.ever_had_keyboard = true; }
	if (has_pointer)  { breezy->input.ever_had_pointer  = true; }

	bz_info(BZ_LOG_INPUT, __FILE__, __LINE__,
		"Device %s. New counts: [keyboards: %d], [pointers: %d]",
		new_plugged_status ? "plugged in" : "unplugged",
		breezy->input.keyboard_count,
		breezy->input.pointer_count);
}

static void bz_input_process_kb_event(struct bz_breezy *breezy, struct libinput_event_keyboard *kb_event)
{
	// Parse the libinput event
	enum libinput_key_state keystate = libinput_event_keyboard_get_key_state(kb_event);
	const uint32_t keycode = libinput_event_keyboard_get_key(kb_event);

	// Feed into xkbcommon
	const uint32_t xkb_keycode = keycode + 8; // xkb keycode is offset by 8 from evdev
	enum xkb_key_direction press_state = keystate ? XKB_KEY_DOWN : XKB_KEY_UP;
	enum xkb_state_component changes = xkb_state_update_key(breezy->input.xkb_state, xkb_keycode, press_state);
	const uint32_t xkb_keysym = xkb_state_key_get_one_sym(breezy->input.xkb_state, xkb_keycode);

	// Figure out our modifier keys
	const bool super_held = xkb_state_mod_name_is_active(breezy->input.xkb_state, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE);
	const bool ctrl_held  = xkb_state_mod_name_is_active(breezy->input.xkb_state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE);
	const bool alt_held   = xkb_state_mod_name_is_active(breezy->input.xkb_state, XKB_MOD_NAME_ALT,  XKB_STATE_MODS_EFFECTIVE);
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

		// Quit Compositor
		case XKB_KEY_Escape:
			breezy->is_terminating = true;
			wl_display_terminate(breezy->wayland.display);
			return;

		// Start / Stop Applications
		case XKB_KEY_t:
			bz_input_spawn_child(breezy->wayland.socket_name, "/home/ben/git/breezy/build/test-client/test-client");
			return;
		case XKB_KEY_q:
			bz_mgmt_close_active_window(&breezy->window_mgmt);
			return;

		// Change Colors
		case XKB_KEY_1:
			bz_graphics_set_color_index(0);
			return;
		case XKB_KEY_2:
			bz_graphics_set_color_index(1);
			return;
		case XKB_KEY_3:
			bz_graphics_set_color_index(2);
			return;
		case XKB_KEY_Up:
			bz_graphics_change_color(breezy, 20.0f/255);
			return;
		case XKB_KEY_Down:
			bz_graphics_change_color(breezy, -20.0f/255);
			return;

		default:
			break; // Does nothing, but shuts up clang-tidy
		}
	}

	// Any that weren't caught, send onwards to the active surface.
	if (changes) {
		struct xkb_state *xkbstate = breezy->input.xkb_state;
		struct bz_surface *active_surface = bz_mgmt_get_active_surface(&breezy->window_mgmt);
		if (active_surface != nullptr) {
			struct wl_client *client = wl_resource_get_client(active_surface->resource);
			struct bz_client *client_data = wl_client_get_user_data(client);
			struct bz_node *kb_node = client_data->seat->keyboards->head;
			while (kb_node != nullptr) {
				struct wl_resource *keyboard = kb_node->data;
				wl_keyboard_send_key(
					keyboard,
					wl_display_next_serial(breezy->wayland.display),
					libinput_event_keyboard_get_time(kb_event),
					keycode,
					keystate ? WL_KEYBOARD_KEY_STATE_PRESSED : WL_KEYBOARD_KEY_STATE_RELEASED
				);
				if (changes & (XKB_STATE_MODS_EFFECTIVE | XKB_STATE_LAYOUT_EFFECTIVE)) {
					// A modifier changed, so re-send the full modifiers event.
					wl_keyboard_send_modifiers(
						keyboard,
						wl_display_next_serial(breezy->wayland.display),
						xkb_state_serialize_mods(xkbstate,   XKB_STATE_MODS_DEPRESSED),
						xkb_state_serialize_mods(xkbstate,   XKB_STATE_MODS_LATCHED),
						xkb_state_serialize_mods(xkbstate,   XKB_STATE_MODS_LOCKED),
						xkb_state_serialize_layout(xkbstate, XKB_STATE_LAYOUT_EFFECTIVE)
					);
				}
				kb_node = kb_node->next;
			}
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

/**
 * Forks the current process, and runs the provided program_path in its place. The socket_name is
 * provided to the WAYLAND_DISPLAY environment variable.
 */
static void bz_input_spawn_child(const char *socket_name, const char *program_path)
{
	const pid_t pid = fork();
	if (pid == -1) {
		bz_error(BZ_LOG_INPUT, __FILE__, __LINE__, "Failed to fork the child process.");
		return;
	}

	// -- Child Process --
	if (pid == 0) {
		// Unblock all signals
		sigset_t set;
		sigfillset(&set);
		sigprocmask(SIG_UNBLOCK, &set, nullptr);

		// Launch the program
		setenv("WAYLAND_DISPLAY", socket_name, 1);
		const char *prog_name = basename((char *)program_path);
		if (prog_name) {
			execlp(program_path, prog_name, nullptr);
		}

		// This is unreachable when everything goes correctly
		_exit(1);
	}

	// -- Parent Process --
	// Nothing to do!
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

int bz_input_process_events(int /*fd*/, uint32_t /*mask*/, void *data)
{
	struct bz_breezy *breezy = data;
	if (libinput_dispatch(breezy->input.libinput) != 0) {
		bz_error(BZ_LOG_INPUT, __FILE__, __LINE__,
			"Failed to dispatch libinput: %s.", strerror(errno));
		return 0;
	}

	struct libinput_event *event;
	while ((event = libinput_get_event(breezy->input.libinput)) != nullptr) {
		if (libinput_event_get_type(event) == LIBINPUT_EVENT_KEYBOARD_KEY) {
			bz_input_process_kb_event(breezy, libinput_event_get_keyboard_event(event));
		} else if (libinput_event_get_type(event) == LIBINPUT_EVENT_DEVICE_ADDED) {
			bz_input_process_hotplug_event(breezy, libinput_event_get_device(event), 1);
		} else if (libinput_event_get_type(event) == LIBINPUT_EVENT_DEVICE_REMOVED) {
			bz_input_process_hotplug_event(breezy, libinput_event_get_device(event), 0);
		}
		libinput_event_destroy(event);
	}

	return 0;
}
