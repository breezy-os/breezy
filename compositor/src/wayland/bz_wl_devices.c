
#define _GNU_SOURCE

#include "breezy/bz_wl_devices.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <wayland-server.h>
#include <sys/mman.h>
#include <xkbcommon/xkbcommon.h>

#include "breezy/bz_logger.h"
#include "breezy/bz_wayland.h"
#include "breezy/bz_list.h"
#include "breezy/bz_window_management.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

// -- wl_seat --

const struct wl_seat_interface bz_seat_implementation;
static void bz_seat_get_pointer(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void bz_seat_get_keyboard(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void bz_seat_get_touch(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void bz_seat_release(struct wl_client *client, struct wl_resource *resource);

// -- wl_pointer --

const struct wl_pointer_interface bz_pointer_implementation;
static void bz_pointer_set_cursor(struct wl_client *client, struct wl_resource *resource, uint32_t serial, struct wl_resource *surface, int32_t hotspot_x, int32_t hotspot_y);
static void bz_pointer_release(struct wl_client *client, struct wl_resource *resource);
// Helpers
static void bz_write_surface_texture(struct bz_surface *surface_data);

// -- wl_keyboard --

const struct wl_keyboard_interface bz_keyboard_implementation;
static void bz_keyboard_release(struct wl_client *client, struct wl_resource *resource);

// -- wl_output --

const struct wl_output_interface bz_output_implementation;
static void bz_output_release(struct wl_client *client, struct wl_resource *resource);

// -- wl_data_device_manager --

const struct wl_data_device_manager_interface bz_data_device_manager_implementation;
static void bz_data_device_manager_create_data_source(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void bz_data_device_manager_get_data_device(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *seat);

// -- wl_data_device --

void bz_data_device_dtor(struct wl_resource *data);
const struct wl_data_device_interface bz_data_device_implementation;
static void bz_data_device_start_drag(struct wl_client *client, struct wl_resource *resource, struct wl_resource *source, struct wl_resource *origin, struct wl_resource *icon, uint32_t serial);
static void bz_data_device_set_selection(struct wl_client *client, struct wl_resource *resource, struct wl_resource *source, uint32_t serial);
static void bz_data_device_release(struct wl_client *client, struct wl_resource *resource);


// =================================================================================================
//  wl_seat
// -------------------------------------------------------------------------------------------------

void bz_seat_dtor(struct wl_resource *data)
{
	// Clear our reference to the seat since it's about to be destroyed.
	struct wl_client *client = wl_resource_get_client(data);
	struct bz_client *client_data = wl_client_get_user_data(client);
	struct bz_wl_seat *seat_data = wl_resource_get_user_data(data);
	bz_list_remove(client_data->seats, seat_data, nullptr);

	struct bz_data_device *data_device; bz_list_foreach(data_device, seat_data->data_devices) {
		data_device->seat = nullptr;
	}

	bz_list_free(seat_data->keyboards, nullptr);
	bz_list_free(seat_data->pointers, nullptr);
	bz_list_free(seat_data->data_devices, nullptr);

	free(seat_data);
}

/** Gets executed whenever a client binds to wl_seat. */
void bz_seat_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	bz_debug(BZ_LOG_WL_DEVICES, "Binding a client to wl_seat.");

	// Allocate our user data
	struct bz_wl_seat *seat_data = calloc(1, sizeof(*seat_data));
	if (seat_data == nullptr) {
		wl_client_post_no_memory(client);
		goto user_data_alloc_failed;
	}
	seat_data->keyboards = bz_list_create();
	if (seat_data->keyboards == nullptr) {
		wl_client_post_no_memory(client);
		goto keyboard_alloc_failed;
	}
	seat_data->pointers = bz_list_create();
	if (seat_data->pointers == nullptr) {
		wl_client_post_no_memory(client);
		goto pointer_alloc_failed;
	}
	seat_data->data_devices = bz_list_create();
	if (seat_data->data_devices == nullptr) {
		wl_client_post_no_memory(client);
		goto data_device_alloc_failed;
	}

	// Set up the wl_seat resource
	struct wl_resource *res = wl_resource_create(client, &wl_seat_interface, version, id);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		goto resource_failed;
	}
	wl_resource_set_implementation(res, &bz_seat_implementation, seat_data, bz_seat_dtor);

	// Populate the user data
	seat_data->resource = res;

	// Track the client's seat
	struct bz_client *client_data = wl_client_get_user_data(client);
	int status = bz_list_append(client_data->seats, seat_data);
	if (status != 0) {
		goto seat_append_failed;
	}

	// Send the seat's initial capabilities to the client
	uint32_t capabilities =
		(client_data->breezy->input.keyboard_count > 0 ? WL_SEAT_CAPABILITY_KEYBOARD : 0) |
		(client_data->breezy->input.pointer_count  > 0 ? WL_SEAT_CAPABILITY_POINTER  : 0);
	wl_seat_send_name(res, "breezy-seat"); // We only support 1 seat for now, hence a hardcoded name
	wl_seat_send_capabilities(res, capabilities);

	// Success!
	return;

	seat_append_failed:
		wl_resource_destroy(res);
	resource_failed:
		bz_list_free(seat_data->data_devices, nullptr);
	data_device_alloc_failed:
		bz_list_free(seat_data->pointers, nullptr);
	pointer_alloc_failed:
		bz_list_free(seat_data->keyboards, nullptr);
	keyboard_alloc_failed:
		free(seat_data);
	user_data_alloc_failed:
		bz_error(BZ_LOG_WL_DEVICES, "Failed to construct a new Wayland seat.");
}

const struct wl_seat_interface bz_seat_implementation = {
	.get_pointer = bz_seat_get_pointer,
	.get_keyboard = bz_seat_get_keyboard,
	.get_touch = bz_seat_get_touch,
	.release = bz_seat_release
};

static void bz_seat_get_pointer(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id
) {
	struct bz_client *client_data = wl_client_get_user_data(client);

	// Prechecks
	if (!client_data->breezy->input.ever_had_pointer) {
		bz_warn(BZ_LOG_WL_DEVICES, "Cannot get pointer. Seat has never had the pointer capability.");
		wl_resource_post_error(resource, WL_SEAT_ERROR_MISSING_CAPABILITY,
			"Cannot get pointer. Seat has never had the pointer capability.");
		return;
	}

	// Create the resource
	struct wl_resource *res = wl_resource_create(
		client,
		&wl_pointer_interface,
		BZ_POINTER_VERSION,
		id
	);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		goto resource_failed;
	}
	wl_resource_set_implementation(
		res,
		&bz_pointer_implementation,
		nullptr,
		bz_pointer_dtor
	);

	// Add it to our seat's user data
	struct bz_wl_seat *seat_data = wl_resource_get_user_data(resource);
	if (bz_list_append(seat_data->pointers, res) != 0) {
		wl_client_post_no_memory(client);
		goto append_pointer_failed;
	}

	// Everything succeeded!
	return;

	append_pointer_failed:
		wl_resource_destroy(res);
	resource_failed:
		bz_error(BZ_LOG_WL_DEVICES, "Failed to construct a new wl_pointer.");
}

static void bz_seat_get_keyboard(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id
) {
	struct bz_client *client_data = wl_client_get_user_data(client);
	struct bz_breezy *breezy = client_data->breezy;

	// Prechecks
	if (!client_data->breezy->input.ever_had_keyboard) {
		bz_warn(BZ_LOG_WL_DEVICES,
			"Cannot get keyboard. Seat has never had the keyboard capability.");
		wl_resource_post_error(resource, WL_SEAT_ERROR_MISSING_CAPABILITY,
			"Cannot get keyboard. Seat has never had the keyboard capability.");
		return;
	}

	// Create the resource
	struct wl_resource *res = wl_resource_create(
		client,
		&wl_keyboard_interface,
		BZ_KEYBOARD_VERSION,
		id
	);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		goto resource_failed;
	}
	wl_resource_set_implementation(
		res,
		&bz_keyboard_implementation,
		nullptr,
		bz_keyboard_dtor
	);

	// Add it to our seat's user data
	struct bz_wl_seat *seat_data = wl_resource_get_user_data(resource);
	if (bz_list_append(seat_data->keyboards, res) != 0) {
		wl_client_post_no_memory(client);
		goto append_keyboard_failed;
	}

	// Send the "keymap" event
	char *keymap = xkb_keymap_get_as_string(breezy->input.xkb_keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
	size_t size = strlen(keymap) + 1;
	int keymap_fd = memfd_create("breezy-xkb-keymap", MFD_CLOEXEC);
	ftruncate(keymap_fd, size);
	void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, keymap_fd, 0);
	memcpy(ptr, keymap, size);
	munmap(ptr, size);
	wl_keyboard_send_keymap(res, 1, keymap_fd, size);
	close(keymap_fd);
	free(keymap);

	// Send the initial "repeat_info" event. Just sane defaults for now: 25Hz rate, 600ms delay
	wl_keyboard_send_repeat_info(res, 25, 600);

	// If the active/focused surface belongs to the current client, then we should also send it the
	//   keyboard "enter + modifiers" events.
	struct bz_surface *active_surf = bz_mgmt_get_active_surface(&breezy->window_mgmt);
	if (active_surf != nullptr && wl_resource_get_client(active_surf->resource) == client) {
		bz_mgmt_notify_kb_enter(
			wl_display_next_serial(breezy->wayland.display),
			wl_display_next_serial(breezy->wayland.display),
			breezy->input.xkb_state,
			res, active_surf->resource
		);
	}

	// Everything succeeded!
	return;

	append_keyboard_failed:
		wl_resource_destroy(res);
	resource_failed:
		bz_error(BZ_LOG_WL_DEVICES, "Failed to construct a new wl_keyboard.");
}

