
#include "breezy/bz_graphics.h"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>

#include <gbm.h>
#include <libseat.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <glad/gles2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "breezy/bz_logger.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

// -- DRM --

static int drm_fd = -1;
static int drm_device_id = -1;
static uint32_t connector_id = 0;
static uint32_t crtc_id = 0;
static drmModeModeInfo mode_info;
static uint32_t mode_blob_id = 0;
static uint32_t plane_id = 0;

struct bz_drm_prop_ids prop_id_lookup = { 0 };

// Init
static int bz_drm_init(void);
// DRM resource
static drmModeConnector *bz_drm_get_first_valid_connector(const drmModeRes *resources);
static void bz_drm_print_modes(const drmModeConnector *connector);
static uint32_t bz_drm_find_valid_crtc(const drmModeRes *resources, const drmModeConnector *connector, int *crtc_index);
static uint32_t bz_drm_find_valid_plane(int crtc_index);
static uint32_t bz_drm_get_prop_id(uint32_t object_type, uint32_t object_id, char *prop_name);
// GBM / buffer
static void bz_drm_handle_pageflip(int /*fd*/, uint32_t /*sequence*/, uint32_t /*tv_sec*/, uint32_t /*tv_usec*/, void *user_data);
static void bz_drm_gbm_bo_destructor(struct gbm_bo *bo, void *data);
static uint32_t bz_drm_gbm_get_bo_fb(struct gbm_bo *bo);
// Commit
static int bz_drm_activate(void);
static int bz_drm_deactivate(void);
static int bz_drm_clear_plane(void);
static int bz_drm_atomic_commit_initial(uint32_t fb_id);
static int bz_drm_atomic_commit_recurring(uint32_t fb_id, struct gbm_bo *new_bo);

// -- GBM --

struct gbm_device *gbm_device = nullptr;
struct gbm_surface *gbm_surface = nullptr;
struct gbm_bo *prev_bo = nullptr;

// -- OpenGL --

static EGLDisplay bz_egl_display = nullptr;
static EGLConfig bz_egl_config = nullptr;
static EGLContext bz_egl_context = nullptr;
static EGLSurface bz_egl_surface = nullptr;

static int bz_gles_init(void);
static int bz_gles_load_egl_extensions(void);

static int bz_gles_assert_extension(const char *extensionList, const char *extensionName);
static char *bz_get_egl_error_text(EGLint error);
static void bz_gles_print_egl_error(char *function_name, EGLint error);

// -- Libseat --

static struct libseat *seat = nullptr;
static int seat_fd = -1;
static bool seat_active = false;

static int bz_libseat_init(void);

static void handle_enable_seat(struct libseat *s, void *data);
static void handle_disable_seat(struct libseat *s, void *data);


// =================================================================================================
//  DRM Device
// -------------------------------------------------------------------------------------------------

/**
 * Initializes our DRM resources by verifying the machine's capabilities, finding a valid connector,
 * mode, encoder, CRTC, and plane, preparing the atomic prop ID lookup map, and initializing the
 * GBM device.
 *
 * Returns 0 on success, or a negative value on failure.
 */
