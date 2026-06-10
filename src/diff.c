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

    printf(RESET "@@ -%zu,%zu +%zu,%zu @@\n" RESET, h.a_start, h.a_count, h.b_start, h.b_count);
    for (size_t j = 0; j < h.a_count; j++) {
      printf(RED "- %s" RED, a->values[j + h.a_start]);
    }

    for (size_t j = 0; j < h.b_count; j++) {
      printf(GREEN "+ %s" GREEN, b->values[j + h.b_start]);
    }
  }
  printf(RESET "" RESET);
}

void hunk_list_free(struct hunk_list *l) {
  free(l->hunks);
  free(l);
}

bool hunk_collision(struct hunk_list *a, struct hunk_list *b) {
  size_t i = 0;
  size_t j = 0;

  while (i < a->nr && j < b->nr) {
    struct diff_hunk hunk_a = a->hunks[i];
    struct diff_hunk hunk_b = b->hunks[j];

    size_t a_start = hunk_a.a_start;
    size_t a_end = hunk_a.a_start + hunk_a.a_count;

    size_t b_start = hunk_b.a_start;
    size_t b_end = hunk_b.a_start + hunk_b.a_count;

    bool collision = (a_start <= b_start && a_end >= b_start) ||
                     (a_start <= b_end && a_end >= b_end) ||
                     (a_start >= b_start && a_end <= b_end);

    if (collision)
      return true;

    if (a_end < b_start)
      i++;
    else if (b_end < a_start)
      j++;
  }

  return false;
}

struct string_list *merge_diff_3_way(struct string_list *base, struct string_list *a, struct string_list *b, const char *a_name, const char *b_name, bool *out_conflicts) {
  struct string_list *merge = xcalloc(1, struct string_list);
  struct hunk_list *a_hunks = hunk_list(base, a);
  struct hunk_list *b_hunks = hunk_list(base, b);

  size_t i = 0;
  size_t j = 0;

  size_t prev_end = 0;
  *out_conflicts = false;
  while (i < a_hunks->nr && j < b_hunks->nr) {
    struct diff_hunk hunk_a = a_hunks->hunks[i];
    struct diff_hunk hunk_b = b_hunks->hunks[j];

    for (size_t k = prev_end; k < MIN(hunk_a.a_start, hunk_b.a_start); k++) {
      string_list_insert(merge, "%s", base->values[k]);
    }

    size_t a_start = hunk_a.a_start;
    size_t a_end = hunk_a.a_start + hunk_a.a_count;

    size_t b_start = hunk_b.a_start;
    size_t b_end = hunk_b.a_start + hunk_b.a_count;

    bool collision = (a_start <= b_start && a_end >= b_start) ||
                     (a_start <= b_end && a_end >= b_end) ||
                     (a_start >= b_start && a_end <= b_end);

    if (collision) {
      string_list_insert(merge, "<<<<<<<<<<<< %s\n", a_name);
      for (size_t k = b_start; k < a_start; k++) {
        string_list_insert(merge, "%s", base->values[k]);
      }
      for (size_t k = 0; k < hunk_a.b_count; k++) {
        string_list_insert(merge, "%s", a->values[k + hunk_a.b_start]);
      }
      for (size_t k = a_end; k < b_end; k++) {
        string_list_insert(merge, "%s", base->values[k]);
      }
      string_list_insert(merge, "============\n");
      for (size_t k = a_start; k < b_start; k++) {
        string_list_insert(merge, "%s", base->values[k]);
      }
      for (size_t k = 0; k < hunk_b.b_count; k++) {
        string_list_insert(merge, "%s", b->values[k + hunk_b.b_start]);
      }
      for (size_t k = b_end; k < a_end; k++) {
        string_list_insert(merge, "%s", base->values[k]);
      }
      string_list_insert(merge, ">>>>>>>>>>>> %s\n", b_name);
      i++;
      j++;
      *out_conflicts = true;
      prev_end = MAX(hunk_a.a_start + hunk_a.a_count, hunk_b.a_count + hunk_b.a_start);
    }

    if (a_end < b_start) {
      for (size_t k = 0; k < hunk_a.b_count; k++) {
        string_list_insert(merge, "%s", a->values[k + hunk_a.b_start]);
      }
      i++;
      prev_end = hunk_a.a_start + hunk_a.a_count;
    } else if (b_end < a_start) {
      for (size_t k = 0; k < hunk_b.b_count; k++) {
        string_list_insert(merge, "%s", b->values[k + hunk_b.b_start]);
      }
      j++;
      prev_end = hunk_b.a_start + hunk_b.a_count;
    }
  }

  while (i < a_hunks->nr) {
    for (size_t k = prev_end; k < a_hunks->hunks[i].a_start; k++) {
      string_list_insert(merge, "%s", base->values[k]);
    }

    for (size_t k = 0; k < a_hunks->hunks[i].b_count; k++) {
      string_list_insert(merge, "%s", a->values[k + a_hunks->hunks[i].b_start]);
    }
    prev_end = a_hunks->hunks[i].a_start + a_hunks->hunks[i].a_count;
    i++;
  }

  while (j < b_hunks->nr) {
    for (size_t k = prev_end; k < b_hunks->hunks[i].a_start; k++) {
      string_list_insert(merge, "%s", base->values[prev_end + k]);
    }

    for (size_t k = 0; k < b_hunks->hunks[j].b_count; k++) {
      string_list_insert(merge, "%s", b->values[k + b_hunks->hunks[j].b_start]);
    }
    prev_end = b_hunks->hunks[j].a_start + b_hunks->hunks[i].a_count;
    j++;
  }

  for (size_t k = prev_end; k < base->nr; k++) {
    string_list_insert(merge, "%s", base->values[k]);
  }

  hunk_list_free(a_hunks);
  hunk_list_free(b_hunks);
  return merge;
}

struct string_list *merge_diff_2_way(struct string_list *a, struct string_list *b, const char *a_name, const char *b_name, bool *out_conflicts) {
  struct hunk_list *hunks = hunk_list(a, b);
  struct string_list *merge = xcalloc(1, struct string_list);

  size_t prev_end_a = 0;
  *out_conflicts = false;
  for (size_t i = 0; i < hunks->nr; i++) {
    struct diff_hunk hunk = hunks->hunks[i];
    for (size_t k = prev_end_a; k < hunk.a_start; k++) {
      string_list_insert(merge, "%s", a->values[k]);
    }

    if (hunk.a_count && hunk.b_count) {
      string_list_insert(merge, "<<<<<<<<<<<< %s\n", a_name);
      for (size_t j = 0; j < hunk.a_count; j++) {
        string_list_insert(merge, "%s", a->values[hunk.a_start + j]);
      }
      string_list_insert(merge, "============\n");
      for (size_t j = 0; j < hunk.b_count; j++) {
        string_list_insert(merge, "%s", b->values[hunk.b_start + j]);
      }
      string_list_insert(merge, ">>>>>>>>>>>> %s\n", b_name);
      *out_conflicts = true;
    }

    else if (hunk.a_count) {
      for (size_t j = 0; j < hunk.a_count; j++) {
        string_list_insert(merge, "%s", a->values[hunk.a_start + j]);
      }
    } else {
      for (size_t j = 0; j < hunk.b_count; j++) {
        string_list_insert(merge, "%s", b->values[hunk.b_start + j]);
      }
    }
    prev_end_a = hunk.a_start + hunk.a_count;
  }

  for (size_t k = prev_end_a; k < a->nr; k++) {
    string_list_insert(merge, "%s", a->values[k]);
  }

  hunk_list_free(hunks);

  return merge;
}
