#include "pathspec.h"
#include "helper.h"
#include "strbuf.h"
#include <dirent.h>
#include <fnmatch.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int cmp(const void *a, const void *b) {
  return strcmp(*(const char **)a, *(const char **)b);
}

void resolved_pathspec_free(struct resolved_pathspec *rp) {
  for (size_t i = 0; i < rp->nr; i++) {
    free(rp->matching_paths[i]);
  }

  free(rp->matching_paths);
  free(rp);
}

static char **get_all_subentries(const char *path, size_t *out_n) {

  DIR *dir = opendir(path);
  if (!dir) {
    *out_n = 0;
    return NULL;
  }

  struct dirent *ent;
  char **paths = NULL;
  size_t curr_len = 0;
  while ((ent = readdir(dir))) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;
    struct strbuf entpath = STRBUF_INIT;
    strbuf_addf(&entpath, "%s/%s", path, ent->d_name);

    struct stat st;
    if (stat(entpath.buf, &st) != 0) {
      strbuf_release(&entpath);
      *out_n = 0;
      closedir(dir);
      return NULL;
    }

    if (S_ISDIR(st.st_mode)) {
      size_t more;
      char **moremore = get_all_subentries(entpath.buf, &more);

      if (more > 0) {
        paths = xrealloc(paths, curr_len + more, char *);
        for (size_t i = curr_len; i < curr_len + more; i++) {
          paths[i] = moremore[i - curr_len];
        }
        curr_len += more;
      }
      strbuf_release(&entpath);
      free(moremore);
    } else {
      paths = xrealloc(paths, ++curr_len, char *);
      paths[curr_len - 1] = entpath.buf;
    }
  }

  closedir(dir);

  *out_n = curr_len;
  return paths;
}

struct resolved_pathspec *resolve_pathspec(const char *pathspec) {

  char *copy = strdup(pathspec);
  char *last = strpbrk(copy, "?*[]");

  char **paths;
  size_t n;
  char *pattern = NULL;
  if (!last) {
    paths = get_all_subentries(pathspec, &n);
  } else {
    char temp = *last;
    *last = '\0';
    int last_slash = last_index_of(copy, '/');
    *last = temp;

    if (last_slash == -1) {
      paths = get_all_subentries(".", &n);
      pattern = last;
    } else {
      pattern = &copy[last_slash + 1];
      copy[last_slash] = '\0';
      paths = get_all_subentries(copy, &n);
    }
  }

  struct resolved_pathspec *rp = xcalloc(1, struct resolved_pathspec);
  for (size_t i = 0; i < n; i++) {
    int should_add = !pattern || fnmatch(pattern, paths[i], 0) == 0;
    if (should_add) {
      rp->matching_paths = xrealloc(rp->matching_paths, ++rp->nr, char *);
      rp->matching_paths[rp->nr - 1] = normalize_path(paths[i]);
    }
    free(paths[i]);
  }
  free(copy);
  free(paths);

  qsort(rp->matching_paths, rp->nr, sizeof(char *), cmp);
  for (size_t i = 0; i < rp->nr; i++) {
    printf("%s\n", rp->matching_paths[i]);
  }

  return rp;
}
