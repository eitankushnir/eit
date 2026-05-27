#include "command.h"
#include "commit.h"
#include "helper.h"
#include "object.h"
#include "object_store.h"
#include "parse-options.h"
#include "repository.h"
#include "sha256.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int cmd_commit_tree(int argc, char **argv, struct repository *repo) {

  if (!repo->repodir) {
    die("Cannot run commit-tree outside of a repository");
  }

  char *message = NULL;
  struct string_array_value parents = {0};

  struct option opts[] = {
      MKOPT_STRING(NULL, 'm', message, "message", "Commit message"),
      MKOPT_STRING_ARRAY(NULL, 'p', parents, "[<hash>...]", "Hashes of the parent commits"),
      MKOPT_END,
  };

  char *usage[] = {
      "Usage: eit commit-tree <tree> -p [<parent ids>...] -m <message>",
      NULL,
  };

  argc = parse_options(argc, argv, opts, usage);
  if (argc != 2) {
    die("Incorrect amount of arguments.");
  }

  char *tree_hex = argv[1];
  enum autocomplete_error err;

  struct object_store *store = repo_get_object_store(repo);

  struct object_id tree_id;
  err = complete_hex(store, tree_hex, &tree_id);
  if (err == AMBIGOUS_HEX) {
    die("Error: Tree hex is ambigous.");
  } else if (err == NO_SUCH_HEX) {
    die("Error: Tree hex does not match any hex");
  } else if (err == PARTIAL_TOO_SHORT) {
    die("Error: Tree hex must be at least 3 characters");
  } else if (object_read_type(store, &tree_id) != OBJ_TREE) {
    die("Error: %s does not match a tree", tree_hex);
  }

  struct object_id *parent_ids = xmalloc(parents.size, struct object_id);
  char *run = parents.ptr;
  for (size_t i = 0; i < parents.size; i++) {
    err = complete_hex(store, run, parent_ids + i);
    if (err == AMBIGOUS_HEX) {
      die("Error: %s is ambigous.", run);
    } else if (err == NO_SUCH_HEX) {
      die("Error: %s does not match any hex", run);
    } else if (err == PARTIAL_TOO_SHORT) {
      die("Error: %s must be at least 3 characters", run);
    } else if (object_read_type(store, parent_ids + i) != OBJ_COMMIT) {
      die("Error: %s does not match a commit", run);
    }
    run += strlen(run) + 1;
  }

  time_t now = time(NULL);
  struct tm *local = localtime(&now);
  char tz[6];
  strftime(tz, sizeof(tz), "%z", local);
  struct commit_info info = {
      .parent_oids = parent_ids,
      .parent_nr = parents.size,
      .tree_oid = &tree_id,

      .author = "eitan",
      .author_email = "hi@gm.com",
      .author_time = now,
      .author_tz = tz,

      .committer = "eitan",
      .committer_email = "hi@g.c",
      .commit_time = now,
      .committer_tz = tz,

      .message = message,
  };

  size_t size;
  void *buf = write_commit(&info, &size);

  struct object_id oid;
  struct hex_oid hex;
  object_store_write_memory(store, OBJ_COMMIT, buf, size, &oid, 1);

  free(parent_ids);
  free(buf);
  printf("%s\n", oid_to_hex(&oid, &hex));
}
