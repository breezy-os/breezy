
#include "breezy/bz_wl_devices.h"

#include <stdlib.h>

#include "unity.h"
#include "fff.h"
#include "breezy/bz_logger.h"
#include "helpers/bz_test_resources.c"
#include "helpers/bz_test_fakes.c"


// =================================================================================================
//  Set up / tear down / globals
// -------------------------------------------------------------------------------------------------

void setUp(void)
{
	bz_reset_fakes();
	bz_log_initialize(BZ_LOG_OFF);
}

void tearDown(void) {}


// =================================================================================================
//  Helper functions for some of our tests
// -------------------------------------------------------------------------------------------------

extern const struct wl_keyboard_interface bz_keyboard_implementation;


// =================================================================================================
//  Test bz_keyboard_release()
// -------------------------------------------------------------------------------------------------

/** All references to our keyboard should be cleared. */
void test_keyboard_release__clears_our_keyboard_reference(void)
{
	// Set up our mocks / data
	struct wl_resource *keyboard1 = calloc(1, sizeof(*keyboard1));
	struct wl_resource *keyboard2 = calloc(1, sizeof(*keyboard2));

	struct bz_wl_seat *seat_data = bz_create_seat_data();
	bz_list_append(seat_data->keyboards, keyboard1);
	bz_list_append(seat_data->keyboards, keyboard2);
	wl_resource_get_user_data_fake.return_val = seat_data;

	struct bz_client *client_data = bz_create_client_data();
	bz_list_append(client_data->seats, seat_data);
	wl_client_get_user_data_fake.return_val = client_data;

	// Run our test
	TEST_ASSERT_EQUAL_INT(1, client_data->seats->length);
	TEST_ASSERT_EQUAL_PTR(seat_data, client_data->seats->head->data);
	TEST_ASSERT_EQUAL_INT(2, seat_data->keyboards->length);
	bz_keyboard_implementation.release(nullptr, keyboard1);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_destroy_fake.call_count);
	// ".release" only triggers "wl_resource_destroy", which is faked. We should call our dtor manually.
	bz_keyboard_dtor(keyboard1);
	TEST_ASSERT_EQUAL_INT(1, client_data->seats->length);
	TEST_ASSERT_EQUAL_PTR(seat_data, client_data->seats->head->data);
	TEST_ASSERT_EQUAL_INT(1, seat_data->keyboards->length); // Just one now. The other was removed.

	// Clean up
	bz_free_client_data(client_data);
	bz_free_seat_data(seat_data);
	free(keyboard2);
	// Don't free keyboard1. It should be freed by bz_seat_dtor, and if it's not,
	//   this test needs to fail.
}


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// Test bz_keyboard_release()
	RUN_TEST(test_keyboard_release__clears_our_keyboard_reference);

	return UNITY_END();
}
