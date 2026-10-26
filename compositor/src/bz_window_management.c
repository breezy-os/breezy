
#include "breezy/bz_window_management.h"

#include <limits.h>
#include <stdlib.h>

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

static void bz_mgmt_free_section_recursively(struct bz_section *section);

static void bz_mgmt_untrack_surface_on_destroy(struct wl_listener *listener, void *resource);

static void bz_mgmt_change_keyboard_focus(struct bz_window_mgmt *mgmt, struct bz_surface *orig_surface, struct bz_surface *new_surface);
static void bz_mgmt_change_pointer_focus(struct bz_window_mgmt *mgmt, struct bz_surface *orig_surface, struct bz_surface *new_surface);
static void bz_mgmt_update_focus_for_full_tree(struct bz_surface *surface, bool is_focused);
static void bz_mgmt_update_focus_for_children(struct bz_surface *surface, bool is_focused);

static struct bz_surface *bz_mgmt_check_surface_overlap(struct bz_window_mgmt *mgmt, struct bz_surface *root_surf, struct bz_surface *surface);


// =================================================================================================
//  Initialization / Teardown
// -------------------------------------------------------------------------------------------------

int bz_mgmt_initialize(struct bz_breezy *breezy)
{
	struct bz_window_mgmt *mgmt = &breezy->window_mgmt;

	mgmt->activable_surfaces = bz_list_create();
	if (mgmt->activable_surfaces == nullptr) {
		bz_error(BZ_LOG_WINDOW_MGMT, "Failed to allocate space for activable surfaces.");
		return 1;
	}

	mgmt->root_tile = calloc(1, sizeof(*mgmt->root_tile));
	if (mgmt->root_tile == nullptr) {
		bz_error(BZ_LOG_WINDOW_MGMT, "Failed to allocate space for root tile.");
		return 2;
	}
	mgmt->root_tile->bounds = breezy->drm.current_output->compositor_bounds;
	mgmt->root_tile->type = BZ_SECTION_UNINITIALIZED;

	return 0;
}

void bz_mgmt_cleanup(struct bz_window_mgmt *mgmt)
{
	if (mgmt->root_tile != nullptr) {
		bz_mgmt_free_section_recursively(mgmt->root_tile);
	}
	if (mgmt->activable_surfaces != nullptr) {
		bz_list_free(mgmt->activable_surfaces, nullptr);
	}
}

static void bz_mgmt_free_section_recursively(struct bz_section *section)
{
	switch (section->type) {
	case BZ_SECTION_UNINITIALIZED:
	case BZ_SECTION_WINDOW:
		free(section);
		break;
	case BZ_SECTION_SPLIT:
		struct bz_section *nested; bz_list_foreach(nested, section->split->sections) {
			bz_mgmt_free_section_recursively(nested);
		}
		bz_list_free(section->split->sections, nullptr);
		break;
	}
}


// =================================================================================================
//  Window Lifecycle
// -------------------------------------------------------------------------------------------------

void bz_mgmt_place_window(struct bz_breezy *breezy, struct bz_surface *surface)
{
	struct bz_user_config *config = &breezy->user_config;
	struct bz_window_mgmt *mgmt = &breezy->window_mgmt;
	// TODO-dl13: Take into consideration min/max size
	// if (bzsurf->active_state->min_size) {
	// 	if (bzsurf->active_state->min_size->w > 0) {
	// 		rec_size.w = bz_max(rec_size.w, bzsurf->active_state->min_size->w);
	// 	}
	// 	if (bzsurf->active_state->min_size->h > 0) {
	// 		rec_size.h = bz_max(rec_size.h, bzsurf->active_state->min_size->h);
	// 	}
	// }
	// if (bzsurf->active_state->max_size) {
	// 	if (bzsurf->active_state->max_size->w > 0) {
	// 		rec_size.w = bz_min(rec_size.w, bzsurf->active_state->max_size->w);
	// 	}
	// 	if (bzsurf->active_state->max_size->h > 0) {
	// 		rec_size.h = bz_min(rec_size.h, bzsurf->active_state->max_size->h);
	// 	}
	// }

	// If it's the first tile, return the full bounds
	if (mgmt->root_tile->type == BZ_SECTION_UNINITIALIZED) {
		mgmt->root_tile->type = BZ_SECTION_WINDOW;
		mgmt->root_tile->window = surface;
		surface->section = mgmt->root_tile;
		return;
	}

	// Otherwise, split the active window
	struct bz_surface *active_surf = bz_mgmt_get_active_surface(mgmt);
	// TODO-dl13 - if trying to split when no active window, add to root tile as a fallback.
	// if (active_surf == nullptr) {
	// 	// This really shouldn't come up, but just in case, it's a fallback.
	// 	bz_mgmt_add_to_section_after(mgmt->root_tile, )
	// }
	struct bz_section *target_section = active_surf->section;
	enum bz_split_direction should_split_in = target_section->bounds.w > target_section->bounds.h
		? BZ_SPLIT_DIR_ROW : BZ_SPLIT_DIR_COLUMN;
	if (target_section->parent != nullptr && target_section->parent->direction == should_split_in) {
		// Add to the existing split list of the parent
		bz_mgmt_create_window_after_section(config, target_section->parent->section, surface, target_section);
	} else {
		// Create a new split list at the current node
		bz_mgmt_create_window_after_section(config, target_section, surface, target_section);
	}
}

