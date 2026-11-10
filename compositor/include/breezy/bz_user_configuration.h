#ifndef BZ_USER_CONFIGURATION_H
#define BZ_USER_CONFIGURATION_H
// #################################################################################################

#include "breezy/bz_breezy.h"

// -- Initialization / Teardown --
int bz_user_config_initialize(struct bz_user_config *conf);
void bz_user_config_cleanup(struct bz_user_config *conf);

void bz_load_random_desktop_background(struct bz_user_config *conf);

// #################################################################################################
#endif
