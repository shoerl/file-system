// based on cs3650 starter code

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <bsd/string.h>
#include <assert.h>
#include <time.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>
#include "pages.h"
#include "directory.h"
#include "inode.h"
#include "bitmap.h"
#include "util.h"

// declaration of nufs_truncate so can call before implementation
int nufs_truncate(const char *path, off_t size);

// implementation for: man 2 access
// Checks if a file exists.
int
nufs_access(const char *path, int mask)
{
    int rv = 0;
    inode* inode = path_to_inode(path);
    if (inode == 0) {
        rv = -ENOENT;
    }
    printf("access(%s, %04o) -> %d\n", path, mask, rv);
    return rv;
}

// implementation for: man 2 stat
// gets an object's attributes (type, permissions, size, etc)
int
nufs_getattr(const char *path, struct stat *st)
{
    int rv = 0;
    int node_num = tree_lookup(path);
    if (node_num == -1) {
    	// file doesnt exist error
        rv = -ENOENT;
    } else {
        inode* node = get_inode(node_num);
        st->st_mode = node->mode;
        st->st_size = node->size;
        st->st_ino = node_num;
        st->st_uid = getuid();
        st->st_nlink = 1;
        st->st_atime = node->atime;
        st->st_ctime = node->ctime;
        st->st_mtime = node->mtime;
    }
    printf("getattr(%s) -> (%d) {mode: %04o, size: %ld}\n", path, rv, st->st_mode, st->st_size);
    return rv;
}

// implementation for: man 2 readdir
// lists the contents of a directory
int
nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi)
{
    struct stat st;
    int rv = 0;
    inode* inode = path_to_inode(path);
    if (inode == 0) {
        rv = -ENOENT;
    } else if (!S_ISDIR(inode->mode)) {
        // not a directory
        rv = -ENOTDIR;
    } else {
        // getattr return value
        int garv = nufs_getattr(path, &st);
        assert(garv == 0);
        filler(buf, ".", &st, 0);
        slist* list = directory_list(path);
        while (list != 0) {
            filler(buf, list->data, &st, 0);
            list = list->next;
        }
    }
    printf("readdir(%s) -> %d\n", path, rv);
    return rv;
}

// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
int
nufs_mknod(const char *path, mode_t mode, dev_t rdev)
{
    int num = alloc_inode();
    int rv = 0;
    if (num == -1) {
        // no space left
        rv = -ENOSPC;
    } else if (S_ISDIR(mode)) {
        int pagenum = alloc_page();
        if (pagenum == -1) {
            rv = -ENOSPC;
        } else {
            inode* node = get_inode(num);
            node->ptrs[0] = pagenum;
            fill_inode_and_place(num, node, mode, path);
        }
    } else if (S_ISREG(mode)) {
        //reg file
        inode* node = get_inode(num);
        fill_inode_and_place(num, node, mode, path);
    }
    printf("mknod(%s, %04o) -> %d\n", path, mode, rv);
    return rv;
}

// most of the following callbacks implement
// another system call; see section 2 of the manual
int
nufs_mkdir(const char *path, mode_t mode)
{
    int rv = nufs_mknod(path, mode | 040000, 0);
    printf("mkdir(%s) -> %d\n", path, rv);
    return rv;
}

int
nufs_unlink(const char *path)
{
    inode* node = path_to_inode(get_all_but_last_arg(path));
    if (node->refs == 1) {
        nufs_truncate(path, 0);
    }
    node->refs -= 1;
    int rv = directory_delete(node, get_last_arg(path));
    printf("unlink(%s) -> %d\n", path, rv);
    return rv;
}

int
nufs_link(const char *from, const char *to)
{
    int rv = 0;
    int inum = tree_lookup(from);
    inode* other_node = get_inode(inum);
    other_node->refs += 1;
    if (inum == -1) {
        rv = -ENOENT;
    }
    inode* node = path_to_inode(get_all_but_last_arg(to));
    directory_put(node, get_last_arg(to), inum);
    printf("link(%s => %s) -> %d\n", from, to, rv);
	return rv;
}

int
nufs_rmdir(const char *path)
{
    int rv = -1;
    printf("rmdir(%s) -> %d\n", path, rv);
    return rv;
}

// implements: man 2 rename
// called to move a file within the same filesystem
int
nufs_rename(const char *from, const char *to)
{
    int rv = 0;
    int inum = tree_lookup(from);
    if (inum == -1) {
        rv = -ENOENT;
        //does not exist
    }
    directory_put(path_to_inode(get_all_but_last_arg(to)), get_last_arg(to), inum);
    directory_delete(path_to_inode(get_all_but_last_arg(from)), get_last_arg(from));
    printf("rename(%s => %s) -> %d\n", from, to, rv);
    return rv;
}