void bz_mgmt_open_window(struct bz_window_mgmt *mgmt, struct bz_surface *surface)
{
	// Make sure it wasn't already mapped / is being displayed.
	if (bz_list_contains(mgmt->activable_surfaces, surface)) {
		return;
	}

	// Make sure it's a supported surface role
	if (surface->role == BZ_SURF_ROLE_XDG_TOPLEVEL) {
		struct wl_resource *toplevel_res = surface->xdgtoplevel->resource;

		// Remove it when the surface is destroyed.
		surface->disable_on_destroy.notify = bz_mgmt_untrack_surface_on_destroy;
		wl_resource_add_destroy_listener(toplevel_res, &surface->disable_on_destroy);

		// Focus the new window
		struct bz_surface *orig_surf = bz_mgmt_get_active_surface(mgmt);
		bz_mgmt_change_keyboard_focus(mgmt, orig_surf, surface);

		// Also update our pointer's position to be in the window (which also adjusts its focus)
		struct wl_client *client = wl_resource_get_client(surface->resource);
		struct bz_client *client_data = wl_client_get_user_data(client);
		struct bz_renderable *cursor = &client_data->breezy->gl.cursor;
		cursor->compositor_bounds.x = surface->renderable.compositor_bounds.x + surface->renderable.compositor_bounds.w/2;
		cursor->compositor_bounds.y = surface->renderable.compositor_bounds.y + surface->renderable.compositor_bounds.h/2;
		bz_mgmt_update_pointer_position(mgmt, cursor);

	} else if (surface->role == BZ_SURF_ROLE_XDG_POPUP) {
		struct wl_resource *popup_res = surface->xdgpopup->resource;

		// Remove it when the surface is destroyed.
		surface->disable_on_destroy.notify = bz_mgmt_untrack_surface_on_destroy;
		wl_resource_add_destroy_listener(popup_res, &surface->disable_on_destroy);

		// Add it as the focused surface, clearing the previously focused surface
		if (bz_mgmt_get_active_surface(mgmt) != nullptr) {
			bz_mgmt_update_focus_for_full_tree(bz_mgmt_get_active_surface(mgmt), false);
		}
		bz_list_append(mgmt->activable_surfaces, surface);
		bz_mgmt_update_focus_for_full_tree(surface, true);
	} else {
		bz_warn(BZ_LOG_WINDOW_MGMT, "Non-toplevel surface provided in window manager: %d", surface->role);
		return;
	}
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
	} else if (surf_data->role == BZ_SURF_ROLE_XDG_POPUP) {
		struct bz_surface *toplevel = bz_popup_get_associated_toplevel(surf_data);
		if (toplevel != nullptr) {
			xdg_toplevel_send_close(toplevel->xdgtoplevel->resource);
		}
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

	// Remove the bz_section from our tile manager.
	if (surface_data->section != nullptr) {
		bz_mgmt_delete_section(&breezy->user_config, surface_data->section);
	}

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
		bz_mgmt_update_focus_for_full_tree(new_surf_data, true);
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

		// Make all surfaces belonging to this stack unfocused
		bz_mgmt_update_focus_for_full_tree(orig_surface, false);
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
	// Make all surfaces belonging to this stack focused
	bz_mgmt_update_focus_for_full_tree(new_surface, true);
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
				if (wl_resource_get_version(pointer) >= WL_POINTER_FRAME_SINCE_VERSION) {
					wl_pointer_send_frame(pointer);
				}
			}
		}
	}

	// Iterate over each pointer resource of the new client, emitting "enter" events
	if (new_surface != nullptr) {
		struct wl_client *new_client = wl_resource_get_client(new_surface->resource);
		struct bz_client *new_client_data = wl_client_get_user_data(new_client);
		wl_fixed_t x_pos = wl_fixed_from_int(mgmt->last_cursor_loc.x - new_surface->renderable.compositor_bounds.x);
		wl_fixed_t y_pos = wl_fixed_from_int(mgmt->last_cursor_loc.y - new_surface->renderable.compositor_bounds.y);
		uint32_t serial = wl_display_next_serial(new_client_data->breezy->wayland.display);
		new_client_data->last_enter_serial = serial;
		struct bz_wl_seat *seat; bz_list_foreach(seat, new_client_data->seats) {
			struct wl_resource *pointer; bz_list_foreach(pointer, seat->pointers) {
				wl_pointer_send_enter(pointer, serial, new_surface->resource, x_pos, y_pos);
				if (wl_resource_get_version(pointer) >= WL_POINTER_FRAME_SINCE_VERSION) {
					wl_pointer_send_frame(pointer);
				}
			}
		}
	}

	// Update our tracked pointer focus
	mgmt->pointer_focus = new_surface;
}

