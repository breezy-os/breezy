
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
}

void test_clamp__big_number_is_reduced(void)
{
	TEST_ASSERT_EQUAL_INT(5, bz_clamp(8, 2, 5));
}

void test_clamp__valid_number_is_unchanged(void)
{
	TEST_ASSERT_EQUAL_INT(2, bz_clamp(2, 2, 5));
	TEST_ASSERT_EQUAL_INT(3, bz_clamp(3, 2, 5));
	TEST_ASSERT_EQUAL_INT(5, bz_clamp(5, 2, 5));
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

	return UNITY_END();
}