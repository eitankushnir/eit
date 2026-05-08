#ifndef PATHSPEC_H
#define PATHSPEC_H

#include <stddef.h>
struct resolved_pathspec {
  char **matching_paths;
  size_t nr;
};

typedef int (*filter)(const char *path);

/*
 * Recieves a pathspec pattern and returns a struct with all matching absolute
 * paths. Please free this struct with resolved_pathspec_free.
 *
 * The pathspec will not resolve to any paths that match the filter function.
 */
struct resolved_pathspec *resolve_pathspec(const char *pathspec, filter filter);

void resolved_pathspec_free(struct resolved_pathspec *rp);
#endif
