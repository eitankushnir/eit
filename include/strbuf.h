#ifndef STRBUF_H
#define STRBUF_H

#include <stddef.h>
#include <string.h>

/**
 * A dynamically growing string buffer.
 * This struct handles memory allocation and reallocation automattically.
 * 'buf' will alway be null terminated.
 * so basically 'buf' can be treated as a c-string.
 */
typedef struct {
  char *buf;
  size_t len;
  size_t alloc;
} strbuf;

// Initialize a strbuf. example: strbuf sb = STRBUF_INIT
#define STRBUF_INIT {.buf = NULL, .len = 0, .alloc = 0}

// Initialize a strubuf, in not using the macro.
void strbuf_init(strbuf *sb);

// Free a strbuf
void strbuf_free(strbuf *sb);

// Retrieve the malloc'd buffer from the strbuf. The strbuf is reset to
// STRBUF_INIT. size will recieve the allocated size of the buffer. The caller
// is now responsible for freeing the buffer.
char *strbuf_detach(strbuf *sb, size_t *size);

// MODIFICATION FUNCTIONS.

// Grow the buffer at list 'extra' bytes.
void strbuf_grow(strbuf *sb, size_t extra);

// Append raw bytes to the buffer
void strbuf_add(strbuf *sb, void *data, size_t len);

// Append a c-string to the buffer.
static inline void strbuf_addstr(strbuf *sb, const char *str) {
  strbuf_add(sb, (void *)str, strlen(str));
}

// Append formatted strings to the buffer.
void strbuf_addf(strbuf *sb, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

// FILE I/O

// Read file contents into buffer.
// returns 0 on success -1 on failure.
int strbuf_read_file(strbuf *sb, const char *path);
#endif
