#ifndef OBJECT_H
#define OBJECT_H

enum object_type {
  OBJ_NONE = 0, // Usually means some sort of failure to find or identify.
  OBJ_BLOB = 1,
  OBJ_TREE = 2,
  OBJ_COMMIT = 3,
};

#endif
