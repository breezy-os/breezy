#ifndef BZ_MATH_H
#define BZ_MATH_H
// #################################################################################################

#include <stdint.h>

struct bz_position  { int32_t x; int32_t y; };
struct bz_dimension { int32_t w; int32_t h; };

// -- Matrix format --
// [0  3  6]
// [1  4  7]
// [2  5  8]
typedef float bz_mat3[9];
// [0  4  8  12]
// [1  5  9  13]
// [2  6  10 14]
// [3  7  11 15]
typedef float bz_mat4[16];

void bz_fill_projection_matrix(bz_mat3 proj,
	float x1, float y1, float w1, float h1,
	float x2, float y2, float w2, float h2);

double bz_distance(int x1, int y1, int x2, int y2);

int32_t bz_rand_int(int min, int max);

int32_t bz_clamp(int32_t value, int32_t min, int32_t max);

// #################################################################################################
#endif