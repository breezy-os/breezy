
#include <stdio.h>
#include <stdlib.h>

#include "unity.h"

#include "breezy/bz_logger.h"
#include "breezy/bz_math.h"


// =================================================================================================
//  Set up / tear down / globals
// -------------------------------------------------------------------------------------------------

void setUp(void)
{
	bz_log_initialize(BZ_LOG_OFF);
}

void tearDown(void) {}

static void assert_rect_dimensions(struct bz_rect rect, int x, int y, int w, int h)
{
	TEST_ASSERT_EQUAL_INT(x, rect.x);
	TEST_ASSERT_EQUAL_INT(y, rect.y);
	TEST_ASSERT_EQUAL_INT(w, rect.w);
	TEST_ASSERT_EQUAL_INT(h, rect.h);
}


// =================================================================================================
//  Test bz_sizes_are_equal()
// -------------------------------------------------------------------------------------------------

void test_sizes_are_equal__nulls_are_equal(void)
{
	TEST_ASSERT_TRUE(bz_sizes_are_equal(nullptr, nullptr));
}

void test_sizes_are_equal__mixed_nulls_not_equal(void)
{
	TEST_ASSERT_FALSE(bz_sizes_are_equal(&(struct bz_size){ .w = 1, .h = 2 }, nullptr));
	TEST_ASSERT_FALSE(bz_sizes_are_equal(nullptr, &(struct bz_size){ .w = 1, .h = 2 }));
}

void test_sizes_are_equal__equal_widths_and_heights(void)
{
	TEST_ASSERT_TRUE(bz_sizes_are_equal(
		&(struct bz_size){ .w = 1, .h = 2 },
		&(struct bz_size){ .w = 1, .h = 2 }
	));
	TEST_ASSERT_TRUE(bz_sizes_are_equal(
		&(struct bz_size){ .w = -1, .h = -2 },
		&(struct bz_size){ .w = -1, .h = -2 }
	));
	TEST_ASSERT_TRUE(bz_sizes_are_equal(
		&(struct bz_size){ .w = 0, .h = 0 },
		&(struct bz_size){ .w = 0, .h = 0 }
	));
}

void test_sizes_are_equal__unequal_widths_and_heights(void)
{
	TEST_ASSERT_FALSE(bz_sizes_are_equal(
		&(struct bz_size){ .w = 1, .h = 2 },
		&(struct bz_size){ .w = -1, .h = -2 }
	));
	TEST_ASSERT_FALSE(bz_sizes_are_equal(
		&(struct bz_size){ .w = 1, .h = 1 },
		&(struct bz_size){ .w = 2, .h = 2 }
	));
	TEST_ASSERT_FALSE(bz_sizes_are_equal(
		&(struct bz_size){ .w = 0, .h = 0 },
		&(struct bz_size){ .w = 10, .h = 10 }
	));
}


// =================================================================================================
//  Test bz_rects_are_equal()
// -------------------------------------------------------------------------------------------------

void test_rects_are_equal__nulls_are_equal(void)
{
	TEST_ASSERT_TRUE(bz_rects_are_equal(nullptr, nullptr));
}

void test_rects_are_equal__mixed_nulls_not_equal(void)
{
	TEST_ASSERT_FALSE(bz_rects_are_equal(
		&(struct bz_rect){ .x = 1, .y = 2, .w = 3, .h = 4 },
		nullptr
	));
	TEST_ASSERT_FALSE(bz_rects_are_equal(
		nullptr,
		&(struct bz_rect){ .x = 1, .y = 2, .w = 3, .h = 4 }
	));
}

