#include "command.h"
#include "helper.h"
#include "parse-options.h"
#include "pathspec.h"
#include "repository.h"
#include "stage.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

int cmd_add(int argc, char **argv, struct repository *repo) {

  char *usage[] = {
      "Usage: git add [--] [<pathspec>...]",
      NULL,
  };

  argc = parse_options(argc, argv, NULL, usage);

  struct resolved_pathspec **specs = NULL;
  size_t c = 0;
  for (int i = 1; i < argc; i++) {
    struct resolved_pathspec *spec = resolve_pathspec(argv[i], NULL, NULL);
    if (spec->nr == 0) {
      free(spec);
      continue;
    }
    specs = xrealloc(specs, ++c, struct resolved_pathspec *);
    specs[c - 1] = spec;
  }

  if (c == 0)
    die("Error: Empty pathspec");

  for (size_t i = 0; i < c; i++) {
    struct resolved_pathspec *spec = specs[i];
    for (size_t j = 0; j < spec->nr; j++) {
      if (repo_stage_file(repo, spec->matching_paths[j]) != SUCCESS)
        fprintf(stderr, "Failed to stage %s\n", spec->matching_paths[j]);
    }
  }

  write_stage_disk(repo_get_stage(repo));
  return 0;
}
