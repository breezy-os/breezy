
#include "breezy/bz_window_management.h"

#include <wayland-server.h>
#include <xdg-shell-server-protocol.h>
#include <xkbcommon/xkbcommon.h>

#include "breezy/bz_wayland.h"
#include "breezy/bz_list.h"
#include "breezy/bz_logger.h"
#include "breezy/bz_wl_devices.h"
#include "breezy/bz_xdg_shell.h"
#include "breezy/bz_breezy.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

static void bz_mgmt_untrack_surface_on_destroy(struct wl_listener *listener, void *resource);

static void bz_mgmt_change_keyboard_focus(struct bz_window_mgmt *mgmt, struct bz_surface *orig_surface, struct bz_surface *new_surface);
static void bz_mgmt_change_pointer_focus(struct bz_window_mgmt *mgmt, struct bz_surface *orig_surface, struct bz_surface *new_surface);


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

void bz_mgmt_open_window(struct bz_window_mgmt *mgmt, struct bz_surface *surface)
{
	// Make sure it's a supported surface role
	struct wl_resource *res = nullptr;
	if (surface->role == BZ_SURF_ROLE_XDG_TOPLEVEL) {
		res = surface->xdgtoplevel->resource;
	} else {
		bz_warn(BZ_LOG_WINDOW_MGMT, "Unsupported surface role in window manager: %d", surface->role);
		return;
	}

	// Make sure it wasn't already mapped / is being displayed.
	if (bz_list_contains(mgmt->activable_surfaces, surface)) {
		return;
	}

	// Remove it when the surface is destroyed.
	surface->disable_on_destroy.notify = bz_mgmt_untrack_surface_on_destroy;
	wl_resource_add_destroy_listener(res, &surface->disable_on_destroy);

	// Focus the new window
	struct bz_surface *orig_surf = bz_mgmt_get_active_surface(mgmt);
	bz_mgmt_change_keyboard_focus(mgmt, orig_surf, surface);

	// Also update our pointer's position to be in the window (which also adjusts its focus)
	struct wl_client *client = wl_resource_get_client(surface->resource);
	struct bz_client *client_data = wl_client_get_user_data(client);
	struct bz_renderable *cursor = &client_data->breezy->gl.cursor;
	cursor->position.x = surface->renderable.position.x;
	cursor->position.y = surface->renderable.position.y;
	bz_mgmt_update_pointer_position(mgmt, cursor);
}

int bz_mgmt_close_active_window(struct bz_window_mgmt *mgmt)
{
	if (mgmt->activable_surfaces->length == 0) {
		bz_info(BZ_LOG_WINDOW_MGMT, "No surfaces to terminate.");
		return 0;
	}
	struct bz_surface *surf_data = mgmt->activable_surfaces->tail->data;
	if (surf_data->role == BZ_SURF_ROLE_XDG_TOPLEVEL) {
		xdg_toplevel_send_close(surf_data->xdgtoplevel->resource);
	} else {
		bz_error(BZ_LOG_WINDOW_MGMT, "Unrecognized surface role when closing.");
		return -1;
	}

	return 0;
}

/** Handles surfaces closing by removing the surface from our global tracking list. */
static void bz_mgmt_untrack_surface_on_destroy(struct wl_listener *listener, void *resource)
{
	bz_debug(BZ_LOG_WINDOW_MGMT, "Handling surface destroy.");

	// Collect some useful references
	struct bz_surface *surface_data = wl_container_of(listener, surface_data, disable_on_destroy);
	struct wl_client *client = wl_resource_get_client(resource);
	struct bz_client *client_data = wl_client_get_user_data(client);
	struct bz_breezy *breezy = client_data->breezy;

	// Remove the bz_surface from our globally-tracked list of activable surfaces
	bool was_last_item = breezy->window_mgmt.activable_surfaces->tail->data == surface_data;
	bz_list_remove(breezy->window_mgmt.activable_surfaces, surface_data, nullptr);
	breezy->window_mgmt.pointer_focus = nullptr;

	// If we removed the surface that had keyboard focus, then look for a new surface to focus.
	if (was_last_item && breezy->window_mgmt.activable_surfaces->length > 0) {
		// First, try the window where our mouse cursor is (if applicable)
		struct bz_surface *new_focus = bz_mgmt_update_pointer_position(
			&breezy->window_mgmt,
			&breezy->gl.cursor
		);
		// Found a surface! Also update our keyboard's focus, then early exit. The code inside
		//   bz_mgmt_update_pointer_position isn't sufficient if the window gaining focus is
		//   already the new "last item" in the list.
		if (new_focus != nullptr) {
			bz_mgmt_change_keyboard_focus(&breezy->window_mgmt, nullptr, new_focus);
			return;
		}

		// If that fails, then let's just grab the last window in our list. No need to set pointer
		//   focus because we don't want to move the user's mouse on them in this situation.
		struct bz_surface *new_surf_data = breezy->window_mgmt.activable_surfaces->tail->data;
		struct wl_client *new_client = wl_resource_get_client(new_surf_data->resource);
		struct bz_client *new_client_data = wl_client_get_user_data(new_client);
		struct wl_display *display = new_client_data->breezy->wayland.display;
		struct xkb_state *xkbstate = new_client_data->breezy->input.xkb_state;
		uint32_t enter_serial = wl_display_next_serial(display);
		uint32_t modifiers_serial = wl_display_next_serial(display);
		struct bz_wl_seat *seat; bz_list_foreach(seat, new_client_data->seats) {
			struct wl_resource *keyboard; bz_list_foreach(keyboard, seat->keyboards) {
				bz_mgmt_notify_kb_enter(
					enter_serial,
					modifiers_serial,
					xkbstate,
					keyboard,
					new_surf_data->resource
				);
			}
		}
	}
}

