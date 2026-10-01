
#include "breezy/bz_wp_fractional_scale.h"

#include <stdlib.h>

#include <wayland-server-core.h>
#include <fractional-scale-v1-server-protocol.h>

#include "unity.h"
#include "fff.h"
#include "breezy/bz_logger.h"
#include "breezy/bz_wayland.h"
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

extern const struct wp_fractional_scale_manager_v1_interface bz_wp_fractional_scale_manager_v1_implementation;


// =================================================================================================
//  Test bz_fractional_scale_manager_v1_constructor()
// -------------------------------------------------------------------------------------------------

/** bz_fractional_scale_manager_v1_constructor() properly initializes our resource. */
void test_wp_fractional_scale_manager_v1_constructor__initializes_resource(void)
{
	// Initialize our mocks
	struct wl_resource *frac_scale = malloc(sizeof(*frac_scale));
	wl_resource_create_fake.return_val = frac_scale;

	// Run our test!
	bz_wp_fractional_scale_manager_v1_constructor(nullptr, nullptr, 0, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_PTR(frac_scale, wl_resource_set_implementation_fake.arg0_val);

	// Clean up
	free(frac_scale);
}

/** bz_fractional_scale_manager_v1_constructor() posts a no memory error for failed Wayland resource creation. */
void test_wp_fractional_scale_manager_v1_constructor__posts_no_mem_for_failed_resource(void)
{
	// Set up our mocks
	wl_resource_create_fake.return_val = nullptr;

	// Run our test!
	bz_wp_fractional_scale_manager_v1_constructor(nullptr, nullptr, 0, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_set_implementation_fake.call_count);
}


// =================================================================================================
//  Test bz_wp_fractional_scale_manager_v1_get_fractional_scale()
// -------------------------------------------------------------------------------------------------

void test_wp_fractional_scale_manager_v1_get_fractional_scale__fs_is_added_to_surface(void)
{
	// Set up test data
	struct bz_surface *surface_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = surface_data;
	struct wl_resource *fractional_scale = calloc(1, sizeof(*fractional_scale));
	wl_resource_create_fake.return_val = fractional_scale;

	// Run the test
	bz_wp_fractional_scale_manager_v1_implementation.get_fractional_scale(nullptr, nullptr, 0, nullptr);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	// Verify user data
	struct bz_wp_fractional_scale *fs_data = wl_resource_set_implementation_fake.arg2_val;
	TEST_ASSERT_NOT_NULL(fs_data);
	TEST_ASSERT_EQUAL_PTR(fractional_scale, fs_data->resource);
	TEST_ASSERT_EQUAL_PTR(surface_data->fractional_scale, fs_data);
	TEST_ASSERT_EQUAL_PTR(surface_data, fs_data->surface);

	// Clean up
	free(fractional_scale);
	bz_free_wp_fractional_scale_data(fs_data);
	bz_free_surface_data(surface_data);
}

void test_wp_fractional_scale_manager_v1_get_fractional_scale__existing_fs_raises_error(void)
{
	// Set up test data
	struct bz_wp_fractional_scale *fractional_scale_data = bz_create_wp_fractional_scale_data();
	struct bz_surface *surface_data = bz_create_surface_data();
	surface_data->fractional_scale = fractional_scale_data;
	wl_resource_get_user_data_fake.return_val = surface_data;

	// Run the test
	bz_wp_fractional_scale_manager_v1_implementation.get_fractional_scale(nullptr, nullptr, 0, nullptr);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WP_FRACTIONAL_SCALE_MANAGER_V1_ERROR_FRACTIONAL_SCALE_EXISTS, wl_resource_post_error_fake.arg1_val);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_create_fake.call_count);

	// Clean up
	bz_free_wp_fractional_scale_data(fractional_scale_data);
	bz_free_surface_data(surface_data);
}


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// Test bz_wp_fractional_scale_manager_v1_constructor()
	RUN_TEST(test_wp_fractional_scale_manager_v1_constructor__initializes_resource);
	RUN_TEST(test_wp_fractional_scale_manager_v1_constructor__posts_no_mem_for_failed_resource);

	// Test bz_wp_fractional_scale_manager_v1_get_fractional_scale()
	RUN_TEST(test_wp_fractional_scale_manager_v1_get_fractional_scale__fs_is_added_to_surface);
	RUN_TEST(test_wp_fractional_scale_manager_v1_get_fractional_scale__existing_fs_raises_error);

	return UNITY_END();
}
