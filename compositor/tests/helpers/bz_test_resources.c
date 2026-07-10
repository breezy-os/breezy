
#include <stdlib.h>

#include "breezy/bz_wl_display.h"
#include "breezy/bz_xdg_shell.h"
#include "breezy/bz_list.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

// -- bz_surface --
struct bz_surface *bz_create_surface_data(void);
void bz_free_surface_data(struct bz_surface *data);

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
//  bz_surface
// -------------------------------------------------------------------------------------------------

struct bz_surface *bz_create_surface_data(void)
{
	struct bz_surface *data = calloc(1, sizeof(*data));

	data->pending_state = calloc(1, sizeof(*data->pending_state));
	data->pending_state->frame_callbacks = bz_list_create();

	data->active_state  = calloc(1, sizeof(*data->active_state));
	data->active_state->frame_callbacks = bz_list_create();

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
