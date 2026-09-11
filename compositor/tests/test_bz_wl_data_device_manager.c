
#include "breezy/bz_wl_display.h"

#include <stdlib.h>
#include <wayland-server-core.h>

#include "unity.h"
#include "fff.h"
#include "breezy/bz_logger.h"
#include "breezy/bz_wayland.h"
#include "breezy/bz_wl_devices.h"

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

extern const struct wl_data_device_manager_interface bz_data_device_manager_implementation;


// =================================================================================================
//  Test bz_data_device_manager_constructor()
// -------------------------------------------------------------------------------------------------

/** bz_data_device_manager_constructor() properly initializes our resource. */
void test_ddm_constructor__initializes_resource(void)
{
	// Initialize our mocks
	struct wl_resource *ddm_res = malloc(sizeof(*ddm_res));
	wl_resource_create_fake.return_val = ddm_res;

	// Run our test!
	bz_data_device_manager_constructor(nullptr, nullptr, 0, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_PTR(ddm_res, wl_resource_set_implementation_fake.arg0_val);

	// Clean up
	free(ddm_res);
}

/** bz_seat_constructor() posts a no memory error for failed Wayland resource creation. */
void test_ddm_constructor__posts_no_mem_for_failed_resource(void)
{
	// Set up our mocks
	wl_resource_create_fake.return_val = nullptr;

	// Run our test!
	bz_data_device_manager_constructor(nullptr, nullptr, 0, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_set_implementation_fake.call_count);
}


// =================================================================================================
//  Test bz_data_device_manager_get_data_device()
// -------------------------------------------------------------------------------------------------

/** A new pointer is created, and its fields are properly initialized. */
void test_ddm_get_data_device__creates_a_dd_on_a_seat(void)
{
	// Set up our mocks
	struct wl_resource *seat = calloc(1, sizeof(*seat));
	struct bz_wl_seat *seat_data = bz_create_seat_data();
	seat_data->resource = seat;
	wl_resource_get_user_data_fake.return_val = seat_data;

	struct wl_resource *data_dev = calloc(1, sizeof(*data_dev));
	wl_resource_create_fake.return_val = data_dev;

	// Run our test
	TEST_ASSERT_EQUAL_INT(0, seat_data->data_devices->length);
	bz_data_device_manager_implementation.get_data_device(nullptr, nullptr, 0, seat);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_PTR(data_dev, wl_resource_set_implementation_fake.arg0_val);
	// ...also on the user data
	TEST_ASSERT_NOT_NULL(wl_resource_set_implementation_fake.arg2_val);
	struct bz_data_device *dd_data = wl_resource_set_implementation_fake.arg2_val;
	TEST_ASSERT_EQUAL_PTR(seat_data, dd_data->seat);
	TEST_ASSERT_EQUAL_INT(1, seat_data->data_devices->length);
	TEST_ASSERT_EQUAL_PTR(dd_data, seat_data->data_devices->head->data);

	// Clean up
	bz_free_data_device_data(dd_data);
	free(data_dev);
	bz_free_seat_data(seat_data);
	free(seat);
}


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// Test bz_data_device_manager_constructor()
	RUN_TEST(test_ddm_constructor__initializes_resource);
	RUN_TEST(test_ddm_constructor__posts_no_mem_for_failed_resource);

	// Test bz_data_device_manager_get_data_device()
	RUN_TEST(test_ddm_get_data_device__creates_a_dd_on_a_seat);

	return UNITY_END();
}