static void bz_seat_get_touch(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
	bz_error(BZ_LOG_WL_DEVICES, "wl_seat.get_touch not implemented");
	// TODO
}

static void bz_seat_release(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}


// =================================================================================================
//  wl_pointer
// -------------------------------------------------------------------------------------------------

void bz_pointer_dtor(struct wl_resource *data)
{
	struct wl_client *client = wl_resource_get_client(data);
	struct bz_client *client_data = wl_client_get_user_data(client);

	// We don't know which seat it's part of ...but we know it can only be part of one.
	struct bz_wl_seat *seat; bz_list_foreach(seat, client_data->seats) {
		if (bz_list_remove(seat->pointers, data, nullptr) == 1) {
			break; // Found it, so early exit our loop.
		}
	}
}

const struct wl_pointer_interface bz_pointer_implementation = {
	.set_cursor = bz_pointer_set_cursor,
	.release = bz_pointer_release,
};

static void bz_pointer_set_cursor(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t serial,
	struct wl_resource *surface,
	int32_t hotspot_x,
	int32_t hotspot_y
) {
	struct bz_client *client_data = wl_client_get_user_data(client);

	// There needs to be a pointer-focused surface to have arrived here.
	struct bz_surface *focused_surface = client_data->breezy->window_mgmt.pointer_focus;
	if (focused_surface == nullptr) {
		bz_info(BZ_LOG_WL_DEVICES,
			"wl_pointer::set_cursor called without a surface having pointer focus.");
		return;
	}

	// Make sure the submitted serial matches the latest "enter" serial for this seat.
	if (client_data->last_enter_serial != serial) {
		bz_info(BZ_LOG_WL_DEVICES,
			"wl_pointer::set_cursor called with an outdated serial.");
		return;
	}

	// Cursor only changes if the focus for this `wl_pointer` is one of the requesting
	//   client's surfaces.
	struct wl_client *focused_client = wl_resource_get_client(focused_surface->resource);
	if (focused_client != client) {
		bz_info(BZ_LOG_WL_DEVICES,
			"A currently inactive client called pointer::set_cursor.");
		return;
	}

	// If the given surface is NULL, the pointer image is hidden.
	if (surface == nullptr) {
		client_data->cursor_surface = nullptr;
		return;
	}

	// If the surface already has another role, it raises a protocol error.
	struct bz_surface *surface_data = wl_resource_get_user_data(surface);
	if (surface_data->role != BZ_SURF_ROLE_NONE && surface_data->role != BZ_SURF_ROLE_WL_CURSOR) {
		wl_resource_post_error(resource, WL_POINTER_ERROR_ROLE, "Surface role cannot be changed.");
		goto initial_checks_failed;
	}

	// Ok, all checks out. Let's make the change!
	surface_data->renderable.offset.x = -hotspot_x;
	surface_data->renderable.offset.y = -hotspot_y;
	surface_data->renderable.position.x = client_data->breezy->window_mgmt.last_cursor_loc.x;
	surface_data->renderable.position.y = client_data->breezy->window_mgmt.last_cursor_loc.y;
	surface_data->role = BZ_SURF_ROLE_WL_CURSOR;
	client_data->cursor_surface = surface_data;

	// Everything succeeded!
	return;

	initial_checks_failed:
		bz_error(BZ_LOG_WL_DEVICES, "Failed to execute 'pointer::set_cursor'.");
}

