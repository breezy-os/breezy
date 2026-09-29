
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
//  Test bz_xdg_toplevel_set_max_size()
// -------------------------------------------------------------------------------------------------

void test_xdg_toplevel_set_max_size__initializes_new_state(void)
{
	// Set up
	struct bz_xdg_toplevel *toplevel = bz_create_xdg_toplevel_data();
	struct bz_surface *surface = bz_create_surface_data();
	toplevel->wlsurface = surface;
	wl_resource_get_user_data_fake.return_val = toplevel;

	// Test
	TEST_ASSERT_NULL(surface->pending_state->max_size);
	bz_xdg_toplevel_implementation.set_max_size(nullptr, nullptr, 1, 2);
	TEST_ASSERT_NOT_NULL(surface->pending_state->max_size);
	TEST_ASSERT_EQUAL_INT(1, surface->pending_state->max_size->w);
	TEST_ASSERT_EQUAL_INT(2, surface->pending_state->max_size->h);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_error_fake.call_count);

	// Clean up
	bz_free_surface_data(surface);
	bz_free_xdg_toplevel_data(toplevel);
}

void test_xdg_toplevel_set_max_size__does_not_reinitialize_state(void)
{
	// Set up
	struct bz_xdg_toplevel *toplevel = bz_create_xdg_toplevel_data();
	struct bz_surface *surface = bz_create_surface_data();
	toplevel->wlsurface = surface;
	wl_resource_get_user_data_fake.return_val = toplevel;

	// Call once to initialize the state.
	bz_xdg_toplevel_implementation.set_max_size(nullptr, nullptr, 1, 2);
	TEST_ASSERT_NOT_NULL(surface->pending_state->max_size);
	struct bz_size *original = surface->pending_state->max_size;
	TEST_ASSERT_EQUAL_INT(1, surface->pending_state->max_size->w);
	TEST_ASSERT_EQUAL_INT(2, surface->pending_state->max_size->h);

	// Call again, making sure it's the same reference/pointer, but the values are replaced
	bz_xdg_toplevel_implementation.set_max_size(nullptr, nullptr, 3, 4);
	TEST_ASSERT_NOT_NULL(surface->pending_state->max_size);
	TEST_ASSERT_EQUAL_PTR(original, surface->pending_state->max_size);
	TEST_ASSERT_EQUAL_INT(3, surface->pending_state->max_size->w);
	TEST_ASSERT_EQUAL_INT(4, surface->pending_state->max_size->h);
	TEST_ASSERT_EQUAL_INT(3, original->w);
	TEST_ASSERT_EQUAL_INT(4, original->h);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_error_fake.call_count);

	// Clean up
	bz_free_surface_data(surface);
	bz_free_xdg_toplevel_data(toplevel);
}

void test_xdg_toplevel_set_max_size__zero_is_ok(void)
{
	// Set up
	struct bz_xdg_toplevel *toplevel = bz_create_xdg_toplevel_data();
	struct bz_surface *surface = bz_create_surface_data();
	toplevel->wlsurface = surface;
	wl_resource_get_user_data_fake.return_val = toplevel;

	// Test
	TEST_ASSERT_NULL(surface->pending_state->max_size);
	bz_xdg_toplevel_implementation.set_max_size(nullptr, nullptr, 0, 0);
	TEST_ASSERT_NOT_NULL(surface->pending_state->max_size);
	TEST_ASSERT_EQUAL_INT(0, surface->pending_state->max_size->w);
	TEST_ASSERT_EQUAL_INT(0, surface->pending_state->max_size->h);

	// Clean up
	bz_free_surface_data(surface);
	bz_free_xdg_toplevel_data(toplevel);
}

void test_xdg_toplevel_set_max_size__negative_raises_error(void)
{
	// Set up
	struct bz_xdg_toplevel *toplevel = bz_create_xdg_toplevel_data();
	struct bz_surface *surface = bz_create_surface_data();
	toplevel->wlsurface = surface;
	wl_resource_get_user_data_fake.return_val = toplevel;

	// Test
	TEST_ASSERT_NULL(surface->pending_state->max_size);
	bz_xdg_toplevel_implementation.set_max_size(nullptr, nullptr, -1, -1);
	TEST_ASSERT_NULL(surface->pending_state->max_size);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(XDG_TOPLEVEL_ERROR_INVALID_SIZE, wl_resource_post_error_fake.arg1_val);

	// Clean up
	bz_free_surface_data(surface);
	bz_free_xdg_toplevel_data(toplevel);
}


// =================================================================================================
//  Test bz_xdg_toplevel_set_min_size()
// -------------------------------------------------------------------------------------------------

