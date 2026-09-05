
#include "breezy/bz_wl_display.h"

#include <stdlib.h>

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

extern const struct wl_subcompositor_interface bz_subcompositor_implementation;


// =================================================================================================
//  Test bz_subcompositor_constructor()
// -------------------------------------------------------------------------------------------------

/** bz_subcompositor_constructor() properly initializes our resource. */
void test_subcompositor_constructor__initializes_resource(void)
{
	// Set up our mocks
	struct wl_resource *subcompositor = calloc(1, sizeof(*subcompositor));
	wl_resource_create_fake.return_val = subcompositor;

	// Run our test!
	bz_subcompositor_constructor(nullptr, nullptr, 0, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_PTR(subcompositor, wl_resource_set_implementation_fake.arg0_val);

	// Clean up!
	free(subcompositor);
}

/** bz_subcompositor_constructor() posts a no memory error for failed Wayland resource creation. */
void test_subcompositor_constructor__posts_no_mem_for_failed_resource(void)
{
	// Subcompositor creation should return a nullptr to trigger failure.
	wl_resource_create_fake.return_val = nullptr;

	// Run our test!
	bz_subcompositor_constructor(nullptr, nullptr, 0, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_set_implementation_fake.call_count);
}


// =================================================================================================
//  Test bz_subcompositor_destroy()
// -------------------------------------------------------------------------------------------------

/** bz_subcompositor_destroy() unbinds the client from this interface. */
void test_destroy__destroys_the_resource(void)
{
	TEST_ASSERT_EQUAL_INT(0, wl_resource_destroy_fake.call_count);
	bz_subcompositor_implementation.destroy(nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_destroy_fake.call_count);
}


// =================================================================================================
//  Test bz_subcompositor_get_subsurface()
// -------------------------------------------------------------------------------------------------

/** bz_subcompositor_get_subsurface() assigns the correct role when everything goes correctly. */
void test_get_subsurface__assigns_subsurface_role(void)
{
	// Set up our mocks
	struct bz_surface *surface_data = bz_create_surface_data();
	struct bz_surface *parent_data = bz_create_surface_data();
	void *surface_datas[2] = { surface_data, parent_data };
	SET_RETURN_SEQ(wl_resource_get_user_data, surface_datas, 2);

	struct wl_resource *subsurface = calloc(1, sizeof(*subsurface));
	wl_resource_create_fake.return_val = subsurface;

	// Run our test
	bz_subcompositor_implementation.get_subsurface(nullptr, nullptr, 0, nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	struct bz_subsurface *subsurface_data = wl_resource_set_implementation_fake.arg2_val;
	TEST_ASSERT_EQUAL_INT(BZ_SURF_ROLE_WL_SUBSURFACE, surface_data->role);
	TEST_ASSERT_EQUAL_PTR(subsurface_data, surface_data->subsurface);
	TEST_ASSERT_EQUAL_PTR(surface_data, subsurface_data->surface);

	// Clean up
	free(subsurface);
	bz_free_subsurface_data(subsurface_data);
	bz_free_surface_data(parent_data);
	bz_free_surface_data(surface_data);
}

/** bz_subcompositor_get_subsurface() is properly linked to the parent surface. */
void test_get_subsurface__gets_linked_to_parent(void)
{
	// Set up our mocks
	struct bz_surface *surface_data = bz_create_surface_data();
	struct bz_surface *parent_data = bz_create_surface_data();
	void *surface_datas[2] = { surface_data, parent_data };
	SET_RETURN_SEQ(wl_resource_get_user_data, surface_datas, 2);

	struct wl_resource *subsurface = calloc(1, sizeof(*subsurface));
	wl_resource_create_fake.return_val = subsurface;

	// Run our test
	bz_subcompositor_implementation.get_subsurface(nullptr, nullptr, 0, nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	struct bz_subsurface *subsurface_data = wl_resource_set_implementation_fake.arg2_val;
	TEST_ASSERT_EQUAL_PTR(parent_data, subsurface_data->parent);
	TEST_ASSERT_TRUE(bz_list_contains(parent_data->surface_stack, surface_data));

	// Clean up
	free(subsurface);
	bz_free_subsurface_data(subsurface_data);
	bz_free_surface_data(parent_data);
	bz_free_surface_data(surface_data);
}

/** If another role is already assigned, a "bad_surface" protocol error is raised. */
void test_get_subsurface__existing_role_raises_error(void)
{
	// Set up our mocks
	struct bz_surface *surface_data = bz_create_surface_data();
	struct bz_surface *parent_data = bz_create_surface_data();
	void *surface_datas[2] = { surface_data, parent_data };
	SET_RETURN_SEQ(wl_resource_get_user_data, surface_datas, 2);
	surface_data->role = BZ_SURF_ROLE_XDG_TOPLEVEL;

	// Run our test
	bz_subcompositor_implementation.get_subsurface(nullptr, nullptr, 0, nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE, wl_resource_post_error_fake.arg1_val);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_create_fake.call_count);

	// Clean up
	bz_free_surface_data(parent_data);
	bz_free_surface_data(surface_data);
}

/** If another subsurface is already assigned, a "bad_surface" protocol error is raised. */
void test_get_subsurface__existing_subsurface_raises_error(void)
{
	// Set up our mocks
	struct bz_surface *surface_data = bz_create_surface_data();
	struct bz_surface *parent_data = bz_create_surface_data();
	void *surface_datas[2] = { surface_data, parent_data };
	SET_RETURN_SEQ(wl_resource_get_user_data, surface_datas, 2);

	struct bz_subsurface *bad_subsurface_data = bz_create_subsurface_data();
	surface_data->subsurface = bad_subsurface_data;

	// Run our test
	bz_subcompositor_implementation.get_subsurface(nullptr, nullptr, 0, nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE, wl_resource_post_error_fake.arg1_val);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_create_fake.call_count);

	// Clean up
	bz_free_subsurface_data(bad_subsurface_data);
	bz_free_surface_data(parent_data);
	bz_free_surface_data(surface_data);
}

/** If the given parent is equal to the to-be child, a "bad_parent" protocol error is raised. */
void test_get_subsurface__parent_equaling_child_raises_error(void)
{
	// Set up our mocks
	struct bz_surface *surface_data = bz_create_surface_data();
	void *surface_datas[2] = { surface_data, surface_data };
	SET_RETURN_SEQ(wl_resource_get_user_data, surface_datas, 2);

	// Run our test
	bz_subcompositor_implementation.get_subsurface(nullptr, nullptr, 0, nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_SUBCOMPOSITOR_ERROR_BAD_PARENT, wl_resource_post_error_fake.arg1_val);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_create_fake.call_count);

	// Clean up
	bz_free_surface_data(surface_data);
}

