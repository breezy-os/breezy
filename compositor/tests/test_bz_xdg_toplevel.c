
#include "breezy/bz_xdg_shell.h"

#include <stdlib.h>

#include <xdg-shell-server-protocol.h>

#include "unity.h"
#include "fff.h"
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
//  Test bz_xdg_toplevel_set_title()
// -------------------------------------------------------------------------------------------------

void test_xdg_toplevel_set_title__sets_the_title(void)
{
	// Set up
	struct bz_xdg_toplevel *toplevel = bz_create_xdg_toplevel_data();
	wl_resource_get_user_data_fake.return_val = toplevel;

	// Test
	bz_xdg_toplevel_implementation.set_title(nullptr, nullptr, "Hello there");
	TEST_ASSERT_EQUAL_STRING("Hello there", toplevel->title);

	// Clean up
	bz_free_xdg_toplevel_data(toplevel);
}


// =================================================================================================
//  Test bz_xdg_toplevel_set_app_id()
// -------------------------------------------------------------------------------------------------

void test_xdg_toplevel_set_app_id__sets_the_app_id(void)
{
	// Set up
	struct bz_xdg_toplevel *toplevel = bz_create_xdg_toplevel_data();
	wl_resource_get_user_data_fake.return_val = toplevel;

	// Test
	bz_xdg_toplevel_implementation.set_app_id(nullptr, nullptr, "dev.zenittini.breezy.Test");
	TEST_ASSERT_EQUAL_STRING("dev.zenittini.breezy.Test", toplevel->app_id);

	// Clean up
	bz_free_xdg_toplevel_data(toplevel);
}


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// Test bz_xdg_toplevel_destroy()
	RUN_TEST(test_xdg_toplevel_destroy__destroys_the_resource);

	// Test bz_xdg_toplevel_set_title()
	RUN_TEST(test_xdg_toplevel_set_title__sets_the_title);

	// Test bz_xdg_toplevel_set_app_id()
	RUN_TEST(test_xdg_toplevel_set_app_id__sets_the_app_id);

	return UNITY_END();
}
