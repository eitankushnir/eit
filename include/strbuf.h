#ifndef STRBUF_H
#define STRBUF_H

#include <stddef.h>

#define STRBUF_INIT_ALLOC_SIZE 80
typedef struct {
  char *buf;    // The string buffer.
  size_t len;   // The length of the string.
  size_t alloc; // The number of bytes allocated.
} strbuf;

#define STRBUF_INIT {.buf = NULL, .len = 0, .alloc = 0}

void strbuf_init(strbuf *sb);
void strbuf_release(strbuf *sb);

// Append a string to the strbuf buffer.
// Returns a pointer to the string.
char *strbuf_addstr(strbuf *sb, const char *str);
char *strbuf_addf(strbuf *sb, const char *format, ...)
    __attribute__((format(printf, 2, 3)));

#endif
