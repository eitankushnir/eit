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
  strbuf_addf(&head, "%s/HEAD", location);

  store->branches_location = heads.buf;
  store->remotes_location = remotes.buf;
  store->tags_location = tags.buf;
  store->head_location = head.buf;
  store->head_mode = HEAD_UNPARSED;

  store->head_branch = NULL;
  memset(store->head_id.hash, 0, 32);
  store->head_has_base = 0;

  return store;
}

void ref_store_free(struct ref_store *store) {
  free(store->branches_location);
  free(store->remotes_location);
  free(store->tags_location);
  free(store->head_location);
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
    store->head_has_base = 1;
  } else {
    store->head_branch = strdup(colonptr + 2);
    store->head_mode = HEAD_NORMAL;
    store->head_has_base = ref_store_read_branch(store, store->head_branch, &store->head_id) == 0;
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

int ref_store_has_branch(struct ref_store *store, const char *branch_name) {
  char *path = branch_path(store, branch_name);
  struct stat st;
  int res = stat(path, &st);
  free(path);
  return res == 0;
}

void ref_store_attach_head(struct ref_store *store, const char *branch_name) {
  FILE *headfile = fopen(store->head_location, "wb");

  if (!headfile)
    die("Error: Failed to update head");

  fprintf(headfile, "branch: %s", branch_name);
  store->head_mode = HEAD_NORMAL;
  store->head_has_base = ref_store_read_branch(store, branch_name, &store->head_id) == 0;
  fclose(headfile);
}

void ref_store_detach_head(struct ref_store *store, struct object_id *oid) {
  FILE *headfile = fopen(store->head_location, "wb");

  if (!headfile)
    die("Error: Failed to update head");

  struct hex_oid hex;
  fprintf(headfile, "%s", oid_to_hex(oid, &hex));
  store->head_mode = HEAD_DETACHED;
  fclose(headfile);
}
