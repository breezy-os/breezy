
#include "breezy/bz_wl_protocol.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include <xdg-shell-client-protocol.h>
#include <xkbcommon/xkbcommon.h>

#include "breezy/bz_client_utils.h"
#include "breezy/bz_logger.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

// -- wl_registry --

static const struct wl_registry_listener bz_registry_implementation;
static void bz_registry_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
static void bz_registry_global_remove(void *data, struct wl_registry *registry, uint32_t name);

// -- wl_shm --

static const struct wl_shm_listener bz_shm_implementation;
static void bz_shm_format(void *data, struct wl_shm *shm, uint32_t format);

// -- wl_buffer --

const struct wl_buffer_listener bz_buffer_implementation;
static void bz_buffer_release(void *data, struct wl_buffer *buffer);

// -- wl_seat --

static const struct wl_seat_listener bz_seat_implementation;
static void bz_seat_capabilities(void *data, struct wl_seat *wl_seat, uint32_t capabilities);
static void bz_seat_name(void *data, struct wl_seat *wl_seat, const char *name);

// -- wl_keyboard --

static const struct wl_keyboard_listener bz_keyboard_implementation;
static void bz_keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard, uint32_t format, int32_t fd, uint32_t size);
static void bz_keyboard_enter(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys);
static void bz_keyboard_leave(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, struct wl_surface *surface);
static void bz_keyboard_key(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
static void bz_keyboard_modifiers(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
static void bz_keyboard_repeat_info(void *data, struct wl_keyboard *wl_keyboard, int32_t rate, int32_t delay);

// -- wl_pointer --

static const struct wl_pointer_listener bz_pointer_implementation;
static void bz_pointer_enter(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y);
static void bz_pointer_leave(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface);
static void bz_pointer_motion(void *data, struct wl_pointer *wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y);
static void bz_pointer_button(void *data, struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
static void bz_pointer_axis(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
static void bz_pointer_frame(void *data, struct wl_pointer *wl_pointer);
static void bz_pointer_axis_source(void *data, struct wl_pointer *wl_pointer, uint32_t axis_source);
static void bz_pointer_axis_stop(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis);
static void bz_pointer_axis_discrete(void *data, struct wl_pointer *wl_pointer, uint32_t axis, int32_t discrete);
static void bz_pointer_axis_value120(void *data, struct wl_pointer *wl_pointer, uint32_t axis, int32_t value120);
static void bz_pointer_axis_relative_direction(void *data, struct wl_pointer *wl_pointer, uint32_t axis, uint32_t direction);

// -- xdg_wm_base --

static const struct xdg_wm_base_listener bz_xdg_wm_base_implementation;
static void bz_xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial);

// -- xdg_surface --

static const struct xdg_surface_listener bz_xdg_surface_implementation;
static void bz_xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial);
// Helpers
static void bz_initialize_surface_buffers(struct bz_client_globals *globals, struct bz_application_window* window);
static void bz_update_circle(struct bz_application_window *window, uint32_t new_time);
static void bz_control_circle(struct bz_application_window *window, uint32_t new_time);
static void bz_draw_frame(struct bz_application_window *window);
static void bz_submit_frame(struct bz_application_window* window);
static void bz_render(void *data, struct wl_callback *wl_callback, uint32_t callback_data);

// -- xdg_toplevel --

static const struct xdg_toplevel_listener bz_xdg_toplevel_implementation;
static void bz_xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states);
static void bz_xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel);
static void bz_xdg_toplevel_configure_bounds(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height);
static void bz_xdg_toplevel_wm_capabilities(void *data, struct xdg_toplevel *xdg_toplevel, struct wl_array *capabilities);


// =================================================================================================
//  wl_registry
// -------------------------------------------------------------------------------------------------

