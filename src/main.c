
#include "breezy/bz_graphics.h"
#include "breezy/bz_logger.h"

int main(void) {
	// Set up our logger
	bz_log_initialize(BZ_LOG_WARN);
	bz_log_set_level(BZ_LOG_GRAPHICS, BZ_LOG_INFO);

	// DRM science
	const int retval = bz_graphics_initialize();
	if (retval != 0) {
		bz_error(BZ_LOG_GRAPHICS, __FILE__, __LINE__,
			"Failed to initialize graphics code. Code: %d", retval);
		// (Still need to call "bz_graphics_cleanup()"
	} else {
		bz_graphics_loop_iteration();
		bz_graphics_loop_iteration();
		bz_graphics_loop_iteration();
		bz_graphics_loop_iteration();
	}
	bz_graphics_cleanup();
}
