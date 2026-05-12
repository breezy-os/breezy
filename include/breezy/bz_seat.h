#ifndef BZ_SEAT_H
#define BZ_SEAT_H
// #################################################################################################


#include "breezy/bz_breezy.h"

int bz_seat_initialize(struct bz_breezy *breezy);
void bz_seat_handle_libseat_event(struct bz_breezy *breezy);
void bz_seat_cleanup(struct bz_breezy *breezy);

const char *bz_seat_name(struct bz_breezy *breezy);
int bz_seat_open_device(struct bz_breezy *breezy, const char *path, int *fd);
int bz_seat_close_device(struct bz_breezy *breezy, int device_id);


// #################################################################################################
#endif