void bz_registry_constructor(struct bz_client_globals *globals)
{
	bz_debug(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Setting up registry listener.");
	globals->registry = wl_display_get_registry(globals->display);
	wl_registry_add_listener(globals->registry, &bz_registry_implementation, globals);
}

static const struct wl_registry_listener bz_registry_implementation = {
	.global = bz_registry_global,
	.global_remove = bz_registry_global_remove,
};

static void bz_registry_global(
	void *data,
	struct wl_registry *registry,
	uint32_t name,
	const char *interface,
	uint32_t version
) {
	bz_info(BZ_LOG_WAYLAND, __FILE__, __LINE__,
		"Registering interface: '%s', version: %d, name: %d", interface, version, name);

	struct bz_client_globals *globals = data;

	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		globals->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 6);
		globals->compositor_name = name;
	}
	else if (strcmp(interface, wl_subcompositor_interface.name) == 0) {
		// Version 2
	}
	else if (strcmp(interface, wl_shm_interface.name) == 0) {
		globals->shm = wl_registry_bind(registry, name, &wl_shm_interface, 2);
		globals->shm_name = name;
		wl_shm_add_listener(globals->shm, &bz_shm_implementation, data);
	}
	else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		globals->xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 7);
		globals->xdg_wm_base_name = name;
		xdg_wm_base_add_listener(globals->xdg_wm_base, &bz_xdg_wm_base_implementation, data);
	}
	else if (strcmp(interface, wl_data_device_manager_interface.name) == 0) {
		// Version 3
	}
	else if (strcmp(interface, wl_seat_interface.name) == 0) {
		// Bind to the seat
		globals->seat = wl_registry_bind(registry, name, &wl_seat_interface, 10);
		globals->seat_name = name;
		// Create the user data
		struct bz_seat *data = calloc(1, sizeof(*data));
		data->resource = globals->seat;
		data->globals = globals;
		// Attach our listener
		wl_seat_add_listener(globals->seat, &bz_seat_implementation, data);
	}
	else if (strcmp(interface, wl_output_interface.name) == 0) {
		// Version 4
	}
}

