// based on cs3650 starter code

#ifndef BITMAP_H
#define BITMAP_H

int bitmap_get(void* bm, int ii);
void bitmap_put(void* bm, int ii, int vv);
void bitmap_print(void* bm, int size);
// finds the first spot in bitmap that is zero (empty), returns -1 if full
int bitmap_find_first_zero(void* bm, int size);

#endif
