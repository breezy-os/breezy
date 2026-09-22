
#include "breezy/bz_xdg_shell.h"

#include <stdlib.h>

#include <wayland-server.h>
#include <xdg-shell-server-protocol.h>

#include "unity.h"
#include "fff.h"
#include "breezy/bz_list.h"
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

extern const struct xdg_wm_base_interface bz_xdg_wm_base_implementation;

struct bz_get_xdg_surface_test_data {
	struct wl_resource *wlsurf_res;
	struct bz_surface *wlsurf_data;
	struct wl_resource *xdgsurf_res;
};

struct bz_get_xdg_surface_test_data *bz_bootstrap_get_xdg_surface_test()
{
	struct bz_get_xdg_surface_test_data *data = calloc(1, sizeof(*data));

	// Wayland Surface Resource
	data->wlsurf_res = calloc(1, sizeof(*data->wlsurf_res));

	// Wayland Surface User Data
	data->wlsurf_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = data->wlsurf_data;

	// XDG Surface Resource
	data->xdgsurf_res = calloc(1, sizeof(*data->xdgsurf_res));
	wl_resource_create_fake.return_val = data->xdgsurf_res;

	return data;
}

void bz_cleanup_get_xdg_surface_test(
	struct bz_get_xdg_surface_test_data *test_data,
	struct bz_xdg_surface *user_data
) {
	if (user_data) {
		bz_list_free(user_data->pending_configures, nullptr);
		free(user_data);
	}
	if (test_data) {
		if (test_data->wlsurf_res) { free(test_data->wlsurf_res); }
		if (test_data->xdgsurf_res) { free(test_data->xdgsurf_res); }
		if (test_data->wlsurf_data) { bz_free_surface_data(test_data->wlsurf_data); }
		free(test_data);
	}
}


// =================================================================================================
//  Test bz_xdg_wm_base_constructor()
// -------------------------------------------------------------------------------------------------

