#include "tree.h"
#include "helper.h"
#include "sha256.h"
#include "strbuf.h"
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void tree_add_entry(struct tree *t, struct tree_entry *ent) {
  struct strbuf s = STRBUF_INIT;
  uint32_t stored_mode = htonl(ent->mode);
  strbuf_addraw(&s, &stored_mode, sizeof(uint32_t));
  strbuf_addraw(&s, ent->oid.hash, 32);
  uint32_t stored_len = htonl(ent->filename_len);
  strbuf_addraw(&s, &stored_len, sizeof(uint32_t));
  strbuf_addstr(&s, ent->filename);
  t->buf = xrealloc(t->buf, t->size + s.len, char);
  memcpy(t->buf + t->size, s.buf, s.len);
  t->size += s.len;
  strbuf_release(&s);
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

  t->ent.filename = xcalloc(t->ent.filename_len + 1, char);
  memcpy(t->ent.filename, t->buf, t->ent.filename_len);
  t->buf += t->ent.filename_len;
  t->size -= t->ent.filename_len;

  return &t->ent;
}

void tree_pretty_print(struct tree *t) {
  struct tree_iterator it;
  tree_get_iterator(t, &it);

  struct tree_entry *ent;
  struct hex_oid hex;
  while ((ent = tree_iterate(&it))) {
    printf("%06o %s %s\n", ent->mode, oid_to_hex(&ent->oid, &hex), ent->filename);
    free(ent->filename);
  }
}
