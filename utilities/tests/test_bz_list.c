
#include <stdlib.h>

#include "unity.h"

#include "breezy/bz_list.h"
#include "breezy/bz_logger.h"


// =================================================================================================
//  Helper functions for some of our tests
// -------------------------------------------------------------------------------------------------

/** Returns true if the given item is less than the value. Item and value are int pointers. */
static bool is_less_than(void *item, void *value)
{
	int *i = item;
	int *v = value;
	return *i < *v;
}

/** Creates a copy of the given data, which is assumed to be an int pointer. */
static void *clone_int(void *data)
{
	int *d = data;
	int *copy = malloc(sizeof(*copy));
	*copy = *d;
	return copy;
}


// =================================================================================================
//  Set up / tear down / globals
// -------------------------------------------------------------------------------------------------

void setUp(void)
{
	bz_log_initialize(BZ_LOG_OFF);
}

void tearDown(void)
{
}


// =================================================================================================
//  Test bz_list_create()
// -------------------------------------------------------------------------------------------------

void test_list_create_returns_empty_list(void)
{
	struct bz_list *list = bz_list_create();
	TEST_ASSERT_NOT_NULL(list);
	TEST_ASSERT_NULL(list->head);
	TEST_ASSERT_NULL(list->tail);
	TEST_ASSERT_EQUAL_INT(0, list->length);
	bz_list_free(list, free);
}


// =================================================================================================
//  Test bz_list_append()
// -------------------------------------------------------------------------------------------------

void test_list_append_fails_for_uninitialized(void)
{
	const int actual = bz_list_append(nullptr, "test");
	TEST_ASSERT_EQUAL_INT(-1, actual);
}

void test_list_append_adds_to_end(void)
{
	struct bz_list *list = bz_list_create();
	char *one = "one";
	char *two = "two";
	if (bz_list_append(list, one) != 0) { TEST_FAIL(); }
	if (bz_list_append(list, two) != 0) { TEST_FAIL(); }

	// 2 items in the list
	TEST_ASSERT_NOT_NULL(list);
	TEST_ASSERT_EQUAL_INT(2, list->length);
	// First item is "one"
	TEST_ASSERT_NOT_NULL(list->head);
	TEST_ASSERT_EQUAL_PTR(list->head->data, one);
	// Last item is "two"
	TEST_ASSERT_NOT_NULL(list->tail);
	TEST_ASSERT_EQUAL_PTR(list->tail->data, two);
	// They point to each other
	TEST_ASSERT_EQUAL_PTR(list->head->next->data, two);
	TEST_ASSERT_EQUAL_PTR(list->tail->prev->data, one);

	bz_list_free(list, nullptr);
}


// =================================================================================================
//  Test bz_list_insert()
// -------------------------------------------------------------------------------------------------

void test_list_insert_fails_for_uninitialized(void)
{
	const int actual = bz_list_append(nullptr, "test");
	TEST_ASSERT_EQUAL_INT(-1, actual);
}

void test_list_insert_adds_to_beginning_with_nullptr(void)
{
	struct bz_list *list = bz_list_create();
	char *one = "one";
	char *two = "two";
	if (bz_list_insert(list, one, nullptr) != 0) { TEST_FAIL(); }
	if (bz_list_insert(list, two, nullptr) != 0) { TEST_FAIL(); }

	// 2 items in the list
	TEST_ASSERT_NOT_NULL(list);
	TEST_ASSERT_EQUAL_INT(2, list->length);
	// First item is "two"
	TEST_ASSERT_NOT_NULL(list->head);
	TEST_ASSERT_EQUAL_PTR(list->head->data, two);
	// Second item is "one"
	TEST_ASSERT_NOT_NULL(list->tail);
	TEST_ASSERT_EQUAL_PTR(list->tail->data, one);
	// They point to each other
	TEST_ASSERT_EQUAL_PTR(list->head->next->data, one);
	TEST_ASSERT_EQUAL_PTR(list->tail->prev->data, two);

	bz_list_free(list, nullptr);
}

void test_list_insert_adds_after_given_data(void)
{
	struct bz_list *list = bz_list_create();
	char *one = "one";
	char *two = "two";

	// Insert one at the beginning.
	if (bz_list_insert(list, one, nullptr) != 0) { TEST_FAIL(); }
	// Insert two after one.
	if (bz_list_insert(list, two, one) != 0) { TEST_FAIL(); }

	// 2 items in the list
	TEST_ASSERT_NOT_NULL(list);
	TEST_ASSERT_EQUAL_INT(2, list->length);
	// First item is "one"
	TEST_ASSERT_NOT_NULL(list->head);
	TEST_ASSERT_EQUAL_PTR(list->head->data, one);
	// Last item is "two"
	TEST_ASSERT_NOT_NULL(list->tail);
	TEST_ASSERT_EQUAL_PTR(list->tail->data, two);
	// They point to each other
	TEST_ASSERT_EQUAL_PTR(list->head->next->data, two);
	TEST_ASSERT_EQUAL_PTR(list->tail->prev->data, one);

	bz_list_free(list, nullptr);
}

void test_list_insert_fails_when_not_found(void)
{
	struct bz_list *list = bz_list_create();
	char *one = "one";
	char *two = "two";
	char *three = "three";

	if (bz_list_insert(list, one, nullptr) != 0) { TEST_FAIL(); }

	// Try to insert three after two, but two doesn't exist...
	int actual = bz_list_insert(list, three, two);

	// 1 item in the list. It's "one".
	TEST_ASSERT_NOT_NULL(list);
	TEST_ASSERT_EQUAL_INT(1, list->length);
	TEST_ASSERT_NOT_NULL(list->head);
	TEST_ASSERT_EQUAL_PTR(list->head->data, one);
	TEST_ASSERT_NOT_NULL(list->tail);
	TEST_ASSERT_EQUAL_PTR(list->tail->data, one);
	TEST_ASSERT_NULL(list->head->next);
	TEST_ASSERT_NULL(list->head->prev);

	// The returned error code indicates "could not find data"
	TEST_ASSERT_EQUAL_INT(-2, actual);

	bz_list_free(list, nullptr);
}


