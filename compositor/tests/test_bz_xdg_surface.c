
#include "breezy/bz_xdg_shell.h"

#include <stdlib.h>

#include <xdg-shell-server-protocol.h>

#include "unity.h"
#include "breezy/bz_list.h"
#include "breezy/bz_logger.h"
#include "breezy/bz_wl_display.h"
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

extern const struct xdg_surface_interface bz_xdg_surface_implementation;


// =================================================================================================
//  Test bz_xdg_surface_destroy()
// -------------------------------------------------------------------------------------------------

/** Deletes the xdg_surface when it never held a role. */
void test_xdg_surface_destroy__succeeds_with_no_role(void)
{
	// Set up our test data
	struct bz_xdg_surface *xdg_surface_data = bz_create_xdg_surface_data();
	wl_resource_get_user_data_fake.return_val = xdg_surface_data;

	// Run our test
	bz_xdg_surface_implementation.destroy(nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_destroy_fake.call_count);

	// Clean up
	bz_free_xdg_surface_data(xdg_surface_data);
}

/** Deletes the xdg_surface when it had a role which has since been destroyed. */
void test_xdg_surface_destroy__succeeds_with_destroyed_role(void)
{
	// Set up our test data
	struct bz_xdg_surface *xdg_surface_data = bz_create_xdg_surface_data();
	wl_resource_get_user_data_fake.return_val = xdg_surface_data;

	// Surface should have a role, but not a role object.
	xdg_surface_data->wlsurface->role = BZ_SURF_ROLE_XDG_TOPLEVEL;
	xdg_surface_data->wlsurface->xdgtoplevel = nullptr;

	// Run our test
	bz_xdg_surface_implementation.destroy(nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_destroy_fake.call_count);

	// Clean up
	bz_free_xdg_surface_data(xdg_surface_data);
}

/** The client must destroy the role object first. Otherwise, a defunct_role_object error is sent. */
void test_xdg_surface_destroy__with_role_sends_error(void)
{
	// Set up our test data
	struct bz_xdg_surface *xdg_surface_data = bz_create_xdg_surface_data();
	wl_resource_get_user_data_fake.return_val = xdg_surface_data;

	// Surface should have both a role and role object.
	xdg_surface_data->wlsurface->role = BZ_SURF_ROLE_XDG_TOPLEVEL;
	xdg_surface_data->wlsurface->xdgtoplevel = bz_create_xdg_toplevel_data();

	// Run our test
	bz_xdg_surface_implementation.destroy(nullptr, nullptr);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_destroy_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL_INT(XDG_SURFACE_ERROR_DEFUNCT_ROLE_OBJECT, wl_resource_post_error_fake.arg1_val);

	// Clean up
	bz_free_xdg_toplevel_data(xdg_surface_data->wlsurface->xdgtoplevel);
	bz_free_xdg_surface_data(xdg_surface_data);
}


// =================================================================================================
//  Test bz_xdg_surface_get_toplevel()
// -------------------------------------------------------------------------------------------------

