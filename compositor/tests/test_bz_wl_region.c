
#include "breezy/bz_wl_display.h"

#include <stdlib.h>
#include <wayland-server-protocol.h>

#include "unity.h"
#include "fff.h"
#include "../include/breezy/bz_wl_display.h"
#include "breezy/bz_graphics.h"
#include "breezy/bz_logger.h"
#include "helpers/bz_test_resources.c"


// =================================================================================================
//  Set up / tear down / globals
// -------------------------------------------------------------------------------------------------

DEFINE_FFF_GLOBALS
// -- wl_resource --
FAKE_VALUE_FUNC(void *, wl_resource_get_user_data, struct wl_resource *)
FAKE_VOID_FUNC(wl_resource_destroy, struct wl_resource *)

void setUp(void)
{
	RESET_FAKE(wl_resource_get_user_data);
	RESET_FAKE(wl_resource_destroy);
	FFF_RESET_HISTORY();

	bz_log_initialize(BZ_LOG_OFF);
}

void tearDown(void) {}


// =================================================================================================
//  Helper functions for some of our tests
// -------------------------------------------------------------------------------------------------

extern const struct wl_region_interface bz_region_implementation;


// =================================================================================================
//  Test bz_region_destroy()
// -------------------------------------------------------------------------------------------------

/** Destroys the region resource. */
void test_region_destroy__deletes_region()
{
	// Set up our mocks
	struct wl_resource *region = calloc(1, sizeof(*region));
	struct bz_region *region_data = bz_create_region_data();
	wl_resource_get_user_data_fake.return_val = region_data;
	bz_region_implementation.add(nullptr, nullptr, 1, 2, 3, 4);
	bz_region_implementation.subtract(nullptr, nullptr, 5, 6, 7, 8);

	// First test destroy
	TEST_ASSERT_EQUAL_INT(0, wl_resource_destroy_fake.call_count);
	bz_region_implementation.destroy(nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_destroy_fake.call_count);

	// Since destroy calls the mocked "wl_resource_destroy()", we need to manually call our dtor.
	TEST_ASSERT_EQUAL_INT(2, region_data->mutations->length);
	bz_region_dtor(region);

	// Clean up!
	free(region);
	// Everything else should be freed. If something isn't freed by the dtor, this test should fail.
}


// =================================================================================================
//  Test bz_region_add()
// -------------------------------------------------------------------------------------------------

/** Adds a rectangle to the region. */
void test_region_add__appends_operation_to_region()
{
	// Set up our mocks
	struct bz_region *region_data = bz_create_region_data();
	wl_resource_get_user_data_fake.return_val = region_data;

	// Run the test
	TEST_ASSERT_EQUAL_INT(0, region_data->mutations->length);
	bz_region_implementation.add(nullptr, nullptr, 1, 2, 3, 4);
	TEST_ASSERT_EQUAL_INT(1, region_data->mutations->length);
	struct bz_region_mutation *mutation = region_data->mutations->head->data;
	TEST_ASSERT_EQUAL_INT(OP_ADD, mutation->op);
	TEST_ASSERT_EQUAL_INT(1, mutation->x);
	TEST_ASSERT_EQUAL_INT(2, mutation->y);
	TEST_ASSERT_EQUAL_INT(3, mutation->w);
	TEST_ASSERT_EQUAL_INT(4, mutation->h);

	// Clean up
	bz_free_region_data(region_data);
}


// =================================================================================================
//  Test bz_region_subtract()
// -------------------------------------------------------------------------------------------------

/** Subtracts a rectangle from the region. */
void test_region_subtract__appends_operation_to_region()
{
	// Set up our mocks
	struct bz_region *region_data = bz_create_region_data();
	wl_resource_get_user_data_fake.return_val = region_data;

	// Run the test
	TEST_ASSERT_EQUAL_INT(0, region_data->mutations->length);
	bz_region_implementation.subtract(nullptr, nullptr, 1, 2, 3, 4);
	TEST_ASSERT_EQUAL_INT(1, region_data->mutations->length);
	struct bz_region_mutation *mutation = region_data->mutations->head->data;
	TEST_ASSERT_EQUAL_INT(OP_SUBTRACT, mutation->op);
	TEST_ASSERT_EQUAL_INT(1, mutation->x);
	TEST_ASSERT_EQUAL_INT(2, mutation->y);
	TEST_ASSERT_EQUAL_INT(3, mutation->w);
	TEST_ASSERT_EQUAL_INT(4, mutation->h);

	// Clean up
	bz_free_region_data(region_data);
}


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// Test bz_region_destroy()
	RUN_TEST(test_region_destroy__deletes_region); // TODO

	// Test bz_region_add()
	RUN_TEST(test_region_add__appends_operation_to_region); // TODO

	// Test bz_region_subtract()
	RUN_TEST(test_region_subtract__appends_operation_to_region); // TODO

	return UNITY_END();
}
