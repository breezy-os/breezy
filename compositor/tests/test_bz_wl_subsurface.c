
#include "breezy/bz_xdg_shell.h"

#include <stdlib.h>

#include <xdg-shell-server-protocol.h>

#include "unity.h"
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

extern const struct wl_subsurface_interface bz_subsurface_implementation;

struct bz_surface *prep_subsurface_placement_test()
{
	struct bz_surface *parent_data = bz_create_surface_data();

	// Child 1
	struct bz_surface *surface_data_1 = bz_create_surface_data();
	struct bz_subsurface *subsurface_data_1 = bz_create_subsurface_data();
	surface_data_1->subsurface = subsurface_data_1;
	subsurface_data_1->parent = parent_data;
	subsurface_data_1->surface = surface_data_1;
	bz_list_append(parent_data->surface_stack, surface_data_1);

	// Child 2
	struct bz_surface *surface_data_2 = bz_create_surface_data();
	struct bz_subsurface *subsurface_data_2 = bz_create_subsurface_data();
	surface_data_2->subsurface = subsurface_data_2;
	subsurface_data_2->parent = parent_data;
	subsurface_data_2->surface = surface_data_2;
	bz_list_append(parent_data->surface_stack, surface_data_2);

	// Child 3
	struct bz_surface *surface_data_3 = bz_create_surface_data();
	struct bz_subsurface *subsurface_data_3 = bz_create_subsurface_data();
	surface_data_3->subsurface = subsurface_data_3;
	subsurface_data_3->parent = parent_data;
	subsurface_data_3->surface = surface_data_3;
	bz_list_append(parent_data->surface_stack, surface_data_3);

	return parent_data;
}

void tear_down_subsurface_placement_test(struct bz_surface *parent)
{
	struct bz_surface *child; bz_list_foreach(child, parent->surface_stack) {
		if (child == parent) continue;
		bz_free_subsurface_data(child->subsurface);
		bz_free_surface_data(child);
	}
	bz_free_surface_data(parent);
}


// =================================================================================================
//  Test bz_subsurface_destroy()
// -------------------------------------------------------------------------------------------------

void test_subsurface_destroy__destroys_the_resource(void)
{
	bz_subsurface_implementation.destroy(nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_destroy_fake.call_count);
}


// =================================================================================================
//  Test bz_subsurface_set_position()
// -------------------------------------------------------------------------------------------------

void test_subsurface_set_position__is_buffered_on_parent_state(void)
{
	// Set up our test data
	struct bz_surface *parent_data = bz_create_surface_data();
	struct bz_surface *surface_data = bz_create_surface_data();
	struct bz_subsurface *subsurface_data = bz_create_subsurface_data();
	subsurface_data->parent = parent_data;
	subsurface_data->surface = surface_data;
	bz_list_append(parent_data->surface_stack, surface_data);

	// And our mocks
	wl_resource_get_user_data_fake.return_val = subsurface_data;

	// Run our test
	bz_subsurface_implementation.set_position(nullptr, nullptr, 1, 2);
	TEST_ASSERT_EQUAL_INT(1, parent_data->pending_state->subsurface_states->length);
	struct bz_subsurface_state *state = parent_data->pending_state->subsurface_states->head->data;
	TEST_ASSERT_EQUAL_INT(1, state->position.x);
	TEST_ASSERT_EQUAL_INT(2, state->position.y);

	// Clean up
	bz_free_subsurface_data(subsurface_data);
	bz_free_surface_data(surface_data);
	bz_free_surface_data(parent_data);
}

void test_subsurface_set_position__negative_values_are_allowed(void)
{
	// Set up our test data
	struct bz_surface *parent_data = bz_create_surface_data();
	struct bz_surface *surface_data = bz_create_surface_data();
	struct bz_subsurface *subsurface_data = bz_create_subsurface_data();
	subsurface_data->parent = parent_data;
	subsurface_data->surface = surface_data;
	bz_list_append(parent_data->surface_stack, surface_data);

	// And our mocks
	wl_resource_get_user_data_fake.return_val = subsurface_data;

	// Run our test
	bz_subsurface_implementation.set_position(nullptr, nullptr, -1, -2);
	TEST_ASSERT_EQUAL_INT(1, parent_data->pending_state->subsurface_states->length);
	struct bz_subsurface_state *state = parent_data->pending_state->subsurface_states->head->data;
	TEST_ASSERT_EQUAL_INT(-1, state->position.x);
	TEST_ASSERT_EQUAL_INT(-2, state->position.y);

	// Clean up
	bz_free_subsurface_data(subsurface_data);
	bz_free_surface_data(surface_data);
	bz_free_surface_data(parent_data);
}

