#include "inode.h"
#include "pages.h"
#include "bitmap.h"
#include "directory.h"
#include <sys/stat.h>
#include <stdint.h>
#include "util.h"

void
print_inode(inode* node)
{
	printf("inode refs={%i}, mode={%i}, size={%i}, ptrs={%i, %i}, iptr={%i}",
		node->refs, node->mode, node->size, node->ptrs[0], node->ptrs[1], node->iptr);
}

inode*
get_inode(int inum)
{
	uint8_t* page_ptr;
	// inodes 0-83 are on first page after the bitmaps
	// 48 bytes for bitmaps + 84 inodes * 48 bytes per inodes = 48 + 4032 = 4080 bytes 
	// inodes 84-168 are on the second page
	// 85 inodes * 48 bytes per inode = 4080 bytes
	// enough inodes for writing 100 files + plenty more
	void* node_ptr;
	if (inum < 84) {
		page_ptr = pages_get_page(0);
		node_ptr = page_ptr + 48 + (sizeof(inode) * inum);
	} else {
		page_ptr = pages_get_page(1);
		node_ptr = page_ptr + (sizeof(inode) * inum);
	}
	return (inode*) node_ptr;
}

int
alloc_inode()
{
	void* ibm = get_inode_bitmap();
	int inum = bitmap_find_first_zero(ibm, 169);
	if (inum == -1) {
		// no more space for inodes
		return -1;
	}
	bitmap_put(ibm, inum, 1);
	return inum;
}

void
fill_inode_and_place(int inum, inode* node, mode_t mode, const char* path)
{
	node->refs = 1;
	node->mode = mode;
	node->size = 0;
	time(&node->ctime);
    time(&node->mtime);
    time(&node->atime);
    inode* location = path_to_inode(get_all_but_last_arg(path));
    directory_put(location, get_last_arg(path), inum);

}