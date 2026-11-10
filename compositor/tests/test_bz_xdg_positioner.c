
#include "breezy/bz_xdg_shell.h"

#include <stdlib.h>

#include <xdg-shell-server-protocol.h>

#include "unity.h"
#include "fff.h"
#include "../../build/compositor/tests/xdg-shell-server-protocol.h"
#include "../include/breezy/bz_xdg_shell.h"
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

extern const struct xdg_positioner_interface bz_xdg_positioner_implementation;


// =================================================================================================
//  Test bz_xdg_positioner_destroy()
// -------------------------------------------------------------------------------------------------

void test_xdg_positioner_destroy__destroys_the_resource(void)
{
	bz_xdg_positioner_implementation.destroy(nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_destroy_fake.call_count);
}


// =================================================================================================
//  Test bz_xdg_positioner_set_size()
// -------------------------------------------------------------------------------------------------

void test_xdg_positioner_set_size__assigns_width_and_height(void)
{
	// Set up data
	struct bz_xdg_positioner *positioner = bz_create_xdg_positioner_data();
	wl_resource_get_user_data_fake.return_val = positioner;

	// Run the test
	bz_xdg_positioner_implementation.set_size(nullptr, nullptr, 1, 2);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_NOT_NULL(positioner->size);
	TEST_ASSERT_EQUAL_INT(1, positioner->size->w);
	TEST_ASSERT_EQUAL_INT(2, positioner->size->h);

	// Clean up
	bz_free_xdg_positioner_data(positioner);
}