// =================================================================================================
//  Test bz_list_replace()
// -------------------------------------------------------------------------------------------------

void test_list_replace_fails_for_uninitialized(void)
{
	const int actual = bz_list_replace(nullptr, "old", "new", nullptr);
	TEST_ASSERT_EQUAL_INT(-1, actual);
}

void test_list_replace_fails_when_not_found(void)
{
	// Initialize our list
	struct bz_list *list = bz_list_create();
	char *one = "one";
	char *too = "too";
	char *two = "two";
	bz_list_append(list, one);

	// Try to replace a non-existent list item
	const int actual = bz_list_replace(list, too, two, nullptr);

	// Make sure we got the correct error code
	TEST_ASSERT_EQUAL_INT(-2, actual);
	// Make sure the rest of our list is unchanged
	TEST_ASSERT_NOT_NULL(list);
	TEST_ASSERT_EQUAL_INT(1, list->length);
	TEST_ASSERT_NOT_NULL(list->head);
	TEST_ASSERT_EQUAL_PTR(list->head->data, one);
	TEST_ASSERT_NOT_NULL(list->tail);
	TEST_ASSERT_EQUAL_PTR(list->tail->data, one);
	TEST_ASSERT_NULL(list->head->next);
	TEST_ASSERT_NULL(list->head->prev);

	bz_list_free(list, nullptr);
}

void test_list_replace_replaces_item(void)
{
	// Initialize our list
	struct bz_list *list = bz_list_create();
	char *one = "one";
	char *too = "too";
	char *two = "two";
	bz_list_append(list, one);
	bz_list_append(list, too);

	// Try to replace an existing list item
	const int actual = bz_list_replace(list, too, two, nullptr);

	// Make sure we got the correct response code
	TEST_ASSERT_EQUAL_INT(0, actual);
	// Make sure our list was properly updated
	TEST_ASSERT_NOT_NULL(list);
	TEST_ASSERT_EQUAL_INT(2, list->length);
	TEST_ASSERT_NOT_NULL(list->head);
	TEST_ASSERT_EQUAL_PTR(list->head->data, one);
	TEST_ASSERT_NOT_NULL(list->tail);
	TEST_ASSERT_EQUAL_PTR(list->tail->data, two);
	// ...and that our nodes properly point to each other
	TEST_ASSERT_EQUAL_PTR(list->head->next->data, two);
	TEST_ASSERT_EQUAL_PTR(list->tail->prev->data, one);

	bz_list_free(list, nullptr);
}

void test_list_replace_frees_replaced_item(void)
{
	// Initialize our list
	struct bz_list *list = bz_list_create();
	int *one = malloc(sizeof(*one));
	int *too = malloc(sizeof(*too));
	int *two = malloc(sizeof(*two));
	*one = 1;
	*too = 10;
	*two = 2;
	bz_list_append(list, one);
	bz_list_append(list, too);

	// Try to replace an existing list item
	const int actual = bz_list_replace(list, too, two, free);

	// Make sure we got the correct response code
	TEST_ASSERT_EQUAL_INT(0, actual);
	// Make sure our list was properly updated
	TEST_ASSERT_NOT_NULL(list);
	TEST_ASSERT_EQUAL_INT(2, list->length);
	TEST_ASSERT_NOT_NULL(list->head);
	TEST_ASSERT_EQUAL_PTR(list->head->data, one);
	TEST_ASSERT_NOT_NULL(list->tail);
	TEST_ASSERT_EQUAL_PTR(list->tail->data, two);
	// ...and that our nodes properly point to each other
	TEST_ASSERT_EQUAL_PTR(list->head->next->data, two);
	TEST_ASSERT_EQUAL_PTR(list->tail->prev->data, one);

	// Cleanup
	bz_list_free(list, nullptr);
	free(one);
	// free(too); // This test will fail if "too" is not freed automatically by bz_list_replace()
	free(two);
}

// =================================================================================================
//  Test bz_list_remove()
// -------------------------------------------------------------------------------------------------

void test_list_remove_fails_for_uninitialized(void)
{
	const int actual = bz_list_remove(nullptr, "one", nullptr);
	TEST_ASSERT_EQUAL_INT(-1, actual);
}

void test_list_remove_with_nonexistent_data(void)
{
	// Initialize our test data
	struct bz_list *list = bz_list_create();
	char *one = "one";
	char *two = "two";
	char *three = "three";

	// Empty list
	TEST_ASSERT_EQUAL_INT(0, bz_list_remove(list, three, nullptr));
	TEST_ASSERT_EQUAL_INT(0, list->length);

	// Just one item
	bz_list_append(list, one);
	TEST_ASSERT_EQUAL_INT(0, bz_list_remove(list, three, nullptr));
	TEST_ASSERT_EQUAL_INT(1, list->length);

	// Many items
	bz_list_append(list, two);
	TEST_ASSERT_EQUAL_INT(0, bz_list_remove(list, three, nullptr));
	TEST_ASSERT_EQUAL_INT(2, list->length);

	// Cleanup
	bz_list_free(list, nullptr);
}

void test_list_remove_with_valid_data(void)
{
	// Initialize our test list with MULTIPLE of the same item.
	struct bz_list *list = bz_list_create();
	char *one = "one";
	bz_list_append(list, one);
	bz_list_append(list, one);

	// We should only remove one item per call, even if multiple would match.
	TEST_ASSERT_EQUAL_INT(2, list->length);
	TEST_ASSERT_EQUAL_INT(1, bz_list_remove(list, one, nullptr));
	TEST_ASSERT_NULL(list->head->next);
	TEST_ASSERT_NULL(list->head->prev);
	TEST_ASSERT_EQUAL_INT(1, list->length);
	TEST_ASSERT_EQUAL_INT(1, bz_list_remove(list, one, nullptr));
	TEST_ASSERT_EQUAL_INT(0, list->length);

	// Cleanup
	bz_list_free(list, nullptr);
}

