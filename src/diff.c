#include "diff.h"
#include "helper.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void lcs_table_free(struct lcs_table *t) {
  for (size_t i = 0; i < t->H; i++) {
    free(t->table[i]);
  }
  free(t->table);
  free(t);
}

struct lcs_table *lcs_table(struct string_list *a, struct string_list *b) {
  struct lcs_table *t = xmalloc(1, struct lcs_table);
  t->H = a->nr + 1;
  t->W = b->nr + 1;

  t->table = xmalloc(t->H, size_t *);
  for (size_t i = 0; i < t->H; i++) {
    t->table[i] = xcalloc(t->W, size_t);
  }

  printf("\n");
  for (size_t i = 1; i <= a->nr; i++) {
    for (size_t j = 1; j <= b->nr; j++) {
      if (strcmp(a->values[i - 1], b->values[j - 1]) == 0) {
        t->table[i][j] = t->table[i - 1][j - 1] + 1;
      } else {
        t->table[i][j] = MAX(t->table[i - 1][j], t->table[i][j - 1]);
      }
    }
  }

  return t;
}

size_t lcs_len(struct string_list *a, struct string_list *b) {
  struct lcs_table *t = lcs_table(a, b);
  size_t len = t->table[a->nr][b->nr];
  lcs_table_free(t);
  return len;
}

struct string_list *longest_common_subsequence(struct string_list *a, struct string_list *b) {
  struct lcs_table *t = lcs_table(a, b);
  size_t i = a->nr;
  size_t j = b->nr;

  struct string_list *lcs = xcalloc(1, struct string_list);
  while (i > 0 && j > 0) {
    if (strcmp(a->values[i - 1], b->values[j - 1]) == 0) {
      lcs->values = xrealloc(lcs->values, ++lcs->nr, char *);
      lcs->values[lcs->nr - 1] = strdup(a->values[i - 1]);
      i--;
      j--;
    } else if (t->table[i - 1][j] > t->table[i][j - 1]) {
      i--;
    } else {
      j--;
    }
  }
  lcs_table_free(t);
  return lcs;
}

void lcs_pairs_free(struct lcs_pairs *pairs) {
  free(pairs->pairs);
  free(pairs);
}

struct lcs_pairs *lcs_pairs(struct string_list *a, struct string_list *b) {
  struct lcs_table *t = lcs_table(a, b);
  size_t i = a->nr;
  size_t j = b->nr;

  struct lcs_pairs *pairs = xcalloc(1, struct lcs_pairs);
  while (i > 0 && j > 0) {
    if (strcmp(a->values[i - 1], b->values[j - 1]) == 0) {
      pairs->pairs = xrealloc(pairs->pairs, ++pairs->nr, struct lcs_pair);
      pairs->pairs[pairs->nr - 1].i = i - 1;
      pairs->pairs[pairs->nr - 1].j = j - 1;
      i--;
      j--;
    } else if (t->table[i - 1][j] > t->table[i][j - 1]) {
      i--;
    } else {
      j--;
    }
  }
  lcs_table_free(t);

  for (size_t i = 0; i < pairs->nr / 2; i++) {
    struct lcs_pair tmp = pairs->pairs[i];
    pairs->pairs[i] = pairs->pairs[pairs->nr - i - 1];
    pairs->pairs[pairs->nr - i - 1] = tmp;
  }
  return pairs;
}

void print_differences(struct lcs_pairs *pairs, struct string_list *a, struct string_list *b) {
  size_t line_a = 0;
  size_t line_b = 0;

  for (size_t i = 0; i < pairs->nr; i++) {
    struct lcs_pair p = pairs->pairs[i];
    if (line_a < p.i) {
      while (line_a < p.i) {
        printf("- %s", a->values[line_a++]);
      }
    } else if (line_b < p.j) {
      while (line_b < p.j) {
        printf("+ %s", b->values[line_b++]);
      }
    }

    printf("  %s", a->values[line_a]);
    line_a++;
    line_b++;
  }

  while (line_a < a->nr) {
    printf("- %s", a->values[line_a++]);
  }

  while (line_b < b->nr) {
    printf("+ %s", b->values[line_b++]);
  }
}