static void bz_registry_global_remove(void *data, struct wl_registry * /*registry*/, uint32_t name)
{
	bz_info(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Removing global with name: %d", name);
	struct bz_client_globals *globals = data;

	if (name == globals->compositor_name) {
		wl_compositor_destroy(globals->compositor);
		globals->compositor = nullptr;
		globals->compositor_name = 0;
	} else if (name == globals->shm_name) {
		wl_shm_destroy(globals->shm);
		globals->shm = nullptr;
		globals->shm_name = 0;
	} else if (name == globals->seat_name) {
		// First, free the data
		struct bz_seat *seat_data = wl_seat_get_user_data(globals->seat);
		if (seat_data != nullptr) { free(seat_data); }
		// Then destroy the seat
		wl_seat_destroy(globals->seat);
		globals->seat = nullptr;
		globals->seat_name = 0;
	}
	// TODO: Remember to clean up other global types, such as outputs for hotplug events, etc.
}


// =================================================================================================
//  wl_shm
// -------------------------------------------------------------------------------------------------

static const struct wl_shm_listener bz_shm_implementation = {
	.format = bz_shm_format,
};

static void bz_shm_format(void * /*data*/, struct wl_shm * /*shm*/, uint32_t format)
{
	bz_debug(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Supported shm format: %d", format);
}


// =================================================================================================
//  wl_buffer
// -------------------------------------------------------------------------------------------------

const struct wl_buffer_listener bz_buffer_implementation = {
	.release = bz_buffer_release,
};

static void bz_buffer_release(void *data, struct wl_buffer *buffer)
{
	struct bz_buffer *bzbuff = data;
	bzbuff->is_released = true;
}


// =================================================================================================
//  wl_seat
// -------------------------------------------------------------------------------------------------

static const struct wl_seat_listener bz_seat_implementation = {
	.capabilities = bz_seat_capabilities,
	.name = bz_seat_name,
};

static void bz_seat_capabilities(void *data, struct wl_seat *wl_seat, uint32_t capabilities)
{
	bz_info(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Capabilities: %d", capabilities);
	struct bz_seat *seat_data = wl_seat_get_user_data(wl_seat);

	// Add / remove keyboard resource
	if (seat_data->keyboard == nullptr && capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
		seat_data->keyboard = wl_seat_get_keyboard(wl_seat);
		wl_keyboard_add_listener(seat_data->keyboard, &bz_keyboard_implementation, seat_data);
		seat_data->xkbcontext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	} else if (seat_data->keyboard != nullptr && (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) == 0) {
		wl_keyboard_release(seat_data->keyboard);
		// Destroy xkb state
		if (seat_data->xkbstate   != nullptr) { xkb_state_unref(seat_data->xkbstate); }
		if (seat_data->xkbkeymap  != nullptr) { xkb_keymap_unref(seat_data->xkbkeymap); }
		if (seat_data->xkbcontext != nullptr) { xkb_context_unref(seat_data->xkbcontext); }
	}

	// Add / remove pointer resource
	if (seat_data->pointer == nullptr && capabilities & WL_SEAT_CAPABILITY_POINTER) {
		seat_data->pointer = wl_seat_get_pointer(wl_seat);
		wl_pointer_add_listener(seat_data->pointer, &bz_pointer_implementation, seat_data);
	} else if (seat_data->pointer != nullptr && (capabilities & WL_SEAT_CAPABILITY_POINTER) == 0) {
		wl_pointer_release(seat_data->pointer);
	}
}

static void bz_seat_name(void *data, struct wl_seat *wl_seat, const char *name)
{
	bz_info(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Received seat name: %s", name);
}


// =================================================================================================
//  wl_keyboard
// -------------------------------------------------------------------------------------------------

static const struct wl_keyboard_listener bz_keyboard_implementation = {
	.keymap = bz_keyboard_keymap,
	.enter = bz_keyboard_enter,
	.leave = bz_keyboard_leave,
	.key = bz_keyboard_key,
	.modifiers = bz_keyboard_modifiers,
	.repeat_info = bz_keyboard_repeat_info,
};

static void bz_keyboard_keymap(
	void *data,
	struct wl_keyboard *wl_keyboard,
	uint32_t format,
	int32_t fd,
	uint32_t size
) {
	struct bz_seat *seat_data = data;

	// Make sure it's in xkb format
	if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
		bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Unsupported keyboard keymap format.");
		return;
	}

	// Initialize the rest of our xkb data
	void *map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	seat_data->xkbkeymap = xkb_keymap_new_from_string(
		seat_data->xkbcontext,
		map,
		XKB_KEYMAP_FORMAT_TEXT_V1,
		XKB_KEYMAP_COMPILE_NO_FLAGS
	);
	munmap(map, size);
	close(fd);
	seat_data->xkbstate = xkb_state_new(seat_data->xkbkeymap);
}

static void bz_keyboard_enter(
	void *data,
	struct wl_keyboard *wl_keyboard,
	uint32_t serial,
	struct wl_surface *surface,
	struct wl_array *keys
) {
	struct bz_seat *seat_data = data;
	if (seat_data->globals != nullptr && seat_data->globals->window != nullptr) {
		seat_data->globals->window->is_focused = true;
	}
}

static void bz_keyboard_leave(
	void *data,
	struct wl_keyboard *wl_keyboard,
	uint32_t serial,
	struct wl_surface *surface
) {
	struct bz_seat *seat_data = data;
	if (seat_data->globals != nullptr && seat_data->globals->window != nullptr) {
		seat_data->globals->window->is_focused = false;
	}
}

const uint32_t KONAMI_CODE_KEYSYMS[] = {
	XKB_KEY_Up,
	XKB_KEY_Up,
	XKB_KEY_Down,
	XKB_KEY_Down,
	XKB_KEY_Left,
	XKB_KEY_Right,
	XKB_KEY_Left,
	XKB_KEY_Right,
	XKB_KEY_b,
	XKB_KEY_a,
};

static void bz_keyboard_key(
	void *data,
	struct wl_keyboard *wl_keyboard,
	uint32_t serial,
	uint32_t time,
	uint32_t key,
	uint32_t state
) {
	struct bz_seat *seat_data = data;

	const uint32_t xkb_keycode = key + 8; // xkb keycode is offset by 8
	if (state == WL_KEYBOARD_KEY_STATE_REPEATED) {
		bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Unsupported key state: 'repeated'.");
		return;
	}
	enum xkb_key_direction press_state = state == WL_KEYBOARD_KEY_STATE_PRESSED
		? XKB_KEY_DOWN
		: XKB_KEY_UP;
	xkb_state_update_key(seat_data->xkbstate, xkb_keycode, press_state);

	const uint32_t xkb_keysym = xkb_state_key_get_one_sym(seat_data->xkbstate, xkb_keycode);

	// Update our konami code progress
	if (press_state == XKB_KEY_DOWN && !seat_data->globals->window->konami_active) {
		if (xkb_keysym == KONAMI_CODE_KEYSYMS[seat_data->globals->window->konami_count]) {
			seat_data->globals->window->konami_count++;
		} else {
			seat_data->globals->window->konami_count = 0;
		}
		if (seat_data->globals->window->konami_count == 10) {
			seat_data->globals->window->konami_active = true;
		}
	}

	switch (xkb_keysym) {
	case XKB_KEY_w:
	case XKB_KEY_W:
	case XKB_KEY_Up:
		seat_data->globals->window->wasd[0] = press_state == XKB_KEY_DOWN;
		break;
	case XKB_KEY_a:
	case XKB_KEY_A:
	case XKB_KEY_Left:
		seat_data->globals->window->wasd[1] = press_state == XKB_KEY_DOWN;
		break;
	case XKB_KEY_s:
	case XKB_KEY_S:
	case XKB_KEY_Down:
		seat_data->globals->window->wasd[2] = press_state == XKB_KEY_DOWN;
		break;
	case XKB_KEY_d:
	case XKB_KEY_D:
	case XKB_KEY_Right:
		seat_data->globals->window->wasd[3] = press_state == XKB_KEY_DOWN;
		break;
	case XKB_KEY_Shift_L:
	case XKB_KEY_Shift_R:
		seat_data->globals->window->shift = press_state == XKB_KEY_DOWN;
		break;
	}
}

static void bz_keyboard_modifiers(
	void *data,
	struct wl_keyboard *wl_keyboard,
	uint32_t serial,
	uint32_t mods_depressed,
	uint32_t mods_latched,
	uint32_t mods_locked,
	uint32_t group
) {
	bz_info(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Modifiers received! %d, %d, %d, %d, %d", serial, mods_depressed, mods_latched, mods_locked, group);
	// TODO
}

static void bz_keyboard_repeat_info(
	void *data,
	struct wl_keyboard *wl_keyboard,
	int32_t rate,
	int32_t delay
) {
	bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "wl_keyboard.repeat_info not implemented");
	// TODO
}


// =================================================================================================
//  wl_pointer
// -------------------------------------------------------------------------------------------------

static const struct wl_pointer_listener bz_pointer_implementation = {
	.enter = bz_pointer_enter,
	.leave = bz_pointer_leave,
	.motion = bz_pointer_motion,
	.button = bz_pointer_button,
	.axis = bz_pointer_axis,
	.frame = bz_pointer_frame,
	.axis_source = bz_pointer_axis_source,
	.axis_stop = bz_pointer_axis_stop,
	.axis_discrete = bz_pointer_axis_discrete,
	.axis_value120 = bz_pointer_axis_value120,
	.axis_relative_direction = bz_pointer_axis_relative_direction,
};


static void bz_pointer_enter(
	void *data,
	struct wl_pointer *wl_pointer,
	uint32_t serial,
	struct wl_surface *surface,
	wl_fixed_t surface_x,
	wl_fixed_t surface_y
) {
	bz_info(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Surface entered via pointer! Calling set_cursor.");

	struct bz_seat *seat_data = data;

	// "null" cursor surface will clear the display
	// wl_pointer_set_cursor(wl_pointer, serial, nullptr, 0, 0);
	wl_pointer_set_cursor(wl_pointer, serial, seat_data->globals->cursor->wlsurface, 0, 0);
}

static void bz_pointer_leave(
	void *data,
	struct wl_pointer *wl_pointer,
	uint32_t serial,
	struct wl_surface *surface
) {
	bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "wl_pointer.leave not implemented");
	// TODO
}

static void bz_pointer_motion(
	void *data,
	struct wl_pointer *wl_pointer,
	uint32_t time,
	wl_fixed_t surface_x,
	wl_fixed_t surface_y
) {
	bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "wl_pointer.motion not implemented");
	// TODO
}

static void bz_pointer_button(
	void *data,
	struct wl_pointer *wl_pointer,
	uint32_t serial,
	uint32_t time,
	uint32_t button,
	uint32_t state
) {
	bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "wl_pointer.button not implemented");
	// TODO
}

static void bz_pointer_axis(
	void *data,
	struct wl_pointer *wl_pointer,
	uint32_t time,
	uint32_t axis,
	wl_fixed_t value
) {
	bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "wl_pointer.axis not implemented");
	// TODO
}

