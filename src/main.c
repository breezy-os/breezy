
#include "breezy/bz_drm.h"
#include "breezy/bz_logger.h"

int main(void) {
	// Set up our logger
	bz_log_initialize(BZ_LOG_WARN);
	bz_log_set_level(BZ_LOG_DRM, BZ_LOG_INFO);

	// DRM science
	const int retval = bz_drm_initialize();
	if (retval == 0) {
		bz_drm_loop_iteration();
		bz_drm_loop_iteration();
		bz_drm_loop_iteration();
		bz_drm_loop_iteration();
	}
	bz_drm_cleanup();
}
