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

int index_of(const char *str, char c);
int last_index_of(const char *str, char c);
int ends_with(const char *str, const char *suffix);

// Return a malloc'd string of the absolute path.
char *normalize_path(const char *path);

// Returns a pointer to a char in path that starts the basename.
char *basename_inplace(char *path);

int is_directory(const char *path);

// Removes trailing whitespace and returns a pointer to the first non-whitespace
// character (or to the null terminator if the string is pure spaces)
char *trim(char *str);

int normalize_mode(int mode);

#endif
