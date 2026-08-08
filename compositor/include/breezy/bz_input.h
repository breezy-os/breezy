#ifndef BZ_INPUT_H
#define BZ_INPUT_H
// #################################################################################################


#include "breezy/bz_breezy.h"

int bz_input_initialize(struct bz_breezy *breezy);
void bz_input_cleanup(struct bz_breezy *breezy);

int bz_input_process_events(int fd, uint32_t mask, void *data);

int bz_input_activate(struct bz_breezy *breezy);
void bz_input_deactivate(struct bz_breezy *breezy);

/** Available externally mainly for testing. */
void bz_input_change_device_counts(struct bz_breezy *breezy, int keyboard_delta, int pointer_delta);

// #################################################################################################
#endif
