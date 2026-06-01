#include "object_pool.h"
#include "commit.h"
#include "helper.h"
#include "object.h"
#include "sha256.h"
#include "tree.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct object_pool *object_pool_new(size_t buckets_nr) {
  struct object_pool *p = xmalloc(1, struct object_pool);
  p->buckets_nr = buckets_nr;

  p->buckets = xcalloc(buckets_nr, struct object_bucket *);

  return p;
}

static void object_free(struct object *obj) {
  if (obj->type == OBJ_TREE) {
    struct tree *t = (struct tree *)obj;
    free(t->buf);
  }

  free(obj);
}

void object_pool_free(struct object_pool *pool) {
  for (size_t i = 0; i < pool->buckets_nr; i++) {
    struct object_bucket *b = pool->buckets[i];
    while (b) {
      struct hex_oid hex;
      object_free(b->obj);
      struct object_bucket *tmp = b;
      b = b->next;
      free(tmp);
    }
  }

  free(pool->buckets);
  free(pool);
}

static size_t hash(struct object_pool *pool, struct object_id *oid) {
  size_t first_bytes;
  memcpy(&first_bytes, oid->hash, sizeof(size_t));
  size_t index = first_bytes % pool->buckets_nr;

  return index;
}

static void object_pool_insert(struct object_pool *pool, struct object *obj) {

  size_t index = hash(pool, &obj->oid);
  struct object_bucket *new_buck = xmalloc(1, struct object_bucket);
  new_buck->obj = obj;
  new_buck->next = pool->buckets[index];

  pool->buckets[index] = new_buck;
}

struct object *object_pool_lookup(struct object_pool *pool, struct object_id *oid) {
  size_t index = hash(pool, oid);
  struct object_bucket *b = pool->buckets[index];

  while (b) {
    if (oideq(&b->obj->oid, oid))
      return b->obj;
  }

  return NULL;
}

struct tree *object_pool_lookup_tree(struct object_pool *pool, struct object_id *oid) {
  struct object *obj = object_pool_lookup(pool, oid);
  if (obj) {
    if (obj->type != OBJ_TREE) {
      struct hex_oid hex;
      die("Fatal: object with oid %s is not a tree", oid_to_hex(oid, &hex));
    }

    return (struct tree *)obj;
  }

  struct tree *new_tree = xcalloc(1, struct tree);
  new_tree->obj.oid = *oid;
  new_tree->obj.type = OBJ_TREE;
  new_tree->obj.parsed = 0;

  object_pool_insert(pool, (struct object *)new_tree);
  return new_tree;
}

struct commit *object_pool_lookup_commit(struct object_pool *pool, struct object_id *oid) {
  struct object *obj = object_pool_lookup(pool, oid);
  if (obj) {
    if (obj->type != OBJ_COMMIT) {
      struct hex_oid hex;
      die("Fatal: object with oid %s is not a tree", oid_to_hex(oid, &hex));
    }

    return (struct commit *)obj;
  }

  struct commit *new_commit = xcalloc(1, struct commit);
  new_commit->obj.oid = *oid;
  new_commit->obj.type = OBJ_COMMIT;
  new_commit->obj.parsed = 0;

  object_pool_insert(pool, (struct object *)new_commit);
  return new_commit;
}