static int bz_drm_init(void)
{
	int retval = 0;

	// Make sure our inputs are good.
	if (drm_fd < 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to initialize DRM: drm_fd was not set.");
		retval = -1;
		goto exit;
	}

	// Set our expected capabilities
	if (drmSetClientCap(drm_fd, DRM_CLIENT_CAP_ATOMIC, 1) < 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Atomic modesetting not supported.");
		retval = -2;
		goto exit;
	}
	if (drmSetClientCap(drm_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1) < 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Universal planes not supported.");
		retval = -3;
		goto exit;
	}

	// Get all of our DRM resources so we can sift through our options.
	drmModeRes *resources = drmModeGetResources(drm_fd);
	if (resources == nullptr) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to get DRM resources.");
		retval = -4;
		goto exit;
	}

	// Grab our first connected connector
	drmModeConnector *connector = bz_drm_get_first_valid_connector(resources);
	if (connector == nullptr) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to find a suitable connector.");
		retval = -5;
		goto clean_resources;
	}
	connector_id = connector->connector_id;

	// Set up our chosen mode
	bz_drm_print_modes(connector);
	mode_info = connector->modes[0];
	const int result = drmModeCreatePropertyBlob(drm_fd, &mode_info, sizeof(mode_info), &mode_blob_id);
	if (result != 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to create mode_info blob.");
		retval = -6;
		goto clean_connector;
	}
	bz_info(BZ_LOG_GRAPHICS, __FILE__, __LINE__,
		"Chosen Mode: %dx%d", mode_info.hdisplay, mode_info.vdisplay);

	// Get an encoder / CRTC from the connector
	int crtc_index = -1;
	crtc_id = bz_drm_find_valid_crtc(resources, connector, &crtc_index);
	if (crtc_id == 0 || crtc_index == -1) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to find a suitable CRTC.");
		retval = -7;
		goto clean_connector;
	}

	plane_id = bz_drm_find_valid_plane(crtc_index);
	if (plane_id == 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to find a suitable Plane.");
		retval = -8;
		goto clean_connector;
	}

	// Build up a lookup map for all the properties we care about
	// -- Connector --
	prop_id_lookup.connector_crtc_id = bz_drm_get_prop_id(DRM_MODE_OBJECT_CONNECTOR, connector_id, "CRTC_ID");
	// -- CRTC --
	prop_id_lookup.crtc_active  = bz_drm_get_prop_id(DRM_MODE_OBJECT_CRTC, crtc_id, "ACTIVE");
	prop_id_lookup.crtc_mode_id = bz_drm_get_prop_id(DRM_MODE_OBJECT_CRTC, crtc_id, "MODE_ID");
	// -- Plane --
	prop_id_lookup.plane_fb_id   = bz_drm_get_prop_id(DRM_MODE_OBJECT_PLANE, plane_id, "FB_ID");
	prop_id_lookup.plane_crtc_id = bz_drm_get_prop_id(DRM_MODE_OBJECT_PLANE, plane_id, "CRTC_ID");
	prop_id_lookup.plane_src_x   = bz_drm_get_prop_id(DRM_MODE_OBJECT_PLANE, plane_id, "SRC_X");
	prop_id_lookup.plane_src_y   = bz_drm_get_prop_id(DRM_MODE_OBJECT_PLANE, plane_id, "SRC_Y");
	prop_id_lookup.plane_src_w   = bz_drm_get_prop_id(DRM_MODE_OBJECT_PLANE, plane_id, "SRC_W");
	prop_id_lookup.plane_src_h   = bz_drm_get_prop_id(DRM_MODE_OBJECT_PLANE, plane_id, "SRC_H");
	prop_id_lookup.plane_crtc_x  = bz_drm_get_prop_id(DRM_MODE_OBJECT_PLANE, plane_id, "CRTC_X");
	prop_id_lookup.plane_crtc_y  = bz_drm_get_prop_id(DRM_MODE_OBJECT_PLANE, plane_id, "CRTC_Y");
	prop_id_lookup.plane_crtc_w  = bz_drm_get_prop_id(DRM_MODE_OBJECT_PLANE, plane_id, "CRTC_W");
	prop_id_lookup.plane_crtc_h  = bz_drm_get_prop_id(DRM_MODE_OBJECT_PLANE, plane_id, "CRTC_H");

	// Create our GBM device.
	gbm_device = gbm_create_device(drm_fd);
	if (gbm_device == nullptr) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to create GBM device.");
		retval = -9;
		goto clean_connector;
	}

clean_connector:
	if (connector != nullptr) { drmModeFreeConnector(connector); }
clean_resources:
	if (resources != nullptr) { drmModeFreeResources(resources); }

exit:
	return retval;
}

/**
 * Find a connected connector. We're just using the first connected connector for now, but we'll
 * want to allow for multiple in the future. If a suitable connector is not found, then a nullptr
 * is returned.
 *
 * It's up to the caller to call "drmModeFreeConnector()" on the returned value when
 * they are finished with it.
 */
static drmModeConnector *bz_drm_get_first_valid_connector(const drmModeRes *resources)
{
	drmModeConnector *connector = nullptr;
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Found %d connectors.", resources->count_connectors);
	for (int i = 0; i < resources->count_connectors; i++) {
		connector = drmModeGetConnector(drm_fd, resources->connectors[i]);
		if (!connector) {
			bz_warn(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to get data for DRM connector.");
			continue;
		}
		if (connector->connection == DRM_MODE_CONNECTED) {
			bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "  Using connector %d.", i);
			break;
		}
		drmModeFreeConnector(connector);
	}

	return connector;
}

/**
 * Just a utility/debug function for now. Prints all available modes for the given connector.
 */