/** No errors on a standard happy-path call. Associated wl_surface user data is updated as well. */
void test_xdg_surface_get_toplevel__initializes_properly(void)
{
	// Set up our mocks
	struct bz_client *client_data = bz_create_client_data();
	wl_client_get_user_data_fake.return_val = client_data;
	struct bz_xdg_surface *xdgsurf_data = bz_create_xdg_surface_data();
	wl_resource_get_user_data_fake.return_val = xdgsurf_data;
	struct wl_resource *xdgtoplevel_res = calloc(1, sizeof(*xdgtoplevel_res));
	wl_resource_create_fake.return_val = xdgtoplevel_res;

	// Run our test!
	bz_xdg_surface_implementation.get_toplevel(nullptr, nullptr, 0);

	// Make our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL(xdgtoplevel_res, wl_resource_set_implementation_fake.arg0_val); // Resource
	TEST_ASSERT_NOT_NULL(wl_resource_set_implementation_fake.arg1_val);               // Interface
	TEST_ASSERT_NOT_NULL(wl_resource_set_implementation_fake.arg2_val);               // User Data

	// Also verify some of our (more important) user data
	struct bz_xdg_toplevel *xdgtoplevel_data = wl_resource_set_implementation_fake.arg2_val;
	TEST_ASSERT_EQUAL(xdgtoplevel_res, xdgtoplevel_data->resource);
	TEST_ASSERT_EQUAL(xdgsurf_data, xdgtoplevel_data->xdgsurface);
	TEST_ASSERT_EQUAL(xdgsurf_data->wlsurface, xdgtoplevel_data->wlsurface);
	TEST_ASSERT_EQUAL(BZ_SURF_ROLE_XDG_TOPLEVEL, xdgsurf_data->wlsurface->role); // Surface role is set!

	// Cleanup
	bz_free_xdg_surface_data(xdgsurf_data);
	bz_free_xdg_toplevel_data(xdgtoplevel_data);
	free(xdgtoplevel_res);
	bz_free_client_data(client_data);
}

/** The associated surface holds the "toplevel" role after this call. */
void test_xdg_surface_get_toplevel__assigns_toplevel_role(void)
{
	// Set up our mocks
	struct bz_client *client_data = bz_create_client_data();
	wl_client_get_user_data_fake.return_val = client_data;
	struct bz_xdg_surface *xdgsurf_data = bz_create_xdg_surface_data();
	wl_resource_get_user_data_fake.return_val = xdgsurf_data;
	struct wl_resource *xdgtoplevel_res = calloc(1, sizeof(*xdgtoplevel_res));
	wl_resource_create_fake.return_val = xdgtoplevel_res;

	// Run our test!
	TEST_ASSERT_EQUAL(BZ_SURF_ROLE_NONE, xdgsurf_data->wlsurface->role);
	bz_xdg_surface_implementation.get_toplevel(nullptr, nullptr, 0);
	TEST_ASSERT_EQUAL(BZ_SURF_ROLE_XDG_TOPLEVEL, xdgsurf_data->wlsurface->role);

	// Cleanup
	bz_free_xdg_surface_data(xdgsurf_data);
	bz_free_xdg_toplevel_data(wl_resource_set_implementation_fake.arg2_val);
	free(xdgtoplevel_res);
	bz_free_client_data(client_data);
}

/** If the surface already held a non-toplevel role, a role error is sent. */
void test_xdg_surface_get_toplevel__cannot_change_existing_role(void)
{
	// Set up our mocks
	struct bz_xdg_surface *xdgsurf_data = bz_create_xdg_surface_data();
	wl_resource_get_user_data_fake.return_val = xdgsurf_data;
	struct wl_resource *xdgtoplevel_res = calloc(1, sizeof(*xdgtoplevel_res));
	wl_resource_create_fake.return_val = xdgtoplevel_res;

	// Run our test!
	xdgsurf_data->wlsurface->role = BZ_SURF_ROLE_XDG_POPUP;
	bz_xdg_surface_implementation.get_toplevel(nullptr, nullptr, 0);

	// Do our assertions
	// -- Role error should be posted --
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL(XDG_WM_BASE_ERROR_ROLE, wl_resource_post_error_fake.arg1_val);
	// -- Nothing should have changed / executed --
	TEST_ASSERT_EQUAL_INT(0, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL(BZ_SURF_ROLE_XDG_POPUP, xdgsurf_data->wlsurface->role);

	// Cleanup
	bz_free_xdg_surface_data(xdgsurf_data);
	bz_free_xdg_toplevel_data(wl_resource_set_implementation_fake.arg2_val);
	free(xdgtoplevel_res);
}

/** If the surface already held a toplevel role, everything still works ok. */
void test_xdg_surface_get_toplevel__can_reassign_same_role(void)
{
	// Set up our mocks
	struct bz_client *client_data = bz_create_client_data();
	wl_client_get_user_data_fake.return_val = client_data;
	struct bz_xdg_surface *xdgsurf_data = bz_create_xdg_surface_data();
	wl_resource_get_user_data_fake.return_val = xdgsurf_data;
	struct wl_resource *xdgtoplevel_res = calloc(1, sizeof(*xdgtoplevel_res));
	wl_resource_create_fake.return_val = xdgtoplevel_res;

	// Run our test!
	xdgsurf_data->wlsurface->role = BZ_SURF_ROLE_XDG_TOPLEVEL;
	bz_xdg_surface_implementation.get_toplevel(nullptr, nullptr, 0);

	// Do our assertions
	// -- Role error should NOT be posted --
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_error_fake.call_count);
	// -- Everything should execute like normal --
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_set_implementation_fake.call_count);
	TEST_ASSERT_EQUAL(BZ_SURF_ROLE_XDG_TOPLEVEL, xdgsurf_data->wlsurface->role);

	// Cleanup
	bz_free_xdg_surface_data(xdgsurf_data);
	bz_free_xdg_toplevel_data(wl_resource_set_implementation_fake.arg2_val);
	free(xdgtoplevel_res);
	bz_free_client_data(client_data);
}

