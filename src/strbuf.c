#include "strbuf.h"
#include "wrappers.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void strbuf_init(strbuf* sb)
{
    sb->alloc = 0;
    sb->len = 0;
    sb->buf = NULL;
}

void strbuf_free(strbuf* sb)
{
    if (sb->buf != NULL) {
        free(sb->buf);
    }
    strbuf_init(sb);
}

char* strbuf_detach(strbuf* sb, size_t* size)
{
    char* buf = sb->buf;
    if (size)
        *size = sb->alloc;
    strbuf_init(sb);
    return buf;
}

void strbuf_grow(strbuf* sb, size_t extra)
{
    if (sb->alloc > sb->len + extra) {
        // We already have enough space.
        return;
    }

    // 80 bytes at the start, after that double the space.
    size_t new_alloc = (sb->alloc == 0) ? 80 : sb->alloc * 2;

    // Ensure we have enough allocated space.
    if (new_alloc < sb->len + extra + 1) {
        new_alloc = sb->len + extra + 1;
    }

    sb->buf = xrealloc(sb->buf, new_alloc, char);
    sb->alloc = new_alloc;
}

void strbuf_add(strbuf* sb, void* data, size_t len)
{
    strbuf_grow(sb, len + 1);
    memcpy(sb->buf + sb->len, data, len);

    sb->len += len;
    sb->buf[sb->len] = '\0'; // Ensure null termination.
}

void strbuf_addf(strbuf* sb, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    int len = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    if (len < 0) {
        // Most likely a formatting error.
        return;
    }

    strbuf_grow(sb, len);
    va_start(ap, fmt);

    vsnprintf(sb->buf + sb->len, len + 1, fmt, ap);
    va_end(ap);

    sb->len += len;
}

int strbuf_read_file_line(strbuf* sb, FILE* file)
{
    if (file == NULL)
        return -1;

    char buf[4096];
    int read_something = 0;
    while (fgets(buf, sizeof(buf), file) != NULL) {
        read_something = 1;
        strbuf_addstr(sb, buf);

        if (buf[strlen(buf) - 1] == '\n') {
            break;
        }
    }

    if (!read_something) {
        return -1;
    }

    return 0;
}