static void bz_pointer_frame(void *data, struct wl_pointer *wl_pointer) {
	bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "wl_pointer.frame not implemented");
	// TODO
}

static void bz_pointer_axis_source(
	void *data,
	struct wl_pointer *wl_pointer,
	uint32_t axis_source
) {
	bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "wl_pointer.axis_source not implemented");
	// TODO
}

static void bz_pointer_axis_stop(
	void *data,
	struct wl_pointer *wl_pointer,
	uint32_t time,
	uint32_t axis
) {
	bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "wl_pointer.axis_stop not implemented");
	// TODO
}

static void bz_pointer_axis_discrete(
	void *data,
	struct wl_pointer *wl_pointer,
	uint32_t axis,
	int32_t discrete
) {
	bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "wl_pointer.axis_discrete not implemented");
	// TODO
}

static void bz_pointer_axis_value120(
	void *data,
	struct wl_pointer *wl_pointer,
	uint32_t axis,
	int32_t value120
) {
	bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "wl_pointer.axis_value120 not implemented");
	// TODO
}

static void bz_pointer_axis_relative_direction(
	void *data,
	struct wl_pointer *wl_pointer,
	uint32_t axis,
	uint32_t direction
) {
	bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "wl_pointer.axis_relative_direction not implemented");
	// TODO
}