void test_subsurface_set_position__multiple_calls_updates_prev_value(void)
{
	// Set up our test data
	struct bz_surface *parent_data = bz_create_surface_data();
	struct bz_surface *surface_data = bz_create_surface_data();
	struct bz_subsurface *subsurface_data = bz_create_subsurface_data();
	subsurface_data->parent = parent_data;
	subsurface_data->surface = surface_data;
	bz_list_append(parent_data->surface_stack, surface_data);

	// And our mocks
	wl_resource_get_user_data_fake.return_val = subsurface_data;

	// Run our test
	bz_subsurface_implementation.set_position(nullptr, nullptr, 1, 2);
	bz_subsurface_implementation.set_position(nullptr, nullptr, 3, 4);
	TEST_ASSERT_EQUAL_INT(1, parent_data->pending_state->subsurface_states->length);
	struct bz_subsurface_state *state = parent_data->pending_state->subsurface_states->head->data;
	TEST_ASSERT_EQUAL_INT(3, state->position.x);
	TEST_ASSERT_EQUAL_INT(4, state->position.y);

	// Clean up
	bz_free_subsurface_data(subsurface_data);
	bz_free_surface_data(surface_data);
	bz_free_surface_data(parent_data);
}


// =================================================================================================
//  Test bz_subsurface_place_above()
// -------------------------------------------------------------------------------------------------