void bz_mgmt_notify_kb_enter(
	uint32_t enter_serial,
	uint32_t modifiers_serial,
	struct xkb_state *xkbstate,
	struct wl_resource *keyboard,
	struct wl_resource *surface
) {
	struct wl_array keys;
	wl_array_init(&keys); // TODO-dl??: Populate this properly
	wl_keyboard_send_enter(keyboard, enter_serial, surface, &keys);
	wl_keyboard_send_modifiers(
		keyboard,
		modifiers_serial,
		xkb_state_serialize_mods(xkbstate,   XKB_STATE_MODS_DEPRESSED),
		xkb_state_serialize_mods(xkbstate,   XKB_STATE_MODS_LATCHED),
		xkb_state_serialize_mods(xkbstate,   XKB_STATE_MODS_LOCKED),
		xkb_state_serialize_layout(xkbstate, XKB_STATE_LAYOUT_EFFECTIVE)
	);
	wl_array_release(&keys);
}


// =================================================================================================
//  Window Focus
// -------------------------------------------------------------------------------------------------

struct bz_surface *bz_mgmt_get_active_surface(struct bz_window_mgmt *mgmt)
{
	return mgmt->activable_surfaces->length > 0
		? mgmt->activable_surfaces->tail->data
		: nullptr;
}

static void bz_mgmt_change_keyboard_focus(
	struct bz_window_mgmt *mgmt,
	struct bz_surface *orig_surface,
	struct bz_surface *new_surface
) {
	bz_info(BZ_LOG_WINDOW_MGMT, "Keyboard focus changing: %p to %p", orig_surface, new_surface);

	if (new_surface == nullptr) {
		bz_error(BZ_LOG_WINDOW_MGMT, "New surface must be defined when changing keyboard focus.");
		return;
	}

	// Move the new surface to the end of our active surface list
	const int move_result = bz_list_move_item_to_end(mgmt->activable_surfaces, new_surface);
	if (move_result == -2) {
		// Surface wasn't in the list -- must've just been created. Append it to the list.
		const int append_status = bz_list_append(mgmt->activable_surfaces, new_surface);
		if (append_status < 0) {
			bz_error(BZ_LOG_WINDOW_MGMT,
				"Failed appending the surface to our surface list. %d", append_status);
			return;
		}
	} else if (move_result < 0) {
		bz_error(BZ_LOG_WINDOW_MGMT,
			"Failed moving the surface to the end of our surface list. %d", move_result);
		return;
	}

	// Iterate over each keyboard resource of the original client, emitting "leave" events
	if (orig_surface != nullptr) {
		struct wl_client *orig_client = wl_resource_get_client(orig_surface->resource);
		struct bz_client *orig_client_data = wl_client_get_user_data(orig_client);
		uint32_t serial = wl_display_next_serial(orig_client_data->breezy->wayland.display);
		struct bz_wl_seat *seat; bz_list_foreach(seat, orig_client_data->seats) {
			struct wl_resource *keyboard; bz_list_foreach(keyboard, seat->keyboards) {
				wl_keyboard_send_leave(keyboard, serial, orig_surface->resource);
			}
		}
	}

	// Iterate over each keyboard resource of the new client, emitting "enter" events
	struct wl_client *new_client = wl_resource_get_client(new_surface->resource);
	struct bz_client *new_client_data = wl_client_get_user_data(new_client);
	struct wl_display *display = new_client_data->breezy->wayland.display;
	struct xkb_state *xkbstate = new_client_data->breezy->input.xkb_state;
	uint32_t enter_serial = wl_display_next_serial(display);
	uint32_t modifiers_serial = wl_display_next_serial(display);
	struct bz_wl_seat *seat; bz_list_foreach(seat, new_client_data->seats) {
		struct wl_resource *keyboard; bz_list_foreach(keyboard, seat->keyboards) {
			bz_mgmt_notify_kb_enter(enter_serial, modifiers_serial, xkbstate, keyboard, new_surface->resource);
		}
	}
}