static void bz_drm_print_modes(const drmModeConnector *connector)
{
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Found %d modes.", connector->count_modes);
	for (int i = 0; i < connector->count_modes; i++) {
		const drmModeModeInfo mode = connector->modes[i];
		bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "  Mode: %dx%d, %d",
			mode.hdisplay, mode.vdisplay, mode.vrefresh);
	}
}

/**
 * Returns the id of the first valid CRTC for the given connector, or 0 if no valid CRTCs are found.
 * The provided "crtc_index" is populated with the index of the CRTC inside the "resources->crtcs"
 * array, which is a necessary value for finding a valid plane later on.
 */
static uint32_t bz_drm_find_valid_crtc(
	const drmModeRes *resources,
	const drmModeConnector *connector,
	int *crtc_index
) {
	uint32_t crtc = 0;

	// Search all possible encoders to find a valid one.
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Found %d encoders.", connector->count_encoders);
	for (int i = 0; i < connector->count_encoders && crtc == 0; ++i) {
		drmModeEncoder *encoder = drmModeGetEncoder(drm_fd, connector->encoders[i]);
		if (!encoder) {
			bz_warn(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to get data for DRM encoder.");
			continue;
		}

		// "possible_crtcs" is a bitmask where each set bit is an index into "resources->crtcs[]"
		// that this encoder is wired to.
		for (int j = 0; j < resources->count_crtcs; j++) {
			if (encoder->possible_crtcs & (1 << j)) {
				crtc = resources->crtcs[j];
				*crtc_index = j;
				bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__,
					"  Using CRTC ID %d with index %d.", crtc, j);
				break;
			}
		}

		drmModeFreeEncoder(encoder);
	}

	return crtc;
}

/**
 * Looks up and returns the id for a valid plane for the given CRTC index. Note that the CRTC index
 * is the INDEX of the CRTC connector inside the DRM resources->crtcs array, and *not* the ID of the
 * CRTC connector.
 *
 * If no valid planes are found, then a plane ID of 0 is returned;
 */
static uint32_t bz_drm_find_valid_plane(const int crtc_index)
{
	uint32_t _plane_id = 0;

	drmModePlaneRes *planes = drmModeGetPlaneResources(drm_fd);
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Found %d planes.", planes->count_planes);
	for (uint32_t i = 0; i < planes->count_planes && _plane_id == 0; i++) {
		drmModePlane *plane = drmModeGetPlane(drm_fd, planes->planes[i]);
		if (!plane) {
			bz_warn(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to get data for DRM plane.");
			continue;
		}

		// Determine if this plane works with our chosen CRTC
		if (plane->possible_crtcs & (1 << crtc_index)) {
			_plane_id = plane->plane_id;
			bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "  Using Plane ID %d.", _plane_id);
		}

		drmModeFreePlane(plane);
	}
	drmModeFreePlaneResources(planes);

	return _plane_id;
}

/**
 * Looks up the property id for the given object type, object id, and property name. Returns the id
 * if the property is found, or 0 if not.
 */
static uint32_t bz_drm_get_prop_id(
	const uint32_t object_type,
	const uint32_t object_id,
	char *prop_name
) {
	uint32_t prop_id = 0;
	drmModeObjectProperties *props = drmModeObjectGetProperties(drm_fd, object_id, object_type);
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Found %d object properties", props->count_props);
	for (uint32_t j = 0; j < props->count_props && prop_id == 0; j++) {
		drmModePropertyRes *p = drmModeGetProperty(drm_fd, props->props[j]);
		if (strcmp(p->name, prop_name) == 0) {
			bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__,
				"  Found %s with id %d", p->name, p->prop_id);
			prop_id = p->prop_id;
		}
		drmModeFreeProperty(p);
	}
	drmModeFreeObjectProperties(props);

	if (prop_id == 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__,
			"  Could not find property with name %s.", prop_name);
	}

	return prop_id;
}

/**
 * This function is registered as a callback to our drmEventContext as the page flip handler, and
 * is called whenever the DRM file descriptor has a page flip event published to it. This releases
 * the previous GBM buffer object for reuse and records the new one, which is provided via the
 * user_data parameter.
 */
void bz_drm_handle_pageflip(
	int /*fd*/,
	uint32_t /*sequence*/,
	uint32_t /*tv_sec*/,
	uint32_t /*tv_usec*/,
	void *user_data
) {
	// Release the previous buffer, and record the new one for the next iteration.
	gbm_surface_release_buffer(gbm_surface, prev_bo);
	prev_bo = user_data;
}

