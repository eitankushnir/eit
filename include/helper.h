#ifndef HELPER_H
#define HELPER_H
#include <stddef.h>

#define xmalloc(n, type) (_xmalloc((n), (sizeof(type))))
#define xcalloc(n, type) (_xcalloc((n), (sizeof(type))))
#define xrealloc(ptr, n, type) (_xrealloc((ptr), (n), (sizeof(type))))

void die(const char *format, ...) __attribute__((format(printf, 1, 2)));

void *_xmalloc(size_t num, size_t size);
void *_xcalloc(size_t num, size_t size);
void *_xrealloc(void *ptr, size_t num, size_t size);

void create_directory(const char *path);
int create_directory_gently(const char *path);

#endif
