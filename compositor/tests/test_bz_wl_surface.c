
#include "breezy/bz_wl_display.h"

#include <stdlib.h>

#include "unity.h"
#include "fff.h"
#include "breezy/bz_graphics.h"
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

extern const struct wl_surface_interface bz_surface_implementation;
extern const struct wl_compositor_interface bz_compositor_implementation;


// =================================================================================================
//  Test bz_surface_destroy()
// -------------------------------------------------------------------------------------------------

/** Deletes the surface when it never held a role. */
void test_surface_destroy__succeeds_with_no_role(void)
{
	// Set up our test data
	struct bz_surface *surface_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = surface_data;

	// Run our test
	bz_surface_implementation.destroy(nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_destroy_fake.call_count);

	// Clean up
	bz_free_surface_data(surface_data);
}

/** Deletes the surface when it had a role which has since been destroyed. */
void test_surface_destroy__succeeds_with_destroyed_role(void)
{
	// Set up our test data
	struct bz_surface *surface_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = surface_data;

	// Surface should have a role, but not a role object.
	surface_data->role = BZ_SURF_ROLE_XDG_TOPLEVEL;
	surface_data->xdgtoplevel = nullptr;

	// Run our test
	bz_surface_implementation.destroy(nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_destroy_fake.call_count);

	// Clean up
	bz_free_surface_data(surface_data);
}

/** The client must destroy the role object first. Otherwise, a defunct_role_object error is sent. */
void test_surface_destroy__with_role_sends_error(void)
{
	// Set up our test data
	struct bz_surface *surface_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = surface_data;

	// Surface should have both a role and role object.
	surface_data->role = BZ_SURF_ROLE_XDG_TOPLEVEL;
	surface_data->xdgtoplevel = bz_create_xdg_toplevel_data();

	// Run our test
	bz_surface_implementation.destroy(nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_destroy_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_SURFACE_ERROR_DEFUNCT_ROLE_OBJECT, wl_resource_post_error_fake.arg1_val);

	// Clean up
	bz_free_xdg_toplevel_data(surface_data->xdgtoplevel);
	bz_free_surface_data(surface_data);
}


// =================================================================================================
//  Test bz_surface_attach()
// -------------------------------------------------------------------------------------------------

/** Surface contents are double-buffered state. */
void test_surface_attach__contents_are_double_buffered(void)
{
	// Prep our mocks
	struct bz_surface *surf_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = surf_data;

	// Run our test
	struct wl_resource *surface = calloc(1, sizeof(*surface));
	struct wl_resource *buffer = calloc(1, sizeof(*buffer));
	bz_surface_implementation.attach(nullptr, surface, buffer, 0, 0);
	TEST_ASSERT_EQUAL(buffer, surf_data->pending_state->buffer);
	TEST_ASSERT_NULL(surf_data->active_state->buffer);

	// Cleanup
	bz_free_surface_data(surf_data);
	free(surface);
	free(buffer);
}

/**
 * New size of surface is calculated based on buffer size transformed by the inverse
 * buffer_transform and inverse buffer_scale.
 */
void test_surface_attach__size_is_properly_calculated(void)
{
	// TODO
}

/**
 * X and Y specify the new pending buffer's upper-left corner RELATIVE to the current buffer's
 * upper-left corner, in SURFACE-local coordinates.
 */
void test_surface_attach__x_y_are_relative(void)
{
	// TODO
}

/**
 * When surface version is v5 or higher, passing non-zero X or Y will raise an invalid_offset error.
 */
void test_surface_attach__x_y_for_v5_raises_an_error(void)
{
	// TODO
}

/** When surface version is v4 or lower, passing non-zero X or Y is allowed. */
void test_surface_attach__x_y_for_v4_is_allowed(void)
{
	// TODO
}


// =================================================================================================
//  Test bz_surface_damage()
// -------------------------------------------------------------------------------------------------

/** Damage is double-buffered state. */
void test_surface_damage__damage_is_double_buffered()
{
	// Set up our test data
	struct bz_surface *surface_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = surface_data;

	// Run the test, making sure it's saved on pending state and not active state
	bz_surface_implementation.damage(nullptr, nullptr, 1, 2, 3, 4);
	TEST_ASSERT_EQUAL_INT(0, surface_data->active_state->surface_damage->length);
	TEST_ASSERT_EQUAL_INT(1, surface_data->pending_state->surface_damage->length);
	struct bz_rect *damage = surface_data->pending_state->surface_damage->head->data;
	TEST_ASSERT_EQUAL_INT(1, damage->x);
	TEST_ASSERT_EQUAL_INT(2, damage->y);
	TEST_ASSERT_EQUAL_INT(3, damage->w);
	TEST_ASSERT_EQUAL_INT(4, damage->h);

	// Clean up
	bz_free_surface_data(surface_data);
}

