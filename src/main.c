
#include "breezy/bz_example_utils.h"
#include "breezy/bz_logger.h"

int main(void) {
  // Set up our logger
  bz_log_initialize(BZ_LOG_ERROR);
  bz_log_set_level(BZ_LOG_MAIN, BZ_LOG_DEBUG);

  const int result = add(1, 2);

  bz_info(BZ_LOG_MAIN, __FILE__, __LINE__, "Test %d", result);
}
