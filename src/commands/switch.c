#include "command.h"
#include "commit.h"
#include "helper.h"
#include "object_pool.h"
#include "object_store.h"
#include "parse-options.h"
#include "ref_store.h"
#include "repository.h"
#include "sha256.h"
#include <stddef.h>
#include <stdio.h>

int cmd_switch(int argc, char **argv, struct repository *repo) {

  if (!repo->repodir) {
    die("Cannot run switch outside of a repository");
  }

  int create = 0;
  int detach = 0;

  struct option opts[] = {
      MKOPT_BOOL("create", 'c', create, "Create a new branch"),
      MKOPT_BOOL("detach", 0, detach, "Enter detatch mode"),
      MKOPT_END,
  };

  char *usage[] = {
      "Usage: eit switch <branch>",
      "       or",
      "       eit switch -c <new_branch>",
      "       eit switch --detach <commit-ish>",
      NULL,
  };

  argc = parse_options(argc, argv, opts, usage);

  if (argc < 2) {
    die("Error: Missing branch name");
  }
  if (create & detach) {
    die("Error: --create and --detatch cannot be used together");
  }

  char *branch_name = argv[1];
  struct ref_store *s = repo_get_ref_store(repo);
  if (!create && repo_count_worktree_changes(repo) > 0) {
    die("Error: You have unstage changes");
  }

  if (create) {
    ref_store_parse_head(s);
    if (s->head_has_base)
      ref_store_update_branch(s, branch_name, &s->head_id);

    ref_store_attach_head(s, branch_name);

  } else if (detach) {
    struct object_store *os = repo_get_object_store(repo);
    struct object_id new_head;
    if (ref_store_read_branch(s, branch_name, &new_head) == 0) {
      ref_store_detach_head(s, &new_head);
    } else if (complete_hex(os, branch_name, &new_head) == AUTOCOMPLETE_SUCCESS &&
               object_read_type(os, &new_head) == OBJ_COMMIT) {
      ref_store_detach_head(s, &new_head);
    } else {
      fprintf(stderr, "Error: %s could does not match any commit\n", branch_name);
    }
  } else if (ref_store_has_branch(s, branch_name)) {
    ref_store_attach_head(s, branch_name);

    struct commit *c = object_pool_lookup_commit(repo_get_pool(repo), &s->head_id);
    repo_parse_commit(repo, c);
    struct stage *s = repo_construct_stage(repo, c->tree);

    for (size_t i = 0; i < s->entries_nr; i++) {
      printf("%s\n", s->entries[i]->path);
    }
  } else {
    fprintf(stderr, "Error: no branch named %s\n", branch_name);
  }

  return 0;
}