/**
 * Each call ADDS pending damage. Use the union of all provided damage rectangles in the commit.
 */
void test_surface_damage__multiple_calls_are_unioned()
{
	// Set up our test data
	struct bz_surface *surface_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = surface_data;

	// Run the test, making sure it's saved on pending state and not active state
	bz_surface_implementation.damage(nullptr, nullptr, 1, 2, 3, 4);
	bz_surface_implementation.damage(nullptr, nullptr, 5, 6, 7, 8);
	TEST_ASSERT_EQUAL_INT(0, surface_data->active_state->surface_damage->length);
	TEST_ASSERT_EQUAL_INT(2, surface_data->pending_state->surface_damage->length);
	struct bz_rect *damage1 = surface_data->pending_state->surface_damage->head->data;
	TEST_ASSERT_EQUAL_INT(1, damage1->x);
	TEST_ASSERT_EQUAL_INT(2, damage1->y);
	TEST_ASSERT_EQUAL_INT(3, damage1->w);
	TEST_ASSERT_EQUAL_INT(4, damage1->h);
	struct bz_rect *damage2 = surface_data->pending_state->surface_damage->head->next->data;
	TEST_ASSERT_EQUAL_INT(5, damage2->x);
	TEST_ASSERT_EQUAL_INT(6, damage2->y);
	TEST_ASSERT_EQUAL_INT(7, damage2->w);
	TEST_ASSERT_EQUAL_INT(8, damage2->h);

	// Clean up
	bz_free_surface_data(surface_data);
}

/** Initial value is "no damage". */
void test_surface_damage__starts_out_no_damage()
{
	// Set up our test data
	struct bz_surface *surface_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = surface_data;

	// Make sure our surfaces start out with no damage.
	TEST_ASSERT_EQUAL_INT(0, surface_data->active_state->surface_damage->length);
	TEST_ASSERT_EQUAL_INT(0, surface_data->pending_state->surface_damage->length);

	// Clean up
	bz_free_surface_data(surface_data);
}


// =================================================================================================
//  Test bz_surface_frame()
// -------------------------------------------------------------------------------------------------

/** Frame requests are double-buffered. They don't become active until after a commit. */
void test_surface_frame__requests_are_double_buffered(void)
{
	// Prep our mocks
	struct bz_surface *surf_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = surf_data;
	struct wl_resource *callback_res = calloc(1, sizeof(*callback_res));
	wl_resource_create_fake.return_val = callback_res;

	// Run our test
	TEST_ASSERT_EQUAL_INT(0, surf_data->active_state->frame_callbacks->length);
	TEST_ASSERT_EQUAL_INT(0, surf_data->pending_state->frame_callbacks->length);
	bz_surface_implementation.frame(nullptr, nullptr, 0);
	TEST_ASSERT_EQUAL_INT(0, surf_data->active_state->frame_callbacks->length);
	TEST_ASSERT_EQUAL_INT(1, surf_data->pending_state->frame_callbacks->length);
	TEST_ASSERT_EQUAL(callback_res, surf_data->pending_state->frame_callbacks->head->data);

	// Cleanup
	free(callback_res);
	bz_free_surface_data(surf_data);
}

/** The server should not send frame callbacks if the client is not visible. */
void test_surface_frame__client_not_visible(void)
{
	// TODO
	// Tie into bz_graphics_process_frame_callbacks() for this..?
}

