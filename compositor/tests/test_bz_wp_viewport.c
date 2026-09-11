
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

extern const struct wp_viewport_interface bz_wp_viewport_implementation;


// =================================================================================================
//  Test bz_viewport_set_source()
// -------------------------------------------------------------------------------------------------

void test_viewport_set_source__state_is_double_buffered(void)
{
	wl_fixed_t x = wl_fixed_from_double(1.0);
	wl_fixed_t y = wl_fixed_from_double(2.0);
	wl_fixed_t w = wl_fixed_from_double(3.0);
	wl_fixed_t h = wl_fixed_from_double(4.0);

	// Create our test data
	struct bz_surface *surface_data = bz_create_surface_data();
	struct bz_wp_viewport *vp_data = bz_create_wp_viewport_data();
	vp_data->surface = surface_data;
	wl_resource_get_user_data_fake.return_val = vp_data;

	// Run the test
	TEST_ASSERT_NULL(surface_data->active_state->vp_source);
	TEST_ASSERT_NULL(surface_data->pending_state->vp_source);
	bz_wp_viewport_implementation.set_source(nullptr, nullptr, x, y, w, h);
	TEST_ASSERT_NULL(surface_data->active_state->vp_source);
	TEST_ASSERT_NOT_NULL(surface_data->pending_state->vp_source);
	TEST_ASSERT_TRUE(1.0 == surface_data->pending_state->vp_source->x);
	TEST_ASSERT_TRUE(2.0 == surface_data->pending_state->vp_source->y);
	TEST_ASSERT_TRUE(3.0 == surface_data->pending_state->vp_source->w);
	TEST_ASSERT_TRUE(4.0 == surface_data->pending_state->vp_source->h);

	// Clean up
	bz_free_wp_viewport_data(vp_data);
	bz_free_surface_data(surface_data);
}

void test_viewport_set_source__all_negative_ones_clears_destination(void)
{
	wl_fixed_t x = wl_fixed_from_double(-1.0);
	wl_fixed_t y = wl_fixed_from_double(-1.0);
	wl_fixed_t w = wl_fixed_from_double(-1.0);
	wl_fixed_t h = wl_fixed_from_double(-1.0);

	// Create our test data
	struct bz_surface *surface_data = bz_create_surface_data();
	struct bz_wp_viewport *vp_data = bz_create_wp_viewport_data();
	vp_data->surface = surface_data;
	wl_resource_get_user_data_fake.return_val = vp_data;

	// Start with some initial data for us to clear. Must be calloc'd to properly be freed.
	struct bz_rect_dbl *initial_rect = calloc(1, sizeof(*initial_rect));
	*initial_rect = (struct bz_rect_dbl){ .x = 1.0, .y = 2.0, .w = 3.0, .h = 4.0 };
	surface_data->pending_state->vp_source = initial_rect;

	// Run the test
	TEST_ASSERT_NOT_NULL(surface_data->pending_state->vp_source);
	bz_wp_viewport_implementation.set_source(nullptr, nullptr, x, y, w, h);
	TEST_ASSERT_NULL(surface_data->pending_state->vp_source); // Should be null now.

	// Clean up
	// free(initial_rect); // Don't free initial_rect. Needs to be done by set_source.
	bz_free_wp_viewport_data(vp_data);
	bz_free_surface_data(surface_data);
}

void test_viewport_set_source__nonpositive_w_h_raises_error(void)
{
	wl_fixed_t x = wl_fixed_from_double(1.0);
	wl_fixed_t y = wl_fixed_from_double(2.0);
	wl_fixed_t w = wl_fixed_from_double(3.0);
	wl_fixed_t h = wl_fixed_from_double(4.0);

	// Create our test data
	struct bz_surface *surface_data = bz_create_surface_data();
	struct bz_wp_viewport *vp_data = bz_create_wp_viewport_data();
	vp_data->surface = surface_data;
	wl_resource_get_user_data_fake.return_val = vp_data;

	// Run the test
	TEST_ASSERT_NULL(surface_data->pending_state->vp_source);
	bz_wp_viewport_implementation.set_source(nullptr, nullptr, x, y, wl_fixed_from_double(-3.0), h);
	TEST_ASSERT_NULL(surface_data->pending_state->vp_source); // Source data unchanged
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WP_VIEWPORT_ERROR_BAD_VALUE, wl_resource_post_error_fake.arg1_val);

	// Let's also test height with 0. That should cover enough bases. (negative and zero, width and height)
	bz_wp_viewport_implementation.set_source(nullptr, nullptr, x, y, w, wl_fixed_from_double(0.0));
	TEST_ASSERT_EQUAL_INT(2, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WP_VIEWPORT_ERROR_BAD_VALUE, wl_resource_post_error_fake.arg1_val);

	// Clean up
	bz_free_wp_viewport_data(vp_data);
	bz_free_surface_data(surface_data);
}