// =================================================================================================
//  xdg_wm_base
// -------------------------------------------------------------------------------------------------

static const struct xdg_wm_base_listener bz_xdg_wm_base_implementation = {
	.ping = bz_xdg_wm_base_ping,
};

static void bz_xdg_wm_base_ping(void * /*data*/, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
	xdg_wm_base_pong(xdg_wm_base, serial);
}


// =================================================================================================
//  xdg_surface
// -------------------------------------------------------------------------------------------------

struct xdg_surface *bz_xdg_surface_constructor(
	struct bz_client_globals *globals,
	struct wl_surface *wlsurface
) {
	bz_debug(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Creating an XDG surface");
	struct xdg_surface *xdgsurf = xdg_wm_base_get_xdg_surface(globals->xdg_wm_base, wlsurface);
	xdg_surface_add_listener(xdgsurf, &bz_xdg_surface_implementation, globals);
	return xdgsurf;
}

static const struct xdg_surface_listener bz_xdg_surface_implementation = {
	.configure = bz_xdg_surface_configure,
};

static void bz_xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
	bz_debug(BZ_LOG_WAYLAND, __FILE__, __LINE__,
		"xdg_surface.configure(): Finalizing configure sequence for serial %d.", serial);
	struct bz_client_globals *client_globals = data;
	struct bz_application_window *window = client_globals->window;

	window->pending->serial = serial;

	// Promote our "pending" data to our "finalized" data.
	free(window->finalized);
	window->finalized = window->pending;
	window->pending = calloc(1, sizeof(*window->pending));

	window->size.w = window->finalized->recommended_size.w;
	window->size.h = window->finalized->recommended_size.h;

	window->circle_center.x = window->size.w/2;
	window->circle_center.y = window->size.h/2;

	// Build our buffer, ack our configure, and submit!
	xdg_surface_ack_configure(window->xdgsurface, window->finalized->serial);
	bz_initialize_surface_buffers(client_globals, window);
	bz_draw_frame(window);
	bz_submit_frame(window);
}

// ---  Helpers  -----------------------------------------------------------------------------------

static void bz_initialize_surface_buffers(
	struct bz_client_globals *globals,
	struct bz_application_window *window
) {
	struct bz_buff_alloc *allocation = bz_allocate_shm_buffers(
		window->size.w,
		window->size.h,
		2,
		globals->shm,
		&bz_buffer_implementation
	);

	window->pool_size = allocation->pool_size;
	window->pool_data = allocation->pool_data;
	window->shm_pool = allocation->shm_pool;
	window->buffers = allocation->buffers;
	window->active_buffer = 0;

	free(allocation);
}