/**
 * This function is registered as a callback to each of our GBM buffer objects, and is called
 * whenever a buffer object is destroyed (expected to be during app shutdown). Its purpose is to
 * free custom-allocated user data.
 */
static void bz_drm_gbm_bo_destructor(struct gbm_bo * /*bo*/, void *data)
{
	struct bz_gbm_bo_data *d = data;
	drmModeRmFB(drm_fd, d->fb_id);
	free(d);
}

/**
 * Returns the framebuffer ID associated with the given buffer object. If the buffer object doesn't
 * yet have an associated framebuffer, then a new one is created with the DRM and saved to the
 * buffer object's internal user data.
 */
static uint32_t bz_drm_gbm_get_bo_fb(struct gbm_bo *bo)
{
	// If we've already processed this buffer object, then no need to reregister it.
	struct bz_gbm_bo_data *data = gbm_bo_get_user_data(bo);
	if (data) return data->fb_id;

	// Otherwise, initialize/register it and save off what we need.
	data = malloc(sizeof(*data)); // This *is* being freed in the bz_drm_gbm_bo_destructor above.
	data->fb_id = 0;

	// These are DIFFERENT planes than DRM planes. These planes are for representing memory regions
	// within the same buffer, commonly used for color formats like NV12. XRGB8888 only uses the
	// first plane, which means this should only iterate once, but the drmModeAddFB2 has an API
	// that accepts arrays for the maximum 4 planes.
	uint32_t handles[4] = {0}, strides[4] = {0}, offsets[4] = {0};
	for (int i = 0; i < gbm_bo_get_plane_count(bo); i++) {
		handles[i] = gbm_bo_get_handle_for_plane(bo, i).u32;
		strides[i] = gbm_bo_get_stride_for_plane(bo, i);
		offsets[i] = gbm_bo_get_offset(bo, i);
	}

	drmModeAddFB2(
		drm_fd,
		gbm_bo_get_width(bo),
		gbm_bo_get_height(bo),
		gbm_bo_get_format(bo),
		handles,
		strides,
		offsets,
		&data->fb_id,
		0
	);
	gbm_bo_set_user_data(bo, data, bz_drm_gbm_bo_destructor);

	return data->fb_id;
}

/**
 * Turns on all of our DRM settings whenever we reclaim DRM master. Returns the value of submitting
 * the atomic commit.
 */
static int bz_drm_activate(void)
{
	// Restore our latest, saved settings from our most recent framebuffer
	if (prev_bo != nullptr) {
		const uint32_t fb_id = bz_drm_gbm_get_bo_fb(prev_bo);
		return bz_drm_atomic_commit_initial(fb_id);
	}
	return 0;
}

/**
 * Clears all of our DRM settings whenever we give up DRM master. Returns the value of submitting
 * the atomic commit.
 */
static int bz_drm_deactivate(void)
{
	drmModeAtomicReq *req = drmModeAtomicAlloc();

	// Clear CRTC properties
	drmModeAtomicAddProperty(req, crtc_id, prop_id_lookup.crtc_active, 0);
	drmModeAtomicAddProperty(req, crtc_id, prop_id_lookup.crtc_mode_id, 0);
	// Clear framebuffer properties FOR ALL PLANES
	drmModeAtomicAddProperty(req, plane_id, prop_id_lookup.plane_fb_id, 0);
	drmModeAtomicAddProperty(req, plane_id, prop_id_lookup.plane_crtc_id, 0);

	const int retVal = drmModeAtomicCommit(drm_fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET, nullptr);
	drmModeAtomicFree(req);

	return retVal;
}

/**
 * Clears the plane with a synchronous, atomic modeset. Returns the value of submitting the atomic
 * commit.
 */
static int bz_drm_clear_plane(void)
{
	drmModeAtomicReq *req = drmModeAtomicAlloc();
	drmModeAtomicAddProperty(req, plane_id, prop_id_lookup.plane_fb_id, 0);
	drmModeAtomicAddProperty(req, plane_id, prop_id_lookup.plane_crtc_id, 0);

	const int retVal = drmModeAtomicCommit(drm_fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET, nullptr);
	drmModeAtomicFree(req);

	return retVal;
}

/**
 * Submits the initial atomic commit needed to configure all of our DRM resources. This is a
 * blocking call. Returns the value of submitting the atomic commit.
 */
