
#include "breezy/bz_wl_devices.h"

#include <stdlib.h>

#include "unity.h"
#include "fff.h"
#include "breezy/bz_wayland.h"
#include "breezy/bz_graphics.h"
#include "breezy/bz_logger.h"
#include "helpers/bz_test_resources.c"


// =================================================================================================
//  Set up / tear down / globals
// -------------------------------------------------------------------------------------------------

DEFINE_FFF_GLOBALS
// -- wl_client --
FAKE_VALUE_FUNC(void *, wl_client_get_user_data, struct wl_client *)
// -- wl_resource --
FAKE_VALUE_FUNC(struct wl_client *, wl_resource_get_client, struct wl_resource *)
FAKE_VALUE_FUNC(void *, wl_resource_get_user_data, struct wl_resource *)
FAKE_VOID_FUNC_VARARG(wl_resource_post_error, struct wl_resource *, uint32_t, const char *, ...)
FAKE_VOID_FUNC(wl_resource_destroy, struct wl_resource *)

void setUp(void)
{
	RESET_FAKE(wl_client_get_user_data);
	RESET_FAKE(wl_resource_get_client);
	RESET_FAKE(wl_resource_get_user_data);
	RESET_FAKE(wl_resource_post_error);
	RESET_FAKE(wl_resource_destroy);
	FFF_RESET_HISTORY();

	bz_log_initialize(BZ_LOG_OFF);
}

void tearDown(void) {}


// =================================================================================================
//  Helper functions for some of our tests
// -------------------------------------------------------------------------------------------------

extern const struct wl_pointer_interface bz_pointer_implementation;


// =================================================================================================
//  Test bz_pointer_set_cursor()
// -------------------------------------------------------------------------------------------------

/** Assigns the given surface the role of "cursor". */
void test_pointer_set_cursor__assigns_cursor_role()
{
	// Set up our mocks
	struct wl_client *client = calloc(1, 1); // Size doesn't matter, and "struct wl_client" isn't defined anywhere.
	struct wl_resource *pointer = calloc(1, sizeof(*pointer));
	struct wl_resource *surface = calloc(1, sizeof(*surface));
	struct bz_client *client_data = bz_create_client_data();
	struct bz_surface *focused_surface = bz_create_surface_data();
	struct bz_surface *pointer_surface = bz_create_surface_data();

	wl_client_get_user_data_fake.return_val = client_data;
	wl_resource_get_client_fake.return_val = client;
	wl_resource_get_user_data_fake.return_val = pointer_surface;

	client_data->breezy->window_mgmt.pointer_focus = focused_surface;
	client_data->last_enter_serial = 3;

	// Run our test
	TEST_ASSERT_EQUAL_INT(BZ_SURF_ROLE_NONE, pointer_surface->role);
	bz_pointer_implementation.set_cursor(client, pointer, 3, surface, 0, 0);
	TEST_ASSERT_EQUAL_INT(BZ_SURF_ROLE_WL_CURSOR, pointer_surface->role);

	// Clean up
	bz_free_surface_data(pointer_surface);
	bz_free_surface_data(focused_surface);
	bz_free_client_data(client_data);
	free(pointer);
	free(surface);
	free(client);
}

/** If the surface already has another role, it raises a protocol error. */
void test_pointer_set_cursor__raises_role_error_for_diff_role()
{
	// Set up our mocks
	struct wl_client *client = calloc(1, 1); // Size doesn't matter, and "struct wl_client" isn't defined anywhere.
	struct wl_resource *pointer = calloc(1, sizeof(*pointer));
	struct wl_resource *surface = calloc(1, sizeof(*surface));
	struct bz_client *client_data = bz_create_client_data();
	struct bz_surface *focused_surface = bz_create_surface_data();
	struct bz_surface *pointer_surface = bz_create_surface_data();

	wl_client_get_user_data_fake.return_val = client_data;
	wl_resource_get_client_fake.return_val = client;
	wl_resource_get_user_data_fake.return_val = pointer_surface;

	client_data->breezy->window_mgmt.pointer_focus = focused_surface;
	client_data->last_enter_serial = 3;
	pointer_surface->role = BZ_SURF_ROLE_XDG_TOPLEVEL; // This causes the failure

	// Run our test
	bz_pointer_implementation.set_cursor(client, pointer, 3, surface, 0, 0);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_POINTER_ERROR_ROLE, wl_resource_post_error_fake.arg1_val);
	TEST_ASSERT_EQUAL_INT(BZ_SURF_ROLE_XDG_TOPLEVEL, pointer_surface->role); // Make sure it hasn't changed

	// Clean up
	bz_free_surface_data(pointer_surface);
	bz_free_surface_data(focused_surface);
	bz_free_client_data(client_data);
	free(pointer);
	free(surface);
	free(client);
}