/** Multiple frame requests can all be submitted for a single commit. They should all fire. */
void test_surface_frame__multiple_frame_requests(void)
{
	// Prep our mocks
	struct bz_surface *surf_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = surf_data;
	struct wl_resource *callback_res = calloc(1, sizeof(*callback_res));
	wl_resource_create_fake.return_val = callback_res;
	struct bz_list *surfaces = bz_list_create();
	bz_list_append(surfaces, surf_data);

	// Create multiple frame requests
	bz_surface_implementation.frame(nullptr, nullptr, 0);
	bz_surface_implementation.frame(nullptr, nullptr, 0);
	TEST_ASSERT_EQUAL_INT(2, surf_data->pending_state->frame_callbacks->length);

	// Promote to active state. (Not doing a commit because that does a lot more things.)
	bz_list_move_to_end(
		surf_data->pending_state->frame_callbacks,
		surf_data->active_state->frame_callbacks
	);
	TEST_ASSERT_EQUAL_INT(0, surf_data->pending_state->frame_callbacks->length);
	TEST_ASSERT_EQUAL_INT(2, surf_data->active_state->frame_callbacks->length);

	// Trigger the callbacks to run
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_event_fake.call_count);
	bz_graphics_process_frame_callbacks(surfaces, 0);
	TEST_ASSERT_EQUAL_INT(2, wl_resource_post_event_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_CALLBACK_DONE, wl_resource_post_event_fake.arg1_history[0]);
	TEST_ASSERT_EQUAL_INT(WL_CALLBACK_DONE, wl_resource_post_event_fake.arg1_history[1]);

	// Cleanup
	bz_list_free(surfaces, nullptr);
	free(callback_res);
	bz_free_surface_data(surf_data);
}

/** The compositor should destroy the callback immediately after firing the "done". */
void test_surface_frame__callback_is_destroyed_immediately(void)
{
	// Prep our mocks
	struct bz_surface *surf_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = surf_data;
	struct wl_resource *callback_res = calloc(1, sizeof(*callback_res));
	wl_resource_create_fake.return_val = callback_res;
	struct bz_list *surfaces = bz_list_create();
	bz_list_append(surfaces, surf_data);

	// Create a frame request
	bz_surface_implementation.frame(nullptr, nullptr, 0);
	TEST_ASSERT_EQUAL_INT(1, surf_data->pending_state->frame_callbacks->length);

	// Promote to active state. (Not doing a commit because that does a lot more things.)
	bz_list_move_to_end(
		surf_data->pending_state->frame_callbacks,
		surf_data->active_state->frame_callbacks
	);
	TEST_ASSERT_EQUAL_INT(0, surf_data->pending_state->frame_callbacks->length);
	TEST_ASSERT_EQUAL_INT(1, surf_data->active_state->frame_callbacks->length);

	// Trigger the callback to run
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_event_fake.call_count);
	bz_graphics_process_frame_callbacks(surfaces, 0);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_event_fake.call_count);

	// Verify the thing being tested
	TEST_ASSERT_EQUAL_INT(1, wl_resource_destroy_fake.call_count);
	TEST_ASSERT_EQUAL(callback_res, wl_resource_destroy_fake.arg0_val);

	// Cleanup
	bz_list_free(surfaces, nullptr);
	free(callback_res);
	bz_free_surface_data(surf_data);
}

/** A no memory error is posted for failed Wayland resource creation. */
void test_surface_frame__posts_no_mem_for_failed_resource(void)
{
	// Prep our mocks
	wl_resource_create_fake.return_val = nullptr;

	// Run our test
	bz_surface_implementation.frame(nullptr, nullptr, 0);
	TEST_ASSERT_EQUAL_INT(1, wl_client_post_no_memory_fake.call_count);
}


// =================================================================================================
//  Test bz_surface_set_opaque_region()
// -------------------------------------------------------------------------------------------------

void test_surface_set_opaque_region__requests_are_double_buffered(void)
{
	// Set up our test data
	struct wl_resource *region = calloc(1, sizeof(*region));
	struct bz_region *region_data = bz_create_region_data();
	region_data->resource = region;

	struct bz_region_mutation *mut1 = calloc(1, sizeof(*mut1));
	*mut1 = (struct bz_region_mutation){ .op = OP_ADD, .x = 1, .y = 2, .w = 3, .h = 4 };
	bz_list_append(region_data->mutations, mut1);

	struct bz_surface *surface_data = bz_create_surface_data();

	// Set up our mocks
	void *ret_vals[2] = { surface_data, region_data };
	SET_RETURN_SEQ(wl_resource_get_user_data, ret_vals, 2);

	// Run our test
	TEST_ASSERT_FALSE(surface_data->pending_state->dirty_opaque_region);
	bz_surface_implementation.set_opaque_region(nullptr, nullptr, region);
	TEST_ASSERT_TRUE(surface_data->pending_state->dirty_opaque_region);
	TEST_ASSERT_EQUAL_INT(1, surface_data->pending_state->opaque_region->length);

	// Clean up
	bz_free_surface_data(surface_data);
	bz_free_region_data(region_data);
	free(region);
}