static int bz_drm_atomic_commit_initial(const uint32_t fb_id)
{
	drmModeAtomicReq *req = drmModeAtomicAlloc();

	// Set Connector properties
	drmModeAtomicAddProperty(req, connector_id, prop_id_lookup.connector_crtc_id, crtc_id);

	// Set CRTC properties
	drmModeAtomicAddProperty(req, crtc_id, prop_id_lookup.crtc_active, 1);
	drmModeAtomicAddProperty(req, crtc_id, prop_id_lookup.crtc_mode_id, mode_blob_id);

	// Set Plane properties
	const uint32_t width = mode_info.hdisplay;
	const uint32_t height = mode_info.vdisplay;
	drmModeAtomicAddProperty(req, plane_id, prop_id_lookup.plane_fb_id, fb_id);
	drmModeAtomicAddProperty(req, plane_id, prop_id_lookup.plane_crtc_id, crtc_id);
	drmModeAtomicAddProperty(req, plane_id, prop_id_lookup.plane_src_x, 0 << 16);
	drmModeAtomicAddProperty(req, plane_id, prop_id_lookup.plane_src_y, 0 << 16);
	drmModeAtomicAddProperty(req, plane_id, prop_id_lookup.plane_src_w, width << 16);
	drmModeAtomicAddProperty(req, plane_id, prop_id_lookup.plane_src_h, height << 16);
	drmModeAtomicAddProperty(req, plane_id, prop_id_lookup.plane_crtc_x, 0);
	drmModeAtomicAddProperty(req, plane_id, prop_id_lookup.plane_crtc_y, 0);
	drmModeAtomicAddProperty(req, plane_id, prop_id_lookup.plane_crtc_w, width);
	drmModeAtomicAddProperty(req, plane_id, prop_id_lookup.plane_crtc_h, height);

	const int retVal = drmModeAtomicCommit(drm_fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET, nullptr);
	drmModeAtomicFree(req);

	return retVal;
}

/**
 * Submits the recurring atomic commits needed to update the framebuffer ID for our
 * double-buffering. This is a non-blocking call. Returns the value of submitting the atomic commit.
 */
static int bz_drm_atomic_commit_recurring(const uint32_t fb_id, struct gbm_bo *new_bo)
{
	// Only need to update the framebuffer id for most frames.
	drmModeAtomicReq *req = drmModeAtomicAlloc();
	drmModeAtomicAddProperty(req, plane_id, prop_id_lookup.plane_fb_id, fb_id);

	constexpr uint32_t flags = DRM_MODE_ATOMIC_NONBLOCK | DRM_MODE_PAGE_FLIP_EVENT;
	const int retVal = drmModeAtomicCommit(drm_fd, req, flags, new_bo);
	drmModeAtomicFree(req);

	return retVal;
}


// =================================================================================================
//  OpenGL
// -------------------------------------------------------------------------------------------------

/**
 * Initializes EGL and OpenGL ES, loading the APIs through GLAD. Also loads and compiles our shader
 * programs. A negative integer is returned on failure, or 0 for success.
 */
