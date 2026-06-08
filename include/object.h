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

enum object_flags {
  PARENT1 = 1 << 0,
  PARENT2 = 1 << 1,
  COMMON = (PARENT1 | PARENT2),
  SEEN = 1 << 2,
  SOLUTION = 1 << 3,
  STALE = 1 << 4,
  REDUNDANT = 1 << 5,
};

struct object {
  struct object_id oid;
  enum object_type type;
  int parsed;
  int flags;
};

#pragma GCC diagnostic ignored "-Wunused-variable"
static const char *object_type_names[] = {"unknown", "blob", "tree", "commit"};

const char *object_type_to_string(enum object_type type);
enum object_type string_to_object_type(const char *s);
#endif
