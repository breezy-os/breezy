
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

struct wl_resource *compositor;

void setUp(void)
{
	RESET_FAKE(wl_client_post_no_memory);
	RESET_FAKE(wl_resource_set_implementation);
	RESET_FAKE(wl_resource_create);
	FFF_RESET_HISTORY();

	bz_log_initialize(BZ_LOG_OFF);
	compositor = malloc(sizeof(*compositor));
}

void tearDown(void)
{
	free(compositor);
	compositor = nullptr;
}


// =================================================================================================
//  Helper functions for some of our tests
// -------------------------------------------------------------------------------------------------

/** Creates and bootstraps a fake wl_compositor resource, returning the interface implementation. */
struct wl_compositor_interface *bz_bootstrap_compositor(void)
{
	wl_resource_create_fake.return_val = compositor;
	bz_compositor_constructor(nullptr, nullptr, 0, 0);
	struct wl_compositor_interface *compositor_impl = wl_resource_set_implementation_fake.arg1_val;
	RESET_FAKE(wl_resource_create);
	RESET_FAKE(wl_resource_set_implementation);
	FFF_RESET_HISTORY();
	return compositor_impl;
}

/** Creates and bootstraps a bz_client struct. When you're finished, call bz_client_free_data(). */
struct bz_client *bz_client_create_data(struct bz_breezy *breezy)
{
	struct bz_client *client_data = calloc(1, sizeof(*client_data));
	client_data->breezy = breezy;
	client_data->surfaces = bz_list_create();
	return client_data;
}

/** Frees the memory allocated as part of bz_client_create_data(). */
void bz_client_free_data(struct bz_client *data)
{
	bz_list_free(data->surfaces, nullptr);
	free(data);
}

struct bz_create_surface_test_data {
	struct wl_compositor_interface *compositor_impl;
	struct bz_breezy *globals;
	struct bz_client *client_data;
	struct wl_resource *surface;
};

struct bz_create_surface_test_data *bz_bootstrap_create_surface_test()
{
	struct bz_create_surface_test_data *data = calloc(1, sizeof(*data));

	// Compositor
	data->compositor_impl = bz_bootstrap_compositor();

	// Globals
	struct bz_breezy *globals = calloc(1, sizeof(*globals));
	globals->drm.mode_info.hdisplay = 1920;
	globals->drm.mode_info.vdisplay = 1080;
	data->globals = globals;

	// Client Data
	data->client_data = bz_client_create_data(globals);
	wl_client_get_user_data_fake.return_val = data->client_data;

	// Surface Resource
	data->surface = calloc(1, sizeof(*data->surface));
	wl_resource_create_fake.return_val = data->surface;

	return data;
}

void bz_cleanup_create_surface_test(
	struct bz_create_surface_test_data *test_data,
	struct bz_surface *user_data
) {
	bz_free_surface_data(user_data);
	if (test_data) {
		if (test_data->client_data) { bz_client_free_data(test_data->client_data); }
		if (test_data->surface)     { free(test_data->surface); }
		if (test_data->globals)     { free(test_data->globals); }
		free(test_data);
	}
}


// =================================================================================================
//  Test bz_compositor_constructor()
// -------------------------------------------------------------------------------------------------

/** bz_compositor_constructor() properly initializes our resource. */
void test_compositor_constructor__initializes_resource(void)
{
	// Create a variable to house our wl_compositor handlers for direct execution
	wl_resource_create_fake.return_val = compositor;

	// Run our test!
	bz_compositor_constructor(nullptr, nullptr, 0, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_PTR(compositor, wl_resource_set_implementation_fake.arg0_val);
}

/** bz_compositor_constructor() posts a no memory error for failed Wayland resource creation. */
void test_compositor_constructor__posts_no_mem_for_failed_resource(void)
{
	// Create a variable to house our wl_compositor handlers for direct execution
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
	struct bz_create_surface_test_data *test_data = bz_bootstrap_create_surface_test();
	struct wl_compositor_interface *compositor_impl = test_data->compositor_impl;
	struct bz_client *client_data = test_data->client_data;
	struct wl_resource *surface = test_data->surface;

	// Run our test!
	compositor_impl->create_surface(nullptr, nullptr, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL(surface, wl_resource_set_implementation_fake.arg0_val); // Resource
	TEST_ASSERT_NOT_NULL(wl_resource_set_implementation_fake.arg1_val);       // Interface
	TEST_ASSERT_NOT_NULL(wl_resource_set_implementation_fake.arg2_val);       // User Data
	TEST_ASSERT_EQUAL_INT(1, client_data->surfaces->length);

	// Also verify some of our (more important) user data
	struct bz_surface *user_data = wl_resource_set_implementation_fake.arg2_val;
	TEST_ASSERT_EQUAL(surface, user_data->resource);
	TEST_ASSERT_EQUAL(BZ_SURF_ROLE_NONE, user_data->role); // Surface does not start with a role.
	TEST_ASSERT_NOT_NULL(user_data->pending_state);
	TEST_ASSERT_NOT_NULL(user_data->active_state);

	// Cleanup
	bz_cleanup_create_surface_test(test_data, user_data);
}

/** bz_compositor_create_surface() should post no memory when the resource fails to create. */
void test_create_surface__resource_failed(void)
{
	// Set up our mocks and data
	struct bz_create_surface_test_data *test_data = bz_bootstrap_create_surface_test();
	struct wl_compositor_interface *compositor_impl = test_data->compositor_impl;
	struct bz_client *client_data = test_data->client_data;
	wl_resource_create_fake.return_val = nullptr;

	// Run our test!
	compositor_impl->create_surface(nullptr, nullptr, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, client_data->surfaces->length);

	// Cleanup
	bz_cleanup_create_surface_test(test_data, nullptr);
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
	// TODO

	return UNITY_END();
}
