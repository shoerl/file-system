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

int
shrink_inode(inode* node, int size)
{
	if (node->size > 4096) {
		int numpages = bytes_to_pages(node->size);
		void* blk = pages_get_page(node->iptr);
		if (size == 0) {
			for (int ii = 0; ii < numpages; ii++) {
				free_page(((int*) blk)[ii]);
				((int*) blk)[ii] = 0;
			}
		} else {
			int numpageskeep = bytes_to_pages(size);
			for (int ii = numpageskeep; ii < numpages; ii++) {
				free_page(((int*) blk)[ii]);
				((int*) blk)[ii] = 0;
			}
		}
	} else {
		if (size == 0) {
			free_page(node->ptrs[0]);
			node->ptrs[0] = 0;
		}
	}
	node->size = size;
	return 0;
}

// to handle fragmentation
int
grow_inode(inode* node, int size)
{
	// only gets called if size > 4096 so we know we need a iptr
	if (size > 8192) {
		void* pbm = get_pages_bitmap();
		void* arr = pages_get_page(node->iptr);
		int numpages = bytes_to_pages(size) - 1;
		int last_page = ((int*) arr)[numpages - 1];
		if (!bitmap_get(pbm, last_page + 1)) {
			bitmap_put(pbm, last_page + 1, 1);
			printf("+ alloc_page() -> %i\n", last_page + 1);
			((int*) arr)[numpages] = last_page + 1;
			node->size = size;
			return 0;
		} else {
			int start_page = get_consecutive_pages(numpages + 1);
			if (start_page == -1) {
				return -1;
			}
			for (int ii = 0; ii < numpages; ii++) {
				int old_page = ((int*) arr)[ii];
				int new_page = start_page + ii;
				((int*) arr)[ii] = new_page;
				memcpy(pages_get_page(new_page), pages_get_page(old_page), 4096);
				free_page(old_page);
			}
			((int*) arr)[numpages] = start_page + numpages;
		}
	} else {
		int iptr_page = alloc_page();
		if (iptr_page == -1) {
			return -1;
		}
		void* block = pages_get_page(iptr_page);
		int numpages = bytes_to_pages(size);
		int start_page = get_consecutive_pages(numpages);
		for (int ii = 0; ii < numpages; ii++) {
			((int*) block)[ii] = start_page + ii;
		}
		memcpy(pages_get_page(start_page), pages_get_page(node->ptrs[0]), 4096);
		free_page(node->ptrs[0]);
		node->ptrs[0] = -1;
		node->iptr = iptr_page;
	}
	node->size = size;
	return 0;
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