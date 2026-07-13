
#include "breezy/bz_wl_display.h"

#include <stdlib.h>

#include "unity.h"
#include "fff.h"
#include "breezy/bz_logger.h"
#include "helpers/bz_test_resources.c"


// =================================================================================================
//  Set up / tear down / globals
// -------------------------------------------------------------------------------------------------

DEFINE_FFF_GLOBALS
// -- wl_resource --
FAKE_VALUE_FUNC(void *, wl_resource_get_user_data, struct wl_resource *)

void setUp(void)
{
	RESET_FAKE(wl_resource_get_user_data);
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
	// RUN_TEST(test_surface_frame_...);
	// TODO

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
