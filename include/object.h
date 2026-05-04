#ifndef OBJECT_H
#define OBJECT_H

#include "sha256.h"
#include <stddef.h>
enum object_type {
  OBJ_UNKNOWN = 0, // Usually means some sort of failure to find or identify.
  OBJ_BLOB = 1,
  OBJ_TREE = 2,
  OBJ_COMMIT = 3,
};

struct object {
  struct object_id oid;
  enum object_type type;
};

struct object_bucket {
  struct object_bucket *next;
  struct object *obj;
};

struct parsed_object_pool {
  size_t bucket_nr;
  struct object_bucket **buckets;
};

static const char *object_type_names[] = {"unknown", "blob", "tree", "commit"};

const char *object_type_to_string(enum object_type type);
enum object_type string_to_object_type(const char *s);

// OBJ POOL STUFF

void object_bucket_free(struct object_bucket *bucket);
struct parsed_object_pool *parsed_object_pool_new(size_t initial_bucket_count);
void parsed_object_pool_free(struct parsed_object_pool *pool);

void parsed_object_pool_insert(struct parsed_object_pool *pool,
                               struct object *obj);

struct object *find_object(struct parsed_object_pool *pool,
                           struct object_id *oid);
#endif