static void bz_mgmt_change_pointer_focus(
	struct bz_window_mgmt *mgmt,
	struct bz_surface *orig_surface,
	struct bz_surface *new_surface
) {
	bz_info(BZ_LOG_WINDOW_MGMT, "Pointer focus changing: %p to %p", orig_surface, new_surface);

	// Iterate over each pointer resource of the original client, emitting "leave" events
	if (orig_surface != nullptr) {
		struct wl_client *orig_client = wl_resource_get_client(orig_surface->resource);
		struct bz_client *orig_client_data = wl_client_get_user_data(orig_client);
		struct wl_display *display = orig_client_data->breezy->wayland.display;
		uint32_t serial = wl_display_next_serial(display);
		struct bz_wl_seat *seat; bz_list_foreach(seat, orig_client_data->seats) {
			struct wl_resource *pointer; bz_list_foreach(pointer, seat->pointers) {
				wl_pointer_send_leave(pointer, serial, orig_surface->resource);
				wl_pointer_send_frame(pointer);
			}
		}
	}

	// Iterate over each pointer resource of the new client, emitting "enter" events
	if (new_surface != nullptr) {
		struct wl_client *new_client = wl_resource_get_client(new_surface->resource);
		struct bz_client *new_client_data = wl_client_get_user_data(new_client);
		int32_t x_pos = mgmt->last_cursor_loc.x - new_surface->renderable.position.x;
		int32_t y_pos = mgmt->last_cursor_loc.y - new_surface->renderable.position.y;
		uint32_t serial = wl_display_next_serial(new_client_data->breezy->wayland.display);
		new_client_data->last_enter_serial = serial;
		struct bz_wl_seat *seat; bz_list_foreach(seat, new_client_data->seats) {
			struct wl_resource *pointer; bz_list_foreach(pointer, seat->pointers) {
				wl_pointer_send_enter(pointer, serial, new_surface->resource, x_pos, y_pos);
				wl_pointer_send_frame(pointer);
			}
		}
	}

	// Update our tracked pointer focus
	mgmt->pointer_focus = new_surface;
}

struct bz_surface *bz_mgmt_update_pointer_position(struct bz_window_mgmt *mgmt, struct bz_renderable *cursor)
{
	mgmt->last_cursor_loc.x = cursor->position.x - cursor->offset.x;
	mgmt->last_cursor_loc.y = cursor->position.y - cursor->offset.y;

	// Iterate over our list, BACKWARDS, until we overlap with a window (or run out of items)
	struct bz_surface *surf; bz_list_foreach_rev(surf, mgmt->activable_surfaces) {
		struct bz_renderable surf_rect = surf->renderable;

		// Check if cursor is overlapping
		if (bz_contains_point(&surf_rect.position, &surf_rect.size, &mgmt->last_cursor_loc)) {
			// If it doesn't already have pointer focus, send an enter event for pointers.
			if (mgmt->pointer_focus != surf) {
				bz_mgmt_change_pointer_focus(mgmt, mgmt->pointer_focus, surf);
			}

			// If it doesn't have keyboard focus, send an enter event for keyboards.
			struct bz_surface *original_focus = bz_mgmt_get_active_surface(mgmt);
			if (original_focus != surf) {
				bz_mgmt_change_keyboard_focus(mgmt, original_focus, surf);
			}

			return surf;
		}
	}

	// If we made it here, then our cursor is not overlapping ANY window. Let's clear its focus.
	if (mgmt->pointer_focus != nullptr) {
		bz_mgmt_change_pointer_focus(mgmt, mgmt->pointer_focus, nullptr);
	}
	return nullptr;
}