static void bz_pointer_release(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}


// =================================================================================================
//  wl_keyboard
// -------------------------------------------------------------------------------------------------

void bz_keyboard_dtor(struct wl_resource *data)
{
	struct wl_client *client = wl_resource_get_client(data);
	struct bz_client *client_data = wl_client_get_user_data(client);

	// We don't know which seat it's part of ...but we know it can only be part of one.
	struct bz_wl_seat *seat; bz_list_foreach(seat, client_data->seats) {
		if (bz_list_remove(seat->keyboards, data, nullptr) == 1) {
			break; // Found it, so early exit our loop.
		}
	}
}

const struct wl_keyboard_interface bz_keyboard_implementation = {
	.release = bz_keyboard_release,
};

static void bz_keyboard_release(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}


// =================================================================================================
//  wl_output
// -------------------------------------------------------------------------------------------------

/** Gets executed whenever a client binds to wl_output. */
void bz_output_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	bz_debug(BZ_LOG_WL_DEVICES, "Binding a client to wl_output.");

	struct wl_resource *res = wl_resource_create(client, &wl_output_interface, version, id);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(res, &bz_output_implementation, nullptr, nullptr);

	// Emit events describing the output.
	// TODO-dl12: Currently just filler data Some of this we can get from the DRM mode connector.
	struct bz_breezy *breezy = data;
	wl_output_send_geometry(res, 0, 0, 500, 330, WL_OUTPUT_SUBPIXEL_UNKNOWN, "Dell", "Monitor", WL_OUTPUT_TRANSFORM_NORMAL);
	wl_output_send_mode(res, WL_OUTPUT_MODE_CURRENT, breezy->drm.mode_info.hdisplay, breezy->drm.mode_info.vdisplay, breezy->drm.mode_info.vrefresh);
	wl_output_send_scale(res, 1);
	wl_output_send_name(res, breezy->drm.mode_info.name); // TODO: This isn't a good name..
	// wl_output_send_description(res, "I am a monitor.");
	wl_output_send_done(res);
}

