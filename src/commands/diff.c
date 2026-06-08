#include "diff.h"
#include "command.h"
#include "helper.h"
#include "object.h"
#include "object_store.h"
#include "repository.h"
#include "sha256.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

int cmd_diff(int argc, char **argv, struct repository *repo) {

  size_t size_a, size_b;
  enum object_type t;
  void *raw_a = file_read_raw(argv[1], &size_a);
  void *raw_b = file_read_raw(argv[2], &size_b);

  struct string_list *a = raw_to_lines(raw_a, size_a);
  struct string_list *b = raw_to_lines(raw_b, size_b);

  struct lcs_pairs *ps = lcs_pairs(a, b);
  print_differences(ps, a, b);

  printf("\n\n\n\n");
  struct hunk_list *l = hunk_list(a, b);
  print_hunks(l, a, b);

  lcs_pairs_free(ps);
  hunk_list_free(l);
  free(raw_a);
  free(raw_b);
  string_list_free(a);
  string_list_free(b);

  return 0;
}
