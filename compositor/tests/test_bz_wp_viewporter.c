
#include "breezy/bz_wp_viewporter.h"

#include <stdlib.h>
#include <wayland-server-core.h>

#include "unity.h"
#include "fff.h"
#include "../../build/compositor/src/viewporter-server-protocol.h"
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

extern const struct wp_viewporter_interface bz_wp_viewporter_implementation;


// =================================================================================================
//  Test bz_viewporter_constructor()
// -------------------------------------------------------------------------------------------------

/** bz_viewporter_constructor() properly initializes our resource. */
void test_wp_viewporter_constructor__initializes_resource(void)
{
	// Initialize our mocks
	struct wl_resource *viewporter = malloc(sizeof(*viewporter));
	wl_resource_create_fake.return_val = viewporter;

	// Run our test!
	bz_wp_viewporter_constructor(nullptr, nullptr, 0, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_PTR(viewporter, wl_resource_set_implementation_fake.arg0_val);

	// Clean up
	free(viewporter);
}

/** bz_seat_constructor() posts a no memory error for failed Wayland resource creation. */
void test_wp_viewporter_constructor__posts_no_mem_for_failed_resource(void)
{
	// Set up our mocks
	wl_resource_create_fake.return_val = nullptr;

	// Run our test!
	bz_wp_viewporter_constructor(nullptr, nullptr, 0, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_set_implementation_fake.call_count);
}


// =================================================================================================
//  Test bz_wp_viewporter_get_viewport()
// -------------------------------------------------------------------------------------------------

void test_wp_viewporter_get_viewport__vp_is_added_to_surface(void)
{
	// Set up test data
	struct bz_surface *surface_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = surface_data;
	struct wl_resource *viewport = calloc(1, sizeof(*viewport));
	wl_resource_create_fake.return_val = viewport;

	// Run the test
	bz_wp_viewporter_implementation.get_viewport(nullptr, nullptr, 0, nullptr);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	// Verify user data
	struct bz_wp_viewport *vp_data = wl_resource_set_implementation_fake.arg2_val;
	TEST_ASSERT_NOT_NULL(vp_data);
	TEST_ASSERT_EQUAL_PTR(viewport, vp_data->resource);
	TEST_ASSERT_EQUAL_PTR(surface_data->viewport, vp_data);
	TEST_ASSERT_EQUAL_PTR(surface_data, vp_data->surface);
	// Surface pending state should be unchanged
	TEST_ASSERT_NULL(surface_data->pending_state->vp_source);
	TEST_ASSERT_NULL(surface_data->pending_state->vp_dest);

	// Clean up
	free(viewport);
	bz_free_wp_viewport_data(vp_data);
	bz_free_surface_data(surface_data);
}

void test_wp_viewporter_get_viewport__existing_vp_raises_error(void)
{
	// Set up test data
	struct bz_wp_viewport *viewport_data = bz_create_wp_viewport_data();
	struct bz_surface *surface_data = bz_create_surface_data();
	surface_data->viewport = viewport_data;
	wl_resource_get_user_data_fake.return_val = surface_data;

	// Run the test
	bz_wp_viewporter_implementation.get_viewport(nullptr, nullptr, 0, nullptr);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WP_VIEWPORTER_ERROR_VIEWPORT_EXISTS, wl_resource_post_error_fake.arg1_val);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_create_fake.call_count);

	// Clean up
	bz_free_wp_viewport_data(viewport_data);
	bz_free_surface_data(surface_data);
}


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// Test bz_wp_viewporter_constructor()
	RUN_TEST(test_wp_viewporter_constructor__initializes_resource);
	RUN_TEST(test_wp_viewporter_constructor__posts_no_mem_for_failed_resource);

	// Test bz_wp_viewporter_get_viewport()
	RUN_TEST(test_wp_viewporter_get_viewport__vp_is_added_to_surface);
	RUN_TEST(test_wp_viewporter_get_viewport__existing_vp_raises_error);

	return UNITY_END();
}