void test_surface_set_opaque_region__initial_value_is_null(void)
{
	// Set up our mocks and data
	struct bz_client *client_data = bz_create_client_data();
	wl_client_get_user_data_fake.return_val = client_data;
	struct wl_resource *surface = calloc(1, sizeof(*surface));
	wl_resource_create_fake.return_val = surface;

	// Run our test!
	bz_compositor_implementation.create_surface(nullptr, nullptr, 0);
	struct bz_surface *surface_data = wl_resource_set_implementation_fake.arg2_val;
	TEST_ASSERT_NULL(surface_data->pending_state->opaque_region);

	// Clean up
	free(surface);
	bz_free_surface_data(surface_data);
	bz_free_client_data(client_data);
}

void test_surface_set_opaque_region__null_value_can_be_given(void)
{
	// Set up our test data
	struct wl_resource *region = calloc(1, sizeof(*region));
	struct bz_region *region_data = bz_create_region_data();
	region_data->resource = region;

	struct bz_region_mutation *mut1 = calloc(1, sizeof(*mut1));
	*mut1 = (struct bz_region_mutation){ .op = OP_ADD, .x = 1, .y = 2, .w = 3, .h = 4 };
	bz_list_append(region_data->mutations, mut1);

	struct bz_surface *surface_data = bz_create_surface_data();

	// Set up our mocks
	void *ret_vals[4] = {
		surface_data, region_data, // First call
		surface_data, region_data, // Second call
	};
	SET_RETURN_SEQ(wl_resource_get_user_data, ret_vals, 4);

	// First, give it a non-NULL value
	bz_surface_implementation.set_opaque_region(nullptr, nullptr, region);
	TEST_ASSERT_EQUAL_INT(1, surface_data->pending_state->opaque_region->length);

	// Now test the NULL case
	bz_surface_implementation.set_opaque_region(nullptr, nullptr, nullptr);
	TEST_ASSERT_NULL(surface_data->pending_state->opaque_region);

	// Clean up
	bz_free_surface_data(surface_data);
	bz_free_region_data(region_data);
	free(region);
}

void test_surface_set_opaque_region__region_has_copy_semantics(void)
{
	// Set up our test data
	struct wl_resource *region = calloc(1, sizeof(*region));
	struct bz_region *region_data = bz_create_region_data();
	region_data->resource = region;

	struct bz_region_mutation *mut1 = calloc(1, sizeof(*mut1));
	*mut1 = (struct bz_region_mutation){ .op = OP_ADD, .x = 1, .y = 2, .w = 3, .h = 4 };
	bz_list_append(region_data->mutations, mut1);

	struct bz_surface *surface_data = bz_create_surface_data();

	// Set up our mocks
	void *ret_vals[2] = { surface_data, region_data };
	SET_RETURN_SEQ(wl_resource_get_user_data, ret_vals, 2);

	// Run our test
	bz_surface_implementation.set_opaque_region(nullptr, nullptr, region);
	TEST_ASSERT_EQUAL_INT(1, surface_data->pending_state->opaque_region->length);
	// Verify the pointers aren't equal
	TEST_ASSERT_NOT_EQUAL(region_data->mutations, surface_data->pending_state->opaque_region);
	TEST_ASSERT_NOT_EQUAL(mut1, surface_data->pending_state->opaque_region->head->data);
	// Destroying the region should be OK because of the copy semantics
	bz_free_region_data(region_data);
	free(region);
	TEST_ASSERT_EQUAL_INT(1, surface_data->pending_state->opaque_region->length);
	struct bz_region_mutation *mut = surface_data->pending_state->opaque_region->head->data;
	TEST_ASSERT_EQUAL_INT(OP_ADD, mut->op);
	TEST_ASSERT_EQUAL_INT(1, mut->x);
	TEST_ASSERT_EQUAL_INT(2, mut->y);
	TEST_ASSERT_EQUAL_INT(3, mut->w);
	TEST_ASSERT_EQUAL_INT(4, mut->h);

	// Clean up
	bz_free_surface_data(surface_data);
}


// =================================================================================================
//  Test bz_surface_set_input_region()
// -------------------------------------------------------------------------------------------------

