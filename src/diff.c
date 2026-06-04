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
    }
    if (line_b < p.j) {
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

static enum hunk_type get_hunk_type(size_t a_count, size_t b_count) {
  if (a_count == 0)
    return HUNK_ADD;
  if (b_count == 0)
    return HUNK_DELETE;

  return HUNK_MOD;
}

static void add_hunk(struct hunk_list *list, size_t a_start, size_t a_count,
                     size_t b_start, size_t b_count, enum hunk_type t) {
  list->hunks = xrealloc(list->hunks, ++list->nr, struct diff_hunk);
  list->hunks[list->nr - 1].a_start = a_start;
  list->hunks[list->nr - 1].a_count = a_count;
  list->hunks[list->nr - 1].b_start = b_start;
  list->hunks[list->nr - 1].b_count = b_count;
  list->hunks[list->nr - 1].type = t;
}

struct hunk_list *hunk_list(struct string_list *a, struct string_list *b) {
  struct lcs_pairs *pairs = lcs_pairs(a, b);
  struct hunk_list *l = xcalloc(1, struct hunk_list);

  if (pairs->nr == 0) {
    add_hunk(l, 0, a->nr, 0, b->nr, HUNK_MOD);
    lcs_pairs_free(pairs);
    return l;
  }

  struct lcs_pair cur = pairs->pairs[0];
  struct lcs_pair prev;

  if (cur.i > 0 || cur.j > 0) {
    size_t a_start = 0;
    size_t b_start = 0;
    size_t a_count = cur.i;
    size_t b_count = cur.j;
    enum hunk_type t = get_hunk_type(a_count, b_count);
    add_hunk(l, a_start, a_count, b_start, b_count, t);
  }

  prev = cur;
  for (size_t i = 1; i < pairs->nr; i++) {
    cur = pairs->pairs[i];

    size_t a_count = cur.i - prev.i - 1;
    size_t b_count = cur.j - prev.j - 1;
    if (a_count != 0 || b_count != 0) {
      size_t a_start = prev.i + 1;
      size_t b_start = prev.j + 1;
      enum hunk_type t = get_hunk_type(a_count, b_count);
      add_hunk(l, a_start, a_count, b_start, b_count, t);
    }

    prev = cur;
  }

  size_t a_count = a->nr - prev.i - 1;
  size_t b_count = b->nr - prev.j - 1;
  if (a_count == 0 && b_count == 0) {
    lcs_pairs_free(pairs);
    return l;
  }

  size_t a_start = prev.i + 1;
  size_t b_start = prev.j + 1;
  enum hunk_type t = get_hunk_type(a_count, b_count);
  add_hunk(l, a_start, a_count, b_start, b_count, t);

  lcs_pairs_free(pairs);
  return l;
}

void print_hunks(struct hunk_list *l, struct string_list *a, struct string_list *b) {

  for (size_t i = 0; i < l->nr; i++) {
    struct diff_hunk h = l->hunks[i];
    printf("@@ -%zu,%zu +%zu,%zu @@\n", h.a_start, h.a_count, h.b_start, h.b_count);
    for (size_t j = 0; j < h.a_count; j++) {
      printf("- %s", a->values[j + h.a_start]);
    }

    for (size_t j = 0; j < h.b_count; j++) {
      printf("+ %s", b->values[j + h.b_start]);
    }
  }
}

void hunk_list_free(struct hunk_list *l) {
  free(l->hunks);
  free(l);
}
