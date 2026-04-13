#include "diff.h"
#include "strbuf.h"
#include "wrappers.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static struct diff_table* init_table(FILE* old, FILE* new)
{
    struct diff_table* t = xmalloc(1, struct diff_table);
    memset(t, 0, sizeof(struct diff_table));

    strbuf line = STRBUF_INIT;
    while (strbuf_read_file_line(&line, old) == 0) {
        t->lines_old_version = xrealloc(t->lines_old_version, ++t->old_version_line_count, char*);
        t->lines_old_version[t->old_version_line_count - 1] = strbuf_detach(&line, 0);
        strbuf_init(&line);
    }

    while (strbuf_read_file_line(&line, new) == 0) {
        t->line_new_version = xrealloc(t->line_new_version, ++t->new_version_line_count, char*);
        t->line_new_version[t->new_version_line_count - 1] = strbuf_detach(&line, 0);
        strbuf_init(&line);
    }

    t->table = xmalloc(t->old_version_line_count, struct diff_table_entry*);
    for (size_t i = 0; i < t->old_version_line_count; i++) {
        t->table[i] = xmalloc(t->new_version_line_count, struct diff_table_entry);
        memset(t->table[i], 0, t->new_version_line_count);
    }
    return t;
}

static void discard_table(struct diff_table* t)
{
    for (size_t i = 0; i < t->old_version_line_count; i++) {
        free(t->table[i]);
    }
    free(t->table);
}

static struct diff_table* build_table(FILE* old, FILE* new)
{
    struct diff_table* t = init_table(old, new);
    for (size_t i = 0; i < t->old_version_line_count; i++) {
        for (size_t j = 0; j < t->new_version_line_count; j++) {
            if (strcmp(t->lines_old_version[i], t->line_new_version[j]) == 0) {
                t->table[i][j].is_match = 1;
                if (i == 0 || j == 0)
                    t->table[i][j].value = 1;
                else
                    t->table[i][j].value = t->table[i - 1][j - 1].value + 1;
            } else {
                t->table[i][j].is_match = 0;
                if (i == 0 && j == 0) {
                    t->table[i][j].value = 0;
                } else if (i == 0) {
                    t->table[i][j].value = t->table[i][j - 1].value;
                } else if (j == 0) {
                    t->table[i][j].value = t->table[i - 1][j].value;
                } else {
                    t->table[i][j].value = max(t->table[i - 1][j].value, t->table[i][j - 1].value);
                }
            }
        }
    }
    return t;
}

static void stack_push(struct diff* s, struct pair p)
{
    s->matches = xrealloc(s->matches, ++s->match_count, struct pair);
    s->matches[s->match_count - 1] = p;
}

struct diff* diff_files(FILE* old, FILE* new)
{
    struct diff_table* t = build_table(old, new);

    struct diff* s = xmalloc(1, struct diff);
    memset(s, 0, sizeof(struct diff));
    s->new_version_lines = t->line_new_version;
    s->old_version_lines = t->lines_old_version;
    s->new_version_line_count = t->new_version_line_count;
    s->old_version_line_count = t->old_version_line_count;

    int i = t->old_version_line_count - 1;
    int j = t->new_version_line_count - 1;
    while (i >= 0 && j >= 0) {
        if (t->table[i][j].is_match) {
            struct pair p = { i, j };
            stack_push(s, p);
            j = j - 1;
            i = i - 1;
        } else if (j == 0) {
            i = i - 1;
        } else if (i == 0) {
            j = j - 1;
        } else if (t->table[i - 1][j].value > t->table[i][j - 1].value) {
            i = i - 1;
        } else {
            j = j - 1;
        }
    }
    discard_table(t);
    free(t);

    return s;
}

void print_diff(struct diff* diff)
{
    size_t i = 0;
    size_t j = 0;
    size_t last_match_found = diff->match_count - 1;
    while (i < diff->old_version_line_count && j < diff->new_version_line_count) {
        struct pair match = diff->matches[last_match_found];

        char* to_print;
        if (match.i == i && match.j == j) {
            to_print = diff->new_version_lines[j];
            i++;
            j++;
            last_match_found--;
        }

        else if (match.i > i) {
            printf("- ");
            to_print = diff->old_version_lines[i];
            i++;
        }

        else {
            printf("+ ");
            to_print = diff->new_version_lines[j];
            j++;
        }

        printf("%s", to_print);
        if (strlen(to_print) > 0 && to_print[strlen(to_print) - 1] != '\n') {
            printf("\n");
        }
    }

    while (i < diff->old_version_line_count) {
        printf("- %s", diff->old_version_lines[i]);
        i++;
    }

    while (j < diff->new_version_line_count) {
        printf("+ %s", diff->new_version_lines[j]);
        j++;
    }
}