void test_list_remove_frees_removed_item(void)
{
	// Initialize our test list.
	struct bz_list *list = bz_list_create();
	int *one = malloc(sizeof(*one));
	*one = 1;
	bz_list_append(list, one);

	// Run our operation
	TEST_ASSERT_EQUAL_INT(1, list->length);
	TEST_ASSERT_EQUAL_PTR(one, list->head->data);
	TEST_ASSERT_EQUAL_INT(1, bz_list_remove(list, one, free));

	// Cleanup
	bz_list_free(list, nullptr);
	// free(one); // If "one" wasn't properly freed by bz_list_remove(), then this test will fail.
}


// =================================================================================================
//  Test bz_list_filter()
// -------------------------------------------------------------------------------------------------

void test_list_filter_fails_for_uninitialized(void)
{
	const int actual = bz_list_filter(nullptr, nullptr, is_less_than, nullptr);
	TEST_ASSERT_EQUAL_INT(-1, actual);
}

void test_list_filter_removes_proper_items(void)
{
	// Set up our test data
	struct bz_list *list = bz_list_create();
	int *one = malloc(sizeof(*one));
	int *two = malloc(sizeof(*two));
	int *three = malloc(sizeof(*three));
	int *four = malloc(sizeof(*four));
	int *threshold = malloc(sizeof(*threshold));
	*one = 1;
	*two = 2;
	*three = 3;
	*four = 4;
	*threshold = 3;
	bz_list_append(list, one);
	bz_list_append(list, three);
	bz_list_append(list, two);
	bz_list_append(list, four);
	TEST_ASSERT_EQUAL_INT(4, list->length);

	// Run our filter. It should remove "three" and "four" since they're not less than our threshold
	const int two_items_removed = bz_list_filter(list, threshold, is_less_than, nullptr);
	TEST_ASSERT_EQUAL_INT(2, two_items_removed);
	TEST_ASSERT_EQUAL_INT(2, list->length);
	TEST_ASSERT_EQUAL_PTR(one, list->head->data);
	TEST_ASSERT_EQUAL_PTR(two, list->tail->data);
	// Our remaining nodes should properly point to each other
	TEST_ASSERT_EQUAL_PTR(list->head->next, list->tail);
	TEST_ASSERT_EQUAL_PTR(list->tail->prev, list->head);

	// Run our filter again - nothing should be removed this time, and 0 should be returned.
	const int zero_items_removed = bz_list_filter(list, threshold, is_less_than, nullptr);
	TEST_ASSERT_EQUAL_INT(0, zero_items_removed);
	TEST_ASSERT_EQUAL_INT(2, list->length);
	TEST_ASSERT_EQUAL_PTR(one, list->head->data);
	TEST_ASSERT_EQUAL_PTR(two, list->tail->data);
	// Our remaining nodes should properly point to each other
	TEST_ASSERT_EQUAL_PTR(list->head->next, list->tail);
	TEST_ASSERT_EQUAL_PTR(list->tail->prev, list->head);

	// Cleanup
	bz_list_free(list, nullptr);
	free(one);
	free(two);
	free(three);
	free(four);
	free(threshold);
}

void test_list_filter_frees_removed_items(void)
{
	// Set up our test data
	struct bz_list *list = bz_list_create();
	int *one = malloc(sizeof(*one));
	int *two = malloc(sizeof(*two));
	int *three = malloc(sizeof(*three));
	int *threshold = malloc(sizeof(*threshold));
	*one = 1;
	*two = 2;
	*three = 3;
	*threshold = 3;
	bz_list_append(list, one);
	bz_list_append(list, three);
	bz_list_append(list, two);
	TEST_ASSERT_EQUAL_INT(3, list->length);

	// Run our filter - it should call "free" on "three"
	const int items_removed = bz_list_filter(list, threshold, is_less_than, free);
	TEST_ASSERT_EQUAL_INT(1, items_removed);
	TEST_ASSERT_EQUAL_INT(2, list->length);
	TEST_ASSERT_EQUAL_PTR(one, list->head->data);
	TEST_ASSERT_EQUAL_PTR(two, list->tail->data);

	// Cleanup
	bz_list_free(list, nullptr);
	free(one);
	free(two);
	// free(three); // This test should fail if "free" isn't called by bz_list_filter() above.
	free(threshold);
}


// =================================================================================================
//  Test bz_list_clear()
// -------------------------------------------------------------------------------------------------

void test_list_clear_does_nothing_for_uninitialized(void)
{
	// Just to make sure we don't segfault or something.
	bz_list_clear(nullptr, nullptr);
}

void test_list_clear_removes_all_items(void)
{
	// Set up our test data
	struct bz_list *list = bz_list_create();
	bz_list_append(list, "one");
	bz_list_append(list, "two");
	bz_list_append(list, "three");
	TEST_ASSERT_EQUAL_INT(3, list->length);
	TEST_ASSERT_NOT_NULL(list->head);
	TEST_ASSERT_NOT_NULL(list->tail);

	// Call clear, and assert on changes.
	bz_list_clear(list, nullptr);
	TEST_ASSERT_EQUAL_INT(0, list->length);
	TEST_ASSERT_NULL(list->head);
	TEST_ASSERT_NULL(list->tail);

	// Cleanup
	bz_list_free(list, nullptr);
}

