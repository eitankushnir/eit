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
