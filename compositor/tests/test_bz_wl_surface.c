
#include "breezy/bz_wl_display.h"

#include <stdlib.h>

#include "unity.h"
#include "fff.h"
#include "breezy/bz_graphics.h"
#include "breezy/bz_logger.h"
#include "helpers/bz_test_resources.c"


// =================================================================================================
//  Set up / tear down / globals
// -------------------------------------------------------------------------------------------------

DEFINE_FFF_GLOBALS
// -- wl_client --
FAKE_VOID_FUNC(wl_client_post_no_memory, struct wl_client *)
// -- wl_resource --
FAKE_VALUE_FUNC(void *, wl_resource_get_user_data, struct wl_resource *)
FAKE_VALUE_FUNC(struct wl_resource *, wl_resource_create, struct wl_client *, const struct wl_interface *, int, uint32_t)
FAKE_VOID_FUNC_VARARG(wl_resource_post_event, struct wl_resource *, uint32_t, ...)
FAKE_VOID_FUNC(wl_resource_destroy, struct wl_resource *)
// -- wl_callback --

void setUp(void)
{
	RESET_FAKE(wl_client_post_no_memory);
	RESET_FAKE(wl_resource_get_user_data);
	RESET_FAKE(wl_resource_create);
	RESET_FAKE(wl_resource_post_event);
	RESET_FAKE(wl_resource_destroy);
	FFF_RESET_HISTORY();

	bz_log_initialize(BZ_LOG_OFF);
}

void tearDown(void) {}


// =================================================================================================
//  Helper functions for some of our tests
// -------------------------------------------------------------------------------------------------

extern const struct wl_surface_interface bz_surface_implementation;


// =================================================================================================
//  Test bz_surface_destroy()
// -------------------------------------------------------------------------------------------------

/** Deletes the surface and invalidates the objectId. */
void test_surface_destroy__deletes_surface(void)
{
	// (Surface should not have a role for this test.)
	// TODO
}

/** The client must destroy the role object first. Otherwise, a defunct_role_object error is sent. */
void test_surface_destroy__with_role_sends_error(void)
{
	// TODO
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
	// TODO
}

/**
 * Coordinates are specified in SURFACE-local coords. X and Y specify upper-left of the rectangle.
 */
void test_surface_damage__x_y_are_surface_local_upper_left()
{
	// TODO
}

/**
 * Each call ADDS pending damage. Use the union of all provided damage rectangles in the commit.
 */
void test_surface_damage__multiple_calls_are_unioned()
{
	// TODO
}

/** Initial value is "no damage". */
void test_surface_damage__starts_out_no_damage()
{
	// TODO
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
	struct bz_client *client_data = bz_create_client_data();
	bz_list_append(client_data->surfaces, surf_data);

	// Create multiple frame requests
	bz_surface_implementation.frame(nullptr, nullptr, 0);
	bz_surface_implementation.frame(nullptr, nullptr, 0);
	TEST_ASSERT_EQUAL_INT(2, surf_data->pending_state->frame_callbacks->length);

	// Promote to active state. (Not doing a commit because that does a lot more things.)
	bz_list_move_to_end(
		surf_data->active_state->frame_callbacks,
		surf_data->pending_state->frame_callbacks
	);
	TEST_ASSERT_EQUAL_INT(0, surf_data->pending_state->frame_callbacks->length);
	TEST_ASSERT_EQUAL_INT(2, surf_data->active_state->frame_callbacks->length);

	// Trigger the callbacks to run
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_event_fake.call_count);
	bz_graphics_process_frame_callbacks(client_data, 0);
	TEST_ASSERT_EQUAL_INT(2, wl_resource_post_event_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_CALLBACK_DONE, wl_resource_post_event_fake.arg1_history[0]);
	TEST_ASSERT_EQUAL_INT(WL_CALLBACK_DONE, wl_resource_post_event_fake.arg1_history[1]);

	// Cleanup
	bz_free_client_data(client_data);
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
	struct bz_client *client_data = bz_create_client_data();
	bz_list_append(client_data->surfaces, surf_data);

	// Create a frame request
	bz_surface_implementation.frame(nullptr, nullptr, 0);
	TEST_ASSERT_EQUAL_INT(1, surf_data->pending_state->frame_callbacks->length);

	// Promote to active state. (Not doing a commit because that does a lot more things.)
	bz_list_move_to_end(
		surf_data->active_state->frame_callbacks,
		surf_data->pending_state->frame_callbacks
	);
	TEST_ASSERT_EQUAL_INT(0, surf_data->pending_state->frame_callbacks->length);
	TEST_ASSERT_EQUAL_INT(1, surf_data->active_state->frame_callbacks->length);

	// Trigger the callback to run
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_event_fake.call_count);
	bz_graphics_process_frame_callbacks(client_data, 0);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_event_fake.call_count);

	// Verify the thing being tested
	TEST_ASSERT_EQUAL_INT(1, wl_resource_destroy_fake.call_count);
	TEST_ASSERT_EQUAL(callback_res, wl_resource_destroy_fake.arg0_val);

	// Cleanup
	bz_free_client_data(client_data);
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


