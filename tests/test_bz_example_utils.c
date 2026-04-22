
#include "unity/unity.h"
#include "fff/fff.h"
DEFINE_FFF_GLOBALS

#include "breezy/bz_example_utils.h"

FAKE_VALUE_FUNC(int, multiply, int, int)

void setUp(void) {}
void tearDown(void) {}

void test_add(void) {
	TEST_ASSERT_EQUAL_INT(3, add(1, 2));
}

void test_power(void) {
	multiply_fake.return_val = 1;
	TEST_ASSERT_EQUAL_INT(1, power(2, 3));

	RESET_FAKE(multiply);
	FFF_RESET_HISTORY();
}

int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_add);
	RUN_TEST(test_power);
	return UNITY_END();
}