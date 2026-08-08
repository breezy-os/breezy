
#include "breezy/bz_math.h"

#include <string.h>
#include <math.h>
#include <stdlib.h>


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
	memset(proj, 0, sizeof(*proj));

	proj[0] = w2 / w1; // x scale
	proj[4] = h2 / h1; // y scale
	proj[6] = x2 - x1; // x translate
	proj[7] = y2 - y1; // y translate

	// Our favorite hardcoded 1 in the bottom right <3
	proj[8] = 1.0f;
}

double bz_distance(int x1, int y1, int x2, int y2)
{
	int dx = x2 - x1;
	int dy = y2 - y1;
	return sqrt(dx * dx + dy * dy);
}

int32_t bz_rand_int(int min, int max)
{
	return (rand() % (max - min + 1)) + min;
}

int32_t bz_clamp(int32_t value, int32_t min, int32_t max)
{
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

/** Rectangle bounds are inclusive. If w/h are negative, the rectangle will be "normalized" first. */
bool bz_contains_point(
	struct bz_position *rect_pos,
	struct bz_dimension *rect_size,
	struct bz_position *point
) {
	struct bz_position normal_pos = {
		.x = rect_size->w > 0 ? rect_pos->x : rect_pos->x + rect_size->w,
		.y = rect_size->h > 0 ? rect_pos->y : rect_pos->y + rect_size->h,
	};
	struct bz_dimension normal_size = {
		.w = abs(rect_size->w),
		.h = abs(rect_size->h),
	};
	return (normal_pos.x <= point->x) && (point->x <= normal_pos.x + normal_size.w) &&
		   (normal_pos.y <= point->y) && (point->y <= normal_pos.y + normal_size.h);
}