static int bz_gles_init(void)
{
	// Create the EGL display
	bz_egl_display = eglGetPlatformDisplay(EGL_PLATFORM_GBM_KHR, gbm_device, nullptr);
	if (bz_egl_display == EGL_NO_DISPLAY) {
		bz_gles_print_egl_error("eglGetPlatformDisplay()", eglGetError());
		return -1;
	}

	// Initialize the EGL display
	int major_egl_version;
	int minor_egl_version;
	if (!eglInitialize(bz_egl_display, &major_egl_version, &minor_egl_version)) {
		bz_gles_print_egl_error("eglInitialize()", eglGetError());
		return -2;
	}
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "EGL initialized with version %d.%d",
		major_egl_version, minor_egl_version);

	// Bind to OpenGL ES
	if (!eglBindAPI(EGL_OPENGL_ES_API)) {
		bz_gles_print_egl_error("eglBindAPI()", eglGetError());
		return -3;
	}

	// Configure EGL
	const EGLint config_attributes[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 0,
		EGL_DEPTH_SIZE, 16,
		EGL_STENCIL_SIZE, 8,
		EGL_NONE
	};
	EGLint num_configs;
	if (!eglChooseConfig(bz_egl_display, config_attributes, &bz_egl_config, 1, &num_configs)) {
		bz_gles_print_egl_error("eglChooseConfig()", eglGetError());
		return -4;
	}
	if (num_configs == 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "No matching EGL configs found.");
		return -5;
	}

	// Create the EGL context
	const EGLint ctx_attrs[] = {
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_CONTEXT_MINOR_VERSION, 2,
		EGL_CONTEXT_OPENGL_DEBUG, EGL_FALSE, // "EGL_TRUE" for debugging.
		EGL_NONE
	};
	bz_egl_context = eglCreateContext(bz_egl_display, bz_egl_config, EGL_NO_CONTEXT, ctx_attrs);
	if (bz_egl_context == EGL_NO_CONTEXT) {
		bz_gles_print_egl_error("eglCreateContext()", eglGetError());
		return -6;
	}

	// Create the GBM surface
	gbm_surface = gbm_surface_create(
		gbm_device,
		mode_info.hdisplay,
		mode_info.vdisplay,
		GBM_FORMAT_XRGB8888,
		GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING
	);
	if (gbm_surface == nullptr) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to create GBM surface.");
		return -7;
	}

	// Create the EGL surface
	bz_egl_surface = eglCreatePlatformWindowSurface(bz_egl_display, bz_egl_config, gbm_surface, nullptr);
	if (bz_egl_surface == EGL_NO_SURFACE) {
		bz_gles_print_egl_error("eglCreatePlatformWindowSurface()", eglGetError());
		return -8;
	}

	if (!eglMakeCurrent(bz_egl_display, bz_egl_surface, bz_egl_surface, bz_egl_context)) {
		bz_gles_print_egl_error("eglMakeCurrent()", eglGetError());
		return -9;
	}

	// Load our OpenGL ES API through GLAD
	const int glad_version = gladLoadGLES2(eglGetProcAddress);
	if (glad_version == 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to laod OpenGL ES via GLAD.");
		return -10;
	}
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "GLAD Version: %d.%d",
		GLAD_VERSION_MAJOR(glad_version), GLAD_VERSION_MINOR(glad_version));

	// Define some GLES configs
	glClearColor(0.16f, 0.164f, 0.196f, 1.0f);

	// Load our shaders
	// TODO: Future video :)

	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Successfully initialized GLES.");
	return 0;
}

/**
 * Verifies all necessary EGL extensions are installed and available. Returns 0 on success, or a
 * negative value on failure.
 */
static int bz_gles_load_egl_extensions(void)
{
	const char *extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Extensions: %s", extensions);

	// Make sure the necessary extension(s) exist
	int failure = 0;
	failure |= bz_gles_assert_extension(extensions, "EGL_KHR_platform_gbm");
	if (failure) return -1;

	return 0;
}

/**
 * Helper function that returns a value of "1" if the given extension name doesn't exist in the
 * provided extensionList, or "0" otherwise. The extensionList is a space-separated list of EGL
 * extensions.
 */
static int bz_gles_assert_extension(const char *extensionList, const char *extensionName)
{
	if (!strstr(extensionList, extensionName)) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Missing extension: %s", extensionName);
		return 1;
	}
	return 0;
}

/** Helper function that returns a human-readable string name for the given EGL error code. */
static char *bz_get_egl_error_text(const EGLint error)
{
	if (error == EGL_SUCCESS)             { return "EGL_SUCCESS"; } // Task failed successfully!
	if (error == EGL_NOT_INITIALIZED)     { return "EGL_NOT_INITIALIZED"; }
	if (error == EGL_BAD_ACCESS)          { return "EGL_BAD_ACCESS"; }
	if (error == EGL_BAD_ALLOC)           { return "EGL_BAD_ALLOC"; }
	if (error == EGL_BAD_ATTRIBUTE)       { return "EGL_BAD_ATTRIBUTE"; }
	if (error == EGL_BAD_CONTEXT)         { return "EGL_BAD_CONTEXT"; }
	if (error == EGL_BAD_CONFIG)          { return "EGL_BAD_CONFIG"; }
	if (error == EGL_BAD_CURRENT_SURFACE) { return "EGL_BAD_CURRENT_SURFACE"; }
	if (error == EGL_BAD_DISPLAY)         { return "EGL_BAD_DISPLAY"; }
	if (error == EGL_BAD_SURFACE)         { return "EGL_BAD_SURFACE"; }
	if (error == EGL_BAD_MATCH)           { return "EGL_BAD_MATCH"; }
	if (error == EGL_BAD_PARAMETER)       { return "EGL_BAD_PARAMETER"; }
	if (error == EGL_BAD_NATIVE_PIXMAP)   { return "EGL_BAD_NATIVE_PIXMAP"; }
	if (error == EGL_BAD_NATIVE_WINDOW)   { return "EGL_BAD_NATIVE_WINDOW"; }
	if (error == EGL_CONTEXT_LOST)        { return "EGL_CONTEXT_LOST"; }
	return "(unrecognized error code)";
}