static void bz_mgmt_update_focus_for_full_tree(struct bz_surface *surface, bool is_focused)
{
	struct bz_surface *root_surf = bz_get_root_surface(surface);
	if (root_surf != nullptr) { // Should never be null.
		bz_mgmt_update_focus_for_children(root_surf, is_focused);
	}
}

static void bz_mgmt_update_focus_for_children(struct bz_surface *surface, bool is_focused)
{
	// First, set the focus for each item in the surface stack.
	struct bz_surface *stack_item; bz_list_foreach(stack_item, surface->active_state->surface_stack) {
		if (stack_item == surface) {
			stack_item->appears_focused = is_focused;
		} else {
			bz_mgmt_update_focus_for_children(stack_item, is_focused);
		}
	}

	// Next, set the focus for each popup.
	struct bz_xdg_popup *popup; bz_list_foreach(popup, surface->popups) {
		bz_mgmt_update_focus_for_children(popup->wlsurface, is_focused);
	}
}

/**
 * Returns the ROOT-level surface containing the cursor.
 */
struct bz_surface *bz_mgmt_update_pointer_position(
	struct bz_window_mgmt *mgmt,
	struct bz_renderable *cursor
) {
	mgmt->last_cursor_loc.x = cursor->compositor_bounds.x;
	mgmt->last_cursor_loc.y = cursor->compositor_bounds.y;

	// Iterate over our list in reverse render order until we either overlap with a window or run out of items.
	struct bz_surface *surf; bz_list_foreach_rev(surf, mgmt->activable_surfaces) {
		// We treat toplevels as "the root of all windows". Popups and subsurfaces are checked through them.
		if (surf->role != BZ_SURF_ROLE_XDG_TOPLEVEL) { continue; }

		// Check each surface belonging to this toplevel
		struct bz_surface *target = bz_mgmt_check_surface_overlap(mgmt, surf, surf);
		if (target != nullptr) {
			return target;
		}
	}

	// If we made it here, then our cursor is not overlapping ANY window. Let's clear its focus.
	if (mgmt->pointer_focus != nullptr) {
		bz_mgmt_change_pointer_focus(mgmt, mgmt->pointer_focus, nullptr);
	}
	return nullptr;
}

void bz_mgmt_focus_window(struct bz_breezy *breezy, struct bz_surface *surface)
{
	struct bz_surface *current_focus = bz_mgmt_get_active_surface(&breezy->window_mgmt);
	if (current_focus == surface) {
		return; // Nothing to do.
	}
	if (surface == nullptr) {
		bz_warn(BZ_LOG_WINDOW_MGMT, "Attempted to focus null surface.");
		return; // Nothing to do.
	}

	// Update cursor position and focus
	struct bz_position new_cursor_location = {
		.x = surface->section->bounds.x + surface->section->bounds.w/2,
		.y = surface->section->bounds.y + surface->section->bounds.h/2,
	};
	breezy->gl.cursor.compositor_bounds.x = new_cursor_location.x;
	breezy->gl.cursor.compositor_bounds.y = new_cursor_location.y;
	breezy->window_mgmt.last_cursor_loc = new_cursor_location;
	bz_mgmt_change_pointer_focus(&breezy->window_mgmt, breezy->window_mgmt.pointer_focus, nullptr);

	// Update keyboard focus
	bz_mgmt_change_keyboard_focus(&breezy->window_mgmt, current_focus, surface);
}

static struct bz_surface *bz_mgmt_check_surface_overlap(
	struct bz_window_mgmt *mgmt,
	struct bz_surface *root_surf,
	struct bz_surface *surface
) {
	// First, check each popup. Popups are considered valid "root_surfs" for keyboard focus.
	struct bz_xdg_popup *popup; bz_list_foreach_rev(popup, surface->popups) {
		// If the popup was *just* removed, we don't want to accidentally reconsider it here.
		if (!bz_list_contains(mgmt->activable_surfaces, popup->wlsurface)) {
			continue;
		}

		struct bz_surface *overlap = bz_mgmt_check_surface_overlap(mgmt, popup->wlsurface, popup->wlsurface);
		if (overlap != nullptr) {
			return overlap;
		}
	}

	// Then, check each item in the stack, which includes both the main "surface" and its subsurfaces>
	struct bz_surface *stack_item; bz_list_foreach_rev(stack_item, surface->active_state->surface_stack) {
		if (stack_item != surface) {
			struct bz_surface *overlap = bz_mgmt_check_surface_overlap(mgmt, root_surf, stack_item);
			if (overlap != nullptr) {
				return overlap;
			}

		} else {
			// Check if cursor is overlapping
			bool contains_cursor = bz_region_contain_point(
				&stack_item->renderable.render_bounds,
				stack_item->active_state->input_region,
				&mgmt->last_cursor_loc
			);
			if (contains_cursor) {
				// If it doesn't already have pointer focus, send an enter event for pointers.
				if (mgmt->pointer_focus != stack_item) {
					bz_mgmt_change_pointer_focus(mgmt, mgmt->pointer_focus, stack_item);
				}

				// TODO-dl13-pu: ...popups can have kb focus, but I don't want to REMOVE their focus
				//     until the mouse cursor has entered a totally different toplevel. ...I think.

				// If it doesn't have keyboard focus, send an enter event for keyboards.
				struct bz_surface *original_focus = bz_mgmt_get_active_surface(mgmt);
				struct bz_surface *new_kb_focus = stack_item->role == BZ_SURF_ROLE_WL_SUBSURFACE ? root_surf : stack_item;
				if (original_focus != new_kb_focus) {
					bz_mgmt_change_keyboard_focus(mgmt, original_focus, new_kb_focus);
				}

				return root_surf;
			}
		}
	}

	return nullptr;
}


