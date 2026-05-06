#ifndef PATHSPEC_H
#define PATHSPEC_H

#include <stddef.h>
struct resolved_pathspec {
  char **matching_paths;
  size_t nr;
};

/*
 * Recieves a pathspec pattern and returns a struct with all matching absolute
 * paths. Please free this struct with resolved_pathspec_free.
 */
struct resolved_pathspec *resolve_pathspec(const char *pathspec);

void resolved_pathspec_free(struct resolved_pathspec *rp);
#endif
