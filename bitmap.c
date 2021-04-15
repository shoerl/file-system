#include "bitmap.h"
#include <stdio.h>
#include <stdlib.h>

const int BITS_PER_INT = 32;

// inspiration from http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html

int
bitmap_get(void* bm, int ii)
{
	// to calculate array index
	int index = ii / BITS_PER_INT;
	// to calculate bit position in int
	int pos = ii % BITS_PER_INT;
	int flag = 1 << pos;
	return (((int*) bm)[index] & flag) != 0;
}

void
bitmap_put(void* bm, int ii, int vv)
{
	// to calculate array index
	int index = ii / BITS_PER_INT;
	// to calculate bit position in int
	int pos = ii % BITS_PER_INT;
	// get it to the right spot
	vv = vv << pos;
	((int*) bm)[index] |= vv;
}

void
bitmap_print(void* bm, int size)
{
	for (int ii = 0; ii < size; ii++) {
		printf("%i", bitmap_get(bm, ii));
		if (ii % BITS_PER_INT == 0) {
			puts("\n");
		}
	}
}

int
bitmap_find_first_zero(void* bm, int size)
{
	for (int ii = 0; ii < size; ii++) {
		if (!bitmap_get(bm, ii)) {
			return ii;
		}
	}
	return -1;
}