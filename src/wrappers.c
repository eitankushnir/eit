#include "wrappers.h"
#include "strbuf.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void* _xmalloc(size_t bytes, int n)
{
    void* ptr;
    if ((ptr = malloc(bytes * n)) == NULL) {
        die("Fatal: Failed to allocate memory. Requested %zu bytes", bytes);
        exit(1);
    }
    return ptr;
}

void die(char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(128);
}

int writeline(char* buf, int linenum, const char* path)
{
    FILE* tempfile = tmpfile();
    strbuf lines = STRBUF_INIT;

    FILE* file = fopen(path, "rb");

    int curr_line = 0;
    while (strbuf_read_file_line(&lines, file) == 0) {
        if (curr_line == linenum) {
            fwrite(buf, sizeof(char), strlen(buf), tempfile);
            fwrite(lines.buf, sizeof(char), lines.len, tempfile);
        } else {
            fwrite(lines.buf, sizeof(char), lines.len, tempfile);
        }
        strbuf_free(&lines);
        strbuf_init(&lines);
        curr_line++;
    }
    // Write at the end
    if (curr_line == linenum) {
        fwrite(buf, sizeof(char), strlen(buf), tempfile);
    }
    fclose(file);

    file = fopen(path, "wb");
    char buf2[4096];
    fseek(tempfile, 0, SEEK_SET);
    while (fread(buf2, sizeof(char), sizeof(buf2), tempfile) > 0) {
        fwrite(buf2, sizeof(char), strlen(buf2), file);
    }

    fclose(file);

    return 0;
}