/** If the given surface is `NULL`, the pointer image is hidden. */
void test_pointer_set_cursor__null_hides_cursor()
{
	// Set up our mocks
	struct wl_client *client = calloc(1, 1); // Size doesn't matter, and "struct wl_client" isn't defined anywhere.
	struct wl_resource *pointer = calloc(1, sizeof(*pointer));
	struct bz_client *client_data = bz_create_client_data();

	wl_client_get_user_data_fake.return_val = client_data;
	wl_resource_get_client_fake.return_val = client; // resource = focused_surface
	client_data->last_enter_serial = 3;

	// Run our test
	bz_pointer_implementation.set_cursor(client, pointer, 3, nullptr, 0, 0);
	TEST_ASSERT_NULL(client_data->cursor_surface);

	// Clean up
	bz_free_client_data(client_data);
	free(pointer);
	free(client);
}

/** Cursor changes if the focus for this `wl_pointer` is one of the requesting client's surfaces. */
void test_pointer_set_cursor__unfocused_client_is_ignored()
{
	// Set up our mocks
	struct wl_client *client1 = calloc(1, 1); // Size doesn't matter, and "struct wl_client" isn't defined anywhere.
	struct wl_client *client2 = calloc(1, 1); // Size doesn't matter, and "struct wl_client" isn't defined anywhere.
	struct wl_resource *pointer = calloc(1, sizeof(*pointer));
	struct wl_resource *pointer_surface = calloc(1, sizeof(*pointer_surface));
	struct bz_client *client_data = bz_create_client_data();
	struct bz_surface *focused_surface = bz_create_surface_data();
	struct bz_surface *pointer_surface_data = bz_create_surface_data();

	wl_client_get_user_data_fake.return_val = client_data;
	wl_resource_get_client_fake.return_val = client1; // Focused surface's client should not be "client2"
	wl_resource_get_user_data_fake.return_val = pointer_surface_data;

	client_data->cursor_surface = nullptr;
	client_data->breezy->window_mgmt.pointer_focus = focused_surface;
	client_data->last_enter_serial = 3;

	// Run our test
	TEST_ASSERT_EQUAL_INT(BZ_SURF_ROLE_NONE, pointer_surface_data->role);
	TEST_ASSERT_NULL(client_data->cursor_surface);
	bz_pointer_implementation.set_cursor(client2, pointer, 3, pointer_surface, 0, 0);
	// Nothing should change
	TEST_ASSERT_EQUAL_INT(BZ_SURF_ROLE_NONE, pointer_surface_data->role);
	TEST_ASSERT_NULL(client_data->cursor_surface);

	// Clean up
	bz_free_surface_data(pointer_surface_data);
	bz_free_surface_data(focused_surface);
	bz_free_client_data(client_data);
	free(pointer);
	free(pointer_surface);
	free(client1);
	free(client2);
}

/** If there was a previous surface set with this request, it is replaced. */
void test_pointer_set_cursor__previous_surface_is_replaced()
{
	// Set up our mocks
	struct wl_client *client = calloc(1, 1); // Size doesn't matter, and "struct wl_client" isn't defined anywhere.
	struct wl_resource *pointer = calloc(1, sizeof(*pointer));
	struct wl_resource *surface = calloc(1, sizeof(*surface));
	struct bz_client *client_data = bz_create_client_data();
	struct bz_surface *focused_surface = bz_create_surface_data();
	struct bz_surface *pointer_surface = bz_create_surface_data();

	wl_client_get_user_data_fake.return_val = client_data;
	wl_resource_get_client_fake.return_val = client;
	wl_resource_get_user_data_fake.return_val = pointer_surface;

	client_data->breezy->window_mgmt.pointer_focus = focused_surface;
	client_data->last_enter_serial = 3;
	client_data->cursor_surface = nullptr;

	// Run our test
	TEST_ASSERT_NULL(client_data->cursor_surface);
	bz_pointer_implementation.set_cursor(client, pointer, 3, surface, 0, 0);
	TEST_ASSERT_EQUAL_INT(pointer_surface, client_data->cursor_surface);

	// Clean up
	bz_free_surface_data(pointer_surface);
	bz_free_surface_data(focused_surface);
	bz_free_client_data(client_data);
	free(pointer);
	free(surface);
	free(client);
}

/** The hotspot parameters define the position of the pointer relative to the pointer location. */
void test_pointer_set_cursor__hotspot_adjusts_position()
{
	// Set up our mocks
	struct wl_client *client = calloc(1, 1); // Size doesn't matter, and "struct wl_client" isn't defined anywhere.
	struct wl_resource *pointer = calloc(1, sizeof(*pointer));
	struct wl_resource *surface = calloc(1, sizeof(*surface));
	struct bz_client *client_data = bz_create_client_data();
	struct bz_surface *focused_surface = bz_create_surface_data();
	struct bz_surface *pointer_surface = bz_create_surface_data();

	wl_client_get_user_data_fake.return_val = client_data;
	wl_resource_get_client_fake.return_val = client;
	wl_resource_get_user_data_fake.return_val = pointer_surface;

	client_data->breezy->window_mgmt.pointer_focus = focused_surface;
	client_data->last_enter_serial = 3;

	// Run our test
	bz_pointer_implementation.set_cursor(client, pointer, 3, surface, 2, 3);
	// The surface's offset is the negated hotspot.
	TEST_ASSERT_EQUAL_INT(-2, pointer_surface->renderable.offset.x);
	TEST_ASSERT_EQUAL_INT(-3, pointer_surface->renderable.offset.y);

	// Clean up
	bz_free_surface_data(pointer_surface);
	bz_free_surface_data(focused_surface);
	bz_free_client_data(client_data);
	free(pointer);
	free(surface);
	free(client);
}

