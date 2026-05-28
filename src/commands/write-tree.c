#include "command.h"
#include "parse-options.h"
#include "repository.h"
#include "sha256.h"
#include <stdio.h>
int cmd_write_tree(int argc, char **argv, struct repository *repo) {

  struct object_id oid;
  struct hex_oid hex;

  char *usage[] = {
      "Usage: git write-tree",
      "       Converts the stage into tree objects to be committed",
      NULL,
  };

  argc = parse_options(argc, argv, NULL, usage);

  if (repo_write_stage_as_tree(repo, &oid) != WRITE_TREE_SUCCESS) {
    fprintf(stderr, "Error: Failed to write tree\n");
    return 1;
  }
  printf("%s\n", oid_to_hex(&oid, &hex));
  return 0;
}
