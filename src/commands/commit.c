#include "commit.h"
#include "command.h"
#include "helper.h"
#include "object_pool.h"
#include "object_store.h"
#include "parse-options.h"
#include "ref_store.h"
#include "repository.h"
#include "sha256.h"
#include "stage.h"
#include <stdlib.h>
#include <time.h>

int cmd_commit(int argc, char **argv, struct repository *repo) {

  if (!repo->repodir) {
    die("Cannot run commit-tree outside of a repository");
  }

  const char *message;
  int allow_empty = 0;

  struct option opts[] = {
      MKOPT_STRING(NULL, 'm', message, "message", "Commit message"),
      MKOPT_BOOL_F("allow-empty", 0, allow_empty, "Enables the abillity to create a commit with no changes", OPT_NONEG),
      MKOPT_END,
  };

  char *usage[] = {
      "Usage: eit commit -m <message>",
      NULL,
  };

  argc = parse_options(argc, argv, opts, usage);
  if (!allow_empty) {
    struct ref_store *refs = repo_get_ref_store(repo);
    ref_store_parse_head(refs);
    if (refs->head_has_base) {
      struct commit *last_commit = object_pool_lookup_commit(repo_get_pool(repo), &refs->head_id);
      repo_parse_commit(repo, last_commit);
      struct stage *last_commit_stage = repo_construct_stage(repo, last_commit->tree);
      if (repo_count_stage_changes(repo, last_commit_stage) == 0)
        die("Error: No staged changes in to commit. Run with --allow-empty if you meant to commit nothing.");

      stage_free(last_commit_stage);
    }
  }

  struct object_id tree_oid;
  if (repo_write_stage_as_tree(repo, &tree_oid) != WRITE_TREE_SUCCESS) {
    die("Error: Failed to commit changes");
  }

  struct ref_store *refs = repo_get_ref_store(repo);
  ref_store_parse_head(refs);

  struct object_id *parent_id = &refs->head_id;

  time_t now = time(NULL);
  struct tm *local = localtime(&now);
  char tz[6];
  strftime(tz, sizeof(tz), "%z", local);

  char *name = repo_config_get_string(repo, "user", "name");
  char *email = repo_config_get_string(repo, "user", "email");
  if (!name || !email)
    die("Error: Must provide user.name and user.email in config. (see eit config -h)");

  struct commit_info info = {
      .parent_oids = parent_id,
      .parent_nr = 1,
      .tree_oid = &tree_oid,

      .author = name,
      .author_email = email,
      .author_time = now,
      .author_tz = tz,

      .committer = name,
      .committer_email = email,
      .commit_time = now,
      .committer_tz = tz,

      .message = message,
  };

  size_t size;
  void *buf = write_commit(&info, &size);

  struct object_id oid;
  struct hex_oid hex;
  struct object_store *objects = repo_get_object_store(repo);
  object_store_write_memory(objects, OBJ_COMMIT, buf, size, &oid, 1);

  free(buf);
  ref_store_update_head(refs, &oid);

  return 0;
}