void test_rects_are_equal__equal_cases(void)
{
	TEST_ASSERT_TRUE(bz_rects_are_equal(
		&(struct bz_rect){ .x = 1, .y = 2, .w = 3, .h = 4 },
		&(struct bz_rect){ .x = 1, .y = 2, .w = 3, .h = 4 }
	));
	TEST_ASSERT_TRUE(bz_rects_are_equal(
		&(struct bz_rect){ .x = -1, .y = -2, .w = 3, .h = 4 },
		&(struct bz_rect){ .x = -1, .y = -2, .w = 3, .h = 4 }
	));
	TEST_ASSERT_TRUE(bz_rects_are_equal(
		&(struct bz_rect){ .x = 1, .y = 2, .w = -3, .h = -4 },
		&(struct bz_rect){ .x = 1, .y = 2, .w = -3, .h = -4 }
	));
	TEST_ASSERT_TRUE(bz_rects_are_equal(
		&(struct bz_rect){ .x = 0, .y = 0, .w = 0, .h = 0 },
		&(struct bz_rect){ .x = 0, .y = 0, .w = 0, .h = 0 }
	));
}

void test_rects_are_equal__unequal_cases(void)
{
	TEST_ASSERT_FALSE(bz_rects_are_equal(
		&(struct bz_rect){ .x = 1, .y = 2, .w = 3, .h = 4 },
		&(struct bz_rect){ .x = 5, .y = 6, .w = 7, .h = 8 }
	));
	TEST_ASSERT_FALSE(bz_rects_are_equal(
		&(struct bz_rect){ .x = -1, .y = -2, .w = 3, .h = 4 },
		&(struct bz_rect){ .x = 1, .y = 2, .w = 3, .h = 4 }
	));
	TEST_ASSERT_FALSE(bz_rects_are_equal(
		&(struct bz_rect){ .x = 1, .y = 2, .w = -3, .h = -4 },
		&(struct bz_rect){ .x = 1, .y = 2, .w = 3, .h = 4 }
	));
}


// =================================================================================================
//  Test bz_dbl_rects_are_equal()
// -------------------------------------------------------------------------------------------------

void test_dbl_rects_are_equal__nulls_are_equal(void)
{
	TEST_ASSERT_TRUE(bz_dbl_rects_are_equal(nullptr, nullptr));
}

void test_dbl_rects_are_equal__mixed_nulls_not_equal(void)
{
	TEST_ASSERT_FALSE(bz_dbl_rects_are_equal(
		&(struct bz_rect_dbl){ .x = 1.0, .y = 2.0, .w = 3.0, .h = 4.0 },
		nullptr
	));
	TEST_ASSERT_FALSE(bz_dbl_rects_are_equal(
		nullptr,
		&(struct bz_rect_dbl){ .x = 1.0, .y = 2.0, .w = 3.0, .h = 4.0 }
	));
}

void test_dbl_rects_are_equal__equal_cases(void)
{
	TEST_ASSERT_TRUE(bz_dbl_rects_are_equal(
		&(struct bz_rect_dbl){ .x = 1.1, .y = 2.2, .w = 3.3, .h = 4.4 },
		&(struct bz_rect_dbl){ .x = 1.1, .y = 2.2, .w = 3.3, .h = 4.4 }
	));
	TEST_ASSERT_TRUE(bz_dbl_rects_are_equal(
		&(struct bz_rect_dbl){ .x = -1.0, .y = -2.0, .w = 3.0, .h = 4.0 },
		&(struct bz_rect_dbl){ .x = -1.0, .y = -2.0, .w = 3.0, .h = 4.0 }
	));
	TEST_ASSERT_TRUE(bz_dbl_rects_are_equal(
		&(struct bz_rect_dbl){ .x = 1.0, .y = 2.0, .w = -3.0, .h = -4.0 },
		&(struct bz_rect_dbl){ .x = 1.0, .y = 2.0, .w = -3.0, .h = -4.0 }
	));
	TEST_ASSERT_TRUE(bz_dbl_rects_are_equal(
		&(struct bz_rect_dbl){ .x = 0.0, .y = 0.0, .w = 0.0, .h = 0.0 },
		&(struct bz_rect_dbl){ .x = 0.0, .y = 0.0, .w = 0.0, .h = 0.0 }
	));
}

