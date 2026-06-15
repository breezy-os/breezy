
#include "breezy/bz_seat.h"

#include <stdlib.h>

#include <libseat.h>

#include "unity.h"
#include "fff.h"
#include "breezy/bz_breezy.h"
#include "breezy/bz_logger.h"


// =================================================================================================
//  Set up / tear down / globals
// -------------------------------------------------------------------------------------------------

DEFINE_FFF_GLOBALS
FAKE_VALUE_FUNC(const char *, libseat_seat_name, struct libseat *)
FAKE_VALUE_FUNC(int, libseat_switch_session, struct libseat *, int)

void setUp(void)
{
	bz_log_initialize(BZ_LOG_OFF);
}

void tearDown(void)
{
	RESET_FAKE(libseat_seat_name);
	RESET_FAKE(libseat_switch_session);
	FFF_RESET_HISTORY();
}


// =================================================================================================
//  Test bz_seat_name()
// -------------------------------------------------------------------------------------------------

/** bz_seat_name() should return a null value if the seat hasn't been initialized/opened yet. */
void test_seat_name_before_initialized(void)
{
	struct bz_breezy breezy = {0};
	const char *actual = bz_seat_name(&breezy);
	TEST_ASSERT_EQUAL_STRING(nullptr, actual);
}

/** bz_seat_name() should return the seat name from libseat once it has been initialized. */
void test_seat_name_after_initialized(void)
{
	// Mocks!
	libseat_seat_name_fake.return_val = "test-seat";

	// Fake-initialize the breezy seat.
	struct bz_breezy breezy = {0};
	int dummy;
	breezy.seat.seat = (struct libseat *)&dummy;

	// Call our function
	const char *actual = bz_seat_name(&breezy);

	// Make our assertions
	TEST_ASSERT_EQUAL_STRING("test-seat", actual);
	TEST_ASSERT_EQUAL_INT(1, libseat_seat_name_fake.call_count);
	TEST_ASSERT_EQUAL_PTR(&dummy, libseat_seat_name_fake.arg0_val);
}


// =================================================================================================
//  Test bz_seat_change_vt()
// -------------------------------------------------------------------------------------------------

/** bz_seat_change_vt() should NOT call libseat_switch_session for invalid VTs. */
void test_seat_change_vt_invalid_vt(void)
{
	// Two invalid calls
	bz_seat_change_vt(nullptr, 0);
	bz_seat_change_vt(nullptr, 13);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(0, libseat_switch_session_fake.call_count);
}

/** bz_seat_change_vt() should call libseat_switch_session for valid VTs. */
void test_seat_change_vt_valid_vt(void)
{
	struct bz_breezy breezy = {0};

	// Two invalid calls
	bz_seat_change_vt(&breezy, 1);
	bz_seat_change_vt(&breezy, 12);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(2, libseat_switch_session_fake.call_count);
}


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// Test bz_seat_name()
	RUN_TEST(test_seat_name_before_initialized);
	RUN_TEST(test_seat_name_after_initialized);

	// Test bz_seat_change_vt()
	RUN_TEST(test_seat_change_vt_invalid_vt);
	RUN_TEST(test_seat_change_vt_valid_vt);

	return UNITY_END();
}
