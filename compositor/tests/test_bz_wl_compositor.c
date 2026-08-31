
#include "breezy/bz_wl_display.h"

#include <stdlib.h>

#include "unity.h"
#include "fff.h"
#include "breezy/bz_breezy.h"
#include "breezy/bz_list.h"
#include "breezy/bz_logger.h"
#include "breezy/bz_wayland.h"

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
FAKE_VALUE_FUNC(struct wl_resource *, wl_resource_create, struct wl_client *, const struct wl_interface *, int, uint32_t)

void setUp(void)
{
	RESET_FAKE(wl_client_post_no_memory);
	RESET_FAKE(wl_resource_set_implementation);
	RESET_FAKE(wl_resource_create);
	FFF_RESET_HISTORY();

	bz_log_initialize(BZ_LOG_OFF);
}

void tearDown(void) {}


// =================================================================================================
//  Helper functions for some of our tests
// -------------------------------------------------------------------------------------------------

extern const struct wl_compositor_interface bz_compositor_implementation;


// =================================================================================================
//  Test bz_compositor_constructor()
// -------------------------------------------------------------------------------------------------

/** bz_compositor_constructor() properly initializes our resource. */
void test_compositor_constructor__initializes_resource(void)
{
	// Create a variable to house our wl_compositor handlers for direct execution
	struct wl_resource *compositor = calloc(1, sizeof(*compositor));
	wl_resource_create_fake.return_val = compositor;

	// Run our test!
	bz_compositor_constructor(nullptr, nullptr, 0, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_PTR(compositor, wl_resource_set_implementation_fake.arg0_val);

	// Clean up!
	free(compositor);
}

/** bz_compositor_constructor() posts a no memory error for failed Wayland resource creation. */
void test_compositor_constructor__posts_no_mem_for_failed_resource(void)
{
	// Compositor creation should return a nullptr to trigger failure.
	wl_resource_create_fake.return_val = nullptr;

	// Run our test!
	bz_compositor_constructor(nullptr, nullptr, 0, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_set_implementation_fake.call_count);
}


// =================================================================================================
//  Test bz_compositor_create_surface()
// -------------------------------------------------------------------------------------------------

/** bz_compositor_create_surface() should properly create and configure our surface. */
void test_create_surface__initializes_properly(void)
{
	// Set up our mocks and data
	struct bz_client *client_data = bz_create_client_data();
	wl_client_get_user_data_fake.return_val = client_data;
	struct wl_resource *surface = calloc(1, sizeof(*surface));
	wl_resource_create_fake.return_val = surface;

	// Run our test!
	bz_compositor_implementation.create_surface(nullptr, nullptr, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL(surface, wl_resource_set_implementation_fake.arg0_val); // Resource
	TEST_ASSERT_NOT_NULL(wl_resource_set_implementation_fake.arg1_val);       // Interface
	TEST_ASSERT_NOT_NULL(wl_resource_set_implementation_fake.arg2_val);       // User Data

	// Also verify some of our (more important) user data
	struct bz_surface *surface_data = wl_resource_set_implementation_fake.arg2_val;
	TEST_ASSERT_EQUAL(surface, surface_data->resource);
	TEST_ASSERT_EQUAL(BZ_SURF_ROLE_NONE, surface_data->role); // Surface does not start with a role.
	TEST_ASSERT_NOT_NULL(surface_data->pending_state);
	TEST_ASSERT_NOT_NULL(surface_data->active_state);

	// Cleanup
	free(surface);
	bz_free_surface_data(surface_data);
	bz_free_client_data(client_data);
}

/** bz_compositor_create_surface() should post no memory when the resource fails to create. */
void test_create_surface__resource_failed(void)
{
	// Set up our mocks and data
	wl_resource_create_fake.return_val = nullptr;

	// Run our test!
	bz_compositor_implementation.create_surface(nullptr, nullptr, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_set_implementation_fake.call_count);
}


// =================================================================================================
//  Test bz_compositor_create_region()
// -------------------------------------------------------------------------------------------------

/** bz_compositor_create_region() should properly create and configure our region. */
void test_create_region__initializes_properly(void)
{
	// Set up our mocks and data
	struct wl_resource *region = calloc(1, sizeof(*region));
	wl_resource_create_fake.return_val = region;

	// Run our test!
	bz_compositor_implementation.create_region(nullptr, nullptr, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL(region, wl_resource_set_implementation_fake.arg0_val); // Resource
	TEST_ASSERT_NOT_NULL(wl_resource_set_implementation_fake.arg1_val);       // Interface
	TEST_ASSERT_NOT_NULL(wl_resource_set_implementation_fake.arg2_val);       // User Data

	// Also verify some of our (more important) user data
	struct bz_region *region_data = wl_resource_set_implementation_fake.arg2_val;
	TEST_ASSERT_EQUAL(region, region_data->resource);
	TEST_ASSERT_NOT_NULL(region_data->mutations);
	TEST_ASSERT_EQUAL_INT(0, region_data->mutations->length);

	// Cleanup
	free(region);
	bz_free_region_data(region_data);
}

/** bz_compositor_create_region() should post no memory when the resource fails to create. */
void test_create_region__resource_failed(void)
{
	// Set up our mocks and data
	wl_resource_create_fake.return_val = nullptr;

	// Run our test!
	bz_compositor_implementation.create_region(nullptr, nullptr, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_set_implementation_fake.call_count);
}


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// Test bz_compositor_constructor()
	RUN_TEST(test_compositor_constructor__initializes_resource);
	RUN_TEST(test_compositor_constructor__posts_no_mem_for_failed_resource);

	// Test bz_compositor_create_surface()
	RUN_TEST(test_create_surface__initializes_properly);
	RUN_TEST(test_create_surface__resource_failed);

	// Test bz_compositor_create_region()
	RUN_TEST(test_create_region__initializes_properly);
	RUN_TEST(test_create_region__resource_failed);

	return UNITY_END();
}
