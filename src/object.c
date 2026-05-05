#include "object.h"
#include "helper.h"
#include "sha256.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

const char *object_type_to_string(enum object_type type) {
  if (type < 0 || type > 3)
    return "unknown";

  return object_type_names[type];
}

enum object_type string_to_object_type(const char *s) {
  for (int i = 0; i < 4; i++) {
    if (strcmp(object_type_names[i], s) == 0)
      return (enum object_type)i;
  }

  return OBJ_UNKNOWN;
}

struct parsed_object_pool *parsed_object_pool_new(size_t initial_bucket_count) {
  struct parsed_object_pool *pool = xmalloc(1, struct parsed_object_pool);

  pool->bucket_nr = initial_bucket_count;
  pool->buckets = xcalloc(pool->bucket_nr, struct object_bucket *);

  return pool;
}

void object_bucket_free(struct object_bucket *bucket) {
  while (bucket) {
    free(bucket->obj);
    struct object_bucket *temp = bucket;
    bucket = bucket->next;
    free(temp);
  }
}

void parsed_object_pool_free(struct parsed_object_pool *pool) {
  for (size_t i = 0; i < pool->bucket_nr; i++) {
    object_bucket_free(pool->buckets[i]);
  }

  free(pool->buckets);
  free(pool);
}

void parsed_object_pool_insert(struct parsed_object_pool *pool,
                               struct object *obj) {
  size_t first_bytes;
  memcpy(&first_bytes, obj->oid.hash, sizeof(size_t));
  size_t index = first_bytes % pool->bucket_nr;

  struct object_bucket *new_bucket = xmalloc(1, struct object_bucket);
  new_bucket->next = pool->buckets[index];
  new_bucket->obj = obj;

  pool->buckets[index] = new_bucket;
}

struct object *lookup_object(struct parsed_object_pool *pool,
                             struct object_id *oid) {

  size_t first_bytes;
  memcpy(&first_bytes, oid->hash, sizeof(size_t));
  size_t index = first_bytes % pool->bucket_nr;

  if (!pool->buckets[index]) {
    return NULL;
  }

  struct object_bucket *bucket = pool->buckets[index];
  while (bucket) {
    if (oideq(&bucket->obj->oid, oid))
      return bucket->obj;

    bucket = bucket->next;
  }

  // No object, create a fakey so it can be parsed later on.
  struct object *stub = xmalloc(1, struct object);
  stub->parsed = 0;
  stub->oid = *oid;

  return NULL;
}
