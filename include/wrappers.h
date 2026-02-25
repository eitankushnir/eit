#ifndef WRAPPERS_H
#define WRAPPERS_H
#include <stddef.h>
#include <stdio.h>

#define xmalloc(bytes, type) (_xmalloc((bytes), sizeof(type)));

void* _xmalloc(size_t bytes, int n);
//void xfree(void** ptr);
void die(char* fmt, ...)
    __attribute__((format(printf, 1, 2)));

int writeline(char* buf, int linenum, const char* path);

#endif
