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
      "       Stage an untracked file or chages to a tracked file",
      NULL,
  };

  argc = parse_options(argc, argv, NULL, usage);

  struct resolved_pathspec **specs = NULL;
  size_t c = 0;
  for (int i = 1; i < argc; i++) {
    struct resolved_pathspec *spec =
        repo_resolve_pathspec_with_ignore(repo, argv[i]);
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
      if (repo_stage_file(repo, spec->matching_paths[j]) != STAGE_SUCCESS)
        fprintf(stderr, "Failed to stage %s\n", spec->matching_paths[j]);
    }
  }

  for (size_t i = 0; i < c; i++) {
    struct resolved_pathspec *spec = specs[i];
    resolved_pathspec_free(spec);
  }

  free(specs);
  return 0;
}