// =================================================================================================
//  Structure and Layout
// -------------------------------------------------------------------------------------------------

struct bz_position bz_mgmt_get_coords_relative_to_active_window(struct bz_breezy *breezy, enum bz_direction direction)
{
	struct bz_surface *surface = bz_mgmt_get_active_surface(&breezy->window_mgmt);
	if (surface == nullptr) { goto return_0_0; }
	struct bz_section *section = surface->section;
	if (section == nullptr) { goto return_0_0; }

	switch (direction) {
	case BZ_UP:
		return (struct bz_position){
			.x = section->bounds.x + section->bounds.w/2,
			.y = section->bounds.y - breezy->user_config.tile_gap - 2
		};
	case BZ_DOWN:
		return (struct bz_position){
			.x = section->bounds.x + section->bounds.w/2,
			.y = section->bounds.y + section->bounds.h + breezy->user_config.tile_gap + 2
		};
	case BZ_LEFT:
		return (struct bz_position){
			.x = section->bounds.x - breezy->user_config.tile_gap - 2,
			.y = section->bounds.y + section->bounds.h/2
		};
	case BZ_RIGHT:
		return (struct bz_position){
			.x = section->bounds.x + section->bounds.w + breezy->user_config.tile_gap + 2,
			.y = section->bounds.y + section->bounds.h/2
		};
	default:
		bz_error(BZ_LOG_WINDOW_MGMT, "Unrecognized direction when getting coordinates. Returning 0,0.");
	}

	return_0_0:
		return (struct bz_position){ .x = 0, .y = 0 };
}

/**
 * Returns nullptr if there is no window within the given section that contains the coords. A leaf
 * is either a window or a divider.
 */
struct bz_section *bz_mgmt_get_leaf_at_coordinates(
	struct bz_section *root,
	struct bz_position coords
) {
	if (!bz_contains_point(&root->bounds, &coords) || root->type == BZ_SECTION_UNINITIALIZED) {
		return nullptr;
	}
	struct bz_section *leafmost_section = root;

	if (root->type == BZ_SECTION_WINDOW) {
		return root;
	}

	// Iterate over the child sections checking for overlap. Each time we drill deeper, update our
	// leafmost section.
	struct bz_section *inner; bz_list_foreach(inner, root->split->sections) {
		if (bz_contains_point(&inner->bounds, &coords)) {
			leafmost_section = bz_mgmt_get_leaf_at_coordinates(inner, coords);
		}
	}

	// If our leafmost section is a window or divider, return it as-is.
	if (leafmost_section->type == BZ_SECTION_WINDOW || leafmost_section->type == BZ_SECTION_DIVIDER) {
		return leafmost_section;
	}

	// Otherwise, no window contains the coordinates; we're in a gap. Create a new divider struct to
	// return, using the leafmost split to identify the neighboring windows.
	enum bz_split_direction split_dir = leafmost_section->split->direction;
	struct bz_divider *divider = calloc(1, sizeof(*divider));
	divider->direction = split_dir == BZ_SPLIT_DIR_COLUMN ? BZ_DIVIDER_HORIZONTAL : BZ_DIVIDER_VERTICAL;
	struct bz_section *divider_sect = calloc(1, sizeof(*divider_sect));
	divider_sect->parent = leafmost_section->split;
	divider_sect->type = BZ_SECTION_DIVIDER;
	divider_sect->divider = divider;
	divider->section = divider_sect;

	// Figure out the sections that surround the divider
	struct bz_section *subsect; bz_list_foreach(subsect, leafmost_section->split->sections) {
		if (
			(split_dir == BZ_SPLIT_DIR_COLUMN && subsect->bounds.y < coords.y) ||
			(split_dir == BZ_SPLIT_DIR_ROW && subsect->bounds.x < coords.x)
		) {
			divider->section_before = subsect;
		} else {
			divider->section_after = subsect;
			break;
		}
	}

	// Compute the bounds of the divider
	if (split_dir == BZ_SPLIT_DIR_COLUMN) {
		divider_sect->bounds = (struct bz_rect){
			.x = divider->section_before->bounds.x,
			.y = divider->section_before->bounds.y + divider->section_before->bounds.h,
			.w = divider->section_before->bounds.w,
			.h = divider->section_after->bounds.y - (divider->section_before->bounds.y + divider->section_before->bounds.h)
		};
	} else {
		divider_sect->bounds = (struct bz_rect){
			.x = divider->section_before->bounds.x + divider->section_before->bounds.w,
			.y = divider->section_before->bounds.y,
			.w = divider->section_after->bounds.x - (divider->section_before->bounds.x + divider->section_before->bounds.w),
			.h = divider->section_before->bounds.h
		};
	}

	return divider_sect;
}

