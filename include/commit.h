#ifndef COMMIT_H
#define COMMIT_H

#include "sha256.h"
#include "tree.h"
#include <stddef.h>
#include <time.h>
struct commit_info {
  struct object_id *tree_oid;
  struct object_id *parent_oids;
  size_t parent_nr;

  const char *author;
  const char *author_email;
  time_t author_time;
  const char *author_tz;

  const char *committer;
  const char *committer_email;
  time_t commit_time;
  const char *committer_tz;

  const char *message;
};

struct commit {
  struct object obj;
  struct tree *tree;
};

// Create a buffer of raw bytes that can be stored on the disk as the commit.
void *write_commit(struct commit_info *info, size_t *out_size);

// Read the data from the raw buffer into the struct.
// The buffer still owns the data.
void hydrate_commit_info(struct commit_info *info, void *buf);
void pretty_print_commit(struct commit_info *info);

#endif