void test_surface_set_input_region__requests_are_double_buffered(void)
{
	// Set up our test data
	struct wl_resource *region = calloc(1, sizeof(*region));
	struct bz_region *region_data = bz_create_region_data();
	region_data->resource = region;

	struct bz_region_mutation *mut1 = calloc(1, sizeof(*mut1));
	*mut1 = (struct bz_region_mutation){ .op = OP_ADD, .x = 1, .y = 2, .w = 3, .h = 4 };
	bz_list_append(region_data->mutations, mut1);

	struct bz_surface *surface_data = bz_create_surface_data();

	// Set up our mocks
	void *ret_vals[2] = { surface_data, region_data };
	SET_RETURN_SEQ(wl_resource_get_user_data, ret_vals, 2);

	// Run our test
	TEST_ASSERT_FALSE(surface_data->pending_state->dirty_input_region);
	bz_surface_implementation.set_input_region(nullptr, nullptr, region);
	TEST_ASSERT_TRUE(surface_data->pending_state->dirty_input_region);
	TEST_ASSERT_EQUAL_INT(1, surface_data->pending_state->input_region->length);

	// Clean up
	bz_free_surface_data(surface_data);
	bz_free_region_data(region_data);
	free(region);
}

void test_surface_set_input_region__initial_value_is_null(void)
{
	// Set up our mocks and data
	struct bz_client *client_data = bz_create_client_data();
	wl_client_get_user_data_fake.return_val = client_data;
	struct wl_resource *surface = calloc(1, sizeof(*surface));
	wl_resource_create_fake.return_val = surface;

	// Run our test!
	bz_compositor_implementation.create_surface(nullptr, nullptr, 0);
	struct bz_surface *surface_data = wl_resource_set_implementation_fake.arg2_val;
	TEST_ASSERT_NULL(surface_data->pending_state->input_region);

	// Clean up
	free(surface);
	bz_free_surface_data(surface_data);
	bz_free_client_data(client_data);
}

void test_surface_set_input_region__null_value_can_be_given(void)
{
	// Set up our test data
	struct wl_resource *region = calloc(1, sizeof(*region));
	struct bz_region *region_data = bz_create_region_data();
	region_data->resource = region;

	struct bz_region_mutation *mut1 = calloc(1, sizeof(*mut1));
	*mut1 = (struct bz_region_mutation){ .op = OP_ADD, .x = 1, .y = 2, .w = 3, .h = 4 };
	bz_list_append(region_data->mutations, mut1);

	struct bz_surface *surface_data = bz_create_surface_data();

	// Set up our mocks
	void *ret_vals[4] = {
		surface_data, region_data, // First call
		surface_data, region_data, // Second call
	};
	SET_RETURN_SEQ(wl_resource_get_user_data, ret_vals, 4);

	// First, give it a non-NULL value
	bz_surface_implementation.set_input_region(nullptr, nullptr, region);
	TEST_ASSERT_EQUAL_INT(1, surface_data->pending_state->input_region->length);

	// Now test the NULL case
	bz_surface_implementation.set_input_region(nullptr, nullptr, nullptr);
	TEST_ASSERT_NULL(surface_data->pending_state->input_region);

	// Clean up
	bz_free_surface_data(surface_data);
	bz_free_region_data(region_data);
	free(region);
}

void test_surface_set_input_region__region_has_copy_semantics(void)
{
	// Set up our test data
	struct wl_resource *region = calloc(1, sizeof(*region));
	struct bz_region *region_data = bz_create_region_data();
	region_data->resource = region;

	struct bz_region_mutation *mut1 = calloc(1, sizeof(*mut1));
	*mut1 = (struct bz_region_mutation){ .op = OP_ADD, .x = 1, .y = 2, .w = 3, .h = 4 };
	bz_list_append(region_data->mutations, mut1);

	struct bz_surface *surface_data = bz_create_surface_data();

	// Set up our mocks
	void *ret_vals[2] = { surface_data, region_data };
	SET_RETURN_SEQ(wl_resource_get_user_data, ret_vals, 2);

	// Run our test
	bz_surface_implementation.set_input_region(nullptr, nullptr, region);
	TEST_ASSERT_EQUAL_INT(1, surface_data->pending_state->input_region->length);
	// Verify the pointers aren't equal
	TEST_ASSERT_NOT_EQUAL(region_data->mutations, surface_data->pending_state->input_region);
	TEST_ASSERT_NOT_EQUAL(mut1, surface_data->pending_state->input_region->head->data);
	// Destroying the region should be OK because of the copy semantics
	bz_free_region_data(region_data);
	free(region);
	TEST_ASSERT_EQUAL_INT(1, surface_data->pending_state->input_region->length);
	struct bz_region_mutation *mut = surface_data->pending_state->input_region->head->data;
	TEST_ASSERT_EQUAL_INT(OP_ADD, mut->op);
	TEST_ASSERT_EQUAL_INT(1, mut->x);
	TEST_ASSERT_EQUAL_INT(2, mut->y);
	TEST_ASSERT_EQUAL_INT(3, mut->w);
	TEST_ASSERT_EQUAL_INT(4, mut->h);

	// Clean up
	bz_free_surface_data(surface_data);
}


