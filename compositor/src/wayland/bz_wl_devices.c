
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

static const struct wl_pointer_interface bz_pointer_implementation;
static void bz_pointer_release(struct wl_client *client, struct wl_resource *resource);

// -- wl_keyboard --

static const struct wl_keyboard_interface bz_keyboard_implementation;
static void bz_keyboard_release(struct wl_client *client, struct wl_resource *resource);

// -- wl_output --

static const struct wl_output_interface bz_output_implementation;
static void bz_output_release(struct wl_client *client, struct wl_resource *resource);

// -- wl_data_device_manager --

static const struct wl_data_device_manager_interface bz_data_device_manager_implementation;
static void bz_data_device_manager_create_data_source(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void bz_data_device_manager_get_data_device(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *seat);


// =================================================================================================
//  wl_seat
// -------------------------------------------------------------------------------------------------

void bz_seat_dtor(struct wl_resource *data)
{
	struct bz_wl_seat *seat_data = wl_resource_get_user_data(data);

	bz_list_free(seat_data->keyboards, nullptr);
	bz_list_free(seat_data->pointers, nullptr);

	free(seat_data);
}

/** Gets executed whenever a client binds to wl_seat. */
void bz_seat_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	bz_debug(BZ_LOG_WL_DEVICES, __FILE__, __LINE__, "Binding a client to wl_seat.");

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
	client_data->seat = seat_data;

	// Send the seat's initial capabilities to the client
	uint32_t capabilities =
		(client_data->breezy->input.keyboard_count > 0 ? WL_SEAT_CAPABILITY_KEYBOARD : 0) |
		(client_data->breezy->input.pointer_count  > 0 ? WL_SEAT_CAPABILITY_POINTER  : 0);
	wl_seat_send_name(res, "breezy-seat"); // We only support 1 seat for now, hence a hardcoded name
	wl_seat_send_capabilities(res, capabilities);

	// Success!
	return;

	resource_failed:
		bz_list_free(seat_data->pointers, nullptr);
	pointer_alloc_failed:
		bz_list_free(seat_data->keyboards, nullptr);
	keyboard_alloc_failed:
		free(seat_data);
	user_data_alloc_failed:
		bz_error(BZ_LOG_WL_DEVICES, __FILE__, __LINE__, "Failed to construct a new Wayland seat.");
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
		bz_warn(BZ_LOG_WL_DEVICES, __FILE__, __LINE__,
			"Cannot get pointer. Seat has never had the pointer capability.");
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
		nullptr
	);

	// Add it to our seat's user data
	struct bz_wl_seat *seat_data = client_data->seat;
	if (bz_list_append(seat_data->pointers, res) != 0) {
		wl_client_post_no_memory(client);
		goto append_pointer_failed;
	}

	// Everything succeeded!
	return;

	append_pointer_failed:
		wl_resource_destroy(res);
	resource_failed:
		bz_error(BZ_LOG_WL_DEVICES, __FILE__, __LINE__, "Failed to construct a new wl_pointer.");
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
		bz_warn(BZ_LOG_WL_DEVICES, __FILE__, __LINE__,
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
		nullptr
	);

	// Add it to our seat's user data
	struct bz_wl_seat *seat_data = client_data->seat;
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
			breezy->wayland.display,
			breezy->input.xkb_state,
			res, active_surf->resource
		);
	}

	// Everything succeeded!
	return;

	append_keyboard_failed:
		wl_resource_destroy(res);
	resource_failed:
		bz_error(BZ_LOG_WL_DEVICES, __FILE__, __LINE__, "Failed to construct a new wl_keyboard.");
}

static void bz_seat_get_touch(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
	bz_error(BZ_LOG_WL_DEVICES, __FILE__, __LINE__, "wl_seat.get_touch not implemented");
	// TODO
}

static void bz_seat_release(struct wl_client *client, struct wl_resource *resource)
{
	// Clear our reference to the seat since it's about to be destroyed.
	struct bz_client *client_data = wl_client_get_user_data(client);
	client_data->seat = nullptr;
}


// =================================================================================================
//  wl_pointer
// -------------------------------------------------------------------------------------------------

static const struct wl_pointer_interface bz_pointer_implementation = {
	.release = bz_pointer_release,
};

static void bz_pointer_release(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_DEVICES, __FILE__, __LINE__, "wl_pointer.release not implemented");
	// TODO
}


// =================================================================================================
//  wl_keyboard
// -------------------------------------------------------------------------------------------------

static const struct wl_keyboard_interface bz_keyboard_implementation = {
	.release = bz_keyboard_release,
};

static void bz_keyboard_release(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_DEVICES, __FILE__, __LINE__, "wl_keyboard.release not implemented");
	// TODO
}


// =================================================================================================
//  wl_output
// -------------------------------------------------------------------------------------------------

/** Gets executed whenever a client binds to wl_output. */
void bz_output_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	bz_debug(BZ_LOG_WL_DEVICES, __FILE__, __LINE__, "Binding a client to wl_output.");

	struct wl_resource *res = wl_resource_create(client, &wl_output_interface, version, id);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(res, &bz_output_implementation, nullptr, nullptr);
}

static const struct wl_output_interface bz_output_implementation = {
	.release = bz_output_release
};

static void bz_output_release(struct wl_client *client, struct wl_resource *resource) {
	bz_error(BZ_LOG_WL_DEVICES, __FILE__, __LINE__, "wl_output.release not implemented");
	// TODO
}


// =================================================================================================
//  wl_data_device_manager
// -------------------------------------------------------------------------------------------------

/** Gets executed whenever a client binds to wl_data_device_manager. */
void bz_data_device_manager_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	bz_debug(BZ_LOG_WL_DEVICES, __FILE__, __LINE__, "Binding a client to wl_data_device_manager.");

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

static const struct wl_data_device_manager_interface bz_data_device_manager_implementation = {
	.create_data_source = bz_data_device_manager_create_data_source,
	.get_data_device = bz_data_device_manager_get_data_device,
	// TODO: This is only defined in version 4, yet I have version 3 installed. Upgrade local install?
	// .release = bz_data_device_manager_release,
};

static void bz_data_device_manager_create_data_source(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id
) {
	bz_error(BZ_LOG_WL_DEVICES, __FILE__, __LINE__,
		"wl_data_device_manager.create_data_source not implemented");
	// TODO
}

static void bz_data_device_manager_get_data_device(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id,
	struct wl_resource *seat
) {
	bz_error(BZ_LOG_WL_DEVICES, __FILE__, __LINE__,
		"wl_data_device_manager.get_data_device not implemented");
	// TODO
}
