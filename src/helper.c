#include "helper.h"
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

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
  if (!ptr) {
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
