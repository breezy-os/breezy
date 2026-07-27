
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

double distance(int x1, int y1, int x2, int y2)
{
	int dx = x2 - x1;
	int dy = y2 - y1;
	return sqrt(dx * dx + dy * dy);
}

int32_t randInt(int min, int max)
{
	return (rand() % (max - min + 1)) + min;
}

int32_t clamp(uint32_t value, uint32_t min, uint32_t max)
{
	if (value < min) return min;
	if (value > max) return max;
	return value;
}