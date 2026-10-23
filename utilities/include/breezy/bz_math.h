#ifndef BZ_MATH_H
#define BZ_MATH_H
// #################################################################################################

#include <stdint.h>

constexpr double BZ_PI = 3.1415926;
constexpr double BZ_PI_2 = BZ_PI / 2.0;

enum bz_direction { BZ_UP, BZ_DOWN, BZ_LEFT, BZ_RIGHT };

// -- Positions --

struct bz_position { int32_t x; int32_t y; };
struct bz_size { int32_t w; int32_t h; };
struct bz_rect { int32_t x; int32_t y; int32_t w; int32_t h; }; // TODO-dl13: make w/h uint32_t?
struct bz_rect_dbl { double x; double y; double w; double h; };

bool bz_sizes_are_equal(struct bz_size *s1, struct bz_size *s2);

bool bz_rects_are_equal(struct bz_rect *r1, struct bz_rect *r2);
bool bz_dbl_rects_are_equal(struct bz_rect_dbl *r1, struct bz_rect_dbl *r2);
struct bz_rect *bz_clone_rect(struct bz_rect *source);
struct bz_rect_dbl *bz_clone_rect_dbl(struct bz_rect_dbl *source);
struct bz_rect *bz_split_rect(struct bz_rect *source, float h_split, float v_split, int32_t cut_size);
bool bz_contains_point(struct bz_rect *rect, struct bz_position *point);
// TODO-dl13: test this
struct bz_rect bz_get_bounding_rect(struct bz_rect *r1, struct bz_rect *r2);

struct bz_size *bz_clone_size(struct bz_size *source);

double bz_distance(int x1, int y1, int x2, int y2);

// -- Matrices --

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
void bz_fill_rotation_matrix(bz_mat3 proj, double radians);
void bz_multiply_matrices(bz_mat3 result, bz_mat3 mat1, bz_mat3 mat2);


// -- Misc --

int32_t bz_rand_int(int min, int max);

int32_t bz_min(int32_t v1, int32_t v2);
int32_t bz_max(int32_t v1, int32_t v2);
int32_t bz_clamp(int32_t value, int32_t min, int32_t max);

bool bz_dbl_is_int(double val);

// #################################################################################################
#endif