struct bz_section *bz_mgmt_get_nearest_window(
	struct bz_section *section,
	struct bz_position coords
) {
	if (section->type == BZ_SECTION_WINDOW) {
		return section;
	}
	if (section->type == BZ_SECTION_DIVIDER) {
		struct bz_section *to_return = section->divider->section_before;
		bz_mgmt_free_section(section, false);
		return bz_mgmt_get_nearest_window(to_return, coords);
	}
	if (section->type == BZ_SECTION_SPLIT) {
		int min_dist = INT_MAX;
		struct bz_section *closest = nullptr;
		struct bz_section *inner; bz_list_foreach(inner, section->split->sections) {
			int distance = section->split->direction == BZ_SPLIT_DIR_ROW
				? abs(inner->bounds.x + inner->bounds.w/2 - coords.x)
				: abs(inner->bounds.y + inner->bounds.h/2 - coords.y);
			if (distance < min_dist || closest == nullptr) {
				closest = inner;
				min_dist = distance;
			} else {
				return bz_mgmt_get_nearest_window(closest, coords);
			}
		}
		// In case the final item in the loop is the closest.
		return bz_mgmt_get_nearest_window(closest, coords);
	}
	bz_error(BZ_LOG_WINDOW_MGMT, "Unsupported section given to get_nearest_window.");
	return nullptr;
}

struct bz_section *bz_mgmt_get_prior_section_in_split(
	struct bz_split *split,
	struct bz_position coords
) {
	struct bz_section *prev = nullptr;
	struct bz_section *inner; bz_list_foreach(inner, split->sections) {
		int delta = split->direction == BZ_SPLIT_DIR_ROW
			? inner->bounds.x + inner->bounds.w/2 - coords.x
			: inner->bounds.y + inner->bounds.h/2 - coords.y;
		if (delta > 0) { break; }
		prev = inner;
	}
	return prev;
}

/** Creates and returns the section, but doesn't add it anywhere. */
struct bz_section *bz_mgmt_create_new_window_section(
	struct bz_rect bounds,
	struct bz_surface *surface
) {
	struct bz_section *section = calloc(1, sizeof(*section));
	if (section == nullptr) {
		bz_error(BZ_LOG_WINDOW_MGMT, "Failed to allocate memory for a new section.");
		return nullptr;
	}
	section->parent = nullptr;
	section->bounds = bounds;
	section->type = BZ_SECTION_WINDOW;
	section->window = surface;
	surface->section = section;
	return section;
}

/**
 * The current root tile becomes a child to a new root tile. The new root tile is then returned. The
 * current root must be a split or else this method returns a nullptr and is considered an error.
 * The new split will be created in the opposite direction, and have only a single section in it.
 */
struct bz_section *bz_mgmt_convert_root_into_child_of_new_split(
	struct bz_window_mgmt *mgmt
) {
	struct bz_section *initial_root = mgmt->root_tile;
	if (initial_root->type != BZ_SECTION_SPLIT || initial_root->split == nullptr) {
		bz_error(BZ_LOG_WINDOW_MGMT, "Attempted to convert a non-split root into a child of a new split.");
		return nullptr;
	}

	struct bz_section *new_root = calloc(1, sizeof(*new_root));
	struct bz_split *new_split = calloc(1, sizeof(*new_split));
	new_root->bounds = initial_root->bounds;
	new_root->type = BZ_SECTION_SPLIT;
	new_root->split = new_split;
	new_split->section = new_root;
	new_split->direction = initial_root->split->direction == BZ_SPLIT_DIR_COLUMN ? BZ_SPLIT_DIR_ROW : BZ_SPLIT_DIR_COLUMN;
	new_split->sections = bz_list_create();
	bz_mgmt_link_to_parent_split(new_split, initial_root, nullptr);

	mgmt->root_tile = new_root;
	return new_root; // TODO-dl13: return status integer..?
}

/**
 * Converts the given section from a window into a split. If it's not a window, this is a no-op.
 */
