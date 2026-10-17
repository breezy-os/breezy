
#include "breezy/bz_math.h"

#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "breezy/bz_logger.h"

bool bz_sizes_are_equal(struct bz_size *s1, struct bz_size *s2)
{
	if (s1 == nullptr && s2 == nullptr) return true;
	if (s1 == nullptr && s2 != nullptr) return false;
	if (s1 != nullptr && s2 == nullptr) return false;
	return s1->w == s2->w && s1->h == s2->h;
}

bool bz_rects_are_equal(struct bz_rect *r1, struct bz_rect *r2)
{
	if (r1 == nullptr && r2 == nullptr) return true;
	if (r1 == nullptr && r2 != nullptr) return false;
	if (r1 != nullptr && r2 == nullptr) return false;
	return (
		r1->x == r2->x &&
		r1->y == r2->y &&
		r1->w == r2->w &&
		r1->h == r2->h
	);
}

bool bz_dbl_rects_are_equal(struct bz_rect_dbl *r1, struct bz_rect_dbl *r2)
{
	if (r1 == nullptr && r2 == nullptr) return true;
	if (r1 == nullptr && r2 != nullptr) return false;
	if (r1 != nullptr && r2 == nullptr) return false;
	return (
		r1->x == r2->x &&
		r1->y == r2->y &&
		r1->w == r2->w &&
		r1->h == r2->h
	);
}

struct bz_rect *bz_clone_rect(struct bz_rect *source)
{
	if (source == nullptr) { return nullptr; }
	struct bz_rect *dest = calloc(1, sizeof(*dest));
	*dest = *source;
	return dest;
}

struct bz_rect_dbl *bz_clone_rect_dbl(struct bz_rect_dbl *source)
{
	if (source == nullptr) { return nullptr; }
	struct bz_rect_dbl *dest = calloc(1, sizeof(*dest));
	*dest = *source;
	return dest;
}

/**
 * Splits the given source rectangle into either 2 or 4 other rectangles, returning the result as a
 * pointer to the split rectangles. If the given source is null, the return value is null.
 *
 * You must provide at least one of "h_split" or "v_split", and they must be between 0 and 1 (exclusive).
 * Providing an out-of-range value to either one of these causes the split in that direction to be ignored.
 * Not providing a valid h_split or v_split will result in the source rectangle being returned.
 *
 * Providing a valid h_split makes a horizontal cut in the source rectangle at the provided percentage height.
 * Providing a valid v_split makes a vertical cut in the source rectangle at the provided percentage width.
 *
 * It's up to the caller to free the returned rectangle pointer.
 */
struct bz_rect *bz_split_rect(struct bz_rect *source, float h_split, float v_split, int32_t cut_size)
{
	if (source == nullptr) { return nullptr; }

	struct bz_rect *results = source;
	struct bz_rect full = *source;

	if (h_split > 0.0f && h_split < 1.0f && v_split > 0.0f && v_split < 1.0f) {
		results = calloc(4, sizeof(*results));
		full.w -= cut_size;
		full.h -= cut_size;
		int32_t w1 = full.w * v_split; int32_t w2 = full.w - w1;
		int32_t h1 = full.h * h_split; int32_t h2 = full.h - h1;
		results[0] = (struct bz_rect){ .x = source->x,                 .y = source->y,                 .w = w1, .h = h1 };
		results[1] = (struct bz_rect){ .x = source->x + w1 + cut_size, .y = source->y,                 .w = w2, .h = h1 };
		results[2] = (struct bz_rect){ .x = source->x,                 .y = source->y + h1 + cut_size, .w = w1, .h = h2 };
		results[3] = (struct bz_rect){ .x = source->x + w1 + cut_size, .y = source->y + h1 + cut_size, .w = w2, .h = h2 };

	} else if (h_split > 0.0f && h_split < 1.0f) {
		results = calloc(2, sizeof(*results));
		full.h -= cut_size;
		int32_t w = full.w;
		int32_t h1 = full.h * h_split; int32_t h2 = full.h - h1;
		results[0] = (struct bz_rect){ .x = source->x, .y = source->y,                 .w = w, .h = h1 };
		results[1] = (struct bz_rect){ .x = source->x, .y = source->y + h1 + cut_size, .w = w, .h = h2 };

	} else if (v_split > 0.0f && v_split < 1.0f) {
		results = calloc(2, sizeof(*results));
		full.w -= cut_size;
		int32_t w1 = full.w * v_split; int32_t w2 = full.w - w1;
		int32_t h = full.h;
		results[0] = (struct bz_rect){ .x = source->x,                 .y = source->y, .w = w1, .h = h };
		results[1] = (struct bz_rect){ .x = source->x + w1 + cut_size, .y = source->y, .w = w2, .h = h };
	}

