
#include "breezy/bz_wl_devices.h"

#include <stdint.h>

#include <wayland-server.h>

#include "breezy/bz_logger.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

// -- wl_seat --

static const struct wl_seat_interface bz_seat_implementation;
static void bz_seat_get_pointer(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void bz_seat_get_keyboard(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void bz_seat_get_touch(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void bz_seat_release(struct wl_client *client, struct wl_resource *resource);

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
	bz_debug(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "Binding a client to wl_seat.");

	struct wl_resource *res = wl_resource_create(client, &wl_seat_interface, version, id);
	if (res == nullptr) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(res, &bz_seat_implementation, nullptr, nullptr);
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
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_seat.get_pointer not implemented");
	// TODO
}

static void bz_seat_get_keyboard(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id
) {
	bz_debug(BZ_LOG_WL_DEVICES, __FILE__, __LINE__, "Getting a seat's keyboard device.");
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_seat.get_keyboard not implemented");
	// TODO
}

static void bz_seat_get_touch(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_seat.get_touch not implemented");
	// TODO
}

static void bz_seat_release(struct wl_client *client, struct wl_resource *resource)
{
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_seat.release not implemented");
	// TODO
}


// =================================================================================================
//  wl_output
// -------------------------------------------------------------------------------------------------

/** Gets executed whenever a client binds to wl_output. */
void bz_output_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	bz_debug(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "Binding a client to wl_output.");

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
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "wl_output.release not implemented");
	// TODO
}


// =================================================================================================
//  wl_data_device_manager
// -------------------------------------------------------------------------------------------------

/** Gets executed whenever a client binds to wl_data_device_manager. */
void bz_data_device_manager_constructor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	bz_debug(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__, "Binding a client to wl_data_device_manager.");

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
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__,
		"wl_data_device_manager.create_data_source not implemented");
	// TODO
}

static void bz_data_device_manager_get_data_device(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id,
	struct wl_resource *seat
) {
	bz_error(BZ_LOG_WL_DISPLAY, __FILE__, __LINE__,
		"wl_data_device_manager.get_data_device not implemented");
	// TODO
}