void test_dbl_rects_are_equal__unequal_cases(void)
{
	TEST_ASSERT_FALSE(bz_dbl_rects_are_equal(
		&(struct bz_rect_dbl){ .x = 1.1, .y = 2.2, .w = 3.3, .h = 4.4 },
		&(struct bz_rect_dbl){ .x = 5.5, .y = 6.6, .w = 7.7, .h = 8.8 }
	));
	TEST_ASSERT_FALSE(bz_dbl_rects_are_equal(
		&(struct bz_rect_dbl){ .x = -1.1, .y = -2.2, .w = 3.3, .h = 4.4 },
		&(struct bz_rect_dbl){ .x = 1.1, .y = 2.2, .w = 3.3, .h = 4.4 }
	));
	TEST_ASSERT_FALSE(bz_dbl_rects_are_equal(
		&(struct bz_rect_dbl){ .x = 1.1, .y = 2.2, .w = -3.3, .h = -4.4 },
		&(struct bz_rect_dbl){ .x = 1.1, .y = 2.2, .w = 3.3, .h = 4.4 }
	));
	TEST_ASSERT_FALSE(bz_dbl_rects_are_equal(
		&(struct bz_rect_dbl){ .x = 0.0, .y = 0.0, .w = 0.0, .h = 0.0 },
		&(struct bz_rect_dbl){ .x = 0.01, .y = 0.02, .w = 0.03, .h = 0.04 }
	));
}


// =================================================================================================
//  Test bz_clone_rect()
// -------------------------------------------------------------------------------------------------

void test_clone_rect__deep_copies_data(void)
{
	struct bz_rect source = { .x = 1, .y = 2, .w = 3, .h = 4 };
	struct bz_rect *result = bz_clone_rect(&source);
	TEST_ASSERT_NOT_EQUAL(&source, result);
	TEST_ASSERT_TRUE(1 == result->x);
	TEST_ASSERT_TRUE(2 == result->y);
	TEST_ASSERT_TRUE(3 == result->w);
	TEST_ASSERT_TRUE(4 == result->h);
	// Sanity check: change a source value, and make sure result doesn't change.
	source.x = 5;
	TEST_ASSERT_TRUE(5 == source.x);
	TEST_ASSERT_TRUE(1 == result->x);
	free(result);
}

void test_clone_rect__succeeds_for_null_source(void)
{
	struct bz_rect *result = bz_clone_rect(nullptr);
	TEST_ASSERT_NULL(result);
}


// =================================================================================================
//  Test bz_clone_rect_dbl()
// -------------------------------------------------------------------------------------------------

void test_clone_rect_dbl__deep_copies_data(void)
{
	struct bz_rect_dbl source = { .x = 1.0, .y = 2.0, .w = 3.0, .h = 4.0 };
	struct bz_rect_dbl *result = bz_clone_rect_dbl(&source);
	TEST_ASSERT_NOT_EQUAL(&source, result);
	TEST_ASSERT_TRUE(1.0 == result->x);
	TEST_ASSERT_TRUE(2.0 == result->y);
	TEST_ASSERT_TRUE(3.0 == result->w);
	TEST_ASSERT_TRUE(4.0 == result->h);
	// Sanity check: change a source value, and make sure result doesn't change.
	source.x = 5.0;
	TEST_ASSERT_TRUE(5.0 == source.x);
	TEST_ASSERT_TRUE(1.0 == result->x);
	free(result);
}

void test_clone_rect_dbl__succeeds_for_null_source(void)
{
	struct bz_rect_dbl *result = bz_clone_rect_dbl(nullptr);
	TEST_ASSERT_NULL(result);
}


// =================================================================================================
//  Test bz_split_rect()
// -------------------------------------------------------------------------------------------------

void test_split_rect__null_source_null_result(void)
{
	TEST_ASSERT_NULL(bz_split_rect(nullptr, 0.5, 0.5, 1));
}

void test_split_rect__invalid_splits_returns_source(void)
{
	struct bz_rect source = { .x = 2, .y = 3, .w = 12, .h = 12 };
	TEST_ASSERT_EQUAL_PTR(&source, bz_split_rect(&source, 0, 1, 2));
}