static void bz_update_circle(struct bz_application_window *window, uint32_t new_time)
{
	// Don't bother updating if this is our first frame.
	if (window->prev_time == 0) {
		window->prev_time = new_time;
		return;
	}

	const uint32_t elapsed_ms = (new_time - window->prev_time);
	window->prev_time = new_time;

	// Calculate the distance the circle should move
	struct bz_position delta = {
		.x = window->circle_speed_x * elapsed_ms,
		.y = window->circle_speed_y * elapsed_ms,
	};

	window->circle_center.x += delta.x;
	window->circle_center.y += delta.y;

	window->circle_center.x %= (2 * window->size.w);
	window->circle_center.y %= (2 * window->size.h);
}

static void bz_control_circle(struct bz_application_window *window, uint32_t new_time)
{
	// Don't bother updating if this is our first frame.
	if (window->prev_time == 0) {
		window->prev_time = new_time;
		return;
	}

	const uint32_t elapsed_ms = (new_time - window->prev_time);
	window->prev_time = new_time;

	// Calculate the distance the circle should move
	struct bz_position delta = { .x = 0, .y = 0 };
	float speed = window->shift ? 2.0f : 1.0f;
	if (window->wasd[0]) { delta.y -= (speed * elapsed_ms); } // w (up)
	if (window->wasd[1]) { delta.x -= (speed * elapsed_ms); } // a (left)
	if (window->wasd[2]) { delta.y += (speed * elapsed_ms); } // s (down)
	if (window->wasd[3]) { delta.x += (speed * elapsed_ms); } // d (right)

	if (delta.x < 0 && -delta.x > window->circle_center.x) {
		window->circle_center.x = 0; // Prevents uint from going negative and wrapping.
	} else {
		window->circle_center.x += delta.x;
	}
	if (delta.y < 0 && -delta.y > window->circle_center.y) {
		window->circle_center.y = 0; // Prevents uint from going negative and wrapping.
	} else {
		window->circle_center.y += delta.y;
	}

	// Clamp to the window's bounds
	window->circle_center.x = bz_clamp(window->circle_center.x, 0, window->size.w);
	window->circle_center.y = bz_clamp(window->circle_center.y, 0, window->size.h);
}

#define BZ_BORDER_WIDTH 8
#define BZ_TITLE_WIDTH 40
#define BZ_CIRCLE_RADIUS 50
static void bz_draw_frame(struct bz_application_window *window)
{
	const struct bz_buffer *buffer = &window->buffers[window->active_buffer];
	if (!buffer->is_released) {
		bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Cannot draw frame on an unreleased buffer.");
		return;
	}

	int circle_x = (window->circle_center.x < window->size.w)
		? window->circle_center.x
		: (window->size.w - (window->circle_center.x - window->size.w));
	int circle_y = (window->circle_center.y < window->size.h)
		? window->circle_center.y
		: (window->size.h - (window->circle_center.y - window->size.h));

	for (int y = 0; y < buffer->size.h; y++) {
		for (int x = 0; x < buffer->size.w; x++) {
			if (window->is_focused && (
				x < BZ_BORDER_WIDTH ||
				x > (buffer->size.w - BZ_BORDER_WIDTH) ||
				y > (buffer->size.h - BZ_BORDER_WIDTH))
			) {
				buffer->pixel_data[y * buffer->size.w + x] = 0xFFFFFFFF;
			} else if (y < BZ_TITLE_WIDTH) {
				buffer->pixel_data[y * buffer->size.w + x] = 0xFFFFFFFF;
			} else if (window->konami_active &&
				abs(circle_x - x) < BZ_CIRCLE_RADIUS &&
				abs(circle_y - y) < BZ_CIRCLE_RADIUS
			) {
				buffer->pixel_data[y * buffer->size.w + x] = window->fg_color;
			} else if (bz_distance(circle_x, circle_y, x, y) < BZ_CIRCLE_RADIUS) {
				buffer->pixel_data[y * buffer->size.w + x] = window->fg_color;
			} else {
				buffer->pixel_data[y * buffer->size.w + x] = window->bg_color;
			}
		}
	}
}

