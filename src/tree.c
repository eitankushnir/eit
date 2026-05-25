#include "tree.h"
#include "helper.h"
#include "sha256.h"
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void tree_add_entry(struct tree *t, struct tree_entry *ent) {
  size_t entry_bytes = 2 * sizeof(uint32_t) + 32 + ent->filename_len + 1;
  t->buf = xrealloc(t->buf, t->size + entry_bytes, char);

  char *tail = (char *)t->buf + t->size;
  uint32_t stored_mode = htonl(ent->mode);
  uint32_t stored_len = htonl(ent->filename_len);
  memcpy(tail, &stored_mode, sizeof(uint32_t));
  memcpy(tail + sizeof(uint32_t), ent->oid.hash, 32);
  memcpy(tail + sizeof(uint32_t) + 32, &stored_len, sizeof(uint32_t));
  memcpy(tail + 2 * sizeof(uint32_t) + 32, ent->filename, ent->filename_len + 1);

  t->size += entry_bytes;
}

void tree_get_iterator(struct tree *t, struct tree_iterator *out_it) {
  out_it->buf = t->buf;
  out_it->size = t->size;
}

struct tree_entry *tree_iterate(struct tree_iterator *t) {
  if (t->size <= 0)
    return NULL;

  memcpy(&t->ent.mode, t->buf, sizeof(uint32_t));
  t->buf += sizeof(uint32_t);
  t->size -= sizeof(uint32_t);
  memcpy(&t->ent.oid.hash, t->buf, 32);
  t->buf += 32;
  t->size -= 32;
  memcpy(&t->ent.filename_len, t->buf, sizeof(uint32_t));
  t->buf += sizeof(uint32_t);
  t->size -= sizeof(uint32_t);

  t->ent.filename_len = ntohl(t->ent.filename_len);
  t->ent.mode = ntohl(t->ent.mode);

  t->ent.filename = t->buf;
  t->buf += t->ent.filename_len + 1;
  t->size -= t->ent.filename_len + 1;

  return &t->ent;
}

void tree_pretty_print(struct tree *t) {
  struct tree_iterator it;
  tree_get_iterator(t, &it);

  struct tree_entry *ent;
  struct hex_oid hex;
  while ((ent = tree_iterate(&it))) {
    printf("%06o %s %s\n", ent->mode, oid_to_hex(&ent->oid, &hex), ent->filename);
  }
}
