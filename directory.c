#include "directory.h"
#include "pages.h"
#include "bitmap.h"
#include "slist.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>


int
directory_put(inode* dd, const char* name, int inum)
{
	// no more space in directory
	if (dd->size == 4096) {
		return -1;
	}
	void* ents = pages_get_page(dd->ptrs[0]);
	dirent* entry = (dirent*) (ents + dd->size);
	strcpy(entry->name, name);
	entry->inum = inum;
	// since we are adding another dir entry
	dd->size += 64;
	return 0;
}

void
directory_resize(void* entries, int amount)
{
	for (int ii = 0; ii < amount; ii++) {
		memcpy(entries, entries + 64, 64);
		entries += 64;
	}

}


int
directory_delete(inode* dd, const char* name)
{
	int entries = dd->size / 64;
	if (entries == 0) {
		return -1;
	}
	void* ents = pages_get_page(dd->ptrs[0]);
	for (int ii = 0; ii < entries; ii++) {
		dirent* entry = (dirent*) ents;
		if (strcmp(name, entry->name) == 0) {
			if (ii == entries - 1) {
				dd->size -= 64;
				return 0;
			} else {
				directory_resize(ents, entries - ii);
				dd->size -= 64;
				return 0;
			}
		}
		ents += 64;
	}
}

int
directory_lookup(inode* dd, const char* name)
{
	int entries = dd->size / 64;
	void* block = pages_get_page(dd->ptrs[0]);
	for (int ii = 0; ii < entries; ii++) {
		dirent* entry = (dirent*) block;
		if (strcmp(name, entry->name) == 0) {
			return entry->inum;
		}
		block += 64;
	}
	return -1;
}

int
tree_lookup(const char* path)
{
	if (strcmp(path, "/") == 0) {
		return 0;
	}
	int num = 0;
	slist* list = s_split(path, '/');
	list = list->next;
	while(list != 0) {
		if (num == -1) {
			return -1;
		}
		num = directory_lookup(get_inode(num), list->data);
		list = list->next;
	}
	return num;
}

inode*
path_to_inode(const char* path) {
	int inode = tree_lookup(path);
	if (inode == -1) {
		return 0;
	}
	return get_inode(inode);
}

slist*
directory_list(const char* path)
{
	int inum = tree_lookup(path);
	inode* dd = get_inode(inum);
	int entries = dd->size / 64;
	void* ents = pages_get_page(dd->ptrs[0]);
	slist* list = 0;
	for (int ii = 0; ii < entries; ii++) {
		dirent* entry = (dirent*) ents;
		list = s_cons(entry->name, list);
		ents += 64;
	}
	return list;
};

void
print_directory(inode* dd)
{
	int entries = dd->size / 64;
	void* ents = pages_get_page(dd->ptrs[0]);
	for (int ii = 0; ii < entries; ii++) {
		dirent* entry = (dirent*) ents;
		printf("dirent name={%s}, inum={%i}\n", entry->name, entry->inum);
		ents += 64;
	}
}