void test_xdg_toplevel_set_min_size__initializes_new_state(void)
{
	// Set up
	struct bz_xdg_toplevel *toplevel = bz_create_xdg_toplevel_data();
	struct bz_surface *surface = bz_create_surface_data();
	toplevel->wlsurface = surface;
	wl_resource_get_user_data_fake.return_val = toplevel;

	// Test
	TEST_ASSERT_NULL(surface->pending_state->min_size);
	bz_xdg_toplevel_implementation.set_min_size(nullptr, nullptr, 1, 2);
	TEST_ASSERT_NOT_NULL(surface->pending_state->min_size);
	TEST_ASSERT_EQUAL_INT(1, surface->pending_state->min_size->w);
	TEST_ASSERT_EQUAL_INT(2, surface->pending_state->min_size->h);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_error_fake.call_count);

	// Clean up
	bz_free_surface_data(surface);
	bz_free_xdg_toplevel_data(toplevel);
}

void test_xdg_toplevel_set_min_size__does_not_reinitialize_state(void)
{
	// Set up
	struct bz_xdg_toplevel *toplevel = bz_create_xdg_toplevel_data();
	struct bz_surface *surface = bz_create_surface_data();
	toplevel->wlsurface = surface;
	wl_resource_get_user_data_fake.return_val = toplevel;

	// Call once to initialize the state.
	bz_xdg_toplevel_implementation.set_min_size(nullptr, nullptr, 1, 2);
	TEST_ASSERT_NOT_NULL(surface->pending_state->min_size);
	struct bz_size *original = surface->pending_state->min_size;
	TEST_ASSERT_EQUAL_INT(1, surface->pending_state->min_size->w);
	TEST_ASSERT_EQUAL_INT(2, surface->pending_state->min_size->h);

	// Call again, making sure it's the same reference/pointer, but the values are replaced
	bz_xdg_toplevel_implementation.set_min_size(nullptr, nullptr, 3, 4);
	TEST_ASSERT_NOT_NULL(surface->pending_state->min_size);
	TEST_ASSERT_EQUAL_PTR(original, surface->pending_state->min_size);
	TEST_ASSERT_EQUAL_INT(3, surface->pending_state->min_size->w);
	TEST_ASSERT_EQUAL_INT(4, surface->pending_state->min_size->h);
	TEST_ASSERT_EQUAL_INT(3, original->w);
	TEST_ASSERT_EQUAL_INT(4, original->h);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_error_fake.call_count);

	// Clean up
	bz_free_surface_data(surface);
	bz_free_xdg_toplevel_data(toplevel);
}

void test_xdg_toplevel_set_min_size__zero_is_ok(void)
{
	// Set up
	struct bz_xdg_toplevel *toplevel = bz_create_xdg_toplevel_data();
	struct bz_surface *surface = bz_create_surface_data();
	toplevel->wlsurface = surface;
	wl_resource_get_user_data_fake.return_val = toplevel;

	// Test
	TEST_ASSERT_NULL(surface->pending_state->min_size);
	bz_xdg_toplevel_implementation.set_min_size(nullptr, nullptr, 0, 0);
	TEST_ASSERT_NOT_NULL(surface->pending_state->min_size);
	TEST_ASSERT_EQUAL_INT(0, surface->pending_state->min_size->w);
	TEST_ASSERT_EQUAL_INT(0, surface->pending_state->min_size->h);

	// Clean up
	bz_free_surface_data(surface);
	bz_free_xdg_toplevel_data(toplevel);
}

void test_xdg_toplevel_set_min_size__negative_raises_error(void)
{
	// Set up
	struct bz_xdg_toplevel *toplevel = bz_create_xdg_toplevel_data();
	struct bz_surface *surface = bz_create_surface_data();
	toplevel->wlsurface = surface;
	wl_resource_get_user_data_fake.return_val = toplevel;

	// Test
	TEST_ASSERT_NULL(surface->pending_state->min_size);
	bz_xdg_toplevel_implementation.set_min_size(nullptr, nullptr, -1, -1);
	TEST_ASSERT_NULL(surface->pending_state->min_size);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(XDG_TOPLEVEL_ERROR_INVALID_SIZE, wl_resource_post_error_fake.arg1_val);

	// Clean up
	bz_free_surface_data(surface);
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

	// Test bz_xdg_toplevel_set_max_size()
	RUN_TEST(test_xdg_toplevel_set_max_size__initializes_new_state);
	RUN_TEST(test_xdg_toplevel_set_max_size__does_not_reinitialize_state);
	RUN_TEST(test_xdg_toplevel_set_max_size__zero_is_ok);
	RUN_TEST(test_xdg_toplevel_set_max_size__negative_raises_error);

	// // Test bz_xdg_toplevel_set_min_size()
	RUN_TEST(test_xdg_toplevel_set_min_size__initializes_new_state);
	RUN_TEST(test_xdg_toplevel_set_min_size__does_not_reinitialize_state);
	RUN_TEST(test_xdg_toplevel_set_min_size__zero_is_ok);
	RUN_TEST(test_xdg_toplevel_set_min_size__negative_raises_error);

	return UNITY_END();
}