void test_subsurface_place_above__is_buffered_on_parent_state(void)
{
	// Set up our test data
	struct bz_surface *parent = prep_subsurface_placement_test();
	struct bz_surface *child1 = parent->surface_stack->head->next->data;
	// struct bz_surface *child2 = parent->surface_stack->head->next->next->data;
	struct bz_surface *child3 = parent->surface_stack->head->next->next->next->data;

	// And our mocks
	void *ret_vals[2] = { child1->subsurface, child3 };
	SET_RETURN_SEQ(wl_resource_get_user_data, ret_vals, 2);

	// Run our test
	bz_subsurface_implementation.place_above(nullptr, nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(1, parent->pending_state->subsurface_states->length);
	struct bz_subsurface_state *state = parent->pending_state->subsurface_states->head->data;
	TEST_ASSERT_EQUAL_INT(BZ_SUBSURFACE_PLACE_ABOVE, state->placement);
	TEST_ASSERT_EQUAL_INT(child3, state->sibling);

	// Clean up
	tear_down_subsurface_placement_test(parent);
}

void test_subsurface_place_above__cannot_place_relative_to_self(void)
{
	// Set up our test data
	struct bz_surface *parent = prep_subsurface_placement_test();
	struct bz_surface *child1 = parent->surface_stack->head->next->data;
	// struct bz_surface *child2 = parent->surface_stack->head->next->next->data;
	// struct bz_surface *child3 = parent->surface_stack->head->next->next->next->data;

	// And our mocks
	void *ret_vals[2] = { child1->subsurface, child1 };
	SET_RETURN_SEQ(wl_resource_get_user_data, ret_vals, 2);

	// Run our test
	bz_subsurface_implementation.place_above(nullptr, nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(0, parent->pending_state->subsurface_states->length);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_SUBSURFACE_ERROR_BAD_SURFACE, wl_resource_post_error_fake.arg1_val);

	// Clean up
	tear_down_subsurface_placement_test(parent);
}

void test_subsurface_place_above__cannot_place_relative_to_unrelated_surface(void)
{
	// Set up our test data
	struct bz_surface *parent = prep_subsurface_placement_test();
	struct bz_surface *child1 = parent->surface_stack->head->next->data;
	// struct bz_surface *child2 = parent->surface_stack->head->next->next->data;
	// struct bz_surface *child3 = parent->surface_stack->head->next->next->next->data;
	struct bz_surface *unrelated = bz_create_surface_data();

	// And our mocks
	void *ret_vals[2] = { child1->subsurface, unrelated };
	SET_RETURN_SEQ(wl_resource_get_user_data, ret_vals, 2);

	// Run our test
	bz_subsurface_implementation.place_above(nullptr, nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(0, parent->pending_state->subsurface_states->length);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_SUBSURFACE_ERROR_BAD_SURFACE, wl_resource_post_error_fake.arg1_val);

	// Clean up
	bz_free_surface_data(unrelated);
	tear_down_subsurface_placement_test(parent);
}

void test_subsurface_place_above__can_be_relative_to_sibling(void)
{
	// Set up our test data
	struct bz_surface *parent = prep_subsurface_placement_test();
	struct bz_surface *child1 = parent->surface_stack->head->next->data;
	struct bz_surface *child2 = parent->surface_stack->head->next->next->data;
	// struct bz_surface *child3 = parent->surface_stack->head->next->next->next->data;

	// And our mocks
	void *ret_vals[2] = { child1->subsurface, child2 };
	SET_RETURN_SEQ(wl_resource_get_user_data, ret_vals, 2);

	// Run our test
	bz_subsurface_implementation.place_above(nullptr, nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(1, parent->pending_state->subsurface_states->length);
	struct bz_subsurface_state *state = parent->pending_state->subsurface_states->head->data;
	TEST_ASSERT_EQUAL_INT(BZ_SUBSURFACE_PLACE_ABOVE, state->placement);
	TEST_ASSERT_EQUAL_INT(child2, state->sibling);

	// Clean up
	tear_down_subsurface_placement_test(parent);
}

void test_subsurface_place_above__can_be_relative_to_parent(void)
{
	// Set up our test data
	struct bz_surface *parent = prep_subsurface_placement_test();
	struct bz_surface *child1 = parent->surface_stack->head->next->data;
	// struct bz_surface *child2 = parent->surface_stack->head->next->next->data;
	// struct bz_surface *child3 = parent->surface_stack->head->next->next->next->data;

	// And our mocks
	void *ret_vals[2] = { child1->subsurface, parent };
	SET_RETURN_SEQ(wl_resource_get_user_data, ret_vals, 2);

	// Run our test
	bz_subsurface_implementation.place_above(nullptr, nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(1, parent->pending_state->subsurface_states->length);
	struct bz_subsurface_state *state = parent->pending_state->subsurface_states->head->data;
	TEST_ASSERT_EQUAL_INT(BZ_SUBSURFACE_PLACE_ABOVE, state->placement);
	TEST_ASSERT_EQUAL_INT(parent, state->sibling);

	// Clean up
	tear_down_subsurface_placement_test(parent);
}

void test_subsurface_place_above__multiple_calls_updates_prev_value(void)
{
	// Set up our test data
	struct bz_surface *parent = prep_subsurface_placement_test();
	struct bz_surface *child1 = parent->surface_stack->head->next->data;
	struct bz_surface *child2 = parent->surface_stack->head->next->next->data;
	struct bz_surface *child3 = parent->surface_stack->head->next->next->next->data;

	// And our mocks
	void *ret_vals[4] = {
		child1->subsurface, child2, // First call
		child1->subsurface, child3, // Second call
	};
	SET_RETURN_SEQ(wl_resource_get_user_data, ret_vals, 4);

	// Run our test
	bz_subsurface_implementation.place_above(nullptr, nullptr, nullptr);
	bz_subsurface_implementation.place_above(nullptr, nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(1, parent->pending_state->subsurface_states->length);
	struct bz_subsurface_state *state = parent->pending_state->subsurface_states->head->data;
	TEST_ASSERT_EQUAL_INT(BZ_SUBSURFACE_PLACE_ABOVE, state->placement);
	TEST_ASSERT_EQUAL_INT(child3, state->sibling);

	// Clean up
	tear_down_subsurface_placement_test(parent);
}


// =================================================================================================
//  Test bz_subsurface_place_below()
// -------------------------------------------------------------------------------------------------

void test_subsurface_place_below__is_buffered_on_parent_state(void)
{
	// Set up our test data
	struct bz_surface *parent = prep_subsurface_placement_test();
	struct bz_surface *child1 = parent->surface_stack->head->next->data;
	// struct bz_surface *child2 = parent->surface_stack->head->next->next->data;
	struct bz_surface *child3 = parent->surface_stack->head->next->next->next->data;

	// And our mocks
	void *ret_vals[2] = { child1->subsurface, child3 };
	SET_RETURN_SEQ(wl_resource_get_user_data, ret_vals, 2);

	// Run our test
	bz_subsurface_implementation.place_below(nullptr, nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(1, parent->pending_state->subsurface_states->length);
	struct bz_subsurface_state *state = parent->pending_state->subsurface_states->head->data;
	TEST_ASSERT_EQUAL_INT(BZ_SUBSURFACE_PLACE_BELOW, state->placement);
	TEST_ASSERT_EQUAL_INT(child3, state->sibling);

	// Clean up
	tear_down_subsurface_placement_test(parent);
}

void test_subsurface_place_below__cannot_place_relative_to_self(void)
{
	// Set up our test data
	struct bz_surface *parent = prep_subsurface_placement_test();
	struct bz_surface *child1 = parent->surface_stack->head->next->data;
	// struct bz_surface *child2 = parent->surface_stack->head->next->next->data;
	// struct bz_surface *child3 = parent->surface_stack->head->next->next->next->data;

	// And our mocks
	void *ret_vals[2] = { child1->subsurface, child1 };
	SET_RETURN_SEQ(wl_resource_get_user_data, ret_vals, 2);

	// Run our test
	bz_subsurface_implementation.place_below(nullptr, nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(0, parent->pending_state->subsurface_states->length);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_SUBSURFACE_ERROR_BAD_SURFACE, wl_resource_post_error_fake.arg1_val);

	// Clean up
	tear_down_subsurface_placement_test(parent);
}

void test_subsurface_place_below__cannot_place_relative_to_unrelated_surface(void)
{
	// Set up our test data
	struct bz_surface *parent = prep_subsurface_placement_test();
	struct bz_surface *child1 = parent->surface_stack->head->next->data;
	// struct bz_surface *child2 = parent->surface_stack->head->next->next->data;
	// struct bz_surface *child3 = parent->surface_stack->head->next->next->next->data;
	struct bz_surface *unrelated = bz_create_surface_data();

	// And our mocks
	void *ret_vals[2] = { child1->subsurface, unrelated };
	SET_RETURN_SEQ(wl_resource_get_user_data, ret_vals, 2);

	// Run our test
	bz_subsurface_implementation.place_below(nullptr, nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(0, parent->pending_state->subsurface_states->length);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(WL_SUBSURFACE_ERROR_BAD_SURFACE, wl_resource_post_error_fake.arg1_val);

	// Clean up
	bz_free_surface_data(unrelated);
	tear_down_subsurface_placement_test(parent);
}

void test_subsurface_place_below__can_be_relative_to_sibling(void)
{
	// Set up our test data
	struct bz_surface *parent = prep_subsurface_placement_test();
	struct bz_surface *child1 = parent->surface_stack->head->next->data;
	struct bz_surface *child2 = parent->surface_stack->head->next->next->data;
	// struct bz_surface *child3 = parent->surface_stack->head->next->next->next->data;

	// And our mocks
	void *ret_vals[2] = { child1->subsurface, child2 };
	SET_RETURN_SEQ(wl_resource_get_user_data, ret_vals, 2);

	// Run our test
	bz_subsurface_implementation.place_below(nullptr, nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(1, parent->pending_state->subsurface_states->length);
	struct bz_subsurface_state *state = parent->pending_state->subsurface_states->head->data;
	TEST_ASSERT_EQUAL_INT(BZ_SUBSURFACE_PLACE_BELOW, state->placement);
	TEST_ASSERT_EQUAL_INT(child2, state->sibling);

	// Clean up
	tear_down_subsurface_placement_test(parent);
}

void test_subsurface_place_below__can_be_relative_to_parent(void)
{
	// Set up our test data
	struct bz_surface *parent = prep_subsurface_placement_test();
	struct bz_surface *child1 = parent->surface_stack->head->next->data;
	// struct bz_surface *child2 = parent->surface_stack->head->next->next->data;
	// struct bz_surface *child3 = parent->surface_stack->head->next->next->next->data;

	// And our mocks
	void *ret_vals[2] = { child1->subsurface, parent };
	SET_RETURN_SEQ(wl_resource_get_user_data, ret_vals, 2);

	// Run our test
	bz_subsurface_implementation.place_below(nullptr, nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(1, parent->pending_state->subsurface_states->length);
	struct bz_subsurface_state *state = parent->pending_state->subsurface_states->head->data;
	TEST_ASSERT_EQUAL_INT(BZ_SUBSURFACE_PLACE_BELOW, state->placement);
	TEST_ASSERT_EQUAL_INT(parent, state->sibling);

	// Clean up
	tear_down_subsurface_placement_test(parent);
}

void test_subsurface_place_below__multiple_calls_updates_prev_value(void)
{
	// Set up our test data
	struct bz_surface *parent = prep_subsurface_placement_test();
	struct bz_surface *child1 = parent->surface_stack->head->next->data;
	struct bz_surface *child2 = parent->surface_stack->head->next->next->data;
	struct bz_surface *child3 = parent->surface_stack->head->next->next->next->data;

	// And our mocks
	void *ret_vals[4] = {
		child1->subsurface, child2, // First call
		child1->subsurface, child3, // Second call
	};
	SET_RETURN_SEQ(wl_resource_get_user_data, ret_vals, 4);

	// Run our test
	bz_subsurface_implementation.place_below(nullptr, nullptr, nullptr);
	bz_subsurface_implementation.place_below(nullptr, nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(1, parent->pending_state->subsurface_states->length);
	struct bz_subsurface_state *state = parent->pending_state->subsurface_states->head->data;
	TEST_ASSERT_EQUAL_INT(BZ_SUBSURFACE_PLACE_BELOW, state->placement);
	TEST_ASSERT_EQUAL_INT(child3, state->sibling);

	// Clean up
	tear_down_subsurface_placement_test(parent);
}


// =================================================================================================
//  Test bz_subsurface_set_sync()
// -------------------------------------------------------------------------------------------------

void test_subsurface_set_sync__updates_the_subsurface_type(void)
{
	// Set up our test data / mocks
	struct bz_subsurface *subsurface_data = bz_create_subsurface_data();
	subsurface_data->is_sync = false;
	wl_resource_get_user_data_fake.return_val = subsurface_data;

	// Run our test
	bz_subsurface_implementation.set_sync(nullptr, nullptr);
	TEST_ASSERT_TRUE(subsurface_data->is_sync);

	// Clean up
	bz_free_subsurface_data(subsurface_data);
}


// =================================================================================================
//  Test bz_subsurface_set_desync()
// -------------------------------------------------------------------------------------------------

void test_subsurface_set_desync__updates_the_subsurface_type(void)
{
	// Set up our test data / mocks
	struct bz_surface *surface_data = bz_create_surface_data();
	struct bz_subsurface *subsurface_data = bz_create_subsurface_data();
	subsurface_data->is_sync = true;
	subsurface_data->surface = surface_data;
	surface_data->subsurface = subsurface_data;
	wl_resource_get_user_data_fake.return_val = subsurface_data;

	// Run our test
	bz_subsurface_implementation.set_desync(nullptr, nullptr);
	TEST_ASSERT_FALSE(subsurface_data->is_sync);

	// Clean up
	bz_free_subsurface_data(subsurface_data);
	bz_free_surface_data(surface_data);
}

void test_subsurface_set_desync__updates_unreachable_CUs_in_queue(void)
{
	// We want the subsurface to have 2 SCUs on it, only one of which has a parent DCU.
	// Then we can verify that the unreachable one becomes a DCU.

	// Create our parent and child surfaces/subsurface.
	struct bz_surface *parent = bz_create_surface_data();
	struct bz_surface *child = bz_create_surface_data();
	struct bz_subsurface *child_sub = bz_create_subsurface_data();
	bz_list_append(parent->surface_stack, child);
	child_sub->is_sync = true;
	child_sub->parent = parent;
	child_sub->surface = child;
	child->subsurface = child_sub;

	// Create our initial content updates
	//   Parent: scu1 --> dcu
	//   Child:             ^-- scu2 --> scu3
	struct bz_content_update *scu1 = bz_create_content_update_data(); scu1->is_sync = true;
	struct bz_content_update *scu2 = bz_create_content_update_data(); scu2->is_sync = true;
	struct bz_content_update *scu3 = bz_create_content_update_data(); scu3->is_sync = true;
	struct bz_content_update *dcu = bz_create_content_update_data();  dcu->is_sync = false;
	bz_list_append(parent->content_updates, scu1); scu1->surface = parent;
	bz_list_append(child->content_updates, scu2);  scu2->surface = child;
	bz_list_append(parent->content_updates, dcu);  dcu->surface = parent;
	bz_list_append(dcu->dependencies, scu1);       scu1->depended_on_by = dcu;
	bz_list_append(dcu->dependencies, scu2);       scu2->claimed_by = dcu;
	bz_list_append(child->content_updates, scu3);  scu3->surface = child;
	bz_list_append(scu3->dependencies, scu2);      scu2->depended_on_by = scu3;

	// Set up our mocks
	wl_resource_get_user_data_fake.return_val = child_sub;

	// Run our test
	bz_subsurface_implementation.set_desync(nullptr, nullptr);
	TEST_ASSERT_FALSE(child_sub->is_sync);
	TEST_ASSERT_EQUAL_INT(2, child->content_updates->length);
	struct bz_content_update *cu1 = child->content_updates->head->data;
	struct bz_content_update *cu2 = child->content_updates->head->next->data;
	TEST_ASSERT_EQUAL_PTR(scu2, cu1);
	TEST_ASSERT_EQUAL_PTR(scu3, cu2);
	TEST_ASSERT_TRUE(cu1->is_sync);
	TEST_ASSERT_FALSE(cu2->is_sync);

	// Clean up
	bz_free_content_update_data(scu1);
	bz_free_content_update_data(scu2);
	bz_free_content_update_data(scu3);
	bz_free_content_update_data(dcu);
	bz_free_subsurface_data(child_sub);
	bz_free_surface_data(child);
	bz_free_surface_data(parent);
}


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// Test bz_subsurface_destroy()
	RUN_TEST(test_subsurface_destroy__destroys_the_resource);

	// Test bz_subsurface_set_position()
	RUN_TEST(test_subsurface_set_position__is_buffered_on_parent_state);
	RUN_TEST(test_subsurface_set_position__negative_values_are_allowed);
	RUN_TEST(test_subsurface_set_position__multiple_calls_updates_prev_value);

	// Test bz_subsurface_place_above()
	RUN_TEST(test_subsurface_place_above__is_buffered_on_parent_state);
	RUN_TEST(test_subsurface_place_above__cannot_place_relative_to_self);
	RUN_TEST(test_subsurface_place_above__cannot_place_relative_to_unrelated_surface);
	RUN_TEST(test_subsurface_place_above__can_be_relative_to_sibling);
	RUN_TEST(test_subsurface_place_above__can_be_relative_to_parent);
	RUN_TEST(test_subsurface_place_above__multiple_calls_updates_prev_value);

	// Test bz_subsurface_place_below()
	RUN_TEST(test_subsurface_place_below__is_buffered_on_parent_state);
	RUN_TEST(test_subsurface_place_below__cannot_place_relative_to_self);
	RUN_TEST(test_subsurface_place_below__cannot_place_relative_to_unrelated_surface);
	RUN_TEST(test_subsurface_place_below__can_be_relative_to_sibling);
	RUN_TEST(test_subsurface_place_below__can_be_relative_to_parent);
	RUN_TEST(test_subsurface_place_below__multiple_calls_updates_prev_value);

	// Test bz_subsurface_set_sync()
	RUN_TEST(test_subsurface_set_sync__updates_the_subsurface_type);

	// Test bz_subsurface_set_desync()
	RUN_TEST(test_subsurface_set_desync__updates_the_subsurface_type);
	RUN_TEST(test_subsurface_set_desync__updates_unreachable_CUs_in_queue);

	return UNITY_END();
}