/**
 * If the serial parameter doesn't match the most recent `wl_pointer.enter` serial number, the
 * request will be ignored.
 */
void test_pointer_set_cursor__mismatched_serial_is_ignored()
{
	// Set up our mocks
	struct wl_client *client = calloc(1, 1); // Size doesn't matter, and "struct wl_client" isn't defined anywhere.
	struct wl_resource *pointer = calloc(1, sizeof(*pointer));
	struct wl_resource *surface = calloc(1, sizeof(*surface));
	struct bz_client *client_data = bz_create_client_data();
	struct bz_surface *focused_surface = bz_create_surface_data();
	struct bz_surface *pointer_surface = bz_create_surface_data();

	wl_client_get_user_data_fake.return_val = client_data;
	wl_resource_get_client_fake.return_val = client;
	wl_resource_get_user_data_fake.return_val = pointer_surface;

	client_data->breezy->window_mgmt.pointer_focus = focused_surface;
	client_data->last_enter_serial = 3;
	client_data->cursor_surface = nullptr;

	// Run our test
	bz_pointer_implementation.set_cursor(client, pointer, 2, surface, 0, 0); // Mismatched serial
	// Nothing should have changed
	TEST_ASSERT_EQUAL_INT(BZ_SURF_ROLE_NONE, pointer_surface->role);
	TEST_ASSERT_NULL(client_data->cursor_surface);

	// Clean up
	bz_free_surface_data(pointer_surface);
	bz_free_surface_data(focused_surface);
	bz_free_client_data(client_data);
	free(pointer);
	free(surface);
	free(client);
}


// =================================================================================================
//  Test bz_pointer_release()
// -------------------------------------------------------------------------------------------------

/** All references to our pointer should be cleared. */
void test_pointer_release__clears_our_pointer_reference(void)
{
	// Set up our mocks / data
	struct wl_resource *pointer1 = calloc(1, sizeof(*pointer1));
	struct wl_resource *pointer2 = calloc(1, sizeof(*pointer2));

	struct bz_wl_seat *seat_data = bz_create_seat_data();
	bz_list_append(seat_data->pointers, pointer1);
	bz_list_append(seat_data->pointers, pointer2);
	wl_resource_get_user_data_fake.return_val = seat_data;

	struct bz_client *client_data = bz_create_client_data();
	bz_list_append(client_data->seats, seat_data);
	wl_client_get_user_data_fake.return_val = client_data;

	// Run our test
	TEST_ASSERT_EQUAL_INT(1, client_data->seats->length);
	TEST_ASSERT_EQUAL_PTR(seat_data, client_data->seats->head->data);
	TEST_ASSERT_EQUAL_INT(2, seat_data->pointers->length);
	bz_pointer_implementation.release(nullptr, pointer1);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_destroy_fake.call_count);
	// ".release" only triggers "wl_resource_destroy", which is faked. We should call our dtor manually.
	bz_pointer_dtor(pointer1);
	TEST_ASSERT_EQUAL_INT(1, client_data->seats->length);
	TEST_ASSERT_EQUAL_PTR(seat_data, client_data->seats->head->data);
	TEST_ASSERT_EQUAL_INT(1, seat_data->pointers->length); // Just one now. The other was removed.

	// Clean up
	bz_free_client_data(client_data);
	bz_free_seat_data(seat_data);
	free(pointer2);
	// Don't free pointer1. It should be freed by bz_seat_dtor, and if it's not,
	//   this test needs to fail.
}


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// Test bz_pointer_set_cursor()
	RUN_TEST(test_pointer_set_cursor__assigns_cursor_role);
	RUN_TEST(test_pointer_set_cursor__raises_role_error_for_diff_role);
	RUN_TEST(test_pointer_set_cursor__null_hides_cursor);
	RUN_TEST(test_pointer_set_cursor__unfocused_client_is_ignored);
	RUN_TEST(test_pointer_set_cursor__previous_surface_is_replaced);
	RUN_TEST(test_pointer_set_cursor__hotspot_adjusts_position);
	RUN_TEST(test_pointer_set_cursor__mismatched_serial_is_ignored);

	// Test bz_pointer_release()
	RUN_TEST(test_pointer_release__clears_our_pointer_reference);

	return UNITY_END();
}