void bz_mgmt_convert_window_to_split(struct bz_user_config *config, struct bz_section *section)
{
	// No-op if it's not a window.
	if (section->type != BZ_SECTION_WINDOW) {
		return;
	}

	// Create a new window section to house the original window.
	struct bz_section *new_window = bz_mgmt_create_new_window_section(section->bounds, section->window);

	// Convert the existing section into a split
	section->type = BZ_SECTION_SPLIT;
	section->split = calloc(1, sizeof(*section->split));
	if (section->parent != nullptr) {
		// If this is a non-root split, we split in the opposite direction of the parent split.
		section->split->direction = section->parent->direction == BZ_SPLIT_DIR_ROW ? BZ_SPLIT_DIR_COLUMN : BZ_SPLIT_DIR_ROW;
	} else {
		// If this is the root split, we split based on dimensions, and also shrink the bounds and x/y by the gap size.
		section->split->direction = section->bounds.w > section->bounds.h ? BZ_SPLIT_DIR_ROW : BZ_SPLIT_DIR_COLUMN;
		section->bounds.x += config->tile_gap;
		section->bounds.y += config->tile_gap;
		section->bounds.w -= (config->tile_gap + config->tile_gap);
		section->bounds.h -= (config->tile_gap + config->tile_gap);
	}
	section->split->sections = bz_list_create();
	section->split->section = section;

	// Link the new window to the original (now-split) section.
	bz_mgmt_link_to_parent_split(section->split, new_window, nullptr);
}

/**
 * Frees the section's memory allocation(s), including its immediate split/divider struct when
 * applicable. If "free_recursively" is set, will traverse into the split's subsections and free
 * those as well, if present.
 */
void bz_mgmt_free_section(
	struct bz_section *section,
	bool free_recursively
) {
	if (section->type == BZ_SECTION_SPLIT && section->split != nullptr) {
		if (free_recursively) {
			struct bz_section *inner; bz_list_foreach(inner, section->split->sections) {
				bz_mgmt_free_section(inner, free_recursively);
			}
		}
		bz_list_free(section->split->sections, nullptr);
		free(section->split);
	} else if (section->type == BZ_SECTION_DIVIDER && section->divider != nullptr) {
		free(section->divider);
	}
	free(section);
}

void bz_mgmt_drag_divider(
	struct bz_user_config *config,
	struct bz_divider *divider,
	double distance
) {
	struct bz_section *before = divider->section_before;
	struct bz_section *after = divider->section_after;
	if (divider->direction == BZ_DIVIDER_HORIZONTAL) {
		if (before->bounds.h + distance < 200 || after->bounds.h - distance < 200) {
			return; // Enforces a minimum size.
		}
		before->bounds.h += distance;
		after->bounds.y += distance;
		after->bounds.h -= distance;
	} else {
		if (before->bounds.w + distance < 200 || after->bounds.w - distance < 200) {
			return; // Enforces a minimum size.
		}
		before->bounds.w += distance;
		after->bounds.x += distance;
		after->bounds.w -= distance;
	}
	bz_mgmt_redistribute_section(config, before, before->bounds);
	bz_mgmt_redistribute_section(config, after, after->bounds);
}

/**
 * Grows or shrinks the given section to fit into the new bounds. Works recursively for split
 * sections, and reconfigures any toplevels as appropriate.
 */
void bz_mgmt_redistribute_section(
	struct bz_user_config *config,
	struct bz_section *section,
	struct bz_rect bounds
) {
	if (section->type == BZ_SECTION_WINDOW) {
		section->bounds = bounds;
		bz_xdg_surface_configure_toplevel(section->window);
		return;
	}

	struct bz_section *inner; // Used for below loops.

	// Step 1: Figure out the total dimension of the inner sections in the direction of the split.
	uint32_t total_size = 0;
	bz_list_foreach(inner, section->split->sections) {
		total_size += section->split->direction == BZ_SPLIT_DIR_COLUMN
			? inner->bounds.h
			: inner->bounds.w;
	}
	total_size += (config->tile_gap * (section->split->sections->length-1));

	// Step 2: Adjust the bounds of each section to be their percentage of the total, making sure we
	//   don't go over. Also update the position and the "other dimension" as we go.
	uint32_t bounds_size = (section->split->direction == BZ_SPLIT_DIR_COLUMN ? bounds.h : bounds.w);
	uint32_t remaining_size = bounds_size;
	struct bz_position pos = { .x = bounds.x, .y = bounds.y };
	bz_list_foreach(inner, section->split->sections) {
		// Determine the new "rescaled" dimension
		uint32_t new_size = remaining_size;
		if (section->split->sections->tail->data != inner) {
			// If this isn't the last item, then compute new size based on percentages
			uint64_t old_size = (section->split->direction == BZ_SPLIT_DIR_COLUMN) ? inner->bounds.h : inner->bounds.w;
			new_size = (uint32_t)(old_size * bounds_size / total_size);
		}
		// Determine the new bounds for this section
		struct bz_rect new_bounds = {
			.x = pos.x,
			.y = pos.y,
			.w = (section->split->direction == BZ_SPLIT_DIR_COLUMN) ? bounds.w : new_size,
			.h = (section->split->direction == BZ_SPLIT_DIR_COLUMN) ? new_size : bounds.h,
		};
		bz_mgmt_redistribute_section(config, inner, new_bounds);
		if (section->split->direction == BZ_SPLIT_DIR_COLUMN) {
			pos.y += new_size + config->tile_gap;
		} else {
			pos.x += new_size + config->tile_gap;
		}
		remaining_size -= (new_size + config->tile_gap);
	}

	// Step 3: Update the official bounds of the section.
	section->bounds = bounds;
}