/** bz_xdg_wm_base_constructor() properly initializes our resource. */
void test_xdg_wm_base_constructor__initializes_resource(void)
{
	// Create a variable to house our xdg_wm_base handlers for direct execution
	struct wl_resource *xdg_wm_base_res = malloc(sizeof(*xdg_wm_base_res));
	wl_resource_create_fake.return_val = xdg_wm_base_res;

	// Run our test!
	bz_xdg_wm_base_constructor(nullptr, nullptr, 0, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_PTR(xdg_wm_base_res, wl_resource_set_implementation_fake.arg0_val);

	// Clean up
	free(xdg_wm_base_res);
}

/** bz_xdg_wm_base_constructor() posts a no memory error for failed Wayland resource creation. */
void test_xdg_wm_base_constructor__posts_no_mem_for_failed_resource(void)
{
	// Create a variable to house our wl_compositor handlers for direct execution
	wl_resource_create_fake.return_val = nullptr;

	// Run our test!
	bz_xdg_wm_base_constructor(nullptr, nullptr, 0, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_set_implementation_fake.call_count);
}


// =================================================================================================
//  Test bz_xdg_wm_base_destroy()
// -------------------------------------------------------------------------------------------------


// =================================================================================================
//  Test bz_xdg_wm_base_create_positioner()
// -------------------------------------------------------------------------------------------------


// =================================================================================================
//  Test bz_xdg_wm_base_get_xdg_surface()
// -------------------------------------------------------------------------------------------------

/** bz_xdg_wm_base_get_xdg_surface() should properly create and configure our surface. */
void test_get_xdg_surface__initializes_properly(void)
{
	// Set up our mocks and data
	struct bz_get_xdg_surface_test_data *test_data = bz_bootstrap_get_xdg_surface_test();
	struct wl_resource *wlsurf_res = test_data->wlsurf_res;
	struct bz_surface *wlsurf_data = test_data->wlsurf_data;
	struct wl_resource *xdgsurf_res = test_data->xdgsurf_res;

	// Run our test!
	bz_xdg_wm_base_implementation.get_xdg_surface(nullptr, nullptr, 0, wlsurf_res);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL(xdgsurf_res, wl_resource_set_implementation_fake.arg0_val); // Resource
	TEST_ASSERT_NOT_NULL(wl_resource_set_implementation_fake.arg1_val);           // Interface
	TEST_ASSERT_NOT_NULL(wl_resource_set_implementation_fake.arg2_val);           // User Data

	// Also verify some of our (more important) user data
	struct bz_xdg_surface *user_data = wl_resource_set_implementation_fake.arg2_val;
	TEST_ASSERT_EQUAL(xdgsurf_res, user_data->resource);
	TEST_ASSERT_EQUAL(wlsurf_data, user_data->wlsurface);
	TEST_ASSERT_EQUAL(user_data, wlsurf_data->xdgsurface);
	TEST_ASSERT_EQUAL(BZ_SURF_ROLE_NONE, wlsurf_data->role); // Surface still does not have a role

	// Cleanup
	bz_cleanup_get_xdg_surface_test(test_data, user_data);
}

/** It is illegal to create an xdg_surface for a wl_surface which already has an assigned role. */
void test_get_xdg_surface__existing_role_causes_role_error(void)
{
	// Set up our mocks and data
	struct bz_get_xdg_surface_test_data *test_data = bz_bootstrap_get_xdg_surface_test();
	struct wl_resource *wlsurf_res = test_data->wlsurf_res;
	struct bz_surface *wlsurf_data = test_data->wlsurf_data;
	wlsurf_data->role = BZ_SURF_ROLE_XDG_TOPLEVEL;

	// Run our test!
	bz_xdg_wm_base_implementation.get_xdg_surface(nullptr, nullptr, 0, wlsurf_res);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(0, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);

	// Cleanup
	bz_cleanup_get_xdg_surface_test(test_data, nullptr);
}

/** This does not set the role for a wl_surface. (That's done by get_toplevel or get_popup.) */
void test_get_xdg_surface__does_not_assign_role(void)
{
	// Set up our mocks and data
	struct bz_get_xdg_surface_test_data *test_data = bz_bootstrap_get_xdg_surface_test();
	struct wl_resource *wlsurf_res = test_data->wlsurf_res;
	struct bz_surface *wlsurf_data = test_data->wlsurf_data;

	// Run our test!
	bz_xdg_wm_base_implementation.get_xdg_surface(nullptr, nullptr, 0, wlsurf_res);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL(BZ_SURF_ROLE_NONE, wlsurf_data->role); // Surface still does not have a role

	// Cleanup
	struct bz_xdg_surface *user_data = wl_resource_set_implementation_fake.arg2_val;
	bz_cleanup_get_xdg_surface_test(test_data, user_data);
}

/** bz_xdg_wm_base_get_xdg_surface() posts no memory error for failed Wayland resource creation. */
void test_get_xdg_surface__resource_failed(void)
{
	// Set up our mocks and data
	struct bz_get_xdg_surface_test_data *test_data = bz_bootstrap_get_xdg_surface_test();
	struct wl_resource *wlsurf_res = test_data->wlsurf_res;
	wl_resource_create_fake.return_val = nullptr;

	// Run our test!
	bz_xdg_wm_base_implementation.get_xdg_surface(nullptr, nullptr, 0, wlsurf_res);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_set_implementation_fake.call_count);

	// Cleanup
	struct bz_xdg_surface *user_data = wl_resource_set_implementation_fake.arg2_val;
	bz_cleanup_get_xdg_surface_test(test_data, user_data);
}


// =================================================================================================
//  Test bz_xdg_wm_base_pong()
// -------------------------------------------------------------------------------------------------


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// Test bz_xdg_wm_base_constructor()
	RUN_TEST(test_xdg_wm_base_constructor__initializes_resource);
	RUN_TEST(test_xdg_wm_base_constructor__posts_no_mem_for_failed_resource);

	// Test bz_xdg_wm_base_destroy()
	// TODO

	// Test bz_xdg_wm_base_create_positioner()
	// TODO

	// Test bz_xdg_wm_base_get_xdg_surface()
	RUN_TEST(test_get_xdg_surface__initializes_properly);
	RUN_TEST(test_get_xdg_surface__resource_failed);
	RUN_TEST(test_get_xdg_surface__existing_role_causes_role_error);
	RUN_TEST(test_get_xdg_surface__does_not_assign_role);

	// Test bz_xdg_wm_base_pong()
	// TODO

	return UNITY_END();
}