/** A "no memory" error is posted for failed Wayland resource creation. */
void test_xdg_surface_get_toplevel__resource_failed(void)
{
	// Set up our mocks
	struct bz_xdg_surface *xdgsurf_data = bz_create_xdg_surface_data();
	wl_resource_get_user_data_fake.return_val = xdgsurf_data;

	// Run our test!
	wl_resource_create_fake.return_val = nullptr;
	bz_xdg_surface_implementation.get_toplevel(nullptr, nullptr, 0);

	// Do our assertions
	TEST_ASSERT_EQUAL_INT(1, wl_client_post_no_memory_fake.call_count);
	TEST_ASSERT_EQUAL_INT(1, wl_resource_create_fake.call_count);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_set_implementation_fake.call_count);

	// Cleanup
	bz_free_xdg_surface_data(xdgsurf_data);
}


// =================================================================================================
//  Test bz_xdg_surface_ack_configure()
// -------------------------------------------------------------------------------------------------

/** The xdg_surface's user data should have an updated last_acked_configure. */
void test_xdg_surface_ack_configure__state_is_updated(void)
{
	// Set up our mocks
	struct bz_xdg_surface_configure *config_2 = bz_create_xdg_surface_configure(2);
	struct bz_xdg_surface *xdgsurf_data = bz_create_xdg_surface_data();
	bz_list_append(xdgsurf_data->pending_configures, bz_create_xdg_surface_configure(1));
	bz_list_append(xdgsurf_data->pending_configures, config_2);
	bz_list_append(xdgsurf_data->pending_configures, bz_create_xdg_surface_configure(3));
	wl_resource_get_user_data_fake.return_val = xdgsurf_data;

	// Run our test!
	TEST_ASSERT_NULL(xdgsurf_data->last_acked_configure);
	bz_xdg_surface_implementation.ack_configure(nullptr, nullptr, 2);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_error_fake.call_count); // config event was found
	TEST_ASSERT_EQUAL(config_2, xdgsurf_data->last_acked_configure);

	// Cleanup!
	bz_free_xdg_surface_data(xdgsurf_data);
}

/**
 * Sending an ack consumes the serial number sent with the request, as well as ALL prior serial
 * numbers for this xdg_surface.
 */
void test_xdg_surface_ack_configure__consumes_previous_serials(void)
{
	// Set up our mocks
	struct bz_xdg_surface_configure *config_3 = bz_create_xdg_surface_configure(3);
	struct bz_xdg_surface *xdgsurf_data = bz_create_xdg_surface_data();
	bz_list_append(xdgsurf_data->pending_configures, bz_create_xdg_surface_configure(1));
	bz_list_append(xdgsurf_data->pending_configures, bz_create_xdg_surface_configure(2));
	bz_list_append(xdgsurf_data->pending_configures, config_3);
	wl_resource_get_user_data_fake.return_val = xdgsurf_data;

	// Run our test!
	bz_xdg_surface_implementation.ack_configure(nullptr, nullptr, 2);
	TEST_ASSERT_EQUAL_INT(1, xdgsurf_data->pending_configures->length);
	TEST_ASSERT_EQUAL(config_3, xdgsurf_data->pending_configures->head->data);

	// Cleanup!
	bz_free_xdg_surface_data(xdgsurf_data);
}