// =================================================================================================
//  Test bz_surface_commit()
// -------------------------------------------------------------------------------------------------


// =================================================================================================
//  Test bz_surface_set_buffer_transform()
// -------------------------------------------------------------------------------------------------

void test_surface_set_buffer_transform__is_double_buffered(void)
{
	struct bz_surface *surface_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = surface_data;

	bz_surface_implementation.set_buffer_transform(nullptr, nullptr, WL_OUTPUT_TRANSFORM_90);
	TEST_ASSERT_EQUAL_INT(WL_OUTPUT_TRANSFORM_90, surface_data->pending_state->transform);
	TEST_ASSERT_EQUAL_INT(WL_OUTPUT_TRANSFORM_NORMAL, surface_data->active_state->transform);

	bz_free_surface_data(surface_data);
}

void test_surface_set_buffer_transform__initial_value_is_normal(void)
{
	// Set up our mocks and data
	struct bz_client *client_data = bz_create_client_data();
	wl_client_get_user_data_fake.return_val = client_data;
	struct wl_resource *surface = calloc(1, sizeof(*surface));
	wl_resource_create_fake.return_val = surface;

	// Run our test!
	bz_compositor_implementation.create_surface(nullptr, nullptr, 0);
	struct bz_surface *surface_data = wl_resource_set_implementation_fake.arg2_val;
	TEST_ASSERT_EQUAL_INT(WL_OUTPUT_TRANSFORM_NORMAL, surface_data->pending_state->transform);
	TEST_ASSERT_EQUAL_INT(WL_OUTPUT_TRANSFORM_NORMAL, surface_data->active_state->transform);

	// Clean up
	free(surface);
	bz_free_surface_data(surface_data);
	bz_free_client_data(client_data);
}

void test_surface_set_buffer_transform__invalid_value_raises_error(void)
{
	struct bz_surface *surface_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = surface_data;

	bz_surface_implementation.set_buffer_transform(nullptr, nullptr, -1);
	TEST_ASSERT_EQUAL_INT(WL_OUTPUT_TRANSFORM_NORMAL, surface_data->pending_state->transform);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_SURFACE_ERROR_INVALID_TRANSFORM, wl_resource_post_error_fake.arg1_val);

	bz_free_surface_data(surface_data);
}


// =================================================================================================
//  Test bz_surface_set_buffer_scale()
// -------------------------------------------------------------------------------------------------

void test_surface_set_buffer_scale__is_double_buffered(void)
{
	struct bz_surface *surface_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = surface_data;

	bz_surface_implementation.set_buffer_scale(nullptr, nullptr, 2);
	TEST_ASSERT_EQUAL_INT(2, surface_data->pending_state->scale);
	TEST_ASSERT_EQUAL_INT(1, surface_data->active_state->scale);

	bz_free_surface_data(surface_data);
}

void test_surface_set_buffer_scale__initial_value_is_one(void)
{
	// Set up our mocks and data
	struct bz_client *client_data = bz_create_client_data();
	wl_client_get_user_data_fake.return_val = client_data;
	struct wl_resource *surface = calloc(1, sizeof(*surface));
	wl_resource_create_fake.return_val = surface;

	// Run our test!
	bz_compositor_implementation.create_surface(nullptr, nullptr, 0);
	struct bz_surface *surface_data = wl_resource_set_implementation_fake.arg2_val;
	TEST_ASSERT_EQUAL_INT(1, surface_data->pending_state->scale);
	TEST_ASSERT_EQUAL_INT(1, surface_data->active_state->scale);

	// Clean up
	free(surface);
	bz_free_surface_data(surface_data);
	bz_free_client_data(client_data);
}

