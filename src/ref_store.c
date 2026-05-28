#include "ref_store.h"
#include "helper.h"
#include "sha256.h"
#include "strbuf.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
struct ref_store *ref_store_new(const char *location) {

  struct ref_store *store = xmalloc(1, struct ref_store);
  struct strbuf heads = STRBUF_INIT,
                remotes = STRBUF_INIT,
                tags = STRBUF_INIT,
                head = STRBUF_INIT;

  strbuf_addf(&heads, "%s/refs/heads", location);
  strbuf_addf(&remotes, "%s/refs/remotes", location);
  strbuf_addf(&tags, "%s/refs/tags", location);
  strbuf_addf(&tags, "%s/HEAD", location);

  store->branches_location = heads.buf;
  store->remotes_location = remotes.buf;
  store->tags_location = tags.buf;
  store->head_location = head.buf;
  store->head_mode = HEAD_UNPARSED;

  store->head_branch = NULL;
  memset(store->head_id.hash, 0, 32);

  return store;
}

void ref_store_free(struct ref_store *store) {
  free(store->branches_location);
  free(store->remotes_location);
  free(store->tags_location);
  if (store->head_branch)
    free(store->head_branch);

  free(store);
}

void ref_store_parse_head(struct ref_store *store) {
  char buf[4096];
  struct strbuf content = STRBUF_INIT;

  FILE *headfile = fopen(store->head_location, "rb");
  if (!headfile)
    die("Fatal: Missing head file");

  size_t read;
  while ((read = fread(buf, sizeof(char), sizeof(buf), headfile))) {
    strbuf_addraw(&content, buf, read);
  }

  // Because in normal mode we start with ref:<space><branch name>
  char *colonptr = strchr(content.buf, ':');
  if (!colonptr) {
    // Detached mode.
    struct hex_oid hex;
    memcpy(hex.hex, content.buf, 65);
    hex_to_oid(&hex, &store->head_id);
    store->head_mode = HEAD_DETACHED;
  } else {
    store->head_branch = strdup(colonptr + 2);
    store->head_mode = HEAD_NORMAL;
    ref_store_read_branch(store, store->head_branch, &store->head_id);
  }

  strbuf_release(&content);
}

static char *branch_path(struct ref_store *store, const char *branch_name) {

  struct strbuf path = STRBUF_INIT;
  strbuf_addf(&path, "%s/%s", store->branches_location, branch_name);

  return path.buf;
}

int ref_store_read_branch(struct ref_store *store, const char *branch_name, struct object_id *out_oid) {
  char *path = branch_path(store, branch_name);

  struct hex_oid hex;
  FILE *branchfile = fopen(path, "rb");

  if (!branchfile)
    return -1;

  fread(hex.hex, sizeof(char), 64, branchfile);
  hex.hex[64] = '\0';

  hex_to_oid(&hex, out_oid);
  fclose(branchfile);
  free(path);

  return 0;
}

void ref_store_update_branch(struct ref_store *store, const char *branch_name, struct object_id *oid) {
  char *path = branch_path(store, branch_name);
  FILE *branchfile = fopen(path, "wb");

  if (!branchfile)
    die("Error: No branch named %s", branch_name);

  struct hex_oid hex;
  oid_to_hex(oid, &hex);
  fwrite(hex.hex, sizeof(char), 64, branchfile);

  fclose(branchfile);
  free(path);
}

void ref_store_update_head(struct ref_store *store, struct object_id *oid) {
  if (store->head_mode == HEAD_NORMAL) {
    ref_store_update_branch(store, store->head_branch, oid);
  } else if (store->head_mode == HEAD_DETACHED) {
    FILE *headfile = fopen(store->head_location, "wb");

    if (!headfile)
      die("Error: Failed to update head");

    struct hex_oid hex;
    oid_to_hex(oid, &hex);
    fwrite(hex.hex, sizeof(char), 64, headfile);
  }

  store->head_id = *oid;
}

int ref_store_create_branch(struct ref_store *store, const char *base, const char *new_name) {
  struct object_id id;
  if (ref_store_read_branch(store, base, &id) != 0)
    return -1;

  ref_store_update_branch(store, new_name, &id);

  return 0;
}
