#include "inode.h"
#include "pages.h"
#include "bitmap.h"
#include <stdint.h>

void
print_inode(inode* node)
{
	printf("inode refs={%i}, mode={%i}, size={%i}, ptrs={%i, %i}, iptr={%i}",
		node->refs, node->mode, node->size, node->ptrs[0], node->ptrs[1], node->iptr);
}

inode*
get_inode(int inum)
{
	uint8_t* ptr = pages_get_page(0);
	// inodes are on first page after the bitmaps
	// bitmaps take up first 40 bytes
	void* v_ptr = ptr + 40 + (sizeof(inode) * inum);
	return (inode*)v_ptr;
}

int
alloc_inode()
{
	void* ibm = get_inode_bitmap();
	int inum = bitmap_find_first_zero(ibm, 64);
	if (inum == -1) {
		// no more space for inodes
		return -1;
	}
	bitmap_put(ibm, inum, 1);
	return inum;
}

