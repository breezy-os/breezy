
#include <stdio.h>

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

void tearDown(void)
{
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
//  Test bz_contains_point()
// -------------------------------------------------------------------------------------------------

static bool bz_test_contains_point_wrapper(int x, int y, int w, int h, int px, int py)
{
	return bz_contains_point(
		&(struct bz_position ){ .x = x, .y = y },
		&(struct bz_dimension){ .w = w, .h = h },
		&(struct bz_position ){ .x = px, .y = py }
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
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// -- bz_distance() --
	RUN_TEST(test_distance__returns_correct_distance);

	// -- bz_rand_int() --
	RUN_TEST(test_rand_int__stays_in_correct_range);

	// -- bz_clamp() --
	RUN_TEST(test_clamp__small_number_is_increased);
	RUN_TEST(test_clamp__big_number_is_reduced);
	RUN_TEST(test_clamp__valid_number_is_unchanged);

	// -- bz_contains_point() --
	RUN_TEST(test_contains_point__true_cases);
	RUN_TEST(test_contains_point__false_cases);

	return UNITY_END();
}