void test_list_clear_frees_removed_items(void)
{
	// Set up our test data
	struct bz_list *list = bz_list_create();
	int *one = malloc(sizeof(*one));
	int *two = malloc(sizeof(*two));
	*one = 1;
	*two = 2;
	bz_list_append(list, one);
	bz_list_append(list, two);
	TEST_ASSERT_EQUAL_INT(2, list->length);
	TEST_ASSERT_NOT_NULL(list->head);
	TEST_ASSERT_NOT_NULL(list->tail);

	// Call clear, and assert on changes.
	bz_list_clear(list, free);
	TEST_ASSERT_EQUAL_INT(0, list->length);
	TEST_ASSERT_NULL(list->head);
	TEST_ASSERT_NULL(list->tail);

	// Cleanup
	bz_list_free(list, nullptr);
	// free(one) and free(two) should already be called by the above list. The test will fail if
	// they were not.
}


// =================================================================================================
//  Test bz_list_free()
// -------------------------------------------------------------------------------------------------

void test_list_free_does_nothing_for_uninitialized(void)
{
	// Just to make sure we don't segfault or something.
	bz_list_free(nullptr, nullptr);
}

void test_list_free_frees_removed_items(void)
{
	// Set up our test data
	struct bz_list *list = bz_list_create();
	int *one = malloc(sizeof(*one));
	int *two = malloc(sizeof(*two));
	*one = 1;
	*two = 2;
	bz_list_append(list, one);
	bz_list_append(list, two);
	TEST_ASSERT_EQUAL_INT(2, list->length);
	TEST_ASSERT_NOT_NULL(list->head);
	TEST_ASSERT_NOT_NULL(list->tail);

	// Call free, and assert on changes.
	bz_list_free(list, free);
	// free(one) and free(two) should already be called by the above list. The test will fail if
	// they were not.
}


// =================================================================================================
//  Test bz_list_find()
// -------------------------------------------------------------------------------------------------

void test_list_find_returns_null_for_uninitialized(void)
{
	int *one = malloc(sizeof(*one));
	*one = 1;
	const void *actual_retval = bz_list_find(nullptr, one, is_less_than);
	TEST_ASSERT_NULL(actual_retval);
	free(one);
}

void test_list_find_returns_null_for_not_found(void)
{
	// Create our test data
	int *one = malloc(sizeof(*one));
	int *two = malloc(sizeof(*two));
	*one = 1;
	*two = 2;
	struct bz_list *list = bz_list_create();
	bz_list_append(list, one);
	bz_list_append(list, two);
	TEST_ASSERT_EQUAL_INT(2, list->length);

	// Run our test
	const void *actual_retval = bz_list_find(list, one, is_less_than);
	TEST_ASSERT_NULL(actual_retval);

	// Cleanup
	bz_list_free(list, nullptr);
	free(one);
	free(two);
}

void test_list_find_returns_first_matching_data(void)
{
	// Create our test data
	int *one = malloc(sizeof(*one));
	int *two = malloc(sizeof(*two));
	int *three = malloc(sizeof(*three));
	*one = 1;
	*two = 2;
	*three = 3;
	struct bz_list *list = bz_list_create();
	bz_list_append(list, two);
	bz_list_append(list, one);
	bz_list_append(list, three);
	TEST_ASSERT_EQUAL_INT(3, list->length);

	// Run our test -- first item matches
	const void *actual_first = bz_list_find(list, three, is_less_than);
	TEST_ASSERT_NOT_NULL(actual_first);
	TEST_ASSERT_EQUAL_PTR(two, actual_first);

	// Run our test -- second item matches
	const void *actual_second = bz_list_find(list, two, is_less_than);
	TEST_ASSERT_NOT_NULL(actual_second);
	TEST_ASSERT_EQUAL_PTR(one, actual_second);

	// Cleanup
	bz_list_free(list, nullptr);
	free(one);
	free(two);
	free(three);
}


// =================================================================================================
//  Test bz_list_contains()
// -------------------------------------------------------------------------------------------------

void test_list_contains__returns_false_for_uninitialized(void)
{
	int one = 1;
	TEST_ASSERT_FALSE(bz_list_contains(nullptr, &one));
}

void test_list_contains__returns_false_for_not_found(void)
{
	// Create our test data
	int one = 1;
	int two = 2;
	struct bz_list *list = bz_list_create();
	bz_list_append(list, &one);
	TEST_ASSERT_EQUAL_INT(1, list->length);

	// Run our test
	TEST_ASSERT_FALSE(bz_list_contains(list, &two));

	// Cleanup
	bz_list_free(list, nullptr);
}

void test_list_contains__returns_true_for_found(void)
{
	// Create our test data
	int one = 1;
	int two = 2;
	struct bz_list *list = bz_list_create();
	bz_list_append(list, &one);
	bz_list_append(list, &two);
	TEST_ASSERT_EQUAL_INT(2, list->length);

	// Run our tests
	TEST_ASSERT_TRUE(bz_list_contains(list, &one));
	TEST_ASSERT_TRUE(bz_list_contains(list, &two));

	// Cleanup
	bz_list_free(list, nullptr);
}


// =================================================================================================
//  Test bz_list_get_neighbor()
// -------------------------------------------------------------------------------------------------

void test_list_get_neighbor_returns_null_for_uninitialized(void)
{
	char *one = "one";
	const void *retval = bz_list_get_neighbor(nullptr, one);
	TEST_ASSERT_NULL(retval);
}

void test_list_get_neighbor_returns_null_for_not_found(void)
{
	// Set up initial data
	int *one = malloc(sizeof(*one));
	int *two = malloc(sizeof(*two));
	*one = 1;
	*two = 2;
	struct bz_list *list = bz_list_create();
	bz_list_append(list, one);
	TEST_ASSERT_EQUAL_INT(1, list->length);

	// Run our test
	const void *retval = bz_list_get_neighbor(list, two);
	TEST_ASSERT_NULL(retval);

	// Cleanup
	bz_list_free(list, nullptr);
	free(one);
	free(two);
}

