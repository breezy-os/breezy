
#include "breezy/bz_wl_display.h"

#include <stdlib.h>
#include <wayland-server-core.h>
#include <xkbcommon/xkbcommon.h>

#include "unity.h"
#include "fff.h"
#include "breezy/bz_breezy.h"
#include "breezy/bz_input.h"
#include "breezy/bz_logger.h"
#include "breezy/bz_wayland.h"
#include "breezy/bz_wl_devices.h"

#include "helpers/bz_test_resources.c"


// =================================================================================================
//  Set up / tear down / globals
// -------------------------------------------------------------------------------------------------

DEFINE_FFF_GLOBALS
// -- wl_client --
FAKE_VOID_FUNC(wl_client_post_no_memory, struct wl_client *)
FAKE_VALUE_FUNC(void *, wl_client_get_user_data, struct wl_client *)
// -- wl_resource --
FAKE_VOID_FUNC(wl_resource_set_implementation, struct wl_resource *, const void *, void *, wl_resource_destroy_func_t)
FAKE_VOID_FUNC_VARARG(wl_resource_post_event, struct wl_resource *, uint32_t, ...)
FAKE_VALUE_FUNC(struct wl_resource *, wl_resource_create, struct wl_client *, const struct wl_interface *, int, uint32_t)
FAKE_VOID_FUNC_VARARG(wl_resource_post_error, struct wl_resource *, uint32_t, const char *, ...)
FAKE_VALUE_FUNC(void *, wl_resource_get_user_data, struct wl_resource *)
FAKE_VOID_FUNC(wl_resource_destroy, struct wl_resource *)
// -- xkbcommon --
FAKE_VALUE_FUNC(char *, xkb_keymap_get_as_string, struct xkb_keymap *, enum xkb_keymap_format)


void setUp(void)
{
	RESET_FAKE(wl_client_post_no_memory);
	RESET_FAKE(wl_client_get_user_data);
	RESET_FAKE(wl_resource_set_implementation);
	RESET_FAKE(wl_resource_post_event);
	RESET_FAKE(wl_resource_create);
	RESET_FAKE(wl_resource_post_error);
	RESET_FAKE(wl_resource_get_user_data);
	RESET_FAKE(wl_resource_destroy);
	RESET_FAKE(xkb_keymap_get_as_string);
	FFF_RESET_HISTORY();

	bz_log_initialize(BZ_LOG_OFF);
}

void tearDown(void) {}


// =================================================================================================
//  Helper functions for some of our tests
// -------------------------------------------------------------------------------------------------

extern const struct wl_seat_interface bz_seat_implementation;


// =================================================================================================
//  Test bz_seat_constructor()
// -------------------------------------------------------------------------------------------------

/** bz_seat_constructor() properly initializes our resource. */
void test_seat_constructor__initializes_resource(void)
{
	// Initialize our mocks
	struct wl_resource *seat_res = malloc(sizeof(*seat_res));
	wl_resource_create_fake.return_val = seat_res;
	struct bz_client *client_data = bz_create_client_data();
	wl_client_get_user_data_fake.return_val = client_data;

	// Run our test!
	bz_seat_constructor(nullptr, nullptr, 0, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_client_post_no_memory_fake.call_count);

	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_PTR(seat_res, wl_resource_set_implementation_fake.arg0_val);
	struct bz_wl_seat *seat_data = wl_resource_set_implementation_fake.arg2_val;
	TEST_ASSERT_EQUAL_PTR(seat_data, client_data->seats->head->data);

	// Clean up
	bz_free_seat_data(seat_data);
	bz_free_client_data(client_data); // Also frees the seat
	free(seat_res);
}

/** bz_seat_constructor() posts a no memory error for failed Wayland resource creation. */
void test_seat_constructor__posts_no_mem_for_failed_resource(void)
{
	// Set up our mocks
	wl_resource_create_fake.return_val = nullptr;

	// Run our test!
	bz_seat_constructor(nullptr, nullptr, 0, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_set_implementation_fake.call_count);
}


// =================================================================================================
//  Test the "capabilities" event
// -------------------------------------------------------------------------------------------------

