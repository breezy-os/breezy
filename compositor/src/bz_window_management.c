
#include "breezy/bz_window_management.h"

#include <wayland-server.h>
#include <xdg-shell-server-protocol.h>
#include <xkbcommon/xkbcommon.h>

#include "breezy/bz_wayland.h"
#include "breezy/bz_list.h"
#include "breezy/bz_logger.h"
#include "breezy/bz_wl_devices.h"
#include "breezy/bz_xdg_shell.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

static void bz_mgmt_untrack_surface_on_destroy(struct wl_listener *listener, void *resource);


// =================================================================================================
//  Initialization / Teardown
// -------------------------------------------------------------------------------------------------

int bz_mgmt_initialize(struct bz_window_mgmt *mgmt)
{
	mgmt->activable_surfaces = bz_list_create();
	return 0;
}

void bz_mgmt_cleanup(struct bz_window_mgmt *mgmt)
{
	if (mgmt->activable_surfaces != nullptr) {
		bz_list_free(mgmt->activable_surfaces, nullptr);
	}
}


// =================================================================================================
//  Window Lifecycle
// -------------------------------------------------------------------------------------------------

int bz_mgmt_open_window(struct bz_window_mgmt *mgmt, struct bz_surface *surface)
{
	struct wl_client *client = wl_resource_get_client(surface->resource);
	struct bz_client *client_data = wl_client_get_user_data(client);

	// Remove it when the surface is destroyed.
	struct wl_resource *res = nullptr;
	if (surface->role == BZ_SURF_ROLE_XDG_TOPLEVEL) {
		res = surface->xdgtoplevel->resource;
	} else {
		bz_error(BZ_LOG_WINDOW_MGMT, __FILE__, __LINE__,
			"Unsupported surface role in window manager: %d", surface->role);
		return -1;
	}
	surface->disable_on_destroy.notify = bz_mgmt_untrack_surface_on_destroy;
	wl_resource_add_destroy_listener(res, &surface->disable_on_destroy);

	// Add it to our list of activable surfaces
	struct bz_surface *last_focus = mgmt->activable_surfaces->length > 0
		? mgmt->activable_surfaces->tail->data
		: nullptr;
	const int append_status = bz_list_append(mgmt->activable_surfaces, surface);
	if (append_status == -2) {
		wl_client_post_no_memory(client);
	}

	// Send our "leave" event for our previously active surface
	if (last_focus != nullptr) {
		struct wl_client *last_client = wl_resource_get_client(last_focus->resource);
		struct bz_client *last_client_data = wl_client_get_user_data(last_client);
		struct bz_node *node = last_client_data->seat->keyboards->head;
		while (node != nullptr) {
			struct wl_resource *keyboard = node->data;
			uint32_t serial = wl_display_next_serial(wl_client_get_display(last_client));
			wl_keyboard_send_leave(keyboard, serial, last_focus->resource);
			node = node->next;
		}
	}

	// Send our "enter" event for our new active surface
	struct bz_node *node = client_data->seat->keyboards->head;
	while (node != nullptr) {
		struct wl_resource *keyboard = node->data;
		bz_mgmt_notify_enter(client_data->breezy->input.xkb_state, keyboard, surface->resource);
		node = node->next;
	}

	return append_status;
}

int bz_mgmt_close_active_window(struct bz_window_mgmt *mgmt)
{
	if (mgmt->activable_surfaces->length == 0) {
		bz_info(BZ_LOG_WINDOW_MGMT, __FILE__, __LINE__, "No surfaces to terminate.");
		return 0;
	}
	struct bz_surface *surf_data = mgmt->activable_surfaces->tail->data;
	if (surf_data->role == BZ_SURF_ROLE_XDG_TOPLEVEL) {
		xdg_toplevel_send_close(surf_data->xdgtoplevel->resource);
	} else {
		bz_error(BZ_LOG_WINDOW_MGMT, __FILE__, __LINE__, "Unrecognized surface role when closing.");
		return -1;
	}

	return 0;
}

/** Handles surfaces closing by removing the surface from our global tracking list. */
static void bz_mgmt_untrack_surface_on_destroy(struct wl_listener *listener, void *resource)
{
	bz_debug(BZ_LOG_WINDOW_MGMT, __FILE__, __LINE__, "Handling surface destroy.");

	// Collect some useful references
	struct bz_surface *surface_data = wl_container_of(listener, surface_data, disable_on_destroy);
	struct wl_client *client = wl_resource_get_client(resource);
	struct bz_client *client_data = wl_client_get_user_data(client);
	struct bz_breezy *breezy = client_data->breezy;

	// Remove the bz_surface from our globally-tracked list of activable surfaces
	bool last_item = breezy->window_mgmt.activable_surfaces->tail->data == surface_data;
	bz_list_remove(breezy->window_mgmt.activable_surfaces, surface_data, nullptr);

	// Send our "enter" event for our new active surface
	if (last_item && breezy->window_mgmt.activable_surfaces->length > 0) {
		struct bz_surface *new_surf_data = breezy->window_mgmt.activable_surfaces->tail->data;
		struct wl_client *new_client = wl_resource_get_client(new_surf_data->resource);
		struct bz_client *new_client_data = wl_client_get_user_data(new_client);
		struct bz_node *node = new_client_data->seat->keyboards->head;
		while (node != nullptr) {
			struct wl_resource *keyboard = node->data;
			bz_mgmt_notify_enter(breezy->input.xkb_state, keyboard, new_surf_data->resource);
			node = node->next;
		}
	}
}

struct bz_surface *bz_mgmt_get_active_surface(struct bz_window_mgmt *mgmt)
{
	return mgmt->activable_surfaces->length > 0
		? mgmt->activable_surfaces->tail->data
		: nullptr;
}

void bz_mgmt_notify_enter(
	struct xkb_state *xkbstate,
	struct wl_resource *keyboard,
	struct wl_resource *surface
) {
	struct wl_client *client = wl_resource_get_client(keyboard);
	struct wl_display *display = wl_client_get_display(client);

	struct wl_array keys;
	wl_array_init(&keys);
	wl_keyboard_send_enter(keyboard, wl_display_next_serial(display), surface, &keys);
	wl_keyboard_send_modifiers(
		keyboard,
		wl_display_next_serial(display),
		xkb_state_serialize_mods(xkbstate,   XKB_STATE_MODS_DEPRESSED),
		xkb_state_serialize_mods(xkbstate,   XKB_STATE_MODS_LATCHED),
		xkb_state_serialize_mods(xkbstate,   XKB_STATE_MODS_LOCKED),
		xkb_state_serialize_layout(xkbstate, XKB_STATE_LAYOUT_EFFECTIVE)
	);
	wl_array_release(&keys);
}