void test_list_get_neighbor_returns_null_for_no_neighbor(void)
{
	// Set up initial data
	int *one = malloc(sizeof(*one));
	*one = 1;
	struct bz_list *list = bz_list_create();
	bz_list_append(list, one);
	TEST_ASSERT_EQUAL_INT(1, list->length);

	// Run our test
	const void *retval = bz_list_get_neighbor(list, one);
	TEST_ASSERT_NULL(retval);

	// Cleanup
	bz_list_free(list, nullptr);
	free(one);
}

void test_list_get_neighbor_returns_prior_item_when_able(void)
{
	// Set up initial data
	int *one = malloc(sizeof(*one));
	int *two = malloc(sizeof(*two));
	*one = 1;
	*two = 2;
	struct bz_list *list = bz_list_create();
	bz_list_append(list, one);
	bz_list_append(list, two);
	TEST_ASSERT_EQUAL_INT(2, list->length);

	// Run our test
	const void *retval = bz_list_get_neighbor(list, two);
	TEST_ASSERT_NOT_NULL(retval);
	TEST_ASSERT_EQUAL_PTR(one, retval);

	// Cleanup
	bz_list_free(list, nullptr);
	free(one);
	free(two);
}

void test_list_get_neighbor_returns_next_item_when_needed(void)
{
	// Set up initial data
	int *one = malloc(sizeof(*one));
	int *two = malloc(sizeof(*two));
	*one = 1;
	*two = 2;
	struct bz_list *list = bz_list_create();
	bz_list_append(list, one);
	bz_list_append(list, two);
	TEST_ASSERT_EQUAL_INT(2, list->length);

	// Run our test
	const void *retval = bz_list_get_neighbor(list, one);
	TEST_ASSERT_NOT_NULL(retval);
	TEST_ASSERT_EQUAL_PTR(two, retval);

	// Cleanup
	bz_list_free(list, nullptr);
	free(one);
	free(two);
}


// =================================================================================================
//  Test bz_list_move_to_end()
// -------------------------------------------------------------------------------------------------

void test_list_move_to_end__fails_for_uninitialized(void)
{
	// Set up initial data
	struct bz_list *src = bz_list_create();
	struct bz_list *dest = bz_list_create();

	// Run our test
	TEST_ASSERT_EQUAL_INT(-1, bz_list_move_to_end(nullptr, src));
	TEST_ASSERT_EQUAL_INT(-1, bz_list_move_to_end(dest, nullptr));
	TEST_ASSERT_EQUAL_INT(-1, bz_list_move_to_end(nullptr, nullptr));

	// Cleanup
	bz_list_free(src, nullptr);
	bz_list_free(dest, nullptr);
}

void test_list_move_to_end__empty_src_list(void)
{
	// Create our lists...
	struct bz_list *src = bz_list_create();
	struct bz_list *dest = bz_list_create();
	// ...and add some initial values
	int dest_val = 1; bz_list_append(dest, &dest_val);

	// Run our test
	int ret_val = bz_list_move_to_end(dest, src);
	TEST_ASSERT_EQUAL_INT(0, ret_val);
	TEST_ASSERT_EQUAL_INT(0, src->length);
	TEST_ASSERT_EQUAL_INT(1, dest->length);
	TEST_ASSERT_EQUAL_INT(&dest_val, dest->head->data);

	// Cleanup
	bz_list_free(src, nullptr);
	bz_list_free(dest, nullptr);
}

void test_list_move_to_end__empty_dest_list(void)
{
	// Create our lists...
	struct bz_list *src = bz_list_create();
	struct bz_list *dest = bz_list_create();
	// ...and add some initial values
	int src_val = 1; bz_list_append(src, &src_val);

	// Run our test
	int ret_val = bz_list_move_to_end(dest, src);
	TEST_ASSERT_EQUAL_INT(1, ret_val);
	TEST_ASSERT_EQUAL_INT(0, src->length);
	TEST_ASSERT_EQUAL_INT(1, dest->length);
	TEST_ASSERT_EQUAL_INT(&src_val, dest->head->data);

	// Cleanup
	bz_list_free(src, nullptr);
	bz_list_free(dest, nullptr);
}

void test_list_move_to_end__both_lists_populated(void)
{
	// Create our lists...
	struct bz_list *src = bz_list_create();
	struct bz_list *dest = bz_list_create();
	// ...and add some initial values
	int src_val_1  = 1; bz_list_append(src, &src_val_1);
	int src_val_2  = 2; bz_list_append(src, &src_val_2);
	int dest_val_1 = 3; bz_list_append(dest, &dest_val_1);
	int dest_val_2 = 4; bz_list_append(dest, &dest_val_2);

	// Run our test
	int ret_val = bz_list_move_to_end(dest, src);
	TEST_ASSERT_EQUAL_INT(2, ret_val);
	TEST_ASSERT_EQUAL_INT(0, src->length);
	TEST_ASSERT_EQUAL_INT(4, dest->length);
	TEST_ASSERT_EQUAL_INT(&dest_val_1, dest->head->data);
	TEST_ASSERT_EQUAL_INT(&dest_val_2, dest->head->next->data);
	TEST_ASSERT_EQUAL_INT(&src_val_1,  dest->head->next->next->data);
	TEST_ASSERT_EQUAL_INT(&src_val_2,  dest->head->next->next->next->data);
	// Also check the "prev" pointer at the mesh point
	TEST_ASSERT_EQUAL_PTR(&dest_val_2, dest->tail->prev->prev->data);

	// Cleanup
	bz_list_free(src, nullptr);
	bz_list_free(dest, nullptr);
}


// =================================================================================================
//  Test bz_list_move_item_to_end()
// -------------------------------------------------------------------------------------------------

void test_list_move_item_to_end__returns_neg1_when_uninitialized(void)
{
	TEST_ASSERT_EQUAL_INT(-1, bz_list_move_item_to_end(nullptr, nullptr));
}

