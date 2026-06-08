#include "command.h"
#include "helper.h"
#include "ignore.h"
#include "object.h"
#include "object_store.h"
#include "pathspec.h"
#include "repository.h"
#include "sha256.h"
#include "stage.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct command commands[] = {
    {"init", cmd_init},
    {"hash-object", cmd_hash_object},
    {"ls-files", cmd_ls_files},
    {"add", cmd_add},
    {"write-tree", cmd_write_tree},
    {"cat-file", cmd_cat_file},
    {"commit-tree", cmd_commit_tree},
    {"switch", cmd_switch},
    {"commit", cmd_commit},
    {"config", cmd_config},
    {"diff", cmd_diff},
};

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: eit <command> [<args>]\n");
    return 0;
  }

  size_t command_nr = sizeof(commands) / sizeof(struct command);
  char *command = argv[1];

  struct repository repo;
  repository_init(&repo);

  for (size_t i = 0; i < command_nr; i++) {
    if (strcmp(command, commands[i].name) == 0) {
      int result = commands[i].fn(argc - 1, argv + 1, &repo);
      repository_release(&repo);
      return result;
    }
  }

  printf("Unkown command: %s\n", command);
  repository_release(&repo);
  return 1;
}