/**
 * Doesn't resize anything. Just updates the parent's section list and the backreferences for the
 * newly added child_section.
 */
void bz_mgmt_link_to_parent_split(
	struct bz_split *parent_split,
	struct bz_section *child_section,
	struct bz_section *after_section
) {
	if (child_section->parent != nullptr) {
		bz_error(BZ_LOG_WINDOW_MGMT, "Attempted to link a child to a parent, but child already has a parent.");
		return;
	}
	int status = bz_list_insert(parent_split->sections, child_section, after_section);
	if (status < 0) {
		bz_error(BZ_LOG_WINDOW_MGMT, "Failed to insert section into parent split: %d", status);
		return;
	}
	child_section->parent = parent_split;
}

/**
 * Doesn't resize anything. Just updates the parent's section list and the backreferences for the
 * removed child_section.
 */
void bz_mgmt_unlink_from_parent_split(
	struct bz_section *child_section
) {
	if (child_section->parent == nullptr) {
		bz_error(BZ_LOG_WINDOW_MGMT, "Attempted to unlink a child from a parent, but the child had no parent.");
		return;
	}
	int status = bz_list_remove(child_section->parent->sections, child_section, nullptr);
	if (status == 1) {
		child_section->parent = nullptr;
	} else if (status == 0) {
		bz_error(BZ_LOG_WINDOW_MGMT, "Child not found on parent when attempting to unlink the two.");
	} else {
		bz_error(BZ_LOG_WINDOW_MGMT, "Unexpected error when attempting to unlink child from parent.");
	}
}

/**
 * If split just has one section, this combines it into the parent. If it's the root split, then it
 * is turned into the remaining child section. If the split has many sections, this is a no-op.
 */
void bz_mgmt_collapse_split(
	struct bz_user_config *config,
	struct bz_split *split
) {
	if (split == nullptr || split->sections->length > 1) {
		return;
	}

	struct bz_section *remaining_sect = split->sections->head->data;

	// Root split is turned into the remaining child section.
	if (split->section->parent == nullptr) {
		struct bz_section *root_sect = split->section;
		// Don't need the split anymore.
		bz_list_free(split->sections, nullptr);
		free(split);

		// Promote the remaining section into the new root
		root_sect->type = remaining_sect->type;
		if (remaining_sect->type == BZ_SECTION_WINDOW) {
			root_sect->window = remaining_sect->window;
			remaining_sect->window->section = root_sect;
		} else if (remaining_sect->type == BZ_SECTION_SPLIT) {
			root_sect->split = remaining_sect->split;
			remaining_sect->split->section = root_sect;
		}
		free(remaining_sect);

		// Remove the outer margin (if there's just one window left)
		if (root_sect->type == BZ_SECTION_WINDOW) {
			root_sect->bounds.x -= config->tile_gap;
			root_sect->bounds.y -= config->tile_gap;
			root_sect->bounds.w += config->tile_gap + config->tile_gap;
			root_sect->bounds.h += config->tile_gap + config->tile_gap;
			bz_mgmt_redistribute_section(config, root_sect, root_sect->bounds);
		}

		return;
	}

	// Not the root; combine into its parent.
	struct bz_split *parent = split->section->parent;
	bz_list_replace(parent->sections, split->section, remaining_sect, nullptr);
	remaining_sect->parent = parent;
	bz_mgmt_free_section(split->section, false);
	if (remaining_sect->type == BZ_SECTION_SPLIT && remaining_sect->split->direction == parent->direction) {
		bz_mgmt_flatten_similar_nested_splits(parent);
	}
}

void bz_mgmt_flatten_similar_nested_splits(
	struct bz_split *parent
) {
	const struct bz_node *curr = parent->sections->head;
	while (curr != nullptr) {
		struct bz_section *inner = curr->data;
		if (inner->type == BZ_SECTION_SPLIT && inner->split->direction == parent->direction) {
			// Set up for our next iteration
			curr = inner->split->sections->head;
			// Update the children of inner to point to their new parent
			struct bz_section *new_child; bz_list_foreach(new_child, inner->split->sections) {
				new_child->parent = parent;
			}
			// Update the parent's list of sections
			bz_list_insert_list(parent->sections, inner->split->sections, inner);
			bz_list_remove(parent->sections, inner, nullptr);
			// Clean up
			bz_mgmt_free_section(inner, false);
		} else {
			curr = curr->next;
		}
	}
}

