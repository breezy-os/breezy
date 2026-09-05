
#include "breezy/bz_xdg_shell.h"

#include <stdlib.h>

#include <xdg-shell-server-protocol.h>

#include "unity.h"
#include "fff.h"
#include "breezy/bz_logger.h"
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

extern const struct xdg_toplevel_interface bz_xdg_toplevel_implementation;


// =================================================================================================
//  Test bz_xdg_toplevel_destroy()
// -------------------------------------------------------------------------------------------------

void test_xdg_toplevel_destroy__destroys_the_resource(void)
{
	bz_xdg_toplevel_implementation.destroy(nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_destroy_fake.call_count);
}


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// Test bz_xdg_toplevel_destroy()
	RUN_TEST(test_xdg_toplevel_destroy__destroys_the_resource);

	return UNITY_END();
}
