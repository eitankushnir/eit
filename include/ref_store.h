#ifndef REF_STORE_H
#define REF_STORE_H

#include "sha256.h"
enum head_mode {
  HEAD_UNPARSED,
  HEAD_NORMAL,
  HEAD_DETACHED,
};

struct ref_store {
  char *branches_location;
  char *tags_location;
  char *remotes_location;

  char *head_location;
  enum head_mode head_mode;

  char *head_branch; // FOR WHEN IN NORMAL MODE
  struct object_id head_id;
  int head_has_base;
};

struct ref_store *ref_store_new(const char *location);
void ref_store_free(struct ref_store *store);

void ref_store_parse_head(struct ref_store *store);

// Returns -1 if the branch cannot be found.
int ref_store_read_branch(struct ref_store *store, const char *branch_name, struct object_id *out_oid);

// Make branch_name point to the new oid.
void ref_store_update_branch(struct ref_store *store, const char *branch_name, struct object_id *oid);

int ref_store_has_branch(struct ref_store *store, const char *branch_name);

// Make HEAD point to a new oid.
void ref_store_update_head(struct ref_store *store, struct object_id *oid);
void ref_store_attach_head(struct ref_store *store, const char *branch_name);
void ref_store_detach_head(struct ref_store *store, struct object_id *oid);

#endif