void test_split_rect__source_is_unchanged(void)
{
	struct bz_rect source = { .x = 2, .y = 3, .w = 12, .h = 12 };
	struct bz_rect *results = bz_split_rect(&source, 0.0f, 0.5f, 2);
	TEST_ASSERT_NOT_EQUAL(&source, results);
	assert_rect_dimensions(source, 2, 3, 12, 12);
	free(results);
}

void test_split_rect__horizontal_split_with_gap(void)
{
	struct bz_rect source = { .x = 2, .y = 3, .w = 12, .h = 12 };
	struct bz_rect *results = bz_split_rect(&source, 0.8f, 0.0f, 2);
	assert_rect_dimensions(results[0], 2, 3, 12, 8);
	assert_rect_dimensions(results[1], 2, 13, 12, 2);
	free(results);
}

void test_split_rect__vertical_split_with_gap(void)
{
	struct bz_rect source = { .x = 2, .y = 3, .w = 12, .h = 12 };
	struct bz_rect *results = bz_split_rect(&source, 0.0f, 0.3f, 2);
	assert_rect_dimensions(results[0], 2, 3, 3, 12);
	assert_rect_dimensions(results[1], 7, 3, 7, 12);
	free(results);
}

void test_split_rect__both_splits_with_gap(void)
{
	struct bz_rect source = { .x = 2, .y = 3, .w = 12, .h = 12 };
	struct bz_rect *results = bz_split_rect(&source, 0.3f, 0.8f, 2);
	assert_rect_dimensions(results[0],  2, 3, 8, 3);
	assert_rect_dimensions(results[1], 12, 3, 2, 3);
	assert_rect_dimensions(results[2],  2, 8, 8, 7);
	assert_rect_dimensions(results[3], 12, 8, 2, 7);
	free(results);
}

void test_split_rect__both_splits_no_gap(void)
{
	struct bz_rect source = { .x = 2, .y = 3, .w = 10, .h = 10 };
	struct bz_rect *results = bz_split_rect(&source, 0.3f, 0.8f, 0);
	assert_rect_dimensions(results[0],  2, 3, 8, 3);
	assert_rect_dimensions(results[1], 10, 3, 2, 3);
	assert_rect_dimensions(results[2],  2, 6, 8, 7);
	assert_rect_dimensions(results[3], 10, 6, 2, 7);
	free(results);
}


// =================================================================================================
//  Test bz_contains_point()
// -------------------------------------------------------------------------------------------------

static bool bz_test_contains_point_wrapper(int x, int y, int w, int h, int px, int py)
{
	return bz_contains_point(
		&(struct bz_rect){ .x = x, .y = y, .w = w, .h = h },
		&(struct bz_position){ .x = px, .y = py }
	);
}

