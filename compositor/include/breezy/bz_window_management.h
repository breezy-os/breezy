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
void bz_mgmt_untrack_surface_on_destroy(struct bz_surface *surface_data);
void bz_mgmt_notify_kb_enter(uint32_t enter_serial, uint32_t modifiers_serial, struct xkb_state *xkbstate, struct wl_resource *keyboard, struct wl_resource *surface);


// -- Window Focus --

struct bz_surface *bz_mgmt_get_active_surface(struct bz_window_mgmt *mgmt);
struct bz_surface *bz_mgmt_update_pointer_position(struct bz_window_mgmt *mgmt, struct bz_renderable *cursor);
void bz_mgmt_focus_window(struct bz_breezy *breezy, struct bz_surface *surface);


// -- Structure / Layout --

enum bz_section_type {
	BZ_SECTION_UNINITIALIZED,
	BZ_SECTION_WINDOW,
	BZ_SECTION_SPLIT,
	BZ_SECTION_DIVIDER,
};

enum bz_split_direction {
	BZ_SPLIT_DIR_COLUMN, // vertical stack of items
	BZ_SPLIT_DIR_ROW,    // horizontal line of items
};

enum bz_divider_direction {
	BZ_DIVIDER_HORIZONTAL, // for columnar splits
	BZ_DIVIDER_VERTICAL,   // for row-wise splits
};

struct bz_section {
	struct bz_split *parent; // Null for root section
	struct bz_rect bounds;
	enum bz_section_type type;
	union {
		struct bz_split *split;
		struct bz_surface *window;
		struct bz_divider *divider;
	};
};

struct bz_split {
	struct bz_section *section;
	enum bz_split_direction direction;
	struct bz_list *sections; // List of "struct bz_section *". Should be at least 2 items.
};

struct bz_divider {
	struct bz_section *section;
	enum bz_divider_direction direction;

	struct bz_section *section_before; // Nullable for screen edges
	struct bz_section *section_after;  // Nullable for screen edges
};

// Core Operations
struct bz_position bz_mgmt_get_coords_relative_to_active_window(struct bz_breezy *breezy, enum bz_direction direction);
struct bz_section *bz_mgmt_get_leaf_at_coordinates(struct bz_section *root, struct bz_position coords);
struct bz_section *bz_mgmt_get_nearest_window(struct bz_section *section, struct bz_position coords);
struct bz_section *bz_mgmt_get_prior_section_in_split(struct bz_split *split, struct bz_position coords);
struct bz_section *bz_mgmt_create_new_window_section(struct bz_rect bounds, struct bz_surface *surface);
struct bz_section *bz_mgmt_convert_root_into_child_of_new_split(struct bz_window_mgmt *mgmt);
void bz_mgmt_convert_window_to_split(struct bz_user_config *config, struct bz_section *section);
void bz_mgmt_free_section(struct bz_section *section, bool free_recursively);
void bz_mgmt_drag_divider(struct bz_user_config *config, struct bz_divider *divider, double distance);
void bz_mgmt_redistribute_section(struct bz_user_config *config, struct bz_section *section, struct bz_rect bounds);
void bz_mgmt_link_to_parent_split(struct bz_split *parent_split, struct bz_section *child_section, struct bz_section *after_section);
void bz_mgmt_unlink_from_parent_split(struct bz_section *child_section);
void bz_mgmt_collapse_split(struct bz_user_config *config, struct bz_split *split);
void bz_mgmt_flatten_similar_nested_splits(struct bz_split *parent);

// Compound Operations
struct bz_section *bz_mgmt_create_window_after_section(struct bz_user_config *config, struct bz_section *target_section, struct bz_surface *surface, struct bz_section *after_section);
void bz_mgmt_add_section_to_section(struct bz_user_config *config, struct bz_section *target_section, struct bz_section *new_section, struct bz_section *after_section);
void bz_mgmt_remove_section_from_split(struct bz_user_config *config, struct bz_section *to_remove);
void bz_mgmt_move_section(struct bz_user_config *config, struct bz_section *destination_parent, struct bz_section *section, struct bz_section *after_section);
void bz_mgmt_delete_section(struct bz_user_config *config, struct bz_section *section);

// #################################################################################################
#endif