void test_viewport_set_source__negative_x_y_raises_error(void)
{
	wl_fixed_t x = wl_fixed_from_double(1.0);
	wl_fixed_t y = wl_fixed_from_double(2.0);
	wl_fixed_t w = wl_fixed_from_double(3.0);
	wl_fixed_t h = wl_fixed_from_double(4.0);

	// Create our test data
	struct bz_surface *surface_data = bz_create_surface_data();
	struct bz_wp_viewport *vp_data = bz_create_wp_viewport_data();
	vp_data->surface = surface_data;
	wl_resource_get_user_data_fake.return_val = vp_data;

	// Run the test
	TEST_ASSERT_NULL(surface_data->pending_state->vp_source);
	bz_wp_viewport_implementation.set_source(nullptr, nullptr, wl_fixed_from_double(-1.0), y, w, h);
	TEST_ASSERT_NULL(surface_data->pending_state->vp_source); // Source data unchanged
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WP_VIEWPORT_ERROR_BAD_VALUE, wl_resource_post_error_fake.arg1_val);

	// Let's also test y.
	bz_wp_viewport_implementation.set_source(nullptr, nullptr, x, wl_fixed_from_double(-2.0), w, h);
	TEST_ASSERT_EQUAL_INT(2, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WP_VIEWPORT_ERROR_BAD_VALUE, wl_resource_post_error_fake.arg1_val);

	// Clean up
	bz_free_wp_viewport_data(vp_data);
	bz_free_surface_data(surface_data);
}

void test_viewport_set_source__null_surface_raises_error(void)
{
	wl_fixed_t x = wl_fixed_from_double(1.0);
	wl_fixed_t y = wl_fixed_from_double(2.0);
	wl_fixed_t w = wl_fixed_from_double(3.0);
	wl_fixed_t h = wl_fixed_from_double(4.0);

	// Create our test data
	struct bz_wp_viewport *vp_data = bz_create_wp_viewport_data();
	vp_data->surface = nullptr; // Indicates the surface was destroyed prior to this viewport.
	wl_resource_get_user_data_fake.return_val = vp_data;

	// Run the test
	bz_wp_viewport_implementation.set_source(nullptr, nullptr, x, y, w, h);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WP_VIEWPORT_ERROR_NO_SURFACE, wl_resource_post_error_fake.arg1_val);

	// Clean up
	bz_free_wp_viewport_data(vp_data);
}


// =================================================================================================
//  Test bz_viewport_set_destination()
// -------------------------------------------------------------------------------------------------

void test_viewport_set_destination__state_is_double_buffered(void)
{
	// Create our test data
	struct bz_surface *surface_data = bz_create_surface_data();
	struct bz_wp_viewport *vp_data = bz_create_wp_viewport_data();
	vp_data->surface = surface_data;
	wl_resource_get_user_data_fake.return_val = vp_data;

	// Run the test
	TEST_ASSERT_NULL(surface_data->active_state->vp_dest);
	TEST_ASSERT_NULL(surface_data->pending_state->vp_dest);
	bz_wp_viewport_implementation.set_destination(nullptr, nullptr, 1, 2);
	TEST_ASSERT_NULL(surface_data->active_state->vp_dest);
	TEST_ASSERT_NOT_NULL(surface_data->pending_state->vp_dest);
	TEST_ASSERT_EQUAL_INT(1, surface_data->pending_state->vp_dest->w);
	TEST_ASSERT_EQUAL_INT(2, surface_data->pending_state->vp_dest->h);

	// Clean up
	bz_free_wp_viewport_data(vp_data);
	bz_free_surface_data(surface_data);
}

