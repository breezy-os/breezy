
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

	// Try to replace a non-existent list item
	const int actual = bz_list_replace(list, too, two, nullptr);

	// Make sure we got the correct error code
	TEST_ASSERT_EQUAL_INT(0, actual);
	// Make sure the rest of our list is unchanged
	TEST_ASSERT_NOT_NULL(list);
	TEST_ASSERT_EQUAL_INT(2, list->length);
	TEST_ASSERT_NOT_NULL(list->head);
	TEST_ASSERT_EQUAL_PTR(list->head->data, one);
	TEST_ASSERT_NOT_NULL(list->tail);
	TEST_ASSERT_EQUAL_PTR(list->tail->data, two);

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

	// Try to replace a non-existent list item
	const int actual = bz_list_replace(list, too, two, free);

	// Make sure we got the correct error code
	TEST_ASSERT_EQUAL_INT(0, actual);
	// Make sure the rest of our list is unchanged
	TEST_ASSERT_NOT_NULL(list);
	TEST_ASSERT_EQUAL_INT(2, list->length);
	TEST_ASSERT_NOT_NULL(list->head);
	TEST_ASSERT_EQUAL_PTR(list->head->data, one);
	TEST_ASSERT_NOT_NULL(list->tail);
	TEST_ASSERT_EQUAL_PTR(list->tail->data, two);

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

	// Run our filter again - nothing should be removed this time, and 0 should be returned.
	const int zero_items_removed = bz_list_filter(list, threshold, is_less_than, nullptr);
	TEST_ASSERT_EQUAL_INT(0, zero_items_removed);
	TEST_ASSERT_EQUAL_INT(2, list->length);
	TEST_ASSERT_EQUAL_PTR(one, list->head->data);
	TEST_ASSERT_EQUAL_PTR(two, list->tail->data);

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

	// Test bz_list_get_neighbor()
	RUN_TEST(test_list_get_neighbor_returns_null_for_uninitialized);
	RUN_TEST(test_list_get_neighbor_returns_null_for_not_found);
	RUN_TEST(test_list_get_neighbor_returns_null_for_no_neighbor);
	RUN_TEST(test_list_get_neighbor_returns_prior_item_when_able);
	RUN_TEST(test_list_get_neighbor_returns_next_item_when_needed);

	// Test bz_list_clone()
	RUN_TEST(test_list_clone_returns_null_for_uninitialized);
	RUN_TEST(test_list_clone_returns_null_for_null_clone_fn);
	RUN_TEST(test_list_clone_returns_different_list);
	RUN_TEST(test_list_clone_returns_different_but_equal_values);

	return UNITY_END();
}
