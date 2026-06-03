#ifndef STRBUF_H
#define STRBUF_H

#include <stddef.h>

#define STRBUF_INIT_ALLOC_SIZE 80
struct strbuf {
  char *buf;    // The string buffer.
  size_t len;   // The length of the string.
  size_t alloc; // The number of bytes allocated.
};

#define STRBUF_INIT {.buf = NULL, .len = 0, .alloc = 0}

void strbuf_init(struct strbuf *sb);
void strbuf_release(struct strbuf *sb);

// Append a string to the strbuf buffer.
// Returns a pointer to the string.
char *strbuf_addstr(struct strbuf *sb, const char *str);
char *strbuf_addf(struct strbuf *sb, const char *format, ...)
    __attribute__((format(printf, 2, 3)));

char *strbuf_addraw(struct strbuf *sb, void *buf, size_t len);

char *strbuf_setlen(struct strbuf *sb, size_t index);

#endif