/** If the given parent descends from the to-be child, a "bad_parent" protocol error is raised. */
void test_get_subsurface__parent_descending_child_raises_error(void)
{
	// Set up our mocks
	struct bz_surface *surface_data = bz_create_surface_data();
	struct bz_surface *parent_data = bz_create_surface_data();
	void *surface_datas[2] = { surface_data, parent_data };
	SET_RETURN_SEQ(wl_resource_get_user_data, surface_datas, 2);

	// Make the parent a child of the surface.
	bz_list_append(surface_data->surface_stack, parent_data);

	// Run our test
	bz_subcompositor_implementation.get_subsurface(nullptr, nullptr, 0, nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_SUBCOMPOSITOR_ERROR_BAD_PARENT, wl_resource_post_error_fake.arg1_val);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_create_fake.call_count);

	// Clean up
	bz_free_surface_data(parent_data);
	bz_free_surface_data(surface_data);
}


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// Test bz_subcompositor_constructor()
	RUN_TEST(test_subcompositor_constructor__initializes_resource);
	RUN_TEST(test_subcompositor_constructor__posts_no_mem_for_failed_resource);

	// Test bz_subcompositor_destroy()
	RUN_TEST(test_destroy__destroys_the_resource);

	// Test bz_subcompositor_get_subsurface()
	RUN_TEST(test_get_subsurface__assigns_subsurface_role);
	RUN_TEST(test_get_subsurface__gets_linked_to_parent);
	RUN_TEST(test_get_subsurface__existing_role_raises_error);
	RUN_TEST(test_get_subsurface__existing_subsurface_raises_error);
	RUN_TEST(test_get_subsurface__parent_equaling_child_raises_error);
	RUN_TEST(test_get_subsurface__parent_descending_child_raises_error);

	return UNITY_END();
}
