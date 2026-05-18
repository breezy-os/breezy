
#include "breezy/bz_graphics.h"

#include <string.h>
#include <stdlib.h>

#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <glad/gles2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "breezy/bz_logger.h"
#include "breezy/bz_seat.h"


// =================================================================================================
//  File Variables / Declarations
// -------------------------------------------------------------------------------------------------

// -- DRM --

static struct bz_drm_prop_ids prop_id_lookup = { 0 };
// Init
static int bz_drm_init(struct bz_breezy *breezy);
// DRM resource
static drmModeConnector *bz_drm_get_first_valid_connector(int drm_fd, const drmModeRes *resources);
static void bz_drm_print_modes(const drmModeConnector *connector);
static uint32_t bz_drm_find_valid_crtc(int drm_fd, const drmModeRes *resources, const drmModeConnector *connector, int *crtc_index);
static uint32_t bz_drm_find_valid_plane(int drm_fd, int crtc_index);
static uint32_t bz_drm_get_prop_id(int drm_fd, uint32_t object_type, uint32_t object_id, char *prop_name);
// GBM / buffer
static void bz_drm_handle_pageflip(int /*fd*/, uint32_t /*sequence*/, uint32_t /*tv_sec*/, uint32_t /*tv_usec*/, void *user_data);
static void bz_drm_gbm_bo_destructor(struct gbm_bo *bo, void *data);
static uint32_t bz_drm_gbm_get_bo_fb(struct bz_breezy *breezy, struct gbm_bo *bo);
// Commit
static int bz_drm_clear_plane(struct bz_breezy *breezy);
static int bz_drm_atomic_commit_initial(struct bz_breezy *breezy, uint32_t fb_id);
static int bz_drm_atomic_commit_recurring(struct bz_breezy *breezy, uint32_t fb_id);

// -- OpenGL --

