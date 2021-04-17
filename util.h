// based on cs3650 starter code

#ifndef UTIL_H
#define UTIL_H

#include <string.h>
#include <stdlib.h>
#include "slist.h"

static int
streq(const char* aa, const char* bb)
{
    return strcmp(aa, bb) == 0;
}

static int
min(int x, int y)
{
    return (x < y) ? x : y;
}

static int
max(int x, int y)
{
    return (x > y) ? x : y;
}

static int
clamp(int x, int v0, int v1)
{
    return max(v0, min(x, v1));
}

static int
bytes_to_pages(int bytes)
{
    int quo = bytes / 4096;
    int rem = bytes % 4096;
    if (rem == 0) {
        return quo;
    }
    else {
        return quo + 1;
    }
}

static void
join_to_path(char* buf, char* item)
{
    int nn = strlen(buf);
    if (buf[nn - 1] != '/') {
        strcat(buf, "/");
    }
    strcat(buf, item);
}

// returns 1 if dir, 0 if otherwise
static int
is_dir(char* path)
{
    int idx = strlen(path) - 1;
    return path[idx] == '/';
}

static char*
get_last_arg(const char* path)
{
    slist* list = s_split(path, '/');
    // keep a pointer to beginning of slist for freeing
    slist* for_free = list;
    while (list->next != 0) {
        list = list->next;
    }
    int len = strlen(list->data);
    char* data = malloc(len + 1);
    strncpy(data, list->data, len + 1);
    s_free(for_free);
    return data;
}

static char*
get_all_but_last_arg(const char* path)
{
    //wahoo
    int len = strlen(path);
    for (int len = strlen(path) - 1; len > -1; len--) {
        if (path[len] == '/') {
            char* newstring = malloc(len + 2);
            strncpy(newstring, path, len + 1);
            newstring[len + 1] = '\0';
            return newstring;
        }
    }
}

#endif
