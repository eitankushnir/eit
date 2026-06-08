#include "pathspec.h"
#include "helper.h"
#include "strbuf.h"
#include <dirent.h>
#include <fnmatch.h>
#include <linux/limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int fs_iterator_next(struct path_iterator *iter, const char **path_out) {
  struct fs_iterator *fs = (struct fs_iterator *)iter;

  while (fs->count > 0) {
    char *current = fs->path_stack[--fs->count];

    if (fs->filter_func && fs->filter_func(current, fs->filter_data)) {
      free(current);
      continue;
    }

    struct stat st;
    if (lstat(current, &st) != 0) {
      free(current);
      continue;
    } else if (!S_ISDIR(st.st_mode)) {
      *path_out = current;
      return 0;
    }

    DIR *d = opendir(current);
    if (!d) {
      free(current);
      continue;
    }

    struct dirent *ent;
    while ((ent = readdir(d))) {
      if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
        continue;

      struct strbuf fullpath = STRBUF_INIT;
      strbuf_addf(&fullpath, "%s/%s", current, ent->d_name);
      if (fs->alloc < ++fs->count) {
        fs->path_stack = xrealloc(fs->path_stack, fs->count * 2, char *);
        fs->alloc = fs->count * 2;
      }
      fs->path_stack[fs->count - 1] = fullpath.buf;
    }

    closedir(d);
    free(current);
    continue;
  }

  return -1; // EOF
}

struct path_iterator *fs_iterator_create(const char *starting_point) {
  struct fs_iterator *fs = xmalloc(1, struct fs_iterator);
  fs->alloc = 10;
  fs->count = 1;
  fs->path_stack = xmalloc(fs->alloc, char *);
  fs->path_stack[0] = normalize_path(starting_point);

  fs->iter.next = fs_iterator_next;
  fs->iter.free = fs_iterator_free;
  fs->filter_func = NULL;
  fs->filter_data = NULL;
  return (struct path_iterator *)fs;
}

void fs_iterator_free(struct path_iterator *iter) {
  struct fs_iterator *fs = (struct fs_iterator *)iter;
  free(fs->path_stack);
  free(fs);
}

void pathspec_add(struct pathspec *spec, const char *pattern) {
  if (spec->alloc < ++spec->nr) {
    spec->alloc = spec->nr * 2;
    spec->items = xrealloc(spec->items, spec->alloc, struct pathspec_item);
  }

  struct strbuf final_pattern = STRBUF_INIT;
  strbuf_addf(&final_pattern, "%s", pattern);

  struct pathspec_item *item = spec->items + spec->nr - 1;
  struct stat st;
  const char *glob = strpbrk(pattern, "*?[");
  if (lstat(pattern, &st) == 0 && !glob && S_ISDIR(st.st_mode)) {
    strbuf_addstr(&final_pattern, "/*");
  }

  item->pattern = normalize_path(final_pattern.buf);
  item->original = strdup(final_pattern.buf);
  glob = strpbrk(item->pattern, "*?[");

  item->len_noglob = glob ? glob - item->pattern : strlen(item->pattern);
  item->len = strlen(item->pattern);
  strbuf_release(&final_pattern);
}

bool pathspec_match(struct pathspec *spec, const char *path) {
  size_t len = strlen(path);
  for (size_t i = 0; i < spec->nr; i++) {
    struct pathspec_item *item = spec->items + i;
    if (len < item->len_noglob)
      continue;

    if (strncmp(item->pattern, path, item->len_noglob) != 0)
      continue;

    if (fnmatch(item->pattern + item->len_noglob, path + item->len_noglob, 0) == 0)
      return true;
  }

  return false;
}

void pathspec_release(struct pathspec *spec) {
  for (size_t i = 0; i < spec->nr; i++) {
    struct pathspec_item *item = spec->items + i;
    free(item->original);
    free(item->pattern);
  }

  free(spec->items);
}
