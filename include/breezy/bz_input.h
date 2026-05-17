#ifndef BZ_INPUT_H
#define BZ_INPUT_H
// #################################################################################################


#include "breezy/bz_breezy.h"

int bz_input_initialize(struct bz_breezy *breezy);
void bz_input_cleanup(struct bz_breezy *breezy);

int bz_input_activate(struct bz_breezy *breezy);
void bz_input_deactivate(struct bz_breezy *breezy);
int bz_input_process_events(struct bz_breezy *breezy);

// #################################################################################################
#endif
