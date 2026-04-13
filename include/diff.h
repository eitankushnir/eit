#ifndef DIFF_H
#define DIFF_H

#include <stddef.h>
#include <stdio.h>
struct diff_table_entry {
    int value;
    int is_match;
};

struct diff_table {
    char** lines_old_version;
    char** line_new_version;
    struct diff_table_entry** table;
    size_t old_version_line_count;
    size_t new_version_line_count;
};

struct pair {
    size_t i;
    size_t j;
};

struct diff {
    struct pair* matches;
    size_t match_count;

    char** old_version_lines;
    char** new_version_lines;

    size_t old_version_line_count;
    size_t new_version_line_count;
};

struct diff* diff_files(FILE* old, FILE* new);
void print_diff(struct diff* diff);

#endif
