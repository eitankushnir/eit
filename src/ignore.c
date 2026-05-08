#include "ignore.h"
#include "helper.h"
#include <fnmatch.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ignores *parse_ignores(const char *path) {
  FILE *igfile = fopen(path, "rb");
  if (!igfile)
    return NULL;

  struct ignores *ig = xmalloc(1, struct ignores);
  char *abs = normalize_path(path);
  abs[last_index_of(abs, '/')] = '\0';
  ig->loc = abs;
  ig->nr = 0;
  ig->patterns = NULL;

  char line[IGNORE_PATTERN_LEN];
  while (fgets(line, sizeof(line), igfile)) {
    size_t len = strlen(line);
    if (len == 0 || line[0] == '\n')
      continue;

    if (line[len - 1] == '\n') {
      line[len - 1] = '\0';
    }

    if (line[0] == '#') // comment
      continue;

    char *hashtag = strchr(line, '#');
    if (hashtag && *(hashtag - 1) != '\\')
      *hashtag = '\0';

    ig->patterns = xrealloc(ig->patterns, ++ig->nr, line);
    strncpy(ig->patterns[ig->nr - 1], trim(line), IGNORE_PATTERN_LEN);
  }

  return ig;
}

void ignores_free(struct ignores *ignores) {
  free(ignores->patterns);
  free(ignores->loc);
  free(ignores);
}

int ignores_is_ignored(struct ignores *ignores, const char *path) {
  char *abs = normalize_path(path);
  // Ignores are relative to their location.
  if (strstr(abs, ignores->loc) != abs) {
    free(abs);
    return 0;
  }

  char *tomatch = abs + strlen(ignores->loc);

  for (size_t i = 0; i < ignores->nr; i++) {
    char *cur = ignores->patterns[i];
    int negate = 0;
    if (cur[0] == '!') {
      negate = 1;
      cur++;
    } else if (cur[0] == '\\')
      cur++;

    int first_slash = index_of(cur, '/');
    int last_slash = last_index_of(cur, '/');

    int only_dirs = last_slash == strlen(cur) - 1;
    if (only_dirs)
      cur[last_slash] = '\0';

    if (first_slash == -1 || first_slash == strlen(cur) - 1) {
      char *basename = basename_inplace(tomatch);
      if (!negate && fnmatch(cur, basename, 0) == 0) {
        if (only_dirs && !is_directory(abs)) {
          continue;
        }
        free(abs);
        return 1;
      }
    } else {
      // match directory explictly
      if (cur[0] != '/')
        tomatch++; // get rid of leading slash.

      if (!negate && fnmatch(cur, tomatch, FNM_PATHNAME) == 0) {
        if (only_dirs && !is_directory(abs)) {
          continue;
        }
        free(abs);
        return 1;
      }
    }
  }

  free(abs);
  return 0;
}
