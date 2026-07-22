
#include "breezy/bz_wl_devices.h"

#include <stdint.h>
#include <stdlib.h>

#include <wayland-server.h>

#include "breezy/bz_logger.h"
#include "breezy/bz_wayland.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

// -- wl_seat --

static const struct wl_seat_interface bz_seat_implementation;
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

/** Gets executed whenever a client binds to wl_seat. */
void bz_seat_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	bz_debug(BZ_LOG_WL_DEVICES, __FILE__, __LINE__, "Binding a client to wl_seat.");

	// Set up the wl_seat resource
	struct wl_resource *res = wl_resource_create(client, &wl_seat_interface, version, id);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		return;
	}
	wl_resource_set_implementation(res, &bz_seat_implementation, nullptr, nullptr);

	// Track the client's seat
	struct bz_client *client_data = wl_client_get_user_data(client);
	client_data->seat = res;

	// Send the seat's initial capabilities to the client
	uint32_t capabilities =
		(client_data->breezy->input.keyboard_count > 0 ? WL_SEAT_CAPABILITY_KEYBOARD : 0) |
		(client_data->breezy->input.pointer_count  > 0 ? WL_SEAT_CAPABILITY_POINTER  : 0);
	wl_seat_send_name(client_data->seat, "breezy-seat"); // We only support 1 seat for now, hence a hardcoded name
	wl_seat_send_capabilities(client_data->seat, capabilities);
}

static const struct wl_seat_interface bz_seat_implementation = {
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

	// Everything succeeded!
	return;

	resource_failed:
		bz_error(BZ_LOG_WL_DEVICES, __FILE__, __LINE__, "Failed to construct a new wl_pointer.");
}

static void bz_seat_get_keyboard(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id
) {
	struct bz_client *client_data = wl_client_get_user_data(client);

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

	// Everything succeeded!
	return;

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
