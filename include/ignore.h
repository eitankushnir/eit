#ifndef IGNORE_H
#define IGNORE_H

#include <stddef.h>

#define IGNORE_PATTERN_LEN 256
struct ignores {
  char *loc;
  char (*patterns)[IGNORE_PATTERN_LEN];
  size_t nr;
};

struct ignores *parse_ignores(const char *path);
void ignores_free(struct ignores *ignores);

/*
 * Returns 1 if the path matches the given ignored patterns.
 * Returns 0 if not.
 */
int ignores_is_ignored(struct ignores *ignores, const char *path);

#endif
