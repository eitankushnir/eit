#include "object.h"
#include "common.h"
#include "wrappers.h"
#include <string.h>

const char* type_name(object_type type)
{
    unsigned int idx = (unsigned int) type;
    return type_names[idx];
}

object_type type_from_name(char *name) {
    unsigned int n = ARRAY_SIZE(type_names);
    for (unsigned int i = 1; i < n; i++) {
        if (strcmp(name, type_names[i]) == 0) {
            return (object_type) i;
        }
    }

    die("Invalid type name: %s", name);
}
