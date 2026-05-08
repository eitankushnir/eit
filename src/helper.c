#include "helper.h"
#include "strbuf.h"
#include <asm-generic/errno-base.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void die(const char *format, ...) {
  va_list vargs;
  va_start(vargs, format);
  vfprintf(stderr, format, vargs);
  fprintf(stderr, "\n");
  va_end(vargs);
  exit(128);
}

void *_xmalloc(size_t num, size_t size) {
  void *ptr = malloc(num * size);
  if (!ptr) {
    die("Fatal: Failed to allocate %zu bytes", num * size);
  }

  return ptr;
}

void *_xcalloc(size_t num, size_t size) {
  void *ptr = calloc(num, size);
  if (!ptr) {
    die("Fatal: Failed to allocate %zu bytes", num * size);
  }

  return ptr;
}

void *_xrealloc(void *ptr, size_t num, size_t size) {
  void *new_ptr = realloc(ptr, size * num);
  if (!new_ptr) {
    die("Fatal: Failed to reallocate %zu bytes", num * size);
  }

  return new_ptr;
}

int create_directory_gently(const char *path) {
  if (mkdir(path, 0755) != 0) {
    if (errno != EEXIST)
      return -1;

    // If the path exists but isnt a directory.
    struct stat st;
    if (stat(path, &st) != 0)
      return -1;
    if (!S_ISDIR(st.st_mode))
      return -1;
  }

  return 0;
}

void create_directory(const char *path) {
  if (create_directory_gently(path) != 0)
    die("Fatal: Failed to create directory at: %s", path);
}

int index_of(const char *str, char c) {
  const char *cptr = strchr(str, c);
  if (!cptr)
    return -1;

  return cptr - str;
}

int last_index_of(const char *str, char c) {
  const char *cptr = strrchr(str, c);
  if (!cptr)
    return -1;

  return cptr - str;
}

char *normalize_path(const char *path) {
  if (!path || strlen(path) == 0)
    return NULL;

  char *copy = strdup(path);

  struct strbuf normalized_path = STRBUF_INIT;
  if (copy[0] != '/') {
    char *cwd = getcwd(NULL, 0);
    strbuf_addstr(&normalized_path, cwd);
    free(cwd);
  }

  char *dirname = strtok(copy, "/");
  while (dirname) {
    if (strcmp(dirname, "..") == 0) {
      strbuf_truncate(&normalized_path,
                      last_index_of(normalized_path.buf, '/'));
    } else if (strcmp(dirname, ".") != 0) {
      strbuf_addf(&normalized_path, "/%s", dirname);
    }

    dirname = strtok(NULL, "/");
  }
  free(copy);

  if (normalized_path.len == 0) {
    strbuf_release(&normalized_path);
    return strdup("/");
  }
  return normalized_path.buf;
}

char *basename_inplace(char *path) {
  int ind = last_index_of(path, '/');
  if (ind == -1)
    return path;

  return path + ind + 1;
}

int is_directory(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0)
    return 0;

  return S_ISDIR(st.st_mode);
}

char *trim(char *str) {

  while (*str && isspace(*str))
    str++;

  if (!*str)
    return str;

  char *end = &str[strlen(str) - 1];
  while (end > str && isspace(*end))
    end--;

  if (end != str)
    *(end + 1) = '\0';

  return str;
}
