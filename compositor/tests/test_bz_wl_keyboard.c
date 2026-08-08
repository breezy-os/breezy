
#include "breezy/bz_wl_devices.h"

#include <stdlib.h>

#include "unity.h"
#include "fff.h"
#include "breezy/bz_graphics.h"
#include "breezy/bz_logger.h"
#include "helpers/bz_test_resources.c"


// =================================================================================================
//  Set up / tear down / globals
// -------------------------------------------------------------------------------------------------

DEFINE_FFF_GLOBALS
// // -- wl_client --
// FAKE_VOID_FUNC(wl_client_post_no_memory, struct wl_client *)
// // -- wl_resource --
// FAKE_VALUE_FUNC(void *, wl_resource_get_user_data, struct wl_resource *)
// FAKE_VALUE_FUNC(struct wl_resource *, wl_resource_create, struct wl_client *, const struct wl_interface *, int, uint32_t)
// FAKE_VOID_FUNC_VARARG(wl_resource_post_event, struct wl_resource *, uint32_t, ...)
// FAKE_VOID_FUNC(wl_resource_destroy, struct wl_resource *)

void setUp(void)
{
	// RESET_FAKE(wl_client_post_no_memory);
	// RESET_FAKE(wl_resource_get_user_data);
	// RESET_FAKE(wl_resource_create);
	// RESET_FAKE(wl_resource_post_event);
	// RESET_FAKE(wl_resource_destroy);
	FFF_RESET_HISTORY();

	bz_log_initialize(BZ_LOG_OFF);
}

void tearDown(void) {}


// =================================================================================================
//  Helper functions for some of our tests
// -------------------------------------------------------------------------------------------------

extern const struct wl_keyboard_interface bz_keyboard_implementation;


// =================================================================================================
//  Test bz_keyboard_release()
// -------------------------------------------------------------------------------------------------

/** TODO */
void test_keyboard_release__()
{
	// TODO
}


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// Test bz_keyboard_release()
	// RUN_TEST(test_keyboard_release__); // TODO

	return UNITY_END();
}
