#include "command.h"
#include "helper.h"
#include "object.h"
#include "object_store.h"
#include "parse-options.h"
#include "repository.h"
#include "sha256.h"
#include "strbuf.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct command commands[] = {

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
    if (strcmp(command, commands[i].name) == 0)
      return commands[i].fn(argc - 1, argv + 1, &repo);
  }

  printf("Unkown command: %s\n", command);
  return 1;
}
