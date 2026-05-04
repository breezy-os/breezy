
#include "breezy/bz_drm.h"
#include "breezy/bz_logger.h"

int main(void) {
  // Set up our logger
  bz_log_initialize(BZ_LOG_ERROR);
  bz_log_set_level(BZ_LOG_DRM, BZ_LOG_DEBUG);

  // DRM science
  bz_drm_initialize();
  for (int i = 0; i < 3; i++) { // (placeholder event loop)
    bz_info(BZ_LOG_MAIN, __FILE__, __LINE__, "Iteration %d", i);
    bz_drm_loop_iteration();
  }
  bz_drm_cleanup();
}
