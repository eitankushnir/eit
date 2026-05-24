#ifndef TREE_H
#define TREE_H

#include <stddef.h>

#include "object.h"
#include "sha256.h"

struct tree {
  struct object obj;
  void *buf;
  size_t size;
};

struct tree_entry {
  int mode;
  struct object_id oid;
  const char *filename;
  size_t filename_len;
};

struct tree_iterator {
  const void *buf;
  size_t size;
  struct tree_entry ent;
};

/*
 * Add an entry to the tree's buffer
 */
void tree_add_entry(struct tree *t, struct tree_entry *ent);

void tree_get_iterator(struct tree *t, struct tree_iterator *out_it);
/*
 * Iterate over all the trees entries.
 * Returns a pointer to the current tree entry. (entry is stored inside the
 * iterator struct. do not free).
 * Returns NULL if the end was reached.
 */
struct tree_entry *tree_iterate(struct tree_iterator *t);

void tree_pretty_print(struct tree *t);

#endif