void test_surface_set_buffer_scale__invalid_value_raises_error(void)
{
	struct bz_surface *surface_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = surface_data;

	bz_surface_implementation.set_buffer_scale(nullptr, nullptr, -1);
	TEST_ASSERT_EQUAL_INT(1, surface_data->pending_state->scale);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_SURFACE_ERROR_INVALID_SCALE, wl_resource_post_error_fake.arg1_val);

	bz_free_surface_data(surface_data);
}


// =================================================================================================
//  Test bz_surface_damage_buffer()
// -------------------------------------------------------------------------------------------------

/** Damage is double-buffered state. */
void test_surface_damage_buffer__damage_is_double_buffered(void)
{
	// Set up our test data
	struct bz_surface *surface_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = surface_data;

	// Run the test, making sure it's saved on pending state and not active state
	bz_surface_implementation.damage_buffer(nullptr, nullptr, 1, 2, 3, 4);
	TEST_ASSERT_EQUAL_INT(0, surface_data->active_state->buffer_damage->length);
	TEST_ASSERT_EQUAL_INT(1, surface_data->pending_state->buffer_damage->length);
	struct bz_rect *damage = surface_data->pending_state->buffer_damage->head->data;
	TEST_ASSERT_EQUAL_INT(1, damage->x);
	TEST_ASSERT_EQUAL_INT(2, damage->y);
	TEST_ASSERT_EQUAL_INT(3, damage->w);
	TEST_ASSERT_EQUAL_INT(4, damage->h);

	// Clean up
	bz_free_surface_data(surface_data);
}

/**
 * Each call ADDS pending damage. Use the union of all provided damage rectangles in the commit.
 */
void test_surface_damage_buffer__multiple_calls_are_unioned(void)
{
	// Set up our test data
	struct bz_surface *surface_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = surface_data;

	// Run the test, making sure it's saved on pending state and not active state
	bz_surface_implementation.damage_buffer(nullptr, nullptr, 1, 2, 3, 4);
	bz_surface_implementation.damage_buffer(nullptr, nullptr, 5, 6, 7, 8);
	TEST_ASSERT_EQUAL_INT(0, surface_data->active_state->buffer_damage->length);
	TEST_ASSERT_EQUAL_INT(2, surface_data->pending_state->buffer_damage->length);
	struct bz_rect *damage1 = surface_data->pending_state->buffer_damage->head->data;
	TEST_ASSERT_EQUAL_INT(1, damage1->x);
	TEST_ASSERT_EQUAL_INT(2, damage1->y);
	TEST_ASSERT_EQUAL_INT(3, damage1->w);
	TEST_ASSERT_EQUAL_INT(4, damage1->h);
	struct bz_rect *damage2 = surface_data->pending_state->buffer_damage->head->next->data;
	TEST_ASSERT_EQUAL_INT(5, damage2->x);
	TEST_ASSERT_EQUAL_INT(6, damage2->y);
	TEST_ASSERT_EQUAL_INT(7, damage2->w);
	TEST_ASSERT_EQUAL_INT(8, damage2->h);

	// Clean up
	bz_free_surface_data(surface_data);
}

/** Initial value is "no damage". */
void test_surface_damage_buffer__starts_out_no_damage(void)
{
	// Set up our test data
	struct bz_surface *surface_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = surface_data;

	// Make sure our surfaces start out with no damage.
	TEST_ASSERT_EQUAL_INT(0, surface_data->active_state->buffer_damage->length);
	TEST_ASSERT_EQUAL_INT(0, surface_data->pending_state->buffer_damage->length);

	// Clean up
	bz_free_surface_data(surface_data);
}

/**
 * It is impossible to convert between buffer and surface coordinates until commit time due to
 * buffer transformation changes, so both must be tracked independently. (damage vs damage_buffer)
 */