// =================================================================================================
//  Test bz_surface_set_input_region()
// -------------------------------------------------------------------------------------------------


// =================================================================================================
//  Test bz_surface_commit()
// -------------------------------------------------------------------------------------------------


// =================================================================================================
//  Test bz_surface_set_buffer_transform()
// -------------------------------------------------------------------------------------------------


// =================================================================================================
//  Test bz_surface_set_buffer_scale()
// -------------------------------------------------------------------------------------------------


// =================================================================================================
//  Test bz_surface_damage_buffer()
// -------------------------------------------------------------------------------------------------

/** Damage is double-buffered state. */
void test_surface_damage_buffer__damage_is_double_buffered()
{
	// TODO
}

/**
 * Coordinates are specified in BUFFER-local coords. X and Y specify upper-left of the rectangle.
 */
void test_surface_damage_buffer__x_y_are_buffer_local_upper_left()
{
	// TODO
}

/**
 * Each call ADDS pending damage. Use the union of all provided damage rectangles in the commit.
 */
void test_surface_damage_buffer__multiple_calls_are_unioned()
{
	// TODO
}

/** Initial value is "no damage". */
void test_surface_damage_buffer__starts_out_no_damage()
{
	// TODO
}

/**
 * It is impossible to convert between buffer and surface coordinates until commit time due to
 * buffer transformation changes, so both must be tracked independently. (damage vs damage_buffer)
 */
void test_surface_damage_buffer__tracked_independently_from_surface_damage()
{
	// TODO
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
	RUN_TEST(test_surface_destroy__deletes_surface); // TODO
	RUN_TEST(test_surface_destroy__with_role_sends_error); // TODO

	// Test bz_surface_attach()
	RUN_TEST(test_surface_attach__contents_are_double_buffered);
	RUN_TEST(test_surface_attach__size_is_properly_calculated); // TODO
	RUN_TEST(test_surface_attach__x_y_are_relative); // TODO
	RUN_TEST(test_surface_attach__x_y_for_v5_raises_an_error); // TODO
	RUN_TEST(test_surface_attach__x_y_for_v4_is_allowed); // TODO

	// Test bz_surface_damage()
	RUN_TEST(test_surface_damage__damage_is_double_buffered); // TODO
	RUN_TEST(test_surface_damage__x_y_are_surface_local_upper_left); // TODO
	RUN_TEST(test_surface_damage__multiple_calls_are_unioned); // TODO
	RUN_TEST(test_surface_damage__starts_out_no_damage); // TODO

	// Test bz_surface_frame()
	RUN_TEST(test_surface_frame__requests_are_double_buffered);
	RUN_TEST(test_surface_frame__client_not_visible); // TODO
	RUN_TEST(test_surface_frame__multiple_frame_requests);
	RUN_TEST(test_surface_frame__callback_is_destroyed_immediately);
	RUN_TEST(test_surface_frame__posts_no_mem_for_failed_resource);

	// Test bz_surface_set_opaque_region()
	// RUN_TEST(test_surface_set_opaque_region_...);
	// TODO

	// Test bz_surface_set_input_region()
	// RUN_TEST(test_surface_set_input_region_...);
	// TODO

	// Test bz_surface_commit()
	// RUN_TEST(test_surface_commit__);
	// TODO

	// Test bz_surface_set_buffer_transform()
	// RUN_TEST(test_surface_set_buffer_transform_...);
	// TODO

	// Test bz_surface_set_buffer_scale()
	// RUN_TEST(test_surface_set_buffer_scale_...);
	// TODO

	// Test bz_surface_damage_buffer()
	RUN_TEST(test_surface_damage_buffer__damage_is_double_buffered); // TODO
	RUN_TEST(test_surface_damage_buffer__x_y_are_buffer_local_upper_left); // TODO
	RUN_TEST(test_surface_damage_buffer__multiple_calls_are_unioned); // TODO
	RUN_TEST(test_surface_damage_buffer__starts_out_no_damage); // TODO
	RUN_TEST(test_surface_damage_buffer__tracked_independently_from_surface_damage); // TODO

	// Test bz_surface_offset()
	// RUN_TEST(test_surface_offset_...);
	// TODO

	return UNITY_END();
}
