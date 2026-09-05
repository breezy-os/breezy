
#include <stdlib.h>

#include "breezy/bz_wayland.h"
#include "breezy/bz_wl_devices.h"
#include "breezy/bz_wl_display.h"
#include "breezy/bz_xdg_shell.h"
#include "breezy/bz_list.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

// -- bz_breezy --
struct bz_breezy *bz_create_breezy_data(void);
void bz_free_breezy_data(struct bz_breezy *data);

// -- bz_client --
struct bz_client *bz_create_client_data(void);
void bz_free_client_data(struct bz_client *data);

// -- bz_region --
struct bz_region *bz_create_region_data(void);
void bz_free_region_data(struct bz_region *data);

// -- bz_surface --
struct bz_surface *bz_create_surface_data(void);
void bz_free_surface_data(struct bz_surface *data);

// -- bz_subsurface --
struct bz_subsurface *bz_create_subsurface_data(void);
void bz_free_subsurface_data(struct bz_subsurface *data);

// -- bz_wl_seat --
struct bz_wl_seat *bz_create_seat_data(void);
void bz_free_seat_data(struct bz_wl_seat *data);

// -- bz_xdg_surface --
struct bz_xdg_surface *bz_create_xdg_surface_data();
void bz_free_xdg_surface_data(struct bz_xdg_surface *data);

// -- bz_xdg_toplevel --
struct bz_xdg_toplevel *bz_create_xdg_toplevel_data();
void bz_free_xdg_toplevel_data(struct bz_xdg_toplevel *data);

// -- bz_xdg_surface_configure --
struct bz_xdg_surface_configure *bz_create_xdg_surface_configure(uint32_t serial);
void bz_free_xdg_surface_configure(struct bz_xdg_surface_configure *data);


// =================================================================================================
//  bz_breezy
// -------------------------------------------------------------------------------------------------

struct bz_breezy *bz_create_breezy_data(void)
{
	struct bz_breezy *data = calloc(1, sizeof(*data));

	data->drm.mode_info.hdisplay = 1920;
	data->drm.mode_info.vdisplay = 1080;
	data->window_mgmt.activable_surfaces = bz_list_create();
	data->wayland.clients = bz_list_create();

	return data;
}

void bz_free_breezy_data(struct bz_breezy *data)
{
	if (data) {
		if (data->window_mgmt.activable_surfaces != nullptr) {
			bz_list_free(data->window_mgmt.activable_surfaces, nullptr);
		}
		if (data->wayland.clients != nullptr) {
			bz_list_free(data->wayland.clients, nullptr);
		}
		free(data);
	}
}


// =================================================================================================
//  bz_client
// -------------------------------------------------------------------------------------------------

struct bz_client *bz_create_client_data(void)
{
	struct bz_client *data = calloc(1, sizeof(*data));

	data->breezy = bz_create_breezy_data();
	data->seats = bz_list_create();

	return data;
}

void bz_free_client_data(struct bz_client *data)
{
	if (data) {
		if (data->breezy != nullptr) { bz_free_breezy_data(data->breezy); }
		if (data->seats != nullptr)  { bz_list_free(data->seats, nullptr); }
		free(data);
	}
}


// =================================================================================================
//  bz_region
// -------------------------------------------------------------------------------------------------

struct bz_region *bz_create_region_data(void)
{
	struct bz_region *data = calloc(1, sizeof(*data));

	data->mutations = bz_list_create();

	return data;
}

void bz_free_region_data(struct bz_region *data)
{
	if (data) {
		if (data->mutations) {
			bz_list_free(data->mutations, free);
		}
		free(data);
	}
}


// =================================================================================================
//  bz_surface
// -------------------------------------------------------------------------------------------------

struct bz_surface *bz_create_surface_data(void)
{
	struct bz_surface *data = calloc(1, sizeof(*data));

	data->role = BZ_SURF_ROLE_NONE;

	data->pending_state = calloc(1, sizeof(*data->pending_state));
	data->pending_state->frame_callbacks = bz_list_create();

	data->content_updates = bz_list_create();

	data->active_state  = calloc(1, sizeof(*data->active_state));
	data->active_state->frame_callbacks = bz_list_create();

	data->surface_stack = bz_list_create();
	bz_list_insert(data->surface_stack, data, nullptr);

	return data;
}

void bz_free_surface_data(struct bz_surface *data)
{
	if (data) {
		if (data->pending_state) {
			bz_list_free(data->pending_state->frame_callbacks, nullptr);
			free(data->pending_state);
		}
		if (data->active_state) {
			bz_list_free(data->active_state->frame_callbacks, nullptr);
			free(data->active_state);
		}
		if (data->content_updates) {
			bz_list_free(data->content_updates, nullptr);
		}
		if (data->surface_stack) {
			bz_list_free(data->surface_stack, nullptr);
		}
		free(data);
	}
}


// =================================================================================================
//  bz_subsurface
// -------------------------------------------------------------------------------------------------

struct bz_subsurface *bz_create_subsurface_data(void)
{
	struct bz_subsurface *data = calloc(1, sizeof(*data));

	data->is_sync = true; // This is the default in our code.

	return data;
}

void bz_free_subsurface_data(struct bz_subsurface *data)
{
	free(data);
}


// =================================================================================================
//  bz_seat
// -------------------------------------------------------------------------------------------------

struct bz_wl_seat *bz_create_seat_data(void)
{
	struct bz_wl_seat *data = calloc(1, sizeof(*data));

	data->keyboards = bz_list_create();
	data->pointers = bz_list_create();

	return data;
}

void bz_free_seat_data(struct bz_wl_seat *data)
{
	if (data) {
		if (data->keyboards) {
			bz_list_free(data->keyboards, nullptr);
		}
		if (data->pointers) {
			bz_list_free(data->pointers, nullptr);
		}
		free(data);
	}
}


// =================================================================================================
//  bz_xdg_surface
// -------------------------------------------------------------------------------------------------

struct bz_xdg_surface *bz_create_xdg_surface_data()
{
	struct bz_xdg_surface *data = calloc(1, sizeof(*data));

	struct bz_surface *surf_data = bz_create_surface_data();
	data->wlsurface = surf_data;
	data->pending_configures = bz_list_create();

	return data;
}

void bz_free_xdg_surface_data(struct bz_xdg_surface *data)
{
	if (data) {
		if (data->wlsurface) {
			bz_free_surface_data(data->wlsurface);
		}
		if (data->last_acked_configure) {
			bz_free_xdg_surface_configure(data->last_acked_configure);
		}
		if (data->pending_configures) {
			bz_list_free(data->pending_configures, free);
		}
		free(data);
	}
}


// =================================================================================================
//  bz_xdg_toplevel
// -------------------------------------------------------------------------------------------------

struct bz_xdg_toplevel *bz_create_xdg_toplevel_data()
{
	struct bz_xdg_toplevel *data = calloc(1, sizeof(*data));

	return data;
}

void bz_free_xdg_toplevel_data(struct bz_xdg_toplevel *data)
{
	if (data) {
		free(data);
	}
}


// =================================================================================================
//  bz_xdg_surface_configure
// -------------------------------------------------------------------------------------------------

struct bz_xdg_surface_configure *bz_create_xdg_surface_configure(uint32_t serial)
{
	struct bz_xdg_surface_configure *data = calloc(1, sizeof(*data));

	data->serial = serial;

	return data;
}

void bz_free_xdg_surface_configure(struct bz_xdg_surface_configure *data)
{
	if (data) {
		free(data);
	}
}
