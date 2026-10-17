#ifndef BZ_WINDOW_MANAGEMENT_H
#define BZ_WINDOW_MANAGEMENT_H
// #################################################################################################

#include <xkbcommon/xkbcommon.h>

#include "breezy/bz_wl_display.h"
#include "breezy/bz_breezy.h"


// -- Initialization / Teardown --

int bz_mgmt_initialize(struct bz_breezy *breezy);
void bz_mgmt_cleanup(struct bz_window_mgmt *mgmt);


// -- Window Lifecycle --


void bz_mgmt_place_window(struct bz_breezy *breezy, struct bz_surface *surface);
void bz_mgmt_open_window(struct bz_window_mgmt *mgmt, struct bz_surface *surface);
int bz_mgmt_close_active_window(struct bz_window_mgmt *mgmt);
void bz_mgmt_notify_kb_enter(uint32_t enter_serial, uint32_t modifiers_serial, struct xkb_state *xkbstate, struct wl_resource *keyboard, struct wl_resource *surface);


// -- Window Focus --

struct bz_surface *bz_mgmt_get_active_surface(struct bz_window_mgmt *mgmt);
struct bz_surface *bz_mgmt_update_pointer_position(struct bz_window_mgmt *mgmt, struct bz_renderable *cursor);


// -- Structure / Layout --

enum bz_section_type {
	BZ_SECTION_UNINITIALIZED,
	BZ_SECTION_WINDOW,
	BZ_SECTION_SPLIT,
};

enum bz_split_direction {
	BZ_SPLIT_DIR_COLUMN, // vertical stack of items
	BZ_SPLIT_DIR_ROW,    // horizontal line of items
};

struct bz_section {
	struct bz_split *parent; // Null for root section
	struct bz_rect bounds;
	enum bz_section_type type;
	union {
		struct bz_split *split;
		struct bz_surface *window;
	};
};

struct bz_split {
	struct bz_section *section;
	enum bz_split_direction direction;
	struct bz_list *sections; // List of "struct bz_section *". Should be at least 2 items.
};

// Add / Remove
void bz_mgmt_delete_section(struct bz_user_config *config, struct bz_section *section);
void bz_mgmt_remove_from_split(struct bz_split *split, struct bz_surface *window);
void bz_mgmt_add_to_section_before(struct bz_user_config *config, struct bz_section *section, struct bz_surface *window, struct bz_surface *before_window);
void bz_mgmt_add_to_section_after(struct bz_user_config *config, struct bz_section *section, struct bz_surface *window, struct bz_surface *after_window);
void bz_mgmt_swap_within_split(struct bz_split *split, struct bz_surface *window1, struct bz_surface *window2);
struct bz_section *bz_mgmt_create_window_section(struct bz_split *parent, struct bz_rect bounds, struct bz_surface *window);
void bz_mgmt_free_section(void *section);
// Placement / Sizing
void bz_mgmt_redistribute_split(struct bz_split *split);
void bz_mgmt_collapse_split(struct bz_user_config *config, struct bz_split *split);
void bz_mgmt_grow_section_bounds(struct bz_section *section, struct bz_rect new_bounds);
void bz_mgmt_resize_neighbors(struct bz_split *split, struct bz_section *section_before_split, struct bz_position *new_drag_loc);
struct bz_section *bz_mgmt_get_section_at_coords(struct bz_section *root, enum bz_section_type type, struct bz_position *coords);

// #################################################################################################
#endif
