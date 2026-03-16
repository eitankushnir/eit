#ifndef WRAPPERS_H
#define WRAPPERS_H
#include <stddef.h>
#include <stdio.h>

#define xmalloc(bytes, type) (_xmalloc((bytes), sizeof(type)));
#define xrealloc(ptr, bytes, type) (_xrealloc((ptr), (bytes), sizeof(type)));

void* _xmalloc(size_t bytes, int n);
void* _xrealloc(void* ptr, size_t bytes, int n);
//void xfree(void** ptr);
void die(char* fmt, ...)
    __attribute__((format(printf, 1, 2)));

int writeline(char* buf, int linenum, const char* path);
int removeline(int linenum, const char* path);
int replaceline(char* buf, int linenum, const char* path);
size_t maxlinelen(FILE* f);

int non_opt_count(char** argv, int argc, int optind);
char* substr(const char* str, int count);

int mkabspath(const char* path);
int rmabspath(const char* path, const char* stopping_point);

#endif