// Init
static int bz_gles_init(struct bz_breezy *breezy);
static int bz_gles_load_egl_extensions(void);
// Helpers
static int bz_gles_assert_extension(const char *extensionList, const char *extensionName);
static char *bz_get_egl_error_text(EGLint error);
static void bz_gles_print_egl_error(char *function_name, EGLint error);


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
static int bz_drm_init(struct bz_breezy *breezy)
{
	int retval = 0;

	const int drm_fd = breezy->drm.fd;

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
	drmModeConnector *connector = bz_drm_get_first_valid_connector(drm_fd, resources);
	if (connector == nullptr) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to find a suitable connector.");
		retval = -5;
		goto clean_resources;
	}
	breezy->drm.connector_id = connector->connector_id;

	// Set up our chosen mode
	bz_drm_print_modes(connector);
	breezy->drm.mode_info = connector->modes[0];
	const int result = drmModeCreatePropertyBlob(
		drm_fd,
		&breezy->drm.mode_info,
		sizeof(breezy->drm.mode_info),
		&breezy->drm.mode_blob_id
	);
	if (result != 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to create mode_info blob.");
		retval = -6;
		goto clean_connector;
	}
	bz_info(BZ_LOG_GRAPHICS, __FILE__, __LINE__,
		"Chosen Mode: %dx%d", breezy->drm.mode_info.hdisplay, breezy->drm.mode_info.vdisplay);

	// Get an encoder / CRTC from the connector
	int crtc_index = -1;
	breezy->drm.crtc_id = bz_drm_find_valid_crtc(drm_fd, resources, connector, &crtc_index);
	if (breezy->drm.crtc_id == 0 || crtc_index == -1) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to find a suitable CRTC.");
		retval = -7;
		goto clean_connector;
	}

	breezy->drm.plane_id = bz_drm_find_valid_plane(drm_fd, crtc_index);
	if (breezy->drm.plane_id == 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to find a suitable Plane.");
		retval = -8;
		goto clean_connector;
	}

	// Build up a lookup map for all the properties we care about
	const uint32_t connector_id = breezy->drm.connector_id;
	const uint32_t crtc_id = breezy->drm.crtc_id;
	const uint32_t plane_id = breezy->drm.plane_id;
	// -- Connector --
	prop_id_lookup.connector_crtc_id = bz_drm_get_prop_id(drm_fd, DRM_MODE_OBJECT_CONNECTOR, connector_id, "CRTC_ID");
	// -- CRTC --
	prop_id_lookup.crtc_active  = bz_drm_get_prop_id(drm_fd, DRM_MODE_OBJECT_CRTC, crtc_id, "ACTIVE");
	prop_id_lookup.crtc_mode_id = bz_drm_get_prop_id(drm_fd, DRM_MODE_OBJECT_CRTC, crtc_id, "MODE_ID");
	// -- Plane --
	prop_id_lookup.plane_fb_id   = bz_drm_get_prop_id(drm_fd, DRM_MODE_OBJECT_PLANE, plane_id, "FB_ID");
	prop_id_lookup.plane_crtc_id = bz_drm_get_prop_id(drm_fd, DRM_MODE_OBJECT_PLANE, plane_id, "CRTC_ID");
	prop_id_lookup.plane_src_x   = bz_drm_get_prop_id(drm_fd, DRM_MODE_OBJECT_PLANE, plane_id, "SRC_X");
	prop_id_lookup.plane_src_y   = bz_drm_get_prop_id(drm_fd, DRM_MODE_OBJECT_PLANE, plane_id, "SRC_Y");
	prop_id_lookup.plane_src_w   = bz_drm_get_prop_id(drm_fd, DRM_MODE_OBJECT_PLANE, plane_id, "SRC_W");
	prop_id_lookup.plane_src_h   = bz_drm_get_prop_id(drm_fd, DRM_MODE_OBJECT_PLANE, plane_id, "SRC_H");
	prop_id_lookup.plane_crtc_x  = bz_drm_get_prop_id(drm_fd, DRM_MODE_OBJECT_PLANE, plane_id, "CRTC_X");
	prop_id_lookup.plane_crtc_y  = bz_drm_get_prop_id(drm_fd, DRM_MODE_OBJECT_PLANE, plane_id, "CRTC_Y");
	prop_id_lookup.plane_crtc_w  = bz_drm_get_prop_id(drm_fd, DRM_MODE_OBJECT_PLANE, plane_id, "CRTC_W");
	prop_id_lookup.plane_crtc_h  = bz_drm_get_prop_id(drm_fd, DRM_MODE_OBJECT_PLANE, plane_id, "CRTC_H");

	// Create our GBM device.
	breezy->gbm.device = gbm_create_device(drm_fd);
	if (breezy->gbm.device == nullptr) {
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
static drmModeConnector *bz_drm_get_first_valid_connector(const int drm_fd, const drmModeRes *resources)
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
	const int drm_fd,
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
static uint32_t bz_drm_find_valid_plane(const int drm_fd, const int crtc_index)
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
	const int drm_fd,
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
 * the previous GBM buffer object for reuse and records the new one.
 */
void bz_drm_handle_pageflip(
	int /*fd*/,
	uint32_t /*sequence*/,
	uint32_t /*tv_sec*/,
	uint32_t /*tv_usec*/,
	void *user_data
) {
	struct bz_breezy *breezy = user_data;
	// Release the previous buffer, and record the new one for the next iteration.
	gbm_surface_release_buffer(breezy->gbm.surface, breezy->gbm.prev_bo);
	breezy->gbm.prev_bo = breezy->gbm.new_bo;
	breezy->gbm.new_bo = nullptr;
}

/**
 * This function is registered as a callback to each of our GBM buffer objects, and is called
 * whenever a buffer object is destroyed (expected to be during app shutdown). Its purpose is to
 * free custom-allocated user data.
 */
static void bz_drm_gbm_bo_destructor(struct gbm_bo * /*bo*/, void *data)
{
	struct bz_gbm_bo_data *d = data;
	drmModeRmFB(d->breezy->drm.fd, d->fb_id);
	free(d);
}

/**
 * Returns the framebuffer ID associated with the given buffer object. If the buffer object doesn't
 * yet have an associated framebuffer, then a new one is created with the DRM and saved to the
 * buffer object's internal user data.
 */
static uint32_t bz_drm_gbm_get_bo_fb(struct bz_breezy *breezy, struct gbm_bo *bo)
{
	// If we've already processed this buffer object, then no need to reregister it.
	struct bz_gbm_bo_data *data = gbm_bo_get_user_data(bo);
	if (data) return data->fb_id;

	// Otherwise, initialize/register it and save off what we need.
	data = malloc(sizeof(*data)); // This *is* being freed in the bz_drm_gbm_bo_destructor above.
	data->fb_id = 0;
	data->breezy = breezy;

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
		breezy->drm.fd,
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
 * Clears the plane with a synchronous, atomic modeset. Returns the value of submitting the atomic
 * commit.
 */
static int bz_drm_clear_plane(struct bz_breezy *breezy)
{
	drmModeAtomicReq *req = drmModeAtomicAlloc();
	drmModeAtomicAddProperty(req, breezy->drm.plane_id, prop_id_lookup.plane_fb_id, 0);
	drmModeAtomicAddProperty(req, breezy->drm.plane_id, prop_id_lookup.plane_crtc_id, 0);

	const int retVal = drmModeAtomicCommit(breezy->drm.fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET, breezy);
	drmModeAtomicFree(req);

	return retVal;
}

/**
 * Submits the initial atomic commit needed to configure all of our DRM resources. This is a
 * blocking call. Returns the value of submitting the atomic commit.
 */
static int bz_drm_atomic_commit_initial(struct bz_breezy *breezy, const uint32_t fb_id)
{
	drmModeAtomicReq *req = drmModeAtomicAlloc();
	const uint32_t connector_id = breezy->drm.connector_id;
	const uint32_t crtc_id = breezy->drm.crtc_id;
	const uint32_t mode_blob_id = breezy->drm.mode_blob_id;
	const uint32_t plane_id = breezy->drm.plane_id;

	// Set Connector properties
	drmModeAtomicAddProperty(req, connector_id, prop_id_lookup.connector_crtc_id, crtc_id);

	// Set CRTC properties
	drmModeAtomicAddProperty(req, crtc_id, prop_id_lookup.crtc_active, 1);
	drmModeAtomicAddProperty(req, crtc_id, prop_id_lookup.crtc_mode_id, mode_blob_id);

	// Set Plane properties
	const uint32_t width = breezy->drm.mode_info.hdisplay;
	const uint32_t height = breezy->drm.mode_info.vdisplay;
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

	const int retVal = drmModeAtomicCommit(breezy->drm.fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET, breezy);
	drmModeAtomicFree(req);

	return retVal;
}

/**
 * Submits the recurring atomic commits needed to update the framebuffer ID for our
 * double-buffering. This is a non-blocking call. Returns the value of submitting the atomic commit.
 */
static int bz_drm_atomic_commit_recurring(struct bz_breezy *breezy, const uint32_t fb_id)
{
	// Only need to update the framebuffer id for most frames.
	drmModeAtomicReq *req = drmModeAtomicAlloc();
	drmModeAtomicAddProperty(req, breezy->drm.plane_id, prop_id_lookup.plane_fb_id, fb_id);

	constexpr uint32_t flags = DRM_MODE_ATOMIC_NONBLOCK | DRM_MODE_PAGE_FLIP_EVENT;
	const int retVal = drmModeAtomicCommit(breezy->drm.fd, req, flags, breezy);
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
static int bz_gles_init(struct bz_breezy *breezy)
{
	// Create the EGL display
	breezy->gl.display = eglGetPlatformDisplay(EGL_PLATFORM_GBM_KHR, breezy->gbm.device, nullptr);
	if (breezy->gl.display == EGL_NO_DISPLAY) {
		bz_gles_print_egl_error("eglGetPlatformDisplay()", eglGetError());
		return -1;
	}

	// Initialize the EGL display
	int major_egl_version;
	int minor_egl_version;
	if (!eglInitialize(breezy->gl.display, &major_egl_version, &minor_egl_version)) {
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
	if (!eglChooseConfig(breezy->gl.display, config_attributes, &breezy->gl.config, 1, &num_configs)) {
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
	breezy->gl.context = eglCreateContext(breezy->gl.display, breezy->gl.config, EGL_NO_CONTEXT, ctx_attrs);
	if (breezy->gl.context == EGL_NO_CONTEXT) {
		bz_gles_print_egl_error("eglCreateContext()", eglGetError());
		return -6;
	}

	// Create the GBM surface
	breezy->gbm.surface = gbm_surface_create(
		breezy->gbm.device,
		breezy->drm.mode_info.hdisplay,
		breezy->drm.mode_info.vdisplay,
		GBM_FORMAT_XRGB8888,
		GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING
	);
	if (breezy->gbm.surface == nullptr) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to create GBM surface.");
		return -7;
	}

	// Create the EGL surface
	breezy->gl.surface = eglCreatePlatformWindowSurface(
		breezy->gl.display,
		breezy->gl.config,
		breezy->gbm.surface,
		nullptr
	);
	if (breezy->gl.surface == EGL_NO_SURFACE) {
		bz_gles_print_egl_error("eglCreatePlatformWindowSurface()", eglGetError());
		return -8;
	}

	if (!eglMakeCurrent(breezy->gl.display, breezy->gl.surface, breezy->gl.surface, breezy->gl.context)) {
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

// TODO: Very temp - delete
int color_i = 0;
void bz_graphics_set_color_index(int i) {
	color_i = i;
}
float color[3] = { 0.16f, 0.164f, 0.196f };
void bz_graphics_change_color(struct bz_breezy *breezy, float amount) {
	color[color_i] += amount;
	if (amount > 0 && color[color_i] > 1.0f) color[color_i] = 1.0f;
	if (amount < 0 && color[color_i] < 0.0f) color[color_i] = 0.0f;
	glClearColor(color[0], color[1], color[2], 1.0f);
	breezy->gl.is_dirty = true;
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
//  Exposed API
// -------------------------------------------------------------------------------------------------

/**
 * Initializes our rendering pipeline by verifying the necessary EGL extensions exist, establishing
 * our DRM resource pipeline, and initializing EGL with OpenGL ES.
 *
 * Returns 0 on success, or a negative value on failure.
 *
 * During the application's final cleanup, the calling code is expected to call
 * "bz_graphics_cleanup()" to free and release all DRM-related resources whether this function
 * returned successfully or not.
 */
int bz_graphics_initialize(struct bz_breezy *breezy) {
	// Make sure our EGL extensions are available
	int retval = bz_gles_load_egl_extensions();
	if (retval != 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__,
			"Failed to load EGL extensions. Code: %d", retval);
		return -1;
	}

	// Open our DRM device / graphics card, and claim master
	// TODO: Swap this out to choose a graphics device more intelligently with udev.
	breezy->drm.device_id = bz_seat_open_device(breezy, "/dev/dri/card1", &breezy->drm.fd);
	if (breezy->drm.fd < 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to open DRM device.");
		return -2;
	}
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Successfully opened DRM device.");

	// (Make sure we're master)
	if (!drmIsMaster(breezy->drm.fd)) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to claim DRM master.");
		return -3;
	}
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Successfully claimed DRM master.");

	retval = bz_drm_init(breezy);
	if (retval != 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to initialize DRM. Code: %d", retval);
		return -4;
	}
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Successfully initialized DRM resources.");

	retval = bz_gles_init(breezy);
	if (retval != 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Failed to initialize GLES. Code: %d", retval);
		return -5;
	}
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Successfully initialized EGL/GLES.");

	return 0;
}

/**
 * Handles one loop iteration for our main rendering loop. Checks our main file descriptors for
 * new events to dispatch, renders the current application state, then swaps our front and back
 * buffers.
 *
 * A value of "0" is returned on success, or a negative value for failure.
 */
int bz_graphics_loop_iteration(struct bz_breezy *breezy) {
	// Only redraw if our buffer changed.
	if (!breezy->gl.is_dirty) { return 0; }
	breezy->gl.is_dirty = false;

	// Render!
	glClear(GL_COLOR_BUFFER_BIT);
	// (...other OpenGL render commands go here...)

	// Buffer switcheroo
	if (!gbm_surface_has_free_buffers(breezy->gbm.surface)) {
		bz_warn(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "GBM surface has no free buffers.");
		return -1;
	}
	eglSwapBuffers(breezy->gl.display, breezy->gl.surface);
	struct gbm_bo *bo = gbm_surface_lock_front_buffer(breezy->gbm.surface);
	const uint32_t fb_id = bz_drm_gbm_get_bo_fb(breezy, bo);

	// Scanout!
	int retval = 0;
	if (breezy->gbm.prev_bo == nullptr) {
		// Initial modeset is a blocking operation
		retval = bz_drm_atomic_commit_initial(breezy, fb_id);
		breezy->gbm.prev_bo = bo;
	} else {
		// Recurring commits are non-blocking, so wait for it to finish before releasing the old bo.
		breezy->gbm.new_bo = bo;
		retval = bz_drm_atomic_commit_recurring(breezy, fb_id);
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
void bz_graphics_cleanup(struct bz_breezy *breezy) {
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Cleaning up bz_graphics.");

	// -- GLES --
	// TODO: Delete shader programs (future video)

	// -- EGL --
	if (breezy->gl.display != nullptr) {
		eglMakeCurrent(breezy->gl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	}
	if (breezy->gl.context) {
		eglDestroyContext(breezy->gl.display, breezy->gl.context);
		breezy->gl.context = nullptr;
	}
	if (breezy->gl.surface != nullptr) {
		eglDestroySurface(breezy->gl.display, breezy->gl.surface);
		breezy->gl.surface = nullptr;
	}
	if (breezy->gl.display && !eglTerminate(breezy->gl.display)) {
		bz_gles_print_egl_error("eglTerminate()", eglGetError());
	}
	breezy->gl.display = nullptr;
	if (!eglReleaseThread()) {
		bz_gles_print_egl_error("eglReleaseThread()", eglGetError());
	}

	// -- GBM --
	if (breezy->gbm.surface != nullptr && breezy->gbm.prev_bo != nullptr) {
		gbm_surface_release_buffer(breezy->gbm.surface, breezy->gbm.prev_bo);
		breezy->gbm.prev_bo = nullptr;
	}
	if (breezy->gbm.surface != nullptr) {
		gbm_surface_destroy(breezy->gbm.surface);
		breezy->gbm.surface = nullptr;
	}
	if (breezy->gbm.device != nullptr) {
		gbm_device_destroy(breezy->gbm.device);
		breezy->gbm.device = nullptr;
	}

	// -- DRM --
	if (breezy->drm.plane_id != 0) {
		bz_drm_clear_plane(breezy);
	}
	if (breezy->drm.mode_blob_id != 0 && breezy->drm.fd != -1) {
		drmModeDestroyPropertyBlob(breezy->drm.fd, breezy->drm.mode_blob_id);
		breezy->drm.mode_blob_id = 0;
	}
	if (breezy->drm.device_id != -1) {
		bz_seat_close_device(breezy, breezy->drm.device_id);
		breezy->drm.device_id = -1;
		breezy->drm.fd = -1;
	}

	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "DRM clean-up complete!");
}

/** Handles a pending DRM event. This should only be called when there are events pending. */
void bz_graphics_handle_drm_event(struct bz_breezy *breezy) {
	bz_debug(BZ_LOG_GRAPHICS, __FILE__, __LINE__, "Handling drm_fd event.");
	drmEventContext drm_event_context = {
		.version = DRM_EVENT_CONTEXT_VERSION,
		.page_flip_handler = bz_drm_handle_pageflip,
	};
	drmHandleEvent(breezy->drm.fd, &drm_event_context);
}

/**
 * Turns on all of our DRM settings whenever we reclaim DRM master. Returns the value of submitting
 * the atomic commit.
 */
int bz_graphics_activate(struct bz_breezy *breezy)
{
	// Restore our latest, saved settings from our most recent framebuffer
	if (breezy->gbm.prev_bo != nullptr) {
		const uint32_t fb_id = bz_drm_gbm_get_bo_fb(breezy, breezy->gbm.prev_bo);
		return bz_drm_atomic_commit_initial(breezy, fb_id);
	}
	return 0;
}

/**
 * Clears all of our DRM settings whenever we give up DRM master. Returns the value of submitting
 * the atomic commit.
 */
int bz_graphics_deactivate(struct bz_breezy *breezy)
{
	drmModeAtomicReq *req = drmModeAtomicAlloc();

	// Clear CRTC properties
	drmModeAtomicAddProperty(req, breezy->drm.crtc_id, prop_id_lookup.crtc_active, 0);
	drmModeAtomicAddProperty(req, breezy->drm.crtc_id, prop_id_lookup.crtc_mode_id, 0);
	// Clear framebuffer properties FOR ALL PLANES
	drmModeAtomicAddProperty(req, breezy->drm.plane_id, prop_id_lookup.plane_fb_id, 0);
	drmModeAtomicAddProperty(req, breezy->drm.plane_id, prop_id_lookup.plane_crtc_id, 0);

	const int retVal = drmModeAtomicCommit(breezy->drm.fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET, breezy);
	drmModeAtomicFree(req);

	return retVal;
}
