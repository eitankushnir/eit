#ifndef STAGE_H
#define STAGE_H

#include "sha256.h"
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
struct stage_entry {
  struct stat st;
  struct object_id oid;
  int mode;
  char *path;
  size_t path_len;
  int flags;
};

struct stage {
  char *disk_location;
  struct stage_entry **entries;
  size_t entries_nr;
};

/*
 * Writes the stage information to the file in s->disk_location in the proper
 * format.
 */
int write_stage_disk(struct stage *s);

/*
 * Retuns a heap allocated stage struct with loaded with the info in the file.
 */
struct stage *parse_stage_disk(const char *path);

void stage_free(struct stage *s);

/*
 * Add and remove paths from the stage struct
 */
int stage_add_path(struct stage *s, const char *path, struct object_id *oid);
int stage_remove_path(struct stage *s, const char *path);

#endif