void test_contains_point__true_cases(void)
{
	// Verify fully positive values, and bounds are inclusive
	TEST_ASSERT_TRUE(bz_test_contains_point_wrapper(10, 10, 2, 2, 10, 10));
	TEST_ASSERT_TRUE(bz_test_contains_point_wrapper(10, 10, 2, 2, 11, 11));
	TEST_ASSERT_TRUE(bz_test_contains_point_wrapper(10, 10, 2, 2, 12, 12));
	// Test mixed rectangle bounds (some positive, some negative)
	TEST_ASSERT_TRUE(bz_test_contains_point_wrapper(-5, -5, 10, 10, -5, -5));
	TEST_ASSERT_TRUE(bz_test_contains_point_wrapper(-5, -5, 10, 10, -2, -2));
	TEST_ASSERT_TRUE(bz_test_contains_point_wrapper(-5, -5, 10, 10, 0, 0));
	TEST_ASSERT_TRUE(bz_test_contains_point_wrapper(-5, -5, 10, 10, 3, 3));
	TEST_ASSERT_TRUE(bz_test_contains_point_wrapper(-5, -5, 10, 10, 5, 5));
	// Test fully negative bounds
	TEST_ASSERT_TRUE(bz_test_contains_point_wrapper(-5, -5, 2, 2, -5, -5));
	TEST_ASSERT_TRUE(bz_test_contains_point_wrapper(-5, -5, 2, 2, -4, -4));
	TEST_ASSERT_TRUE(bz_test_contains_point_wrapper(-5, -5, 2, 2, -3, -3));
	// Test negative dimensions
	TEST_ASSERT_TRUE(bz_test_contains_point_wrapper(10, 10, -2, -2, 8, 8));
	TEST_ASSERT_TRUE(bz_test_contains_point_wrapper(10, 10, -2, -2, 9, 9));
	TEST_ASSERT_TRUE(bz_test_contains_point_wrapper(10, 10, -2, -2, 10, 10));
	TEST_ASSERT_TRUE(bz_test_contains_point_wrapper(0, 0, -2, -2, -2, -2));
	TEST_ASSERT_TRUE(bz_test_contains_point_wrapper(0, 0, -2, -2, -1, -1));
	TEST_ASSERT_TRUE(bz_test_contains_point_wrapper(0, 0, -2, -2, 0, 0));
	TEST_ASSERT_TRUE(bz_test_contains_point_wrapper(-1, -1, -2, -2, -3, -3));
	TEST_ASSERT_TRUE(bz_test_contains_point_wrapper(-1, -1, -2, -2, -2, -2));
	TEST_ASSERT_TRUE(bz_test_contains_point_wrapper(-1, -1, -2, -2, -1, -1));
}

void test_contains_point__false_cases(void)
{
	// Verify fully positive values
	TEST_ASSERT_FALSE(bz_test_contains_point_wrapper(10, 10, 2, 2, 9, 9));
	TEST_ASSERT_FALSE(bz_test_contains_point_wrapper(10, 10, 2, 2, 13, 13));
	// Test mixed rectangle bounds (some positive, some negative)
	TEST_ASSERT_FALSE(bz_test_contains_point_wrapper(-5, -5, 10, 10, -6, -6));
	TEST_ASSERT_FALSE(bz_test_contains_point_wrapper(-5, -5, 10, 10, 6, 6));
	// Test fully negative bounds
	TEST_ASSERT_FALSE(bz_test_contains_point_wrapper(-5, -5, 2, 2, -6, -6));
	TEST_ASSERT_FALSE(bz_test_contains_point_wrapper(-5, -5, 2, 2, -2, -2));
	// Test negative dimensions
	TEST_ASSERT_FALSE(bz_test_contains_point_wrapper(10, 10, -2, -2, 7, 7));
	TEST_ASSERT_FALSE(bz_test_contains_point_wrapper(10, 10, -2, -2, 11, 11));
	TEST_ASSERT_FALSE(bz_test_contains_point_wrapper(0, 0, -2, -2, -3, -3));
	TEST_ASSERT_FALSE(bz_test_contains_point_wrapper(0, 0, -2, -2, 1, 1));
	TEST_ASSERT_FALSE(bz_test_contains_point_wrapper(-1, -1, -2, -2, -4, -4));
	TEST_ASSERT_FALSE(bz_test_contains_point_wrapper(-1, -1, -2, -2, 0, 0));
}


// =================================================================================================
//  Test bz_clone_size()
// -------------------------------------------------------------------------------------------------

void test_clone_size__deep_copies_data(void)
{
	struct bz_size source = { .w = 1, .h = 2 };
	struct bz_size *result = bz_clone_size(&source);
	TEST_ASSERT_NOT_EQUAL(&source, result);
	TEST_ASSERT_EQUAL_INT(1, result->w);
	TEST_ASSERT_EQUAL_INT(2, result->h);
	// Sanity check: change a source value, and make sure result doesn't change.
	source.w = 3;
	TEST_ASSERT_EQUAL_INT(3, source.w);
	TEST_ASSERT_EQUAL_INT(1, result->w);
	free(result);
}

void test_clone_size__succeeds_for_null_source(void)
{
	struct bz_size *result = bz_clone_size(nullptr);
	TEST_ASSERT_NULL(result);
}