void test_surface_damage_buffer__tracked_independently_from_surface_damage(void)
{
	// Set up our test data
	struct bz_surface *surface_data = bz_create_surface_data();
	wl_resource_get_user_data_fake.return_val = surface_data;

	// Run the test, making sure it's saved on pending state and not active state
	bz_surface_implementation.damage(nullptr, nullptr, 1, 2, 3, 4);
	bz_surface_implementation.damage_buffer(nullptr, nullptr, 5, 6, 7, 8);
	TEST_ASSERT_EQUAL_INT(0, surface_data->active_state->surface_damage->length);
	TEST_ASSERT_EQUAL_INT(0, surface_data->active_state->buffer_damage->length);
	// First, check surface damage is set.
	TEST_ASSERT_EQUAL_INT(1, surface_data->pending_state->surface_damage->length);
	struct bz_rect *damage1 = surface_data->pending_state->surface_damage->head->data;
	TEST_ASSERT_EQUAL_INT(1, damage1->x);
	TEST_ASSERT_EQUAL_INT(2, damage1->y);
	TEST_ASSERT_EQUAL_INT(3, damage1->w);
	TEST_ASSERT_EQUAL_INT(4, damage1->h);
	// Next, verify buffer damage is separate.
	TEST_ASSERT_EQUAL_INT(1, surface_data->pending_state->buffer_damage->length);
	struct bz_rect *damage2 = surface_data->pending_state->buffer_damage->head->data;
	TEST_ASSERT_EQUAL_INT(5, damage2->x);
	TEST_ASSERT_EQUAL_INT(6, damage2->y);
	TEST_ASSERT_EQUAL_INT(7, damage2->w);
	TEST_ASSERT_EQUAL_INT(8, damage2->h);

	// Clean up
	bz_free_surface_data(surface_data);
}


// =================================================================================================
//  Test bz_surface_offset()
// -------------------------------------------------------------------------------------------------


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// Test bz_surface_destroy()
	RUN_TEST(test_surface_destroy__succeeds_with_no_role);
	RUN_TEST(test_surface_destroy__succeeds_with_destroyed_role);
	RUN_TEST(test_surface_destroy__with_role_sends_error);

	// Test bz_surface_attach()
	RUN_TEST(test_surface_attach__contents_are_double_buffered);
	RUN_TEST(test_surface_attach__size_is_properly_calculated); // TODO
	RUN_TEST(test_surface_attach__x_y_are_relative); // TODO
	RUN_TEST(test_surface_attach__x_y_for_v5_raises_an_error); // TODO
	RUN_TEST(test_surface_attach__x_y_for_v4_is_allowed); // TODO

	// Test bz_surface_damage()
	RUN_TEST(test_surface_damage__damage_is_double_buffered);
	RUN_TEST(test_surface_damage__multiple_calls_are_unioned);
	RUN_TEST(test_surface_damage__starts_out_no_damage);

	// // Test bz_surface_frame()
	RUN_TEST(test_surface_frame__requests_are_double_buffered);
	RUN_TEST(test_surface_frame__client_not_visible); // TODO
	RUN_TEST(test_surface_frame__multiple_frame_requests);
	RUN_TEST(test_surface_frame__callback_is_destroyed_immediately);
	RUN_TEST(test_surface_frame__posts_no_mem_for_failed_resource);

	// Test bz_surface_set_opaque_region()
	RUN_TEST(test_surface_set_opaque_region__requests_are_double_buffered);
	RUN_TEST(test_surface_set_opaque_region__initial_value_is_null);
	RUN_TEST(test_surface_set_opaque_region__null_value_can_be_given);
	RUN_TEST(test_surface_set_opaque_region__region_has_copy_semantics);

	// Test bz_surface_set_input_region()
	RUN_TEST(test_surface_set_input_region__requests_are_double_buffered);
	RUN_TEST(test_surface_set_input_region__initial_value_is_null);
	RUN_TEST(test_surface_set_input_region__null_value_can_be_given);
	RUN_TEST(test_surface_set_input_region__region_has_copy_semantics);

	// Test bz_surface_commit()
	// RUN_TEST(test_surface_commit__);
	// TODO

	// Test bz_surface_set_buffer_transform()
	RUN_TEST(test_surface_set_buffer_transform__is_double_buffered);
	RUN_TEST(test_surface_set_buffer_transform__initial_value_is_normal);
	RUN_TEST(test_surface_set_buffer_transform__invalid_value_raises_error);

	// Test bz_surface_set_buffer_scale()
	RUN_TEST(test_surface_set_buffer_scale__is_double_buffered);
	RUN_TEST(test_surface_set_buffer_scale__initial_value_is_one);
	RUN_TEST(test_surface_set_buffer_scale__invalid_value_raises_error);

	// Test bz_surface_damage_buffer()
	RUN_TEST(test_surface_damage_buffer__damage_is_double_buffered);
	RUN_TEST(test_surface_damage_buffer__multiple_calls_are_unioned);
	RUN_TEST(test_surface_damage_buffer__starts_out_no_damage);
	RUN_TEST(test_surface_damage_buffer__tracked_independently_from_surface_damage);

	// Test bz_surface_offset()
	// RUN_TEST(test_surface_offset_...);
	// TODO

	return UNITY_END();
}