void test_xdg_positioner_set_size__nonpositive_size_raises_error(void)
{
	// Test zero width
	bz_xdg_positioner_implementation.set_size(nullptr, nullptr, 0, 2);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(XDG_POSITIONER_ERROR_INVALID_INPUT, wl_resource_post_error_fake.arg1_val);

	// Test zero height
	bz_xdg_positioner_implementation.set_size(nullptr, nullptr, 1, 0);
	TEST_ASSERT_EQUAL_INT(2, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(XDG_POSITIONER_ERROR_INVALID_INPUT, wl_resource_post_error_fake.arg1_val);

	// Test negative width
	bz_xdg_positioner_implementation.set_size(nullptr, nullptr, -1, 2);
	TEST_ASSERT_EQUAL_INT(3, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(XDG_POSITIONER_ERROR_INVALID_INPUT, wl_resource_post_error_fake.arg1_val);

	// Test negative height
	bz_xdg_positioner_implementation.set_size(nullptr, nullptr, 1, -2);
	TEST_ASSERT_EQUAL_INT(4, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(XDG_POSITIONER_ERROR_INVALID_INPUT, wl_resource_post_error_fake.arg1_val);
}

void test_xdg_positioner_set_size__called_twice_reuses_memory(void)
{
	// Set up data
	struct bz_xdg_positioner *positioner = bz_create_xdg_positioner_data();
	wl_resource_get_user_data_fake.return_val = positioner;

	// Run the test
	bz_xdg_positioner_implementation.set_size(nullptr, nullptr, 1, 2);
	struct bz_size *first_size = positioner->size;
	bz_xdg_positioner_implementation.set_size(nullptr, nullptr, 3, 4);
	TEST_ASSERT_EQUAL(first_size, positioner->size);
	TEST_ASSERT_NOT_NULL(positioner->size);
	TEST_ASSERT_EQUAL_INT(3, first_size->w);
	TEST_ASSERT_EQUAL_INT(4, first_size->h);

	// Clean up
	bz_free_xdg_positioner_data(positioner);
}


// =================================================================================================
//  Test bz_xdg_positioner_set_anchor_rect()
// -------------------------------------------------------------------------------------------------

void test_xdg_positioner_set_anchor_rect__assigns_dimensions(void)
{
	// Set up data
	struct bz_xdg_positioner *positioner = bz_create_xdg_positioner_data();
	wl_resource_get_user_data_fake.return_val = positioner;

	// Run the test
	bz_xdg_positioner_implementation.set_anchor_rect(nullptr, nullptr, 1, 2, 3, 4);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_NOT_NULL(positioner->anchor_rect);
	TEST_ASSERT_EQUAL_INT(1, positioner->anchor_rect->x);
	TEST_ASSERT_EQUAL_INT(2, positioner->anchor_rect->y);
	TEST_ASSERT_EQUAL_INT(3, positioner->anchor_rect->w);
	TEST_ASSERT_EQUAL_INT(4, positioner->anchor_rect->h);

	// Clean up
	bz_free_xdg_positioner_data(positioner);
}

void test_xdg_positioner_set_anchor_rect__negative_size_raises_error(void)
{
	// Test width
	bz_xdg_positioner_implementation.set_anchor_rect(nullptr, nullptr, 0, 0, -1, 2);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(XDG_POSITIONER_ERROR_INVALID_INPUT, wl_resource_post_error_fake.arg1_val);

	// Test height
	bz_xdg_positioner_implementation.set_anchor_rect(nullptr, nullptr, 0, 0, 1, -2);
	TEST_ASSERT_EQUAL_INT(2, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(XDG_POSITIONER_ERROR_INVALID_INPUT, wl_resource_post_error_fake.arg1_val);
}

void test_xdg_positioner_set_anchor_rect__zero_size_is_fine(void)
{
	// Set up data
	struct bz_xdg_positioner *positioner = bz_create_xdg_positioner_data();
	wl_resource_get_user_data_fake.return_val = positioner;

	// Run the test
	bz_xdg_positioner_implementation.set_anchor_rect(nullptr, nullptr, 1, 2, 0, 0);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_NOT_NULL(positioner->anchor_rect);
	TEST_ASSERT_EQUAL_INT(1, positioner->anchor_rect->x);
	TEST_ASSERT_EQUAL_INT(2, positioner->anchor_rect->y);
	TEST_ASSERT_EQUAL_INT(0, positioner->anchor_rect->w);
	TEST_ASSERT_EQUAL_INT(0, positioner->anchor_rect->h);

	// Clean up
	bz_free_xdg_positioner_data(positioner);
}

void test_xdg_positioner_set_anchor_rect__called_twice_reuses_memory(void)
{
	// Set up data
	struct bz_xdg_positioner *positioner = bz_create_xdg_positioner_data();
	wl_resource_get_user_data_fake.return_val = positioner;

	// Run the test
	bz_xdg_positioner_implementation.set_anchor_rect(nullptr, nullptr, 1, 2, 3, 4);
	struct bz_rect *first_anchor_rect = positioner->anchor_rect;
	bz_xdg_positioner_implementation.set_anchor_rect(nullptr, nullptr, 5, 6, 7, 8);
	TEST_ASSERT_EQUAL(first_anchor_rect, positioner->anchor_rect);
	TEST_ASSERT_NOT_NULL(positioner->anchor_rect);
	TEST_ASSERT_EQUAL_INT(5, first_anchor_rect->x);
	TEST_ASSERT_EQUAL_INT(6, first_anchor_rect->y);
	TEST_ASSERT_EQUAL_INT(7, first_anchor_rect->w);
	TEST_ASSERT_EQUAL_INT(8, first_anchor_rect->h);

	// Clean up
	bz_free_xdg_positioner_data(positioner);
}


// =================================================================================================
//  Test bz_xdg_positioner_set_anchor()
// -------------------------------------------------------------------------------------------------

void test_xdg_positioner_set_anchor__valid_value_is_saved(void)
{
	// Set up mocks
	struct bz_xdg_positioner *positioner = bz_create_xdg_positioner_data();
	wl_resource_get_user_data_fake.return_val = positioner;

	// Run test
	TEST_ASSERT_EQUAL_INT(0, positioner->anchor);
	bz_xdg_positioner_implementation.set_anchor(nullptr, nullptr, 2);
	TEST_ASSERT_EQUAL_INT(2, positioner->anchor);

	// Clean up
	bz_free_xdg_positioner_data(positioner);
}

void test_xdg_positioner_set_anchor__invalid_value_raises_error(void)
{
	// Set up mocks
	struct bz_xdg_positioner *positioner = bz_create_xdg_positioner_data();
	wl_resource_get_user_data_fake.return_val = positioner;

	// Run test
	TEST_ASSERT_EQUAL_INT(0, positioner->anchor);
	bz_xdg_positioner_implementation.set_anchor(nullptr, nullptr, 9);
	TEST_ASSERT_EQUAL_INT(0, positioner->anchor);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(XDG_POSITIONER_ERROR_INVALID_INPUT, wl_resource_post_error_fake.arg1_val);

	// Clean up
	bz_free_xdg_positioner_data(positioner);
}


// =================================================================================================
//  Test bz_xdg_positioner_set_gravity()
// -------------------------------------------------------------------------------------------------

void test_xdg_positioner_set_gravity__valid_value_is_saved(void)
{
	// Set up mocks
	struct bz_xdg_positioner *positioner = bz_create_xdg_positioner_data();
	wl_resource_get_user_data_fake.return_val = positioner;

	// Run test
	TEST_ASSERT_EQUAL_INT(0, positioner->gravity);
	bz_xdg_positioner_implementation.set_gravity(nullptr, nullptr, 2);
	TEST_ASSERT_EQUAL_INT(2, positioner->gravity);

	// Clean up
	bz_free_xdg_positioner_data(positioner);
}

void test_xdg_positioner_set_gravity__invalid_value_raises_error(void)
{
	// Set up mocks
	struct bz_xdg_positioner *positioner = bz_create_xdg_positioner_data();
	wl_resource_get_user_data_fake.return_val = positioner;

	// Run test
	TEST_ASSERT_EQUAL_INT(0, positioner->gravity);
	bz_xdg_positioner_implementation.set_gravity(nullptr, nullptr, 9);
	TEST_ASSERT_EQUAL_INT(0, positioner->gravity);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(XDG_POSITIONER_ERROR_INVALID_INPUT, wl_resource_post_error_fake.arg1_val);

	// Clean up
	bz_free_xdg_positioner_data(positioner);
}


// =================================================================================================
//  Test bz_xdg_positioner_set_offset()
// -------------------------------------------------------------------------------------------------

void test_xdg_positioner_set_offset__assigns_offset(void)
{
	// Set up mocks
	struct bz_xdg_positioner *positioner = bz_create_xdg_positioner_data();
	wl_resource_get_user_data_fake.return_val = positioner;

	// Run test
	TEST_ASSERT_NULL(positioner->offset);
	bz_xdg_positioner_implementation.set_offset(nullptr, nullptr, 2, 3);
	TEST_ASSERT_EQUAL_INT(2, positioner->offset->x);
	TEST_ASSERT_EQUAL_INT(3, positioner->offset->y);

	// Clean up
	bz_free_xdg_positioner_data(positioner);
}

void test_xdg_positioner_set_offset__called_twice_reuses_memory(void)
{
	// Set up mocks
	struct bz_xdg_positioner *positioner = bz_create_xdg_positioner_data();
	wl_resource_get_user_data_fake.return_val = positioner;

	// Run the test
	TEST_ASSERT_NULL(positioner->offset);
	bz_xdg_positioner_implementation.set_offset(nullptr, nullptr, 1, 2);
	struct bz_position *first_offset = positioner->offset;
	bz_xdg_positioner_implementation.set_offset(nullptr, nullptr, 3, 4);
	TEST_ASSERT_EQUAL(first_offset, positioner->offset);
	TEST_ASSERT_NOT_NULL(positioner->offset);
	TEST_ASSERT_EQUAL_INT(3, first_offset->x);
	TEST_ASSERT_EQUAL_INT(4, first_offset->y);

	// Clean up
	bz_free_xdg_positioner_data(positioner);
}


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// Test bz_xdg_positioner_destroy()
	RUN_TEST(test_xdg_positioner_destroy__destroys_the_resource);

	// Test bz_xdg_positioner_set_size()
	RUN_TEST(test_xdg_positioner_set_size__assigns_width_and_height);
	RUN_TEST(test_xdg_positioner_set_size__nonpositive_size_raises_error);
	RUN_TEST(test_xdg_positioner_set_size__called_twice_reuses_memory);

	// Test bz_xdg_positioner_set_anchor_rect()
	RUN_TEST(test_xdg_positioner_set_anchor_rect__assigns_dimensions);
	RUN_TEST(test_xdg_positioner_set_anchor_rect__negative_size_raises_error);
	RUN_TEST(test_xdg_positioner_set_anchor_rect__zero_size_is_fine);
	RUN_TEST(test_xdg_positioner_set_anchor_rect__called_twice_reuses_memory);

	// Test bz_xdg_positioner_set_anchor()
	RUN_TEST(test_xdg_positioner_set_anchor__valid_value_is_saved);
	RUN_TEST(test_xdg_positioner_set_anchor__invalid_value_raises_error);

	// Test bz_xdg_positioner_set_gravity()
	RUN_TEST(test_xdg_positioner_set_gravity__valid_value_is_saved);
	RUN_TEST(test_xdg_positioner_set_gravity__invalid_value_raises_error);

	// Test bz_xdg_positioner_set_constraint_adjustment()
	// TODO-dl??: Write tests

	// Test bz_xdg_positioner_set_offset()
	RUN_TEST(test_xdg_positioner_set_offset__assigns_offset);
	RUN_TEST(test_xdg_positioner_set_offset__called_twice_reuses_memory);

	return UNITY_END();
}
