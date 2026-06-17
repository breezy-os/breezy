#ifndef BZ_CLIENT_UTILS_H
#define BZ_CLIENT_UTILS_H
// #################################################################################################

#include <stdint.h>
#include <bits/types.h>

int bz_allocate_shm_file(__off64_t size);
uint32_t bz_random_color(void);

// #################################################################################################
#endif