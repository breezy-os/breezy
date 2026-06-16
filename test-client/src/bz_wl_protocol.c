
#include "breezy/bz_wl_protocol.h"

#include <string.h>

#include "breezy/bz_logger.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

// -- wl_registry --

static const struct wl_registry_listener bz_registry_implementation;
static void bz_registry_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
static void bz_registry_global_remove(void *data, struct wl_registry *registry, uint32_t name);


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
	} else if (strcmp(interface, wl_shm_interface.name) == 0) {
		// TODO
	}
}

static void bz_registry_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
	bz_info(BZ_LOG_WAYLAND, __FILE__, __LINE__, "Removing global with name: %d", name);
	struct bz_client_globals *globals = data;

	if (name == globals->compositor_name) {
		wl_compositor_destroy(globals->compositor);
		globals->compositor = nullptr;
		globals->compositor_name = 0;
	}
	// TODO: Remove other global types, such as outputs for hotplug events, etc.
}