/**
 * Sending multiple ack requests for the same serial, OR acking a serial older than the last acked
 * serial results in an invalid_serial error.
 */
void test_xdg_surface_ack_configure__consumed_serial_causes_invalid_serial_error(void)
{
	// Set up our mocks
	struct bz_xdg_surface *xdgsurf_data = bz_create_xdg_surface_data();
	bz_list_append(xdgsurf_data->pending_configures, bz_create_xdg_surface_configure(1));
	bz_list_append(xdgsurf_data->pending_configures, bz_create_xdg_surface_configure(2));
	bz_list_append(xdgsurf_data->pending_configures, bz_create_xdg_surface_configure(3));
	wl_resource_get_user_data_fake.return_val = xdgsurf_data;

	// First call with "2" works
	bz_xdg_surface_implementation.ack_configure(nullptr, nullptr, 2);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_error_fake.call_count);

	// Next call with "2" fails since it was already consumed
	bz_xdg_surface_implementation.ack_configure(nullptr, nullptr, 2); // Fails, already used
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL(XDG_SURFACE_ERROR_INVALID_SERIAL, wl_resource_post_error_fake.arg1_val);
	RESET_FAKE(wl_resource_post_error);

	// Next call with "1" fails since it's older than 2
	bz_xdg_surface_implementation.ack_configure(nullptr, nullptr, 1); // Fails, it's older
	TEST_ASSERT_EQUAL_INT(1, wl_resource_post_error_fake.call_count);
	TEST_ASSERT_EQUAL(XDG_SURFACE_ERROR_INVALID_SERIAL, wl_resource_post_error_fake.arg1_val);
	RESET_FAKE(wl_resource_post_error);

	// Next call with "3" works since it's newer than 2
	bz_xdg_surface_implementation.ack_configure(nullptr, nullptr, 3);
	TEST_ASSERT_EQUAL_INT(0, wl_resource_post_error_fake.call_count);

	// Cleanup!
	bz_free_xdg_surface_data(xdgsurf_data);
}


// =================================================================================================
//  Runner
// -------------------------------------------------------------------------------------------------

int main(void) {
	UNITY_BEGIN();

	// Test bz_xdg_surface_destroy()
	RUN_TEST(test_xdg_surface_destroy__succeeds_with_no_role);
	RUN_TEST(test_xdg_surface_destroy__succeeds_with_destroyed_role);
	RUN_TEST(test_xdg_surface_destroy__with_role_sends_error);

	// Test bz_xdg_surface_get_toplevel()
	RUN_TEST(test_xdg_surface_get_toplevel__initializes_properly);
	RUN_TEST(test_xdg_surface_get_toplevel__assigns_toplevel_role);
	RUN_TEST(test_xdg_surface_get_toplevel__cannot_change_existing_role);
	RUN_TEST(test_xdg_surface_get_toplevel__can_reassign_same_role);
	RUN_TEST(test_xdg_surface_get_toplevel__resource_failed);

	// Test bz_xdg_surface_get_popup()
	// RUN_TEST(test_xdg_surface_get_popup__);
	// TODO

	// Test bz_xdg_surface_set_window_geometry()
	// RUN_TEST(test_xdg_surface_set_window_geometry__);
	// TODO

	// Test bz_xdg_surface_ack_configure()
	RUN_TEST(test_xdg_surface_ack_configure__state_is_updated);
	RUN_TEST(test_xdg_surface_ack_configure__consumes_previous_serials);
	RUN_TEST(test_xdg_surface_ack_configure__consumed_serial_causes_invalid_serial_error);

	return UNITY_END();
}
