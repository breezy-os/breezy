#ifndef BZ_APPLICATION_H
#define BZ_APPLICATION_H
// #################################################################################################

#include "./bz_client_globals.h"

void bz_run_event_loop(const struct bz_client_globals *globals);
struct bz_application_window *bz_create_app_window(struct bz_client_globals *globals);

// #################################################################################################
#endif