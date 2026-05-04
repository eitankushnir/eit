#include "strbuf.h"
#include "helper.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void strbuf_init(struct strbuf *sb) {
  sb->buf = NULL;
  sb->alloc = 0;
  sb->len = 0;
}

void strbuf_release(struct strbuf *sb) {
  if (sb->buf)
    free(sb->buf);

  sb->buf = NULL;
  sb->alloc = 0;
  sb->len = 0;
}

char *strbuf_addstr(struct strbuf *sb, const char *str) {
  if (sb->alloc == 0) {
    sb->buf = xmalloc(STRBUF_INIT_ALLOC_SIZE, char);
    sb->alloc = STRBUF_INIT_ALLOC_SIZE;
  }

  size_t additional_len = strlen(str);
  if (sb->alloc < sb->len + additional_len + 1) {
    size_t new_alloc = (sb->len + additional_len + 1) * 2;
    sb->buf = xrealloc(sb->buf, new_alloc, char);
    sb->alloc = new_alloc;
  }

  strncpy(sb->buf + sb->len, str, additional_len);
  sb->len = sb->len + additional_len;
  sb->buf[sb->len] = '\0';

  return sb->buf;
}

char *strbuf_addf(struct strbuf *sb, const char *format, ...) {
  va_list vargs;
  va_start(vargs, format);
  size_t len = vsnprintf(NULL, 0, format, vargs);
  va_end(vargs);

  va_start(vargs, format);
  char *finished_string = xmalloc(len + 1, char);
  vsnprintf(finished_string, len + 1, format, vargs);
  va_end(vargs);
  strbuf_addstr(sb, finished_string);
  free(finished_string);

  return sb->buf;
}

char *strbuf_truncate(struct strbuf *sb, size_t index) {
  if (index > sb->len)
    return NULL;

  sb->buf[index] = '\0';
  sb->len = index;
  return sb->buf;
}