/** The capabilities event is sent when the client first binds to the seat. */
void test_seat_capabilities__sent_when_bound_to_seat(void)
{
	// Initialize our mocks
	struct wl_resource *seat_res = malloc(sizeof(*seat_res));
	wl_resource_create_fake.return_val = seat_res;
	struct bz_client *client_data = bz_create_client_data();
	wl_client_get_user_data_fake.return_val = client_data;

	// Run our test!
	bz_seat_constructor(nullptr, nullptr, 0, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(2, wl_resource_post_event_fake.call_count);
	// Name event should be sent *before* the capabilities event.
	TEST_ASSERT_EQUAL_INT(WL_SEAT_NAME, wl_resource_post_event_fake.arg1_history[0]);
	TEST_ASSERT_EQUAL_INT(WL_SEAT_CAPABILITIES, wl_resource_post_event_fake.arg1_history[1]);

	// Clean up
	bz_free_seat_data(client_data->seats->head->data);
	bz_free_client_data(client_data);
	free(seat_res);
}

/** The capabilities event is sent whenever the seat gains a new capability. */
void test_seat_capabilities__sent_when_capabilities_added(void)
{
	// Initialize our data
	struct bz_client *client_data = bz_create_client_data();
	struct bz_wl_seat *seat_data = bz_create_seat_data();
	bz_list_append(client_data->seats, seat_data);
	wl_client_get_user_data_fake.return_val = client_data;
	struct wl_resource *client_res = malloc(sizeof(*client_res));
	bz_list_append(client_data->breezy->wayland.clients, client_res);

	// Starts out with no capabilities
	TEST_ASSERT_EQUAL_INT(0, client_data->breezy->input.keyboard_count);
	TEST_ASSERT_EQUAL_INT(0, client_data->breezy->input.pointer_count);

	// Add the first keyboard -- should emit "capabilities"
	bz_input_change_device_counts(client_data->breezy, 1, 0);
	TEST_ASSERT_EQUAL_INT(1, client_data->breezy->input.keyboard_count);
	TEST_ASSERT_EQUAL_INT(0, client_data->breezy->input.pointer_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_event_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_SEAT_CAPABILITIES, wl_resource_post_event_fake.arg1_val);
	RESET_FAKE(wl_resource_post_event);

	// Add the first pointer -- should emit "capabilities"
	bz_input_change_device_counts(client_data->breezy, 0, 1);
	TEST_ASSERT_EQUAL_INT(1, client_data->breezy->input.keyboard_count);
	TEST_ASSERT_EQUAL_INT(1, client_data->breezy->input.pointer_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_event_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_SEAT_CAPABILITIES, wl_resource_post_event_fake.arg1_val);
	RESET_FAKE(wl_resource_post_event);

	// Clean up
	bz_free_client_data(client_data);
	bz_free_seat_data(seat_data);
	free(client_res);
}

/** The capabilities event is sent whenever the seat loses a capability. */
void test_seat_capabilities__sent_when_capabilities_removed(void)
{
	// Initialize our data
	struct bz_client *client_data = bz_create_client_data();
	struct bz_wl_seat *seat_data = bz_create_seat_data();
	bz_list_append(client_data->seats, seat_data);
	wl_client_get_user_data_fake.return_val = client_data;
	struct wl_resource *client_res = malloc(sizeof(*client_res));
	bz_list_append(client_data->breezy->wayland.clients, client_res);

	// Starts out with 1 keyboard and 1 pointer capability
	client_data->breezy->input.keyboard_count = 1;
	client_data->breezy->input.pointer_count  = 1;
	TEST_ASSERT_EQUAL_INT(1, client_data->breezy->input.keyboard_count);
	TEST_ASSERT_EQUAL_INT(1, client_data->breezy->input.pointer_count);

	// Remove the final keyboard -- should emit "capabilities"
	bz_input_change_device_counts(client_data->breezy, -1, 0);
	TEST_ASSERT_EQUAL_INT(0, client_data->breezy->input.keyboard_count);
	TEST_ASSERT_EQUAL_INT(1, client_data->breezy->input.pointer_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_event_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_SEAT_CAPABILITIES, wl_resource_post_event_fake.arg1_val);
	RESET_FAKE(wl_resource_post_event);

	// Remove the final pointer -- should emit "capabilities"
	bz_input_change_device_counts(client_data->breezy, 0, -1);
	TEST_ASSERT_EQUAL_INT(0, client_data->breezy->input.keyboard_count);
	TEST_ASSERT_EQUAL_INT(0, client_data->breezy->input.pointer_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_event_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_SEAT_CAPABILITIES, wl_resource_post_event_fake.arg1_val);
	RESET_FAKE(wl_resource_post_event);

	// Clean up
	bz_free_client_data(client_data);
	bz_free_seat_data(seat_data);
	free(client_res);
}

/** The capabilities event is NOT sent when a device changes but doesn't alter seat's capabilities. */
void test_seat_capabilities__not_sent_when_capabilities_unchanged(void)
{
	// Initialize our data
	struct bz_client *client_data = bz_create_client_data();
	struct bz_wl_seat *seat_data = bz_create_seat_data();
	bz_list_append(client_data->seats, seat_data);
	wl_client_get_user_data_fake.return_val = client_data;
	struct wl_resource *client_res = malloc(sizeof(*client_res));
	bz_list_append(client_data->breezy->wayland.clients, client_res);

	// Starts out with 1 keyboard and 1 pointer capability
	client_data->breezy->input.keyboard_count = 1;
	client_data->breezy->input.pointer_count  = 1;
	TEST_ASSERT_EQUAL_INT(1, client_data->breezy->input.keyboard_count);
	TEST_ASSERT_EQUAL_INT(1, client_data->breezy->input.pointer_count);

	// Add a second keyboard and pointer -- should NOT emit "capabilities"
	bz_input_change_device_counts(client_data->breezy, 1, 0);
	bz_input_change_device_counts(client_data->breezy, 0, 1);
	TEST_ASSERT_EQUAL_INT(2, client_data->breezy->input.keyboard_count);
	TEST_ASSERT_EQUAL_INT(2, client_data->breezy->input.pointer_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_event_fake.call_count);
	RESET_FAKE(wl_resource_post_event);

	// Clean up
	bz_free_client_data(client_data);
	bz_free_seat_data(seat_data);
	free(client_res);
}


// =================================================================================================
//  Test bz_seat_get_pointer()
// -------------------------------------------------------------------------------------------------

/** A new pointer is created, and its fields are properly initialized. */
void test_seat_get_pointer__creates_new_pointer(void)
{
	// Set up our mocks
	struct bz_wl_seat *seat_data = bz_create_seat_data();
	wl_resource_get_user_data_fake.return_val = seat_data;

	struct bz_client *client_data = bz_create_client_data();
	bz_list_append(client_data->seats, seat_data);
	client_data->breezy->input.ever_had_pointer = true;
	wl_client_get_user_data_fake.return_val = client_data;

	struct wl_resource *pointer_res = calloc(1, sizeof(*pointer_res));
	wl_resource_create_fake.return_val = pointer_res;

	// Run our test
	TEST_ASSERT_EQUAL_INT(0, seat_data->pointers->length);
	bz_seat_implementation.get_pointer(nullptr, nullptr, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_PTR(pointer_res, wl_resource_set_implementation_fake.arg0_val);
	TEST_ASSERT_EQUAL_INT(1, seat_data->pointers->length);

	// Clean up
	free(pointer_res);
	bz_free_client_data(client_data);
	bz_free_seat_data(seat_data);
}

/**
 * It is a protocol violation (`missing_capability`) to issue this request on a seat that has never
 * had this capability.
 */
void test_seat_get_pointer__posts_missing_capability(void)
{
	// Set up our mocks
	struct bz_client *client_data = bz_create_client_data();
	struct bz_wl_seat *seat_data = bz_create_seat_data();
	bz_list_append(client_data->seats, seat_data);
	wl_client_get_user_data_fake.return_val = client_data;
	struct wl_resource *pointer_res = calloc(1, sizeof(*pointer_res));
	wl_resource_create_fake.return_val = pointer_res;

	// Run our test
	TEST_ASSERT_FALSE(client_data->breezy->input.ever_had_pointer);
	bz_seat_implementation.get_pointer(nullptr, nullptr, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_SEAT_ERROR_MISSING_CAPABILITY, wl_resource_post_error_fake.arg1_val);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, seat_data->pointers->length);

	// Clean up
	free(pointer_res);
	bz_free_client_data(client_data);
	bz_free_seat_data(seat_data);
}

/**
 * If the seat once held this capability, the request should still succeed even if it no longer
 * holds that capability.
 */
void test_seat_get_pointer__succeeds_if_ever_had_capability(void)
{
	// Set up our mocks
	struct bz_wl_seat *seat_data = bz_create_seat_data();
	wl_resource_get_user_data_fake.return_val = seat_data;

	struct bz_client *client_data = bz_create_client_data();
	bz_list_append(client_data->seats, seat_data);
	wl_client_get_user_data_fake.return_val = client_data;

	struct wl_resource *pointer_res = calloc(1, sizeof(*pointer_res));
	wl_resource_create_fake.return_val = pointer_res;

	// Run our test
	client_data->breezy->input.ever_had_pointer = true;
	TEST_ASSERT_TRUE(client_data->breezy->input.ever_had_pointer);
	bz_seat_implementation.get_pointer(nullptr, nullptr, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, seat_data->pointers->length);

	// Clean up
	free(pointer_res);
	bz_free_client_data(client_data);
	bz_free_seat_data(seat_data);
}


// =================================================================================================
//  Test bz_seat_get_keyboard()
// -------------------------------------------------------------------------------------------------

/** A new pointer is created, and its fields are properly initialized. */
void test_seat_get_keyboard__creates_new_keyboard(void)
{
	// Set up our mocks
	struct bz_wl_seat *seat_data = bz_create_seat_data();
	wl_resource_get_user_data_fake.return_val = seat_data;
	struct bz_client *client_data = bz_create_client_data();
	client_data->breezy->input.ever_had_keyboard = true;
	bz_list_append(client_data->seats, seat_data);
	wl_client_get_user_data_fake.return_val = client_data;

	struct wl_resource *keyboard_res = calloc(1, sizeof(*keyboard_res));
	wl_resource_create_fake.return_val = keyboard_res;
	// Since our code frees keymap, we can't use a stack-allocated string constant...
	char *keymap = calloc(strlen("keymap") + 1, sizeof(char));
	strcpy(keymap, "keymap");
	xkb_keymap_get_as_string_fake.return_val = keymap;

	// Run our test
	TEST_ASSERT_EQUAL_INT(0, seat_data->keyboards->length);
	bz_seat_implementation.get_keyboard(nullptr, nullptr, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_PTR(keyboard_res, wl_resource_set_implementation_fake.arg0_val);
	TEST_ASSERT_EQUAL_INT(1, seat_data->keyboards->length);

	// Clean up
	free(keyboard_res);
	bz_free_client_data(client_data);
	bz_free_seat_data(seat_data);
}

/** Keymap and repeat info should be sent when the keyboard is created. */
void test_seat_get_keyboard__sends_keymap_and_repeat_info(void)
{
	// Set up our mocks
	struct bz_wl_seat *seat_data = bz_create_seat_data();
	wl_resource_get_user_data_fake.return_val = seat_data;

	struct bz_client *client_data = bz_create_client_data();
	client_data->breezy->input.ever_had_keyboard = true;
	bz_list_append(client_data->seats, seat_data);
	wl_client_get_user_data_fake.return_val = client_data;

	struct wl_resource *keyboard_res = calloc(1, sizeof(*keyboard_res));
	wl_resource_create_fake.return_val = keyboard_res;
	// Since our code frees keymap, we can't use a stack-allocated string constant...
	char *keymap = calloc(strlen("keymap") + 1, sizeof(char));
	strcpy(keymap, "keymap");
	xkb_keymap_get_as_string_fake.return_val = keymap;

	// Run our test
	bz_seat_implementation.get_keyboard(nullptr, nullptr, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(2, wl_resource_post_event_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_KEYBOARD_KEYMAP, wl_resource_post_event_fake.arg1_history[0]);
	TEST_ASSERT_EQUAL_INT(WL_KEYBOARD_REPEAT_INFO, wl_resource_post_event_fake.arg1_history[1]);

	// Clean up
	free(keyboard_res);
	bz_free_client_data(client_data);
	bz_free_seat_data(seat_data);
}

/**
 * It is a protocol violation (`missing_capability`) to issue this request on a seat that has never
 * had this capability.
 */
void test_seat_get_keyboard__posts_missing_capability(void)
{
	// Set up our mocks
	struct bz_client *client_data = bz_create_client_data();
	struct bz_wl_seat *seat_data = bz_create_seat_data();
	bz_list_append(client_data->seats, seat_data);
	wl_client_get_user_data_fake.return_val = client_data;
	struct wl_resource *keyboard_res = calloc(1, sizeof(*keyboard_res));
	wl_resource_create_fake.return_val = keyboard_res;

	// Run our test
	TEST_ASSERT_FALSE(client_data->breezy->input.ever_had_keyboard);
	bz_seat_implementation.get_keyboard(nullptr, nullptr, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_SEAT_ERROR_MISSING_CAPABILITY, wl_resource_post_error_fake.arg1_val);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, seat_data->keyboards->length);

	// Clean up
	free(keyboard_res);
	bz_free_client_data(client_data);
	bz_free_seat_data(seat_data);
}

/**
 * If the seat once held this capability, the request should still succeed even if it no longer
 * holds that capability.
 */
void test_seat_get_keyboard__succeeds_if_ever_had_capability(void)
{
	// Set up our mocks
	struct bz_wl_seat *seat_data = bz_create_seat_data();
	wl_resource_get_user_data_fake.return_val = seat_data;

	struct bz_client *client_data = bz_create_client_data();
	bz_list_append(client_data->seats, seat_data);
	wl_client_get_user_data_fake.return_val = client_data;
	struct wl_resource *keyboard_res = calloc(1, sizeof(*keyboard_res));
	wl_resource_create_fake.return_val = keyboard_res;
	// Since our code frees keymap, we can't use a stack-allocated string constant...
	char *keymap = calloc(strlen("keymap") + 1, sizeof(char));
	strcpy(keymap, "keymap");
	xkb_keymap_get_as_string_fake.return_val = keymap;

	// Run our test
	client_data->breezy->input.ever_had_keyboard = true;
	TEST_ASSERT_TRUE(client_data->breezy->input.ever_had_keyboard);
	bz_seat_implementation.get_keyboard(nullptr, nullptr, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, seat_data->keyboards->length);

	// Clean up
	free(keyboard_res);
	bz_free_client_data(client_data);
	bz_free_seat_data(seat_data);
}


// =================================================================================================
//  Test bz_seat_get_touch()
// -------------------------------------------------------------------------------------------------

/** A new pointer is created, and its fields are properly initialized. */
void test_seat_get_touch__creates_new_touch(void)
{
	// TODO
}

/**
 * It is a protocol violation (`missing_capability`) to issue this request on a seat that has never
 * had this capability.
 */
void test_seat_get_touch__posts_missing_capability(void)
{
	// TODO
}

/**
 * If the seat once held this capability, the request should still succeed even if it no longer
 * holds that capability.
 */
void test_seat_get_touch__succeeds_if_ever_had_capability(void)
{
	// TODO
}


// =================================================================================================
//  Test bz_seat_release()
// -------------------------------------------------------------------------------------------------

/** All references to our seat should be cleared. */
void test_seat_release__clears_our_seat_reference(void)
{
	// Set up our mocks / data
	struct wl_resource *seat = calloc(1, sizeof(*seat));
	struct bz_wl_seat *seat_data = bz_create_seat_data();
	wl_resource_get_user_data_fake.return_val = seat_data;

	struct bz_client *client_data = bz_create_client_data();
	bz_list_append(client_data->seats, seat_data);
	wl_client_get_user_data_fake.return_val = client_data;

	// Run our test
	TEST_ASSERT_EQUAL_INT(1, client_data->seats->length);
	bz_seat_implementation.release(nullptr, seat);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_destroy_fake.call_count);
	// ".release" only triggers "wl_resource_destroy", which is faked. We should call our dtor manually.
	bz_seat_dtor(seat);
	TEST_ASSERT_EQUAL_INT(0, client_data->seats->length);

	// Clean up
	bz_free_client_data(client_data);
	// Don't free seat data. It should be freed by bz_seat_dtor, and if it's not,
	//   this test needs to fail.
}


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// Test bz_seat_constructor()
	RUN_TEST(test_seat_constructor__initializes_resource);
	RUN_TEST(test_seat_constructor__posts_no_mem_for_failed_resource);

	// Test the "capabilities" event
	RUN_TEST(test_seat_capabilities__sent_when_bound_to_seat);
	RUN_TEST(test_seat_capabilities__sent_when_capabilities_added);
	RUN_TEST(test_seat_capabilities__sent_when_capabilities_removed);
	RUN_TEST(test_seat_capabilities__not_sent_when_capabilities_unchanged);

	// Test bz_seat_get_pointer()
	RUN_TEST(test_seat_get_pointer__creates_new_pointer);
	RUN_TEST(test_seat_get_pointer__posts_missing_capability);
	RUN_TEST(test_seat_get_pointer__succeeds_if_ever_had_capability);

	// Test bz_seat_get_keyboard()
	RUN_TEST(test_seat_get_keyboard__creates_new_keyboard);
	RUN_TEST(test_seat_get_keyboard__sends_keymap_and_repeat_info);
	RUN_TEST(test_seat_get_keyboard__posts_missing_capability);
	RUN_TEST(test_seat_get_keyboard__succeeds_if_ever_had_capability);

	// Test bz_seat_get_touch()
	RUN_TEST(test_seat_get_touch__creates_new_touch);
	RUN_TEST(test_seat_get_touch__posts_missing_capability);
	RUN_TEST(test_seat_get_touch__succeeds_if_ever_had_capability);

	// Test bz_seat_release()
	RUN_TEST(test_seat_release__clears_our_seat_reference);

	return UNITY_END();
}