/** Helper function that prints a formatted error for a given, failed EGL function call. */
static void bz_gles_print_egl_error(char *function_name, const EGLint error)
{
	bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to call %s: %s",
		function_name, bz_get_egl_error_text(error));
}


// =================================================================================================
//  Seat Management
// -------------------------------------------------------------------------------------------------

/**
 * Our libseat seat listener interface implementation. Enable and disable seat will be called
 * whenever our seat is activated or deactivated. Our handlers enable/disable the DRM/rendering
 * stuff as necessary.
 */
static const struct libseat_seat_listener seat_listener = {
	.enable_seat = handle_enable_seat,
	.disable_seat = handle_disable_seat,
};

/**
 * Initializes our libseat implementation by opening a seat for our primary graphics card and
 * ensuring we have DRM master. Returns 0 on success, or a negative integer on failure.
 */
static int bz_libseat_init(void) {
	// Open a seat
	seat = libseat_open_seat(&seat_listener, nullptr);
	if (!seat) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to open seat.");
		return -1;
	}

	// Initial dispatch to trigger "handle_enable_seat()"
	if (libseat_dispatch(seat, 1000) < 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Initial libseat_dispatch failed.");
		return -2;
	}

	if (!seat_active) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Seat not active after open.");
		libseat_close_seat(seat);
		return -3;
	}

	seat_fd = libseat_get_fd(seat);

	// Open our DRM device / graphics card, and claim master
	// TODO: Swap this out to choose a graphics device more intelligently with udev.
	drm_device_id = libseat_open_device(seat, "/dev/dri/card1", &drm_fd);
	if (drm_fd < 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to open DRM device.");
		libseat_close_seat(seat);
		return -4;
	}
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Successfully opened DRM device.");

	// (Make sure we're master)
	if (!drmIsMaster(drm_fd)) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to claim DRM master.");
		return -5;
	}
	bz_info(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Successfully claimed DRM master.");

	return 0;
}

/**
 * Enables our seat. If it was previously enabled, the proper DRM commands are run to re-enable
 * our rendering state/buffer.
 */
static void handle_enable_seat(struct libseat * /*s*/, void * /*data*/) {
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Enabling seat and claiming DRM Master.");
	bz_drm_activate();
	seat_active = true;
}

/** Disables our seat, and deactivates our DRM resources. */
static void handle_disable_seat(struct libseat *s, void * /*data*/) {
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Disabling seat and releasing DRM Master.");
	bz_drm_deactivate();
	seat_active = false;

	// Acknowledge we're done, releasing DRM Master.
	if (libseat_disable_seat(s) != 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to release DRM Master.");
	}
}


// =================================================================================================
//  Exposed API
// -------------------------------------------------------------------------------------------------

/**
 * Initializes our rendering pipeline by verifying the necessary EGL extensions exist, initializing
 * libseat and our DRM master claim, establishes our DRM resource pipeline, and initializes EGL with
 * OpenGL ES.
 *
 * Returns 0 on success, or a negative value on failure.
 *
 * During the application's final cleanup, the calling code is expected to call
 * "bz_graphics_cleanup()" to free and release all DRM-related resources whether this function
 * returned successfully or not.
 */
int bz_graphics_initialize(void)
{
	int retval = bz_gles_load_egl_extensions();
	if (retval != 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__,
			"Failed to load EGL extensions. Code: %d", retval);
		return -1;
	}

	retval = bz_libseat_init();
	if (retval != 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__,
			"Failed to initialize libseat. Code: %d", retval);
		return -2;
	}

	retval = bz_drm_init();
	if (retval != 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__,
			"Failed to initialize DRM. Code: %d", retval);
		return -3;
	}

	retval = bz_gles_init();
	if (retval != 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__,
			"Failed to initialize GLES. Code: %d", retval);
		return -4;
	}

	return 0;
}

/**
 * Handles one loop iteration for our main rendering loop. Checks our main file descriptors for
 * new events to dispatch, renders the current application state, then swaps our front and back
 * buffers.
 *
 * A value of "0" is returned on success, "1" for non-failure (but the current seat is not active,
 * so no rendering updates made), or a negative value for failure.
 */
