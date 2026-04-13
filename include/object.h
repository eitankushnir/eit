#ifndef OBJECT_H
#define OBJECT_H

#include "repository.h"
#include "sha256.h"
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    OBJ_BAD = -1,
    OBJ_NONE = 0,
    OBJ_BLOB = 1,
    OBJ_TREE = 2,
    OBJ_COMMIT = 3,
} object_type;

static const char* type_names[] = {
    NULL,
    "blob",
    "tree",
    "commit"
};

typedef struct {
    object_type type;
    object_id oid;
} object;

const char* type_name(object_type type);
object_type type_from_name(char* name);

void write_object(object_type type, FILE* src, repository* repo, object_id* out_oid);
oid_hex complete_hash_hex(const char* short_hash, repository* repo);
FILE* open_object(const oid_hex* hex, repository* repo);
object_type get_type(const oid_hex* hex, repository* repo);

#endif
