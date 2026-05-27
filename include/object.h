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
  int parsed;
};

#pragma GCC diagnostic ignored "-Wunused-variable"
static const char *object_type_names[] = {"unknown", "blob", "tree", "commit"};

const char *object_type_to_string(enum object_type type);
enum object_type string_to_object_type(const char *s);
#endif
