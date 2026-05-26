#ifndef OBJECT_POOL_H
#define OBJECT_POOL_H

#include "object.h"
#include "sha256.h"
#include "tree.h"
#include <stddef.h>

struct object_bucket {
  struct object *obj;
  struct object_bucket *next;
};

struct object_pool {
  struct object_bucket **buckets;
  size_t buckets_nr;
};

void object_pool_free(struct object_pool *pool);
struct object_pool *object_pool_new(size_t buckets_nr);

/*
 * Get a non-owning pointer to an object with a specific oid.
 * Returns NULL if no such object exists in the pool.
 */
struct object *object_pool_lookup(struct object_pool *pool, struct object_id *oid);

/*
 * Returns a non-owning pointer to a tree object in the pool.
 * If the object does not exists creats an un-parsed dummy to be later parsed using parse_tree(...).
 * Program will die if oid does not belong to a tree.
 */
struct tree *object_pool_lookup_tree(struct object_pool *pool, struct object_id *oid);
#endif
