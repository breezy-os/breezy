#include <stdint.h>

#include <wayland-server.h>

#include "fff.h"

DEFINE_FFF_GLOBALS

// -- wl_client --

/* wl_client_get_user_data  */ FAKE_VALUE_FUNC(void *, wl_client_get_user_data, struct wl_client *)
/* wl_client_post_no_memory */ FAKE_VOID_FUNC(wl_client_post_no_memory, struct wl_client *)

// -- wl_resource --

/* wl_resource_add_destroy_listener */ FAKE_VOID_FUNC(wl_resource_add_destroy_listener, struct wl_resource *, struct wl_listener *)
/* wl_resource_create               */ FAKE_VALUE_FUNC(struct wl_resource *, wl_resource_create, struct wl_client *, const struct wl_interface *, int, uint32_t)
/* wl_resource_destroy              */ FAKE_VOID_FUNC(wl_resource_destroy, struct wl_resource *)
/* wl_resource_get_client           */ FAKE_VALUE_FUNC(struct wl_client *, wl_resource_get_client, struct wl_resource *)
/* wl_resource_get_user_data        */ FAKE_VALUE_FUNC(void *, wl_resource_get_user_data, struct wl_resource *)
/* wl_resource_post_error           */ FAKE_VOID_FUNC_VARARG(wl_resource_post_error, struct wl_resource *, uint32_t, const char *, ...)
/* wl_resource_post_event           */ FAKE_VOID_FUNC_VARARG(wl_resource_post_event, struct wl_resource *, uint32_t, ...)
/* wl_resource_set_implementation   */ FAKE_VOID_FUNC(wl_resource_set_implementation, struct wl_resource *, const void *, void *, wl_resource_destroy_func_t)


void bz_reset_fakes()
{
	// Client
	RESET_FAKE(wl_client_get_user_data);
	RESET_FAKE(wl_client_post_no_memory);

	// Resource
	RESET_FAKE(wl_resource_add_destroy_listener);
	RESET_FAKE(wl_resource_create);
	RESET_FAKE(wl_resource_destroy);
	RESET_FAKE(wl_resource_get_client);
	RESET_FAKE(wl_resource_get_user_data);
	RESET_FAKE(wl_resource_post_error);
	RESET_FAKE(wl_resource_post_event);
	RESET_FAKE(wl_resource_set_implementation);
	FFF_RESET_HISTORY();
}