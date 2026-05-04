#ifndef OBJECT_STORE_H
#define OBJECT_STORE_H

#include "object.h"
#include "sha256.h"
#include <stddef.h>
struct object_store {
  char *objectsdir; // Absolute path to the physical object_store.
};

// Allocate a new object_store to operate inside objectsdir
struct object_store *object_store_new(const char *objectsdir);

// Free an allocated object_store.
void object_store_free(struct object_store *store);

// Convert and oid to the absolute path where it's stored or would be stored
// when written.
char *oid_to_path(struct object_store *store, struct object_id *oid);

// Check whether an object with oid exists in the store.
int object_exists(struct object_store *store, struct object_id *oid);

/*
 * Write an object to the object store.
 * Content is the len raw bytes pointed at by buf.
 * Function will output the oid of the new object.
 * Returns 0 on success.
 */
int object_store_write_memory(struct object_store *store, enum object_type type,
                              void *buf, size_t len, struct object_id *out_oid,
                              int write_to_disk);

/*
 * Write an object to the object store.
 * Same as object_store_write_memory but will instead recieve a path to a file
 * and will create an object from it
 * Returns 0 on success.
 */
int object_store_write_file(struct object_store *store, enum object_type type,
                            const char *path, struct object_id *out_oid,
                            int write_to_disk);

#endif