void test_list_move_item_to_end__returns_0_already_last(void)
{
	// Create our list
	struct bz_list *list = bz_list_create();
	int val1 = 1; bz_list_append(list, &val1);
	int val2 = 2; bz_list_append(list, &val2);
	int val3 = 3; bz_list_append(list, &val3);
	int val4 = 4; bz_list_append(list, &val4);

	// Run our test
	int ret_val = bz_list_move_item_to_end(list, &val4);
	TEST_ASSERT_EQUAL_INT(0, ret_val);
	TEST_ASSERT_EQUAL_INT(4, list->length);
	TEST_ASSERT_EQUAL_INT(&val4, list->tail->data);

	// Cleanup
	bz_list_free(list, nullptr);
}

void test_list_move_item_to_end__returns_1_when_moved_weird_case(void)
{
	// Create our list
	struct bz_list *list = bz_list_create();
	int val1 = 1; bz_list_append(list, &val1);
	int val2 = 2; bz_list_append(list, &val2);

	// Run our test
	int ret_val = bz_list_move_item_to_end(list, &val1);
	TEST_ASSERT_EQUAL_INT(1, ret_val);
	TEST_ASSERT_EQUAL_INT(2, list->length);
	TEST_ASSERT_EQUAL_INT(&val1, list->tail->data);
	// Check our list order
	TEST_ASSERT_EQUAL_INT(&val2, list->head->data);
	TEST_ASSERT_EQUAL_INT(&val1, list->head->next->data);

	// Cleanup
	bz_list_free(list, nullptr);
}

void test_list_move_item_to_end__returns_1_when_moved(void)
{
	// Create our list
	struct bz_list *list = bz_list_create();
	int val1 = 1; bz_list_append(list, &val1);
	int val2 = 2; bz_list_append(list, &val2);
	int val3 = 3; bz_list_append(list, &val3);
	int val4 = 4; bz_list_append(list, &val4);

	// Run our test
	int ret_val = bz_list_move_item_to_end(list, &val3);
	TEST_ASSERT_EQUAL_INT(1, ret_val);
	TEST_ASSERT_EQUAL_INT(4, list->length);
	TEST_ASSERT_EQUAL_INT(&val3, list->tail->data);
	// Check our list order
	TEST_ASSERT_EQUAL_INT(&val1, list->head->data);
	TEST_ASSERT_EQUAL_INT(&val2, list->head->next->data);
	TEST_ASSERT_EQUAL_INT(&val4, list->head->next->next->data);
	TEST_ASSERT_EQUAL_INT(&val3, list->head->next->next->next->data);

	// Cleanup
	bz_list_free(list, nullptr);
}

void test_list_move_item_to_end__returns_neg2_when_not_found(void)
{
	// Create our list
	struct bz_list *list = bz_list_create();
	int val1 = 1; bz_list_append(list, &val1);
	int val2 = 2; bz_list_append(list, &val2);
	int val3 = 3; bz_list_append(list, &val3);
	int val4 = 4; bz_list_append(list, &val4);
	int val5 = 5; // Not in the list

	// Run our test
	int ret_val = bz_list_move_item_to_end(list, &val5);
	TEST_ASSERT_EQUAL_INT(-2, ret_val);
	TEST_ASSERT_EQUAL_INT(4, list->length);
	TEST_ASSERT_EQUAL_INT(&val4, list->tail->data);
	// Check our list order
	TEST_ASSERT_EQUAL_INT(&val1, list->head->data);
	TEST_ASSERT_EQUAL_INT(&val2, list->head->next->data);
	TEST_ASSERT_EQUAL_INT(&val3, list->head->next->next->data);
	TEST_ASSERT_EQUAL_INT(&val4, list->head->next->next->next->data);

	// Cleanup
	bz_list_free(list, nullptr);
}

void test_list_move_item_to_end__returns_neg2_when_list_empty(void)
{
	// Create our list
	struct bz_list *list = bz_list_create();
	int val = 1;

	// Run our test
	int ret_val = bz_list_move_item_to_end(list, &val);
	TEST_ASSERT_EQUAL_INT(-2, ret_val);
	TEST_ASSERT_EQUAL_INT(0, list->length);

	// Cleanup
	bz_list_free(list, nullptr);
}


// =================================================================================================
//  Test bz_list_clone()
// -------------------------------------------------------------------------------------------------

void test_list_clone_returns_null_for_uninitialized(void)
{
	const void *retval = bz_list_clone(nullptr, clone_int);
	TEST_ASSERT_NULL(retval);
}

void test_list_clone_returns_null_for_null_clone_fn(void)
{
	struct bz_list *list = bz_list_create();
	const void *retval = bz_list_clone(list, nullptr);
	TEST_ASSERT_NULL(retval);
	bz_list_free(list, nullptr);
}

void test_list_clone_returns_different_list(void)
{
	struct bz_list *list = bz_list_create();
	struct bz_list *clone = bz_list_clone(list, clone_int);
	TEST_ASSERT_NOT_NULL(clone);
	TEST_ASSERT(clone != list);
	bz_list_free(list, nullptr);
	bz_list_free(clone, nullptr);
}

void test_list_clone_returns_different_but_equal_values(void)
{
	// Create our initial data
	int *one = malloc(sizeof(*one));
	int *two = malloc(sizeof(*two));
	*one = 1;
	*two = 2;
	struct bz_list *list = bz_list_create();
	bz_list_append(list, one);
	bz_list_append(list, two);
	TEST_ASSERT_EQUAL_INT(2, list->length);

	// Create a clone
	struct bz_list *clone = bz_list_clone(list, clone_int);
	TEST_ASSERT_NOT_NULL(clone);

	// It should have the same values, but different references
	TEST_ASSERT_EQUAL_INT(2, clone->length);
	TEST_ASSERT(clone->head->data != list->head->data);
	TEST_ASSERT(clone->tail->data != list->tail->data);
	const int *one_clone = clone->head->data;
	const int *two_clone = clone->tail->data;
	TEST_ASSERT_EQUAL_INT(*one, *one_clone);
	TEST_ASSERT_EQUAL_INT(*two, *two_clone);

	// Cleanup
	bz_list_free(list, free);
	bz_list_free(clone, free);
}