/**
 * Creates a new window section for the given surface, adds it to the target section after the "after_section",
 * and returns the new window section. When "after_section" is a nullptr, the new section is inserted as the
 * first item in the target section.
 *
 * If the target section is a window (not a split), then it's turned into a split before the insert occurs.
 */
struct bz_section *bz_mgmt_create_window_after_section(
	struct bz_user_config *config,
	struct bz_section *target_section,
	struct bz_surface *surface,
	struct bz_section *after_section
) {
	// Determine the new bounds of the surface. The "0" values will get populated during the "redistribute" step.
	uint32_t num_items = (target_section->type == BZ_SECTION_SPLIT) ? target_section->split->sections->length : 1;
	struct bz_rect bounds = {
		.x = 0, .y = 0,
		// It doesn't matter which direction the split is in. The incorrect bound will be hard reset when redistributed.
		.w = target_section->bounds.w / num_items,
		.h = target_section->bounds.h / num_items
	};

	// Create the new surface. Bounds are temporary - only the dimensions should be proportionally accurate.
	struct bz_section *new_section = bz_mgmt_create_new_window_section(bounds, surface);

	// Add it to its parent
	bz_mgmt_add_section_to_section(config, target_section, new_section, after_section);

	return new_section;
}

/**
 * Adds the new section to target_section, linking the section with its new parent, and resizing the
 * new section and its neighbor as appropriate. If the target section is not already a split, it will
 * be turned into one.
 */
void bz_mgmt_add_section_to_section(
	struct bz_user_config *config,
	struct bz_section *target_section,
	struct bz_section *new_section,
	struct bz_section *after_section
) {
	bz_mgmt_convert_window_to_split(config, target_section); // no-op if target is already a split.
	// If we just created a split from a window, then we need to make sure to update the original
	//   after_section pointer to be the new, inner window section.
	if (after_section == target_section) {
		after_section = target_section->split->sections->head->data;
	}
	bz_mgmt_link_to_parent_split(target_section->split, new_section, after_section);
	bz_mgmt_redistribute_section(config, target_section, target_section->bounds);
}

/**
 * Removes and unlinks the given section from its parent split, but DOES NOT FREE the section. This
 * allows for moving the section to a new location, or manual freeing afterwords if desired. The
 * parent split/section is resized to fill the gap, and will be collapsed if necessary.
 *
 * If the given section doesn't have a parent split, this is a no-op.
 */
void bz_mgmt_remove_section_from_split(
	struct bz_user_config *config,
	struct bz_section *to_remove
) {
	if (to_remove->parent == nullptr) {
		return;
	}
	struct bz_split *parent_split = to_remove->parent;
	bz_mgmt_unlink_from_parent_split(to_remove);
	bz_mgmt_redistribute_section(config, parent_split->section, parent_split->section->bounds);
	bz_mgmt_collapse_split(config, parent_split); // no-op if not needed
}

/**
 * Moves a section from its parent to a different section, properly collapsing everything and
 * redistributing it as needed. It's a similar process to calling "remove_section_from_split"
 * followed by "add_section_to_section", but the redistribute / collapse step is saved until
 * the end.
*
 * If the given section doesn't have a parent split, this is a no-op.
 */
void bz_mgmt_move_section(
	struct bz_user_config *config,
	struct bz_section *destination_parent,
	struct bz_section *section,
	struct bz_section *after_section
) {
	if (section->parent == nullptr) {
		return;
	}

	// Prepare the destination
	bz_mgmt_convert_window_to_split(config, destination_parent); // no-op if target is already a split.
	// If we just created a split from a window, then we need to make sure to update the original
	//   after_section pointer to be the new, inner window section.
	if (after_section == destination_parent) {
		after_section = destination_parent->split->sections->head->data;
	}

	// Move from source to destination
	struct bz_section *source_parent = section->parent->section;
	bz_mgmt_unlink_from_parent_split(section);
	bz_mgmt_link_to_parent_split(destination_parent->split, section, after_section);

	// Clean up
	bz_mgmt_redistribute_section(config, destination_parent, destination_parent->bounds);
	bz_mgmt_redistribute_section(config, source_parent, source_parent->bounds);
	bz_mgmt_collapse_split(config, source_parent->split); // no-op if not needed
}

/** No-op when section is not a window. */
void bz_mgmt_delete_section(
	struct bz_user_config *config,
	struct bz_section *section
) {
	if (section->type != BZ_SECTION_WINDOW) {
		return;
	}

	// Clear the surface's backreference to this section
	if (section->window != nullptr) {
		section->window->section = nullptr;
	}

	if (section->parent == nullptr) {
		section->type = BZ_SECTION_UNINITIALIZED;
		section->window = nullptr;
	} else {
		bz_mgmt_remove_section_from_split(config, section);
		bz_mgmt_free_section(section, false);
	}
}