int
nufs_chmod(const char *path, mode_t mode)
{
    int rv = 0;
    inode* node = path_to_inode(path);
    if (node == 0) {
        //file doesnt exist error
        rv = -ENOENT;
    } else {
        node->mode = mode;
        time(&node->ctime);
    }
    printf("chmod(%s, %04o) -> %d\n", path, mode, rv);
    return rv;
}

int
nufs_truncate(const char *path, off_t size)
{
    int rv = 0;
    inode* node = path_to_inode(path);
    if (node == 0) {
        //file doesnt exist error
        rv = -ENOENT;
    } else {
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
            node->size = size;
        } else if (node->size > 0) {
            if (size == 0) {
                free_page(node->ptrs[0]);
                node->ptrs[0] = 0;
            }
            node->size = size;
        }
    }
    printf("truncate(%s, %ld bytes) -> %d\n", path, size, rv);
    return rv;
}

// this is called on open, but doesn't need to do much
// since FUSE doesn't assume you maintain state for
// open files.
int
nufs_open(const char *path, struct fuse_file_info *fi)
{
    int rv = 0;
    printf("open(%s) -> %d\n", path, rv);
    return rv;
}

// Actually read data
int
nufs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int rv = 0;
    inode* node = path_to_inode(path);

    if (node == 0) {
        // bad file / doesnt exist
        rv = -EBADF;
    } else if (S_ISDIR(node->mode)) {
        // if trying to read directory
        rv = -EISDIR;
    } else if (size > 4096 || offset >= 4096) {
        int* nodes = pages_get_page(node->iptr);
        void* rd_from = pages_get_page(nodes[0]);
        memcpy(buf, rd_from + offset, size);
        // update last accessed time
        time(&node->atime);
        rv = size;
    } else {
        void* rd_from = pages_get_page(node->ptrs[0]);
        memcpy(buf, rd_from + offset, size);
        // update last accessed time
        time(&node->atime);
        rv = size;
    }
    printf("read(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
    return rv;
}

// Actually write data
int
nufs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int rv = 0;
    inode* node = path_to_inode(path);
    if (node == 0) {
        rv = -EBADF;
    } 
    else if (offset + size > 4096) {
        if (offset + size > node->size) {
            int status = grow_inode(node, offset + size);
            if (status == -1) {
                rv = -ENOSPC;
            }
        }
        if (rv == 0) {
            void* arr = pages_get_page(node->iptr);
            void* wrt_to = pages_get_page(((int*) arr)[0]);
            memcpy(wrt_to + offset, buf, size);
            // update last modified time
            time(&node->mtime);
            rv = size;
        }
    } else {
        int dbnum = alloc_page();
        if (dbnum == -1) {
            rv == -ENOSPC;
        }
        if (rv == 0) {
            node->ptrs[0] = dbnum;
            void* wrt_to = pages_get_page(dbnum);
            memcpy(wrt_to + offset, buf, size);
            if (offset + size > node->size) {
                node->size = offset + size;
            }
            // update last modified time
            time(&node->mtime);
            rv = size;
        }
    }
    printf("write(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
    return rv;
}

// Update the timestamps on a file or directory.
int
nufs_utimens(const char* path, const struct timespec ts[2])
{
    int rv = 0;
    inode* node = path_to_inode(path);
    if (node == 0) {
        rv = -ENOENT;
    }
    //ts[0] is last access time
    node->atime = ts[0].tv_sec;
    //ts[1] is last modification time
    node->mtime = ts[1].tv_sec;
    printf("utimens(%s, [%ld, %ld; %ld %ld]) -> %d\n",
           path, ts[0].tv_sec, ts[0].tv_nsec, ts[1].tv_sec, ts[1].tv_nsec, rv);
	return rv;
}

// Extended operations
int
nufs_ioctl(const char* path, int cmd, void* arg, struct fuse_file_info* fi,
           unsigned int flags, void* data)
{
    int rv = -1;
    printf("ioctl(%s, %d, ...) -> %d\n", path, cmd, rv);
    return rv;
}

void
nufs_init_ops(struct fuse_operations* ops)
{
    memset(ops, 0, sizeof(struct fuse_operations));
    ops->access   = nufs_access;
    ops->getattr  = nufs_getattr;
    ops->readdir  = nufs_readdir;
    ops->mknod    = nufs_mknod;
    ops->mkdir    = nufs_mkdir;
    ops->link     = nufs_link;
    ops->unlink   = nufs_unlink;
    ops->rmdir    = nufs_rmdir;
    ops->rename   = nufs_rename;
    ops->chmod    = nufs_chmod;
    ops->truncate = nufs_truncate;
    ops->open	  = nufs_open;
    ops->read     = nufs_read;
    ops->write    = nufs_write;
    ops->utimens  = nufs_utimens;
    ops->ioctl    = nufs_ioctl;
};

struct fuse_operations nufs_ops;

int
main(int argc, char *argv[])
{
    assert(argc > 2 && argc < 6);
    pages_init(argv[--argc]);
    nufs_init_ops(&nufs_ops);
    return fuse_main(argc, argv, &nufs_ops, NULL);
}

