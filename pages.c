// based on cs3650 starter code

#define _GNU_SOURCE
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>

#include "pages.h"
#include "util.h"
#include "bitmap.h"
#include "inode.h"

const int PAGE_COUNT = 256;
const int NUFS_SIZE  = 4096 * 256; // 1MB

static int   pages_fd   = -1;
static void* pages_base =  0;

void
pages_init(const char* path)
{
    pages_fd = open(path, O_CREAT | O_RDWR, 0644);
    assert(pages_fd != -1);

    int rv = ftruncate(pages_fd, NUFS_SIZE);
    assert(rv == 0);

    pages_base = mmap(0, NUFS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, pages_fd, 0);
    assert(pages_base != MAP_FAILED);
    void* pbm = get_pages_bitmap();
    if (!bitmap_get(pbm, 0)) {
        // init block for bitmaps / 0-83 inodes
        bitmap_put(pbm, 0, 1);
    }
    if (!bitmap_get(pbm, 1)) {
        // init block for 84-168 nodes
        bitmap_put(pbm, 1, 1);
    }
    if (!bitmap_get(pbm, 2)) {
        // init root directory
        int ndnum = alloc_inode();
        assert(ndnum == 0);
        inode* node = get_inode(0);
        node->refs = 0;
        node->mode = 040755;
        node->size = 0;
        node->ptrs[0] = 2;
        bitmap_put(pbm, 2, 1);
    }
}

void
pages_free()
{
    int rv = munmap(pages_base, NUFS_SIZE);
    assert(rv == 0);
}

void*
pages_get_page(int pnum)
{
    return pages_base + 4096 * pnum;
}

void*
get_pages_bitmap()
{
    return pages_get_page(0);
}

void*
get_inode_bitmap()
{
    uint8_t* page = pages_get_page(0);
    return (void*)(page + 32);
}

int
alloc_page()
{
    void* pbm = get_pages_bitmap();
    // page 0 = bms + inodes 0-83
    // page 1 = inodes 84-168
    // page 2 = root dir
    for (int ii = 3; ii < PAGE_COUNT; ++ii) {
        if (!bitmap_get(pbm, ii)) {
            bitmap_put(pbm, ii, 1);
            printf("+ alloc_page() -> %d\n", ii);
            return ii;
        }
    }

    return -1;
}

int
get_consecutive_pages(int amount) {
    void* pbm = get_pages_bitmap();
    int start = -1;
    int last = -1;
    for (int ii = 3; ii < PAGE_COUNT; ++ii) {
        if (!bitmap_get(pbm, ii) && start == -1) {
            //first free page
            start = ii;
            last = ii;
        } else if (!bitmap_get(pbm, ii) && ii == last + 1) {
            last++;
            // ii = last
            if ((last + 1) - start == amount) {
                for (int ii = start; ii < last + 1; ii++) {
                    printf("+ alloc_page() -> %i\n", ii);
                    bitmap_put(pbm, ii, 1);
                }
                return start;
            }
        } else if (bitmap_get(pbm, ii) && ii == last + 1) {
            start = -1;
            last = -1;
        } else {
            //else page is mapped and no starting point, so do nothing
        }
    }
    return -1;
}

void
free_page(int pnum)
{
    printf("+ free_page(%d)\n", pnum);
    void* pbm = get_pages_bitmap();
    bitmap_put(pbm, pnum, 0);
}