int bz_graphics_loop_iteration(void) {
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Executing graphics event loop iteration.");

	// Check/block for changes to our connection status
	struct pollfd fds[2] = {
		{ .fd = seat_fd, .events = POLLIN },
		{ .fd = drm_fd,  .events = POLLIN },
	};

	// 0 indicates a timeout, -1 indicates failure
	const int ret = poll(fds, 2, 1000);
	if (ret < 0) return -1;
	if (ret == 0) bz_info(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Timed out polling FDs.");

	// Handle any potential updates from our seat
	if (fds[0].revents & POLLIN) {
		bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Handling seat_fd event.");
		if (libseat_dispatch(seat, 0) < 0) {
			bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "libseat_dispatch failed.");
		}
	}

	// Handle any potential updates from our DRM device
	if (fds[1].revents & POLLIN) {
		bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Handling drm_fd event.");
		drmEventContext drm_event_context = {
			.version = DRM_EVENT_CONTEXT_VERSION,
			.page_flip_handler = bz_drm_handle_pageflip,
		};
		drmHandleEvent(drm_fd, &drm_event_context);
	}

	if (!seat_active) {
		return 1;
	}

	// Render!
	glClear(GL_COLOR_BUFFER_BIT);
	// (...other OpenGL render commands go here...)

	// Buffer switcheroo
	if (!gbm_surface_has_free_buffers(gbm_surface)) {
		bz_warn(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "GBM surface has no free buffers.");
		return -1;
	}
	eglSwapBuffers(bz_egl_display, bz_egl_surface);
	struct gbm_bo *bo = gbm_surface_lock_front_buffer(gbm_surface);
	const uint32_t fb_id = bz_drm_gbm_get_bo_fb(bo);

	// Scanout!
	int retval = 0;
	if (prev_bo == nullptr) {
		// Initial modeset is a blocking operation
		retval = bz_drm_atomic_commit_initial(fb_id);
		prev_bo = bo;
	} else {
		// Recurring commits are non-blocking, so wait for it to finish before releasing the old bo.
		retval = bz_drm_atomic_commit_recurring(fb_id, bo);
		// (When the DRM page flip completes, "bz_drm_handle_pageflip()" is called, which releases
		// the old buffer object.)
	}
	if (retval != 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to make an atomic commit.");
	}

	return retval;
}

/**
 * Does the final tear-down our graphics system, destroying or clearing all values that may have
 * been initialized. It's safe (and expected) to call this even if initialization failed, and is
 * expected to call this when the application is terminated. No guarantees are made around trying
 * to re-initialize the graphics system after calling this cleanup function.
 */
void bz_graphics_cleanup(void) {
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Cleaning up bz_graphics.");

	// -- GLES --
	// TODO: Delete shader programs (future video)

	// -- EGL --
	if (bz_egl_display != nullptr) {
		eglMakeCurrent(bz_egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	}
	if (bz_egl_context) {
		eglDestroyContext(bz_egl_display, bz_egl_context);
		bz_egl_context = nullptr;
	}
	if (bz_egl_surface != nullptr) {
		eglDestroySurface(bz_egl_display, bz_egl_surface);
		bz_egl_surface = nullptr;
	}
	if (bz_egl_display && !eglTerminate(bz_egl_display)) {
		bz_gles_print_egl_error("eglTerminate()", eglGetError());
	}
	bz_egl_display = nullptr;
	if (!eglReleaseThread()) {
		bz_gles_print_egl_error("eglReleaseThread()", eglGetError());
	}

	// -- GBM --
	if (gbm_surface != nullptr && prev_bo != nullptr) {
		gbm_surface_release_buffer(gbm_surface, prev_bo);
		prev_bo = nullptr;
	}
	if (gbm_surface != nullptr) {
		gbm_surface_destroy(gbm_surface);
		gbm_surface = nullptr;
	}
	if (gbm_device != nullptr) {
		gbm_device_destroy(gbm_device);
		gbm_device = nullptr;
	}

	// -- DRM / Libseat --
	if (plane_id != 0) {
		bz_drm_clear_plane();
	}
	if (mode_blob_id != 0 && drm_fd != -1) {
		drmModeDestroyPropertyBlob(drm_fd, mode_blob_id);
		mode_blob_id = 0;
	}
	if (seat != nullptr && drm_device_id != -1) {
		libseat_close_device(seat, drm_device_id);
		drm_device_id = -1;
	}
	if (drm_fd != -1) {
		close(drm_fd);
		drm_fd = -1;
	}
	if (seat != nullptr) {
		libseat_close_seat(seat);
		seat = nullptr;
	}

	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "DRM clean-up complete!");
}