// =================================================================================================
//  Test bz_distance()
// -------------------------------------------------------------------------------------------------

void test_distance__returns_correct_distance(void)
{
	// Some 3,4,5 triangles for easy validation
	TEST_ASSERT_EQUAL_FLOAT(5, bz_distance(0, 0, 3, 4));
	TEST_ASSERT_EQUAL_FLOAT(5, bz_distance(2, 2, 5, 6));
	TEST_ASSERT_EQUAL_FLOAT(5, bz_distance(-2, -2, 1, 2));
	TEST_ASSERT_EQUAL_FLOAT(5, bz_distance(-5, -5, -2, -1));

	// Same point, zero distance
	TEST_ASSERT_EQUAL_FLOAT(0, bz_distance(1, 2, 1, 2));
}


// =================================================================================================
//  Test bz_rand_int()
// -------------------------------------------------------------------------------------------------

void test_rand_int__stays_in_correct_range(void)
{
	// I don't have a great way to test this ... so ...
	for (uint32_t x = 0; x < 100; x++) {
		int val = bz_rand_int(3, 8);
		TEST_ASSERT_GREATER_OR_EQUAL_INT(3, val);
		TEST_ASSERT_LESS_OR_EQUAL_INT(8, val);
	}
}


// =================================================================================================
//  Test bz_min()
// -------------------------------------------------------------------------------------------------

void test_min__smaller_number_is_returned(void)
{
	TEST_ASSERT_EQUAL_INT(1, bz_min(1, 2));
	TEST_ASSERT_EQUAL_INT(1, bz_min(2, 1));
	TEST_ASSERT_EQUAL_INT(-1, bz_min(-1, 2));
	TEST_ASSERT_EQUAL_INT(-1, bz_min(2, -1));
	TEST_ASSERT_EQUAL_INT(-2, bz_min(1, -2));
	TEST_ASSERT_EQUAL_INT(-2, bz_min(-2, 1));
	TEST_ASSERT_EQUAL_INT(1, bz_min(1, 1));
}


// =================================================================================================
//  Test bz_max()
// -------------------------------------------------------------------------------------------------

void test_max__larger_number_is_returned(void)
{
	TEST_ASSERT_EQUAL_INT(2, bz_max(1, 2));
	TEST_ASSERT_EQUAL_INT(2, bz_max(2, 1));
	TEST_ASSERT_EQUAL_INT(2, bz_max(-1, 2));
	TEST_ASSERT_EQUAL_INT(2, bz_max(2, -1));
	TEST_ASSERT_EQUAL_INT(1, bz_max(1, -2));
	TEST_ASSERT_EQUAL_INT(1, bz_max(-2, 1));
	TEST_ASSERT_EQUAL_INT(1, bz_max(1, 1));
}


// =================================================================================================
//  Test bz_clamp()
// -------------------------------------------------------------------------------------------------

void test_clamp__small_number_is_increased(void)
{
	TEST_ASSERT_EQUAL_INT(2, bz_clamp(1, 2, 5));
	TEST_ASSERT_EQUAL_INT(2, bz_clamp(-3, 2, 5));
	TEST_ASSERT_EQUAL_INT(-5, bz_clamp(-8, -5, -2));
}

void test_clamp__big_number_is_reduced(void)
{
	TEST_ASSERT_EQUAL_INT(5, bz_clamp(8, 2, 5));
	TEST_ASSERT_EQUAL_INT(-5, bz_clamp(-1, -8, -5));
}

void test_clamp__valid_number_is_unchanged(void)
{
	TEST_ASSERT_EQUAL_INT(2, bz_clamp(2, 2, 5));
	TEST_ASSERT_EQUAL_INT(3, bz_clamp(3, 2, 5));
	TEST_ASSERT_EQUAL_INT(5, bz_clamp(5, 2, 5));
	TEST_ASSERT_EQUAL_INT(-5, bz_clamp(-5, -8, -2));
}


// =================================================================================================
//  Test bz_dbl_is_int()
// -------------------------------------------------------------------------------------------------