const struct wl_output_interface bz_output_implementation = {
	.release = bz_output_release
};

static void bz_output_release(struct wl_client *client, struct wl_resource *resource) {
	bz_error(BZ_LOG_WL_DEVICES, "wl_output.release not implemented");
	// TODO
}


// =================================================================================================
//  wl_data_device_manager
// -------------------------------------------------------------------------------------------------

/** Gets executed whenever a client binds to wl_data_device_manager. */
void bz_data_device_manager_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	bz_debug(BZ_LOG_WL_DEVICES, "Binding a client to wl_data_device_manager.");

	struct wl_resource *res = wl_resource_create(
		client,
		&wl_data_device_manager_interface,
		version,
		id
	);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(res, &bz_data_device_manager_implementation, nullptr, nullptr);
}

const struct wl_data_device_manager_interface bz_data_device_manager_implementation = {
	.create_data_source = bz_data_device_manager_create_data_source,
	.get_data_device = bz_data_device_manager_get_data_device,
};

static void bz_data_device_manager_create_data_source(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id
) {
	bz_error(BZ_LOG_WL_DEVICES, "wl_data_device_manager.create_data_source not implemented");
	// TODO
}

static void bz_data_device_manager_get_data_device(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id,
	struct wl_resource *seat
) {
	// Allocate our new user data
	struct bz_data_device *data_device_data = calloc(1, sizeof(*data_device_data));
	if (data_device_data == nullptr) {
		bz_error(BZ_LOG_WL_DEVICES, "Failed to allocate memory for the data device.");
		goto data_device_alloc_failed;
	}

	// Create the resource
	struct wl_resource *res = wl_resource_create(
		client,
		&wl_data_device_interface,
		BZ_DATA_DEVICE_VERSION,
		id
	);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		goto resource_failed;
	}
	wl_resource_set_implementation(
		res,
		&bz_data_device_implementation,
		data_device_data,
		bz_data_device_dtor
	);

	struct bz_wl_seat *seat_data = wl_resource_get_user_data(seat);

	// Save our user data
	data_device_data->resource = res;
	data_device_data->seat = seat_data;

	// Add it to our seat's user data
	if (bz_list_append(seat_data->data_devices, data_device_data) != 0) {
		wl_client_post_no_memory(client);
		goto append_data_device_failed;
	}

	// Everything succeeded!
	return;

	append_data_device_failed:
		wl_resource_destroy(res);
	resource_failed:
		free(data_device_data);
	data_device_alloc_failed:
		bz_error(BZ_LOG_WL_DEVICES, "Failed to construct a new wl_data_device.");
}


// =================================================================================================
//  wl_data_device
// -------------------------------------------------------------------------------------------------

void bz_data_device_dtor(struct wl_resource *data)
{
	struct bz_data_device *data_device_data = wl_resource_get_user_data(data);

	if (data_device_data->seat != nullptr) {
		bz_list_remove(data_device_data->seat->data_devices, data_device_data, nullptr);
	}

	free(data_device_data);
}

const struct wl_data_device_interface bz_data_device_implementation = {
	.start_drag = bz_data_device_start_drag,
	.set_selection = bz_data_device_set_selection,
	.release = bz_data_device_release,
};

static void bz_data_device_start_drag(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *source,
	struct wl_resource *origin,
	struct wl_resource *icon,
	uint32_t serial
) {
	bz_error(BZ_LOG_WL_DEVICES, "wl_data_device.start_drag not implemented");
	// TODO
}

static void bz_data_device_set_selection(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *source,
	uint32_t serial
) {
	bz_error(BZ_LOG_WL_DEVICES, "wl_data_device.set_selection not implemented");
	// TODO
}

static void bz_data_device_release(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_DEVICES, "wl_data_device.release not implemented");
	// TODO
}