	return results;
}

/** Rectangle bounds are inclusive. If w/h are negative, the rectangle will be "normalized" first. */
bool bz_contains_point(struct bz_rect *rect, struct bz_position *point)
{
	struct bz_position normal_pos = {
		.x = rect->w > 0 ? rect->x : rect->x + rect->w,
		.y = rect->h > 0 ? rect->y : rect->y + rect->h,
	};
	struct bz_size normal_size = {
		.w = abs(rect->w),
		.h = abs(rect->h),
	};
	return (normal_pos.x <= point->x) && (point->x <= normal_pos.x + normal_size.w) &&
		   (normal_pos.y <= point->y) && (point->y <= normal_pos.y + normal_size.h);
}

struct bz_size *bz_clone_size(struct bz_size *source)
{
	if (source == nullptr) { return nullptr; }
	struct bz_size *dest = calloc(1, sizeof(*dest));
	*dest = *source;
	return dest;
}

double bz_distance(int x1, int y1, int x2, int y2)
{
	int dx = x2 - x1;
	int dy = y2 - y1;
	return sqrt(dx * dx + dy * dy);
}

/**
 * This method creates a projection matrix which maps the given x1/y1/w1/h1 to x2/y2/w2/h2.
 *
 * To derive the mapping matrix, you can start with what you need:
 *   result = input * scale + translation
 *
 * ...then figure out the scale to shift the (0,width) domain to (0,2)
 * ...then figure out the translation to shift the (0,2) to (-1,1)
 * ...then repeat for height.
 */
void bz_fill_projection_matrix(
	bz_mat3 proj,
	float x1, float y1, float w1, float h1,
	float x2, float y2, float w2, float h2
) {
	// Zero the matrix
	memset(proj, 0, sizeof(bz_mat3));

	proj[0] = w2 / w1; // x scale
	proj[4] = h2 / h1; // y scale
	proj[6] = x2 - x1; // x translate
	proj[7] = y2 - y1; // y translate

	// Our favorite hardcoded 1 in the bottom right <3
	proj[8] = 1.0f;
}

void bz_fill_rotation_matrix(bz_mat3 proj, double radians) {
	// Zero the matrix
	memset(proj, 0, sizeof(bz_mat3));

	proj[0] = cos(radians);
	proj[1] = sin(radians);
	proj[3] = -sin(radians);
	proj[4] = cos(radians);

	// Our favorite hardcoded 1 in the bottom right <3
	proj[8] = 1.0f;
}

void bz_multiply_matrices(bz_mat3 result, bz_mat3 mat1, bz_mat3 mat2) {
	for (int row = 0; row < 3; row++) {
		for (int col = 0; col < 3; col++) {
			result[row + 3*col] =
				mat1[row+0] * mat2[3*col + 0] +
				mat1[row+3] * mat2[3*col + 1] +
				mat1[row+6] * mat2[3*col + 2];
		}
	}
}

int32_t bz_rand_int(int min, int max)
{
	return (rand() % (max - min + 1)) + min;
}

int32_t bz_min(int32_t v1, int32_t v2)
{
	return v1 < v2 ? v1 : v2;
}
int32_t bz_max(int32_t v1, int32_t v2)
{
	return v1 > v2 ? v1 : v2;
}
int32_t bz_clamp(int32_t value, int32_t min, int32_t max)
{
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

bool bz_dbl_is_int(double val)
{
	return isfinite(val) && trunc(val) == val;
}