// =================================================================================================
//  Test bz_list_foreach()
// -------------------------------------------------------------------------------------------------

void test_list_foreach__basic_iteration(void)
{
	// Create our list
	struct bz_list *list = bz_list_create();
	int val1 = 1; bz_list_append(list, &val1);
	int val2 = 2; bz_list_append(list, &val2);
	int val3 = 3; bz_list_append(list, &val3);

	// Run the test
	int counter = 1;
	int *val; bz_list_foreach(val, list) {
		TEST_ASSERT_EQUAL_INT(counter, *val);
		counter++;
	}
	// Make sure we iterated all 3 times.
	TEST_ASSERT_EQUAL_INT(4, counter);

	// Cleanup
	bz_list_free(list, nullptr);
}

void test_list_foreach__early_exit_with_break(void)
{
	// Create our list
	struct bz_list *list = bz_list_create();
	int val1 = 1; bz_list_append(list, &val1);
	int val2 = 2; bz_list_append(list, &val2);
	int val3 = 3; bz_list_append(list, &val3);

	// Run the test
	int counter = 1;
	int *val; bz_list_foreach(val, list) {
		TEST_ASSERT_EQUAL_INT(counter, *val);
		if (counter == 2) {
			break;
		}
		counter++;
	}
	// Make sure we didn't make it to the third iteration.
	TEST_ASSERT_EQUAL_INT(2, counter);

	// Cleanup
	bz_list_free(list, nullptr);
}

void test_list_foreach__early_iteration_with_continue(void)
{
	// Create our list
	struct bz_list *list = bz_list_create();
	int val1 = 1; bz_list_append(list, &val1);
	int val2 = 2; bz_list_append(list, &val2);
	int val3 = 3; bz_list_append(list, &val3);

	// Run the test
	int counter = 1;
	int complete_iterations = 0;
	int *val; bz_list_foreach(val, list) {
		TEST_ASSERT_EQUAL_INT(counter, *val);
		counter++;
		if (counter == 2) {
			continue;
		}
		complete_iterations++;
	}
	// Make sure we iterated enough times
	TEST_ASSERT_EQUAL_INT(4, counter);
	// Make sure we hit the "continue" statement once.
	TEST_ASSERT_EQUAL_INT(2, complete_iterations);

	// Cleanup
	bz_list_free(list, nullptr);
}

void test_list_foreach__nested_loops(void)
{
	// Create our lists
	struct bz_list *list1 = bz_list_create();
	int val1 = 1; bz_list_append(list1, &val1);
	int val2 = 2; bz_list_append(list1, &val2);
	int val3 = 3; bz_list_append(list1, &val3);
	struct bz_list *list2 = bz_list_create();
	int val4 = 4; bz_list_append(list2, &val4);
	int val5 = 5; bz_list_append(list2, &val5);
	int val6 = 6; bz_list_append(list2, &val6);

	// Run the test
	int out_count = 1;
	int in_count = 4;
	int total_count = 0;
	int *outer; bz_list_foreach(outer, list1) {
		in_count = 4;
		int *inner; bz_list_foreach(inner, list2) {
			TEST_ASSERT_EQUAL_INT(out_count, *outer);
			TEST_ASSERT_EQUAL_INT(in_count, *inner);
			in_count++;
			total_count++;
		}
		out_count++;
	}
	// Make sure we iterated enough times.
	TEST_ASSERT_EQUAL_INT(9, total_count);

	// Cleanup
	bz_list_free(list1, nullptr);
	bz_list_free(list2, nullptr);
}


// =================================================================================================
//  Test bz_list_foreach_rev()
// -------------------------------------------------------------------------------------------------

void test_list_foreach_rev__basic_iteration(void)
{
	// Create our list
	struct bz_list *list = bz_list_create();
	int val1 = 1; bz_list_append(list, &val1);
	int val2 = 2; bz_list_append(list, &val2);
	int val3 = 3; bz_list_append(list, &val3);

	// Run the test
	int counter = 3;
	int *val; bz_list_foreach_rev(val, list) {
		TEST_ASSERT_EQUAL_INT(counter, *val);
		counter--;
	}
	// Make sure we iterated all 3 times.
	TEST_ASSERT_EQUAL_INT(0, counter);

	// Cleanup
	bz_list_free(list, nullptr);
}

void test_list_foreach_rev__early_exit_with_break(void)
{
	// Create our list
	struct bz_list *list = bz_list_create();
	int val1 = 1; bz_list_append(list, &val1);
	int val2 = 2; bz_list_append(list, &val2);
	int val3 = 3; bz_list_append(list, &val3);

	// Run the test
	int counter = 3;
	int *val; bz_list_foreach_rev(val, list) {
		TEST_ASSERT_EQUAL_INT(counter, *val);
		if (counter == 2) {
			break;
		}
		counter--;
	}
	// Make sure we didn't make it to the third iteration.
	TEST_ASSERT_EQUAL_INT(2, counter);

	// Cleanup
	bz_list_free(list, nullptr);
}

void test_list_foreach_rev__early_iteration_with_continue(void)
{
	// Create our list
	struct bz_list *list = bz_list_create();
	int val1 = 1; bz_list_append(list, &val1);
	int val2 = 2; bz_list_append(list, &val2);
	int val3 = 3; bz_list_append(list, &val3);

	// Run the test
	int counter = 3;
	int complete_iterations = 0;
	int *val; bz_list_foreach_rev(val, list) {
		TEST_ASSERT_EQUAL_INT(counter, *val);
		counter--;
		if (counter == 2) {
			continue;
		}
		complete_iterations++;
	}
	// Make sure we iterated enough times
	TEST_ASSERT_EQUAL_INT(0, counter);
	// Make sure we hit the "continue" statement once.
	TEST_ASSERT_EQUAL_INT(2, complete_iterations);

	// Cleanup
	bz_list_free(list, nullptr);
}