void test_dbl_is_int__true_cases(void)
{
	TEST_ASSERT_TRUE(bz_dbl_is_int(1.0));
	TEST_ASSERT_TRUE(bz_dbl_is_int(-1.0));
	TEST_ASSERT_TRUE(bz_dbl_is_int(42.0));
	TEST_ASSERT_TRUE(bz_dbl_is_int(0.0));
	TEST_ASSERT_TRUE(bz_dbl_is_int(10));
}

void test_dbl_is_int__false_cases(void)
{
	TEST_ASSERT_FALSE(bz_dbl_is_int(1.1));
	TEST_ASSERT_FALSE(bz_dbl_is_int(-1.1002));
	TEST_ASSERT_FALSE(bz_dbl_is_int(42.9));
	TEST_ASSERT_FALSE(bz_dbl_is_int(0.5));
	TEST_ASSERT_FALSE(bz_dbl_is_int(10.0001));
}


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// -- bz_sizes_are_equal() --
	RUN_TEST(test_sizes_are_equal__nulls_are_equal);
	RUN_TEST(test_sizes_are_equal__mixed_nulls_not_equal);
	RUN_TEST(test_sizes_are_equal__equal_widths_and_heights);
	RUN_TEST(test_sizes_are_equal__unequal_widths_and_heights);

	// -- bz_rects_are_equal() --
	RUN_TEST(test_rects_are_equal__nulls_are_equal);
	RUN_TEST(test_rects_are_equal__mixed_nulls_not_equal);
	RUN_TEST(test_rects_are_equal__equal_cases);
	RUN_TEST(test_rects_are_equal__unequal_cases);

	// -- bz_dbl_rects_are_equal() --
	RUN_TEST(test_dbl_rects_are_equal__nulls_are_equal);
	RUN_TEST(test_dbl_rects_are_equal__mixed_nulls_not_equal);
	RUN_TEST(test_dbl_rects_are_equal__equal_cases);
	RUN_TEST(test_dbl_rects_are_equal__unequal_cases);

	// -- bz_clone_rect() --
	RUN_TEST(test_clone_rect__deep_copies_data);
	RUN_TEST(test_clone_rect__succeeds_for_null_source);

	// -- bz_clone_rect_dbl() --
	RUN_TEST(test_clone_rect_dbl__deep_copies_data);
	RUN_TEST(test_clone_rect_dbl__succeeds_for_null_source);

	// -- bz_split_rect() --
	RUN_TEST(test_split_rect__null_source_null_result);
	RUN_TEST(test_split_rect__invalid_splits_returns_source);
	RUN_TEST(test_split_rect__source_is_unchanged);
	RUN_TEST(test_split_rect__horizontal_split_with_gap);
	RUN_TEST(test_split_rect__vertical_split_with_gap);
	RUN_TEST(test_split_rect__both_splits_with_gap);
	RUN_TEST(test_split_rect__both_splits_no_gap);

	// -- bz_contains_point() --
	RUN_TEST(test_contains_point__true_cases);
	RUN_TEST(test_contains_point__false_cases);

	// -- bz_clone_size() --
	RUN_TEST(test_clone_size__deep_copies_data);
	RUN_TEST(test_clone_size__succeeds_for_null_source);

	// -- bz_distance() --
	RUN_TEST(test_distance__returns_correct_distance);

	// -- bz_rand_int() --
	RUN_TEST(test_rand_int__stays_in_correct_range);

	// -- bz_min() --
	RUN_TEST(test_min__smaller_number_is_returned);

	// -- bz_max() --
	RUN_TEST(test_max__larger_number_is_returned);

	// -- bz_clamp() --
	RUN_TEST(test_clamp__small_number_is_increased);
	RUN_TEST(test_clamp__big_number_is_reduced);
	RUN_TEST(test_clamp__valid_number_is_unchanged);

	// -- bz_dbl_is_int() --
	RUN_TEST(test_dbl_is_int__true_cases);
	RUN_TEST(test_dbl_is_int__false_cases);

	return UNITY_END();
}