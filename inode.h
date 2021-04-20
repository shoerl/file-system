// based on cs3650 starter code

#ifndef INODE_H
#define INODE_H

#include "pages.h"
#include <sys/stat.h>
#include <time.h>

typedef struct inode {
    int refs; // reference count
    int mode; // permission & type
    int size; // bytes
    int ptrs[2]; // direct pointers
    int iptr; // single indirect pointer
    time_t atime; // time of last access
    time_t ctime; // time of last status change
    time_t mtime; // time of last modification
} inode;

// sizeof(int) * 6 + sizeof(time_t) * 3 = sizeof(inode)
// 4 * 6 + 8 * 3 = 48 bytes

void print_inode(inode* node);
inode* get_inode(int inum);
int alloc_inode();
void free_inode();
int grow_inode(inode* node, int size);
int shrink_inode(inode* node, int size);
int inode_get_pnum(inode* node, int fpn);
void fill_inode_and_place(int inum, inode* node, mode_t mode, const char* path);

#endif
