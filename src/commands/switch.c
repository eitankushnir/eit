#include "command.h"
#include "commit.h"
#include "helper.h"
#include "object_pool.h"
#include "object_store.h"
#include "parse-options.h"
#include "ref_store.h"
#include "repository.h"
#include "sha256.h"
#include "stage.h"
#include <stddef.h>
#include <stdio.h>

int cmd_switch(int argc, char **argv, struct repository *repo) {

  if (!repo->repodir) {
    die("Cannot run switch outside of a repository");
  }

  int create = 0;
  int force_create = 0;
  int detach = 0;

  struct option opts[] = {
      MKOPT_BOOL("create", 'c', create, "Create a new branch"),
      MKOPT_BOOL("force-create", 'C', force_create, "Create a new branch. Can override existing branches."),
      MKOPT_BOOL("detach", 0, detach, "Enter detatch mode"),
      MKOPT_END,
  };

  char *usage[] = {
      "Usage: eit switch <branch>",
      "       or",
      "       eit switch (-c|-C) <new_branch>",
      "       eit switch --detach <commit-ish>",
      NULL,
  };

  argc = parse_options(argc, argv, opts, usage);

  if (argc < 2) {
    die("Error: Missing branch name");
  }
  if ((create || force_create) & detach) {
    die("Error: --(force-)create and --detatch cannot be used together");
  }

  char *branch_name = argv[1];
  if (!create && repo_count_worktree_changes(repo) > 0) {
    die("Error: You have unstaged changes");
  }

  struct ref_store *s = repo_get_ref_store(repo);
  ref_store_parse_head(s);
  struct commit *c_head = object_pool_lookup_commit(repo_get_pool(repo), &s->head_id);
  repo_parse_commit(repo, c_head);
  struct stage *head_stage = repo_construct_stage(repo, c_head->tree);
  if (repo_count_stage_changes(repo, head_stage) > 0) {
    die("Error: You have uncommitted changes");
  }
  stage_free(head_stage);

  if (force_create || create) {
    if (ref_store_has_branch(s, branch_name) && !force_create)
      die("Error: Cannot create branch, %s already exists. Use --force-create.", branch_name);
    if (s->head_has_base)
      ref_store_update_branch(s, branch_name, &s->head_id);

    ref_store_attach_head(s, branch_name);
    return 0;
  }

  if (detach) {
    struct object_store *os = repo_get_object_store(repo);
    struct object_id new_head;
    if (ref_store_read_branch(s, branch_name, &new_head) == 0) {
      ref_store_detach_head(s, &new_head);
    } else if (complete_hex(os, branch_name, &new_head) == AUTOCOMPLETE_SUCCESS &&
               object_read_type(os, &new_head) == OBJ_COMMIT) {
      ref_store_detach_head(s, &new_head);
    } else {
      fprintf(stderr, "Error: %s could does not match any commit\n", branch_name);
      return 1;
    }
  } else if (ref_store_has_branch(s, branch_name)) {
    ref_store_attach_head(s, branch_name);
  } else {
    fprintf(stderr, "Error: no branch named %s\n", branch_name);
    return 1;
  }

  struct commit *c = object_pool_lookup_commit(repo_get_pool(repo), &s->head_id);
  repo_parse_commit(repo, c);
  struct stage *new_ver = repo_construct_stage(repo, c->tree);
  repo_swap_stage(repo, new_ver);

  return 0;
}
