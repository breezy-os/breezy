
#include "breezy/bz_example2_utils.h"

int add(const int a, const int b) {
	return a + b;
}

int power(const int a, const int b) {
	int total = a;
	for (int x = 1; x < b; x++) {
		total = multiply(total, a);
	}
	return total;
}
