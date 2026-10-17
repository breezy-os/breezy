
#include "breezy/bz_window_management.h"

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
		bz_mgmt_add_to_section_after(config, target_section->parent->section, surface, active_surf);
	} else {
		// Create a new split list at the current node
		bz_mgmt_add_to_section_after(config, target_section, surface, active_surf);
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

void bz_mgmt_delete_section(struct bz_user_config *config, struct bz_section *section)
{
	// This function is intended for closing windows.
	if (section->type != BZ_SECTION_WINDOW) {
		return;
	}

	// Clear the surface's backreference to this section
	if (section->window != nullptr) {
		section->window->section = nullptr;
	}

	// If this is the root section, we handle it a bit differently
	if (section->parent == nullptr) {
		section->type = BZ_SECTION_UNINITIALIZED;
		section->window = nullptr;
		return;
	}

	// For non-root sections, we need to remove the section from its parent split, and collapse
	//   that split if needed.
	bz_list_remove(section->parent->sections, section, nullptr);
	if (section->parent->sections->length == 1) {
		bz_mgmt_collapse_split(config, section->parent);
	}
	free(section);
}

void bz_mgmt_remove_from_split(struct bz_split *split, struct bz_surface *window)
{
	// TODO-dl13
}

void bz_mgmt_add_to_section_before(
	struct bz_user_config *config,
	struct bz_section *section,
	struct bz_surface *window,
	struct bz_surface *before_window
) {
	// TODO-dl13
}

void bz_mgmt_add_to_section_after(
	struct bz_user_config *config,
	struct bz_section *section,
	struct bz_surface *window,
	struct bz_surface *after_window
) {
	if (section->type == BZ_SECTION_WINDOW) {
		// Create a new split with the windows
		struct bz_split *new_split = calloc(1, sizeof(*new_split));
		if (new_split == nullptr) {
			bz_error(BZ_LOG_WINDOW_MGMT, "Failed to allocate memory for the new split.");
			return;
		}
		new_split->sections = bz_list_create();
		if (new_split->sections == nullptr) {
			bz_error(BZ_LOG_WINDOW_MGMT, "Failed to allocate memory for the new sections list.");
			free(new_split);
			return;
		}
		new_split->section = section;
		new_split->direction = section->bounds.w > section->bounds.h ? BZ_SPLIT_DIR_ROW : BZ_SPLIT_DIR_COLUMN;
		section->type = BZ_SECTION_SPLIT;
		section->split = new_split;
		// The root tile gets a margin when it splits from 1 window down to 2+.
		if (section->parent == nullptr) {
			section->bounds.x += config->tile_gap;
			section->bounds.y += config->tile_gap;
			section->bounds.w -= (config->tile_gap + config->tile_gap);
			section->bounds.h -= (config->tile_gap + config->tile_gap);
		}

		// Create two new sections for our two windows
		struct bz_rect *bounds = new_split->direction == BZ_SPLIT_DIR_COLUMN
			? bz_split_rect(&section->bounds, 0.5f, 0.0f, config->tile_gap)
			: bz_split_rect(&section->bounds, 0.0f, 0.5f, config->tile_gap);
		struct bz_section *section1 = bz_mgmt_create_window_section(new_split, bounds[0], after_window);
		struct bz_section *section2 = bz_mgmt_create_window_section(new_split, bounds[1], window);
		bz_xdg_surface_configure_toplevel(section1->window);
		free(bounds);

		// Add the new sections to our new split
		bz_list_append(new_split->sections, section1);
		bz_list_append(new_split->sections, section2);

	} else if (section->type == BZ_SECTION_SPLIT) {
		// Split the existing area into two pieces
		struct bz_rect *bounds = section->split->direction == BZ_SPLIT_DIR_COLUMN
			? bz_split_rect(&after_window->section->bounds, 0.5f, 0.0f, config->tile_gap)
			: bz_split_rect(&after_window->section->bounds, 0.0f, 0.5f, config->tile_gap);
		// Resize the first, already existing section
		after_window->section->bounds = bounds[0];
		bz_xdg_surface_configure_toplevel(after_window);
		// Create a new section for the second piece
		struct bz_section *new_section = bz_mgmt_create_window_section(section->split, bounds[1], window);
		bz_list_insert(section->split->sections, new_section, after_window->section);
		free(bounds);

	} else {
		bz_warn(BZ_LOG_WINDOW_MGMT, "Attempted to add a window to an uninitialized section.");
	}
}

void bz_mgmt_swap_within_split(
	struct bz_split *split,
	struct bz_surface *window1,
	struct bz_surface *window2
) {
	// TODO-dl13
}

struct bz_section *bz_mgmt_create_window_section(
	struct bz_split *parent,
	struct bz_rect bounds,
	struct bz_surface *window
) {
	struct bz_section *section = calloc(1, sizeof(*section));
	if (section == nullptr) {
		bz_error(BZ_LOG_WINDOW_MGMT, "Failed to allocate memory for a new section.");
		return nullptr;
	}
	section->parent = parent;
	section->bounds = bounds;
	section->type = BZ_SECTION_WINDOW;
	section->window = window;
	window->section = section;
	return section;
}

void bz_mgmt_free_section(void *section)
{
	struct bz_section *sect = section;
	if (sect->type == BZ_SECTION_SPLIT && sect->split != nullptr) {
		bz_list_free(sect->split->sections, bz_mgmt_free_section);
		free(sect->split);
	}
	free(section);
}

void bz_mgmt_redistribute_split(struct bz_split *split)
{
	if (split->sections->length < 2) {
		bz_error(BZ_LOG_WINDOW_MGMT, "A split must have at least 2 sections in order to redistribute.");
		return;
	}

	// TODO-dl13
}

void bz_mgmt_collapse_split(struct bz_user_config *config, struct bz_split *split)
{
	if (split->sections->length != 1) {
		bz_warn(BZ_LOG_WINDOW_MGMT, "Attempted to collapse a split with more than 1 section. Ignoring.");
		return;
	}

	struct bz_split *parent_split = split->section->parent;
	struct bz_section *remaining_section = split->sections->head->data;

	// First, handle the "root node" case
	if (parent_split == nullptr) {
		struct bz_section *root = split->section;
		root->type = remaining_section->type;
		if (remaining_section->type == BZ_SECTION_WINDOW) {
			root->window = remaining_section->window;
			root->window->section = root;
			root->bounds.x -= config->tile_gap;
			root->bounds.y -= config->tile_gap;
			root->bounds.w += config->tile_gap + config->tile_gap;
			root->bounds.h += config->tile_gap + config->tile_gap;
		} else {
			root->split = remaining_section->split;
			root->split->section = root;
		}
		bz_mgmt_grow_section_bounds(remaining_section, root->bounds);

		// Clean up!
		free(remaining_section);
		bz_list_free(split->sections, nullptr);
		free(split);

		return;
	}

	// (Note that when removing a split, any child splits might grow in size in one direction, and would
	//   also merge with the parent in the other direction.)
	struct bz_rect new_size = remaining_section->bounds;
	if (parent_split->direction == BZ_SPLIT_DIR_COLUMN) {
		new_size.x = parent_split->section->bounds.x;
		new_size.w = parent_split->section->bounds.w;
	} else {
		new_size.y = parent_split->section->bounds.y;
		new_size.h = parent_split->section->bounds.h;
	}
	bz_mgmt_grow_section_bounds(remaining_section, new_size);

	// Remove the split, adding the sole section to the parent in place of the split.
	remaining_section->parent = parent_split;
	bz_list_replace(parent_split->sections, split->section, remaining_section, nullptr);
	// Free / clean up old memory
	bz_list_remove(split->sections, remaining_section, nullptr);
	bz_mgmt_free_section(split->section);
}

/**
 * Updates the bounds for the given section, and the bounds of all children, recursively.
 */
void bz_mgmt_grow_section_bounds(struct bz_section *section, struct bz_rect new_bounds)
{
	if (section->type == BZ_SECTION_WINDOW) {
		section->bounds = new_bounds;
		bz_xdg_surface_configure_toplevel(section->window);

	} else if (section->type == BZ_SECTION_SPLIT) {
		// Expand the width of children as needed
		if (section->bounds.w < new_bounds.w) {
			if (section->split->direction == BZ_SPLIT_DIR_COLUMN) {
				// Split is opposite direction of growth. Expand the width of everything within.
				struct bz_section *inner; bz_list_foreach(inner, section->split->sections) {
					struct bz_rect inner_bounds = inner->bounds;
					inner_bounds.x = new_bounds.x;
					inner_bounds.w = new_bounds.w;
					bz_mgmt_grow_section_bounds(inner, inner_bounds);
				}
			} else {
				if (section->bounds.x > new_bounds.x) {
					// The left side moved leftward -- grow the first item to fill.
					struct bz_section *grow = section->split->sections->head->data;
					struct bz_rect inner_bounds = grow->bounds;
					inner_bounds.x = new_bounds.x;
					inner_bounds.w += (new_bounds.w - section->bounds.w);
					bz_mgmt_grow_section_bounds(grow, inner_bounds);
				} else {
					// The right side moved rightward -- grow the last item to fill.
					struct bz_section *grow = section->split->sections->tail->data;
					struct bz_rect inner_bounds = grow->bounds;
					inner_bounds.w += (new_bounds.w - section->bounds.w);
					bz_mgmt_grow_section_bounds(grow, inner_bounds);
				}
			}
		}

		// Expand the height of children as needed
		if (section->bounds.h < new_bounds.h) {
			if (section->split->direction == BZ_SPLIT_DIR_ROW) {
				// Split is opposite direction of growth. Expand the height of everything within.
				struct bz_section *inner; bz_list_foreach(inner, section->split->sections) {
					struct bz_rect inner_bounds = inner->bounds;
					inner_bounds.y = new_bounds.y;
					inner_bounds.h = new_bounds.h;
					bz_mgmt_grow_section_bounds(inner, inner_bounds);
				}
			} else {
				if (section->bounds.y > new_bounds.y) {
					// The top side moved upward -- grow the first item to fill.
					struct bz_section *grow = section->split->sections->head->data;
					struct bz_rect inner_bounds = grow->bounds;
					inner_bounds.y = new_bounds.y;
					inner_bounds.h += (new_bounds.h - section->bounds.h);
					bz_mgmt_grow_section_bounds(grow, inner_bounds);
				} else {
					// The bottom side moved downward -- grow the last item to fill.
					struct bz_section *grow = section->split->sections->tail->data;
					struct bz_rect inner_bounds = grow->bounds;
					inner_bounds.h += (new_bounds.h - section->bounds.h);
					bz_mgmt_grow_section_bounds(grow, inner_bounds);
				}
			}
		}

		// Save the new bounds
		section->bounds = new_bounds;

	} else {
		bz_error(BZ_LOG_WINDOW_MGMT, "Section was not initialized, so cannot update window size. (This should never happen.)");
	}
}

void bz_mgmt_resize_neighbors(
	struct bz_split *split,
	struct bz_section *section_before_split,
	struct bz_position *new_drag_loc
) {
	// TODO-dl13
}

/**
 * Finds and returns the leaf-most section at the given coordinates matching the given section type.
 */
struct bz_section *bz_mgmt_get_section_at_coords(
	struct bz_section *root,
	enum bz_section_type type,
	struct bz_position *coords
) {
	// TODO-dl13
	return nullptr;
}