void test_list_foreach_rev__nested_loops(void)
{
	// Create our lists
	struct bz_list *list1 = bz_list_create();
	int val1 = 1; bz_list_append(list1, &val1);
	int val2 = 2; bz_list_append(list1, &val2);
	int val3 = 3; bz_list_append(list1, &val3);
	struct bz_list *list2 = bz_list_create();
	int val4 = 4; bz_list_append(list2, &val4);
	int val5 = 5; bz_list_append(list2, &val5);
	int val6 = 6; bz_list_append(list2, &val6);

	// Run the test
	int out_count = 3;
	int in_count = 6;
	int total_count = 0;
	int *outer; bz_list_foreach_rev(outer, list1) {
		in_count = 6;
		int *inner; bz_list_foreach_rev(inner, list2) {
			TEST_ASSERT_EQUAL_INT(out_count, *outer);
			TEST_ASSERT_EQUAL_INT(in_count, *inner);
			in_count--;
			total_count++;
		}
		out_count--;
	}
	// Make sure we iterated enough times.
	TEST_ASSERT_EQUAL_INT(9, total_count);

	// Cleanup
	bz_list_free(list1, nullptr);
	bz_list_free(list2, nullptr);
}


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// Test bz_list_create()
	RUN_TEST(test_list_create_returns_empty_list);

	// Test bz_list_append()
	RUN_TEST(test_list_append_fails_for_uninitialized);
	RUN_TEST(test_list_append_adds_to_end);

	// Test bz_list_insert()
	RUN_TEST(test_list_insert_fails_for_uninitialized);
	RUN_TEST(test_list_insert_adds_to_beginning_with_nullptr);
	RUN_TEST(test_list_insert_adds_after_given_data);
	RUN_TEST(test_list_insert_fails_when_not_found);

	// Test bz_list_replace()
	RUN_TEST(test_list_replace_fails_for_uninitialized);
	RUN_TEST(test_list_replace_fails_when_not_found);
	RUN_TEST(test_list_replace_replaces_item);
	RUN_TEST(test_list_replace_frees_replaced_item);

	// Test bz_list_remove()
	RUN_TEST(test_list_remove_fails_for_uninitialized);
	RUN_TEST(test_list_remove_with_nonexistent_data);
	RUN_TEST(test_list_remove_with_valid_data);
	RUN_TEST(test_list_remove_frees_removed_item);

	// Test bz_list_filter()
	RUN_TEST(test_list_filter_fails_for_uninitialized);
	RUN_TEST(test_list_filter_removes_proper_items);
	RUN_TEST(test_list_filter_frees_removed_items);

	// Test bz_list_clear()
	RUN_TEST(test_list_clear_does_nothing_for_uninitialized);
	RUN_TEST(test_list_clear_removes_all_items);
	RUN_TEST(test_list_clear_frees_removed_items);

	// Test bz_list_free()
	RUN_TEST(test_list_free_does_nothing_for_uninitialized);
	RUN_TEST(test_list_free_frees_removed_items);

	// Test bz_list_find()
	RUN_TEST(test_list_find_returns_null_for_uninitialized);
	RUN_TEST(test_list_find_returns_null_for_not_found);
	RUN_TEST(test_list_find_returns_first_matching_data);

	// Test bz_list_contains()
	RUN_TEST(test_list_contains__returns_false_for_uninitialized);
	RUN_TEST(test_list_contains__returns_false_for_not_found);
	RUN_TEST(test_list_contains__returns_true_for_found);

	// Test bz_list_get_neighbor()
	RUN_TEST(test_list_get_neighbor_returns_null_for_uninitialized);
	RUN_TEST(test_list_get_neighbor_returns_null_for_not_found);
	RUN_TEST(test_list_get_neighbor_returns_null_for_no_neighbor);
	RUN_TEST(test_list_get_neighbor_returns_prior_item_when_able);
	RUN_TEST(test_list_get_neighbor_returns_next_item_when_needed);

	// Test bz_list_move_to_end()
	RUN_TEST(test_list_move_to_end__fails_for_uninitialized);
	RUN_TEST(test_list_move_to_end__empty_src_list);
	RUN_TEST(test_list_move_to_end__empty_dest_list);
	RUN_TEST(test_list_move_to_end__both_lists_populated);

	// Test bz_list_move_item_to_end()
	RUN_TEST(test_list_move_item_to_end__returns_neg1_when_uninitialized);
	RUN_TEST(test_list_move_item_to_end__returns_0_already_last);
	RUN_TEST(test_list_move_item_to_end__returns_1_when_moved_weird_case);
	RUN_TEST(test_list_move_item_to_end__returns_1_when_moved);
	RUN_TEST(test_list_move_item_to_end__returns_neg2_when_not_found);
	RUN_TEST(test_list_move_item_to_end__returns_neg2_when_list_empty);

	// Test bz_list_clone()
	RUN_TEST(test_list_clone_returns_null_for_uninitialized);
	RUN_TEST(test_list_clone_returns_null_for_null_clone_fn);
	RUN_TEST(test_list_clone_returns_different_list);
	RUN_TEST(test_list_clone_returns_different_but_equal_values);

	// Test bz_list_foreach() macro
	RUN_TEST(test_list_foreach__basic_iteration);
	RUN_TEST(test_list_foreach__early_exit_with_break);
	RUN_TEST(test_list_foreach__early_iteration_with_continue);
	RUN_TEST(test_list_foreach__nested_loops);

	// Test bz_list_foreach_rev() macro
	RUN_TEST(test_list_foreach_rev__basic_iteration);
	RUN_TEST(test_list_foreach_rev__early_exit_with_break);
	RUN_TEST(test_list_foreach_rev__early_iteration_with_continue);
	RUN_TEST(test_list_foreach_rev__nested_loops);

	return UNITY_END();
}