void test_viewport_set_destination__all_negative_ones_clears_destination(void)
{
	// Create our test data
	struct bz_surface *surface_data = bz_create_surface_data();
	struct bz_wp_viewport *vp_data = bz_create_wp_viewport_data();
	vp_data->surface = surface_data;
	wl_resource_get_user_data_fake.return_val = vp_data;

	// Start with some initial data for us to clear. Must be calloc'd to properly be freed.
	struct bz_dimension *initial_dim = calloc(1, sizeof(*initial_dim));
	*initial_dim = (struct bz_dimension){ .w = 1, .h = 2 };
	surface_data->pending_state->vp_dest = initial_dim;

	// Run the test
	TEST_ASSERT_NOT_NULL(surface_data->pending_state->vp_dest);
	bz_wp_viewport_implementation.set_destination(nullptr, nullptr, -1, -1);
	TEST_ASSERT_NULL(surface_data->pending_state->vp_dest); // Should be null now.

	// Clean up
	// free(initial_dim); // Don't free initial_dim. Needs to be done by set_destination.
	bz_free_wp_viewport_data(vp_data);
	bz_free_surface_data(surface_data);
}

void test_viewport_set_destination__nonpositive_w_h_raises_error(void)
{
	// Create our test data
	struct bz_surface *surface_data = bz_create_surface_data();
	struct bz_wp_viewport *vp_data = bz_create_wp_viewport_data();
	vp_data->surface = surface_data;
	wl_resource_get_user_data_fake.return_val = vp_data;

	// Run the test
	TEST_ASSERT_NULL(surface_data->pending_state->vp_dest);
	bz_wp_viewport_implementation.set_destination(nullptr, nullptr, -1, 2);
	TEST_ASSERT_NULL(surface_data->pending_state->vp_dest); // Source data unchanged
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WP_VIEWPORT_ERROR_BAD_VALUE, wl_resource_post_error_fake.arg1_val);

	// Let's also test height with 0. That should cover enough bases. (negative and zero, width and height)
	bz_wp_viewport_implementation.set_destination(nullptr, nullptr, 1, 0);
	TEST_ASSERT_EQUAL_INT(2, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WP_VIEWPORT_ERROR_BAD_VALUE, wl_resource_post_error_fake.arg1_val);

	// Clean up
	bz_free_wp_viewport_data(vp_data);
	bz_free_surface_data(surface_data);
}

void test_viewport_set_destination__null_surface_raises_error(void)
{
	// Create our test data
	struct bz_wp_viewport *vp_data = bz_create_wp_viewport_data();
	vp_data->surface = nullptr; // Indicates the surface was destroyed prior to this viewport.
	wl_resource_get_user_data_fake.return_val = vp_data;

	// Run the test
	bz_wp_viewport_implementation.set_destination(nullptr, nullptr, 1, 2);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WP_VIEWPORT_ERROR_NO_SURFACE, wl_resource_post_error_fake.arg1_val);

	// Clean up
	bz_free_wp_viewport_data(vp_data);
}


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// Test bz_viewport_set_source()
	RUN_TEST(test_viewport_set_source__state_is_double_buffered);
	RUN_TEST(test_viewport_set_source__all_negative_ones_clears_destination);
	RUN_TEST(test_viewport_set_source__nonpositive_w_h_raises_error);
	RUN_TEST(test_viewport_set_source__negative_x_y_raises_error);
	RUN_TEST(test_viewport_set_source__null_surface_raises_error);

	// Test bz_viewport_set_destination()
	RUN_TEST(test_viewport_set_destination__state_is_double_buffered);
	RUN_TEST(test_viewport_set_destination__all_negative_ones_clears_destination);
	RUN_TEST(test_viewport_set_destination__nonpositive_w_h_raises_error);
	RUN_TEST(test_viewport_set_destination__null_surface_raises_error);

	return UNITY_END();
}
