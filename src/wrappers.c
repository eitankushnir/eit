#include "wrappers.h"
#include "strbuf.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void* _xmalloc(size_t bytes, int n)
{
    void* ptr;
    if ((ptr = malloc(bytes * n)) == NULL) {
        die("Fatal: Failed to allocate memory. Requested %zu bytes", bytes);
        exit(1);
    }
    return ptr;
}

void* _xrealloc(void* ptr, size_t bytes, int n)
{
    void* new_ptr = realloc(ptr, n * bytes);
    if (new_ptr == NULL) {
        die("Fatal: Failed to reallocate memory.\n");
    }

    return new_ptr;
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

int removeline(int linenum, const char* path)
{
    FILE* tempfile = tmpfile();
    strbuf lines = STRBUF_INIT;

    FILE* file = fopen(path, "rb");

    int curr_line = 0;
    while (strbuf_read_file_line(&lines, file) == 0) {
        if (curr_line != linenum) {
            fwrite(lines.buf, sizeof(char), lines.len, tempfile);
        }
        strbuf_free(&lines);
        strbuf_init(&lines);
        curr_line++;
    }
    fclose(file);

    file = fopen(path, "wb");
    char buf[4096];
    fseek(tempfile, 0, SEEK_SET);
    int len;
    while ((len = fread(buf, sizeof(char), sizeof(buf), tempfile)) > 0) {
        fwrite(buf, sizeof(char), len, file);
    }

    fclose(file);
    return 0;
}

int replaceline(char* buf, int linenum, const char* path)
{
    FILE* tempfile = tmpfile();
    strbuf lines = STRBUF_INIT;

    FILE* file = fopen(path, "rb");

    int curr_line = 0;
    while (strbuf_read_file_line(&lines, file) == 0) {
        if (curr_line == linenum) {
            fwrite(buf, sizeof(char), strlen(buf), tempfile);
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

size_t maxlinelen(FILE* f)
{
    size_t largest = 0, current = 0;
    int ch;

    if (f) {
        while ((ch = fgetc(f)) != EOF) {
            if (ch == '\n') {
                if (current > largest)
                    largest = current;
                current = 0;
            } else {
                current++;
            }
        }
        if (current > largest)
            largest = current;
    }
    return largest;
}

int non_opt_count(char** argv, int argc, int optind)
{
    int c = 0;
    for (int i = optind; i < argc; i++) {
        if (argv[i][0] != '-')
            c++;
    }

    return c;
}

char* substr(const char* str, int count)
{
    char* buf;
    if (count == -1)
        count = strlen(str);

    buf = xmalloc(count + 1, char);
    strncpy(buf, str, count);
    buf[count] = '\0';
    return buf;
}

int mkpath(const char* path)
{
    strbuf pathbuf = STRBUF_INIT;
    strbuf_addstr(&pathbuf, "./");
    char* path_copy = strdup(path);
    char* dir = strtok(path_copy, "/");
    while (dir) {
        char* next = strtok(NULL, "/");
        if (next) {
            strbuf_addf(&pathbuf, "%s/", dir);
            mkdir(pathbuf.buf, 0755);
        } else {
            strbuf_addf(&pathbuf, "%s", dir);
            FILE* final = fopen(pathbuf.buf, "a");
            if (!final) {
                free(path_copy);
                strbuf_free(&pathbuf);
                return -1;
            }
            fclose(final);
        }

        dir = next;
    }

    free(path_copy);
    strbuf_free(&pathbuf);
    return 0;
}

int rmpath(const char* path)
{
    if (remove(path) != 0) return -1;
    char* slash_ptr = strrchr(path, '/');
    if (!slash_ptr) return 0;

    int slash_idx = slash_ptr - path;
    char* dirname = substr(path, slash_idx);
    int res = rmpath(dirname);
    free(dirname);
    return res;
}
