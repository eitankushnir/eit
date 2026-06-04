#ifndef DIFF_H
#define DIFF_H

#include "helper.h"
#include <stddef.h>

struct lcs_table {
  size_t **table;
  size_t W;
  size_t H;
};

struct lcs_pair {
  size_t i;
  size_t j;
};

struct lcs_pairs {
  struct lcs_pair *pairs;
  size_t nr;
};

enum hunk_type {
  HUNK_ADD,
  HUNK_DELETE,
  HUNK_MOD,
};

struct diff_hunk {
  size_t a_start, a_count;
  size_t b_start, b_count;
  enum hunk_type type;
};

struct hunk_list {
  struct diff_hunk *hunks;
  size_t nr;
};

struct lcs_table *lcs_table(struct string_list *a, struct string_list *b);
void lcs_table_free(struct lcs_table *t);

size_t lcs_len(struct string_list *a, struct string_list *b);
struct string_list *longest_common_subsequence(struct string_list *a, struct string_list *b);

struct lcs_pairs *lcs_pairs(struct string_list *a, struct string_list *b);
void lcs_pairs_free(struct lcs_pairs *pairs);

void print_differences(struct lcs_pairs *pairs, struct string_list *a, struct string_list *b);

struct hunk_list *hunk_list(struct string_list *a, struct string_list *b);
void hunk_list_free(struct hunk_list *l);
void print_hunks(struct hunk_list *l, struct string_list *a, struct string_list *b);
#endif