static void bz_submit_frame(struct bz_application_window *window)
{
	struct bz_buffer *bzbuffer = &window->buffers[window->active_buffer];
	window->active_buffer = window->active_buffer == 0 ? 1 : 0;
	bzbuffer->is_released = false;

	wl_surface_attach(window->wlsurface, bzbuffer->buffer, 0, 0);
	// TODO-dl12: wl_surface_damage(window->wlsurface, 0, 0, INT32_MAX, INT32_MAX);

	// Set up our frame callback to get notified when our next frame should be drawn
	window->frame_callback = wl_surface_frame(window->wlsurface);
	const struct wl_callback_listener frame_listener = {
		.done = bz_render,
	};
	wl_callback_add_listener(window->frame_callback, &frame_listener, window);

	wl_surface_commit(window->wlsurface);
}

static void bz_render(void *data, struct wl_callback *wl_callback, uint32_t callback_data)
{
	// bz_info(BZ_LOG_WAYLAND, __FILE__, __LINE__, "RENDER CALLBACK TRIGGERED! Time: %d", callback_data); // TODO-dl9: delete

	struct bz_application_window *window = data;

	window->frame_callback = nullptr;
	wl_callback_destroy(wl_callback);

	if (!window->is_focused) {
		bz_update_circle(window, callback_data);
	} else {
		bz_control_circle(window, callback_data);
	}
	bz_draw_frame(window);
	bz_submit_frame(window);
}


// =================================================================================================
//  xdg_toplevel
// -------------------------------------------------------------------------------------------------

struct xdg_toplevel *bz_xdg_toplevel_constructor(
	struct bz_client_globals *globals,
	struct xdg_surface *xdgsurface
) {
	bz_debug(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Creating an XDG toplevel");
	struct xdg_toplevel *xdgtoplevel = xdg_surface_get_toplevel(xdgsurface);
	xdg_toplevel_add_listener(xdgtoplevel, &bz_xdg_toplevel_implementation, globals);
	// xdg_toplevel_set_title(xdgtoplevel, "Greetings from test client!");
	return xdgtoplevel;
}

static const struct xdg_toplevel_listener bz_xdg_toplevel_implementation = {
	.configure = bz_xdg_toplevel_configure,
	.close = bz_xdg_toplevel_close,
	.configure_bounds = bz_xdg_toplevel_configure_bounds,
	.wm_capabilities = bz_xdg_toplevel_wm_capabilities,
};

static void bz_xdg_toplevel_configure(
	void *data,
	struct xdg_toplevel *xdg_toplevel,
	int32_t width,
	int32_t height,
	struct wl_array *states
) {
	bz_debug(BZ_LOG_WAYLAND, __FILE__, __LINE__,
		"xdg_toplevel.configure(): Setting recommended bounds (%dx%d) and states on xdg toplevel.",
		width, height);
	struct bz_client_globals *client_globals = data;
	struct bz_application_window *window = client_globals->window;
	window->pending->recommended_size.w = width;
	window->pending->recommended_size.h = height;
	window->pending->states = states;
}

static void bz_xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
	bz_info(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Closing xdg_toplevel surface.");
	const struct bz_client_globals *client_globals = data;
	constexpr uint64_t val = 1;
	write(client_globals->is_quitting, &val, sizeof(val));
}

static void bz_xdg_toplevel_configure_bounds(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height)
{
	bz_debug(BZ_LOG_WAYLAND, __FILE__, __LINE__,
		"xdg_toplevel.configure_bounds(): Setting max bounds on xdg toplevel to %dx%d.",
		width, height);
	struct bz_client_globals *client_globals = data;
	struct bz_application_window *window = client_globals->window;
	window->pending->max_size.w = width;
	window->pending->max_size.h = height;
}

static void bz_xdg_toplevel_wm_capabilities(void * /*data*/, struct xdg_toplevel *xdg_toplevel, struct wl_array *capabilities)
{
	bz_error(BZ_LOG_WAYLAND, __FILE__, __LINE__, "xdg_toplevel.wm_capabilities not implemented");
	// TODO
}
