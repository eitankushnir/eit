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

static char **get_all_subentries(const char *path, size_t *out_n,
                                 filter dont_go_in, void *userdata,
                                 int *is_dir) {
  if (dont_go_in && dont_go_in(path, userdata)) {
    *is_dir = 0;
    *out_n = 0;
    return NULL;
  }

  struct stat st;
  if (lstat(path, &st) == 0) {
    if (!S_ISDIR(st.st_mode)) {
      char **paths = xmalloc(1, char *);
      paths[0] = strdup(path);
      *out_n = 1;
      *is_dir = 0;
      return paths;
    }
  } else {
    *is_dir = 0;
    *out_n = 0;
    return NULL;
  }

  DIR *dir = opendir(path);
  if (!dir) {
    *is_dir = 0;
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
    if (lstat(entpath.buf, &st) != 0) {
      strbuf_release(&entpath);
      for (size_t i = 0; i < curr_len; i++) {
        free(paths[i]);
      }
      free(paths);
      *out_n = 0;
      closedir(dir);
      *is_dir = 1;
      return NULL;
    }

    if (S_ISDIR(st.st_mode)) {
      size_t more;
      char **moremore =
          get_all_subentries(entpath.buf, &more, dont_go_in, userdata, is_dir);

      if (more > 0) {
        paths = xrealloc(paths, curr_len + more, char *);
        for (size_t i = curr_len; i < curr_len + more; i++) {
          paths[i] = moremore[i - curr_len];
        }
        curr_len += more;
      }
      strbuf_release(&entpath);
      free(moremore);
    } else if (!dont_go_in || !dont_go_in(entpath.buf, userdata)) {
      paths = xrealloc(paths, ++curr_len, char *);
      paths[curr_len - 1] = entpath.buf;
    } else {
      strbuf_release(&entpath);
    }
  }

  closedir(dir);

  *out_n = curr_len;
  *is_dir = 1;
  return paths;
}

struct resolved_pathspec *resolve_pathspec(const char *pathspec, filter filter,
                                           void *userdata) {

  char *copy = strdup(pathspec);
  char *glob = strpbrk(copy, "?*[]");

  char **paths;
  struct strbuf pattern = STRBUF_INIT;
  size_t n;
  int is_dir = 0;
  int no_slash = last_index_of(copy, '/') == -1;
  if (!glob) {
    paths = get_all_subentries(pathspec, &n, filter, userdata, &is_dir);
    strbuf_addstr(&pattern, pathspec);
    if (is_dir)
      strbuf_addstr(&pattern, "/*");
  } else {
    char temp = *glob;
    *glob = '\0';
    int last_slash = last_index_of(copy, '/');
    *glob = temp;

    if (last_slash == -1) {
      paths = get_all_subentries(".", &n, filter, userdata, &is_dir);
      strbuf_addf(&pattern, "%s", pathspec);
    } else {
      copy[last_slash] = '\0';
      paths = get_all_subentries(copy, &n, filter, userdata, &is_dir);
      strbuf_addstr(&pattern, pathspec);
    }
  }

  struct resolved_pathspec *rp = xcalloc(1, struct resolved_pathspec);
  for (size_t i = 0; i < n; i++) {
    char *tomatch = paths[i];
    if (no_slash && glob)
      tomatch = basename_inplace(paths[i]);
    int should_add = fnmatch(pattern.buf, tomatch, 0) == 0;
    if (should_add) {
      rp->matching_paths = xrealloc(rp->matching_paths, ++rp->nr, char *);
      rp->matching_paths[rp->nr - 1] = normalize_path(paths[i]);
    }
    free(paths[i]);
  }
  free(copy);
  free(paths);
  strbuf_release(&pattern);

  qsort(rp->matching_paths, rp->nr, sizeof(char *), cmp);
  return rp;
}
