#ifndef PATHSPEC_H
#define PATHSPEC_H

#include "helper.h"
#include <dirent.h>
#include <stddef.h>
typedef int (*filter)(const char *path, void *userdata);

struct path_iterator {
  int (*next)(struct path_iterator *iter, const char **path_out);
  void (*free)(struct path_iterator *iter);
};

/*
 * Simple path iterator example.
 */
struct fs_iterator {
  struct path_iterator iter;
  filter filter_func; // Directories it should not enter.
  void *filter_data;

  char **path_stack;
  size_t count;
  size_t alloc;
};

struct path_iterator *fs_iterator_create(const char *starting_point);
int fs_iterator_next(struct path_iterator *iter, const char **path_out);
void fs_iterator_free(struct path_iterator *iter);

struct pathspec_item {
  char *pattern;
  char *original;

  size_t len;
  size_t len_noglob;

  int flags;
};

struct pathspec {
  int nr;
  int alloc;
  struct pathspec_item *items;
};

#define PATHSPEC_INIT {.nr = 0, .alloc = 0, .items = NULL}

/*
 * Recieves a pathspec struct.
 * path_iterator, can be used as an intrusive struct.
 * next -> a function that streams absolute paths for the engine. ends with NULL.
 * free -> a function that free the data structure.
 *
 * Returns an owning pointer a the list of absolute paths that matched the pathspec.
 */

void pathspec_add(struct pathspec *spec, const char *pattern, int flags);
bool resolve_pathspec(const char *pathspec, struct path_iterator *iter);
#endif
