#include "command.h"
#include "helper.h"
#include "parse-options.h"
#include "pathspec.h"
#include "repository.h"
#include "stage.h"
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int cmd_add(int argc, char **argv, struct repository *repo) {
  if (!repo->repodir)
    die("Cannot run add outside of a repository");

  char *usage[] = {
      "Usage: git add [--] [<pathspec>...]",
      "       Stage an untracked file or chages to a tracked file",
      NULL,
  };

  argc = parse_options(argc, argv, NULL, usage);

  struct pathspec spec = PATHSPEC_INIT;
  for (size_t i = 1; i < argc; i++) {
    pathspec_add(&spec, argv[i]);
  }
  struct path_iterator *it = repo_path_iterator_create(repo);
  const char *path;
  int matchc = 0;
  while (it->next(it, &path) == 0) {
    if (pathspec_match(&spec, path)) {
      matchc++;

      struct stat st;
      if (stat(path, &st) != 0) {
        if (errno == ENOENT) {
          const char *repo_path = repo_relative_path(repo, path);
          stage_remove_path(repo_get_stage(repo), repo_path);
        }
      } else {
        repo_stage_file(repo, path);
      }
    }
    free(path);
  }
  pathspec_release(&spec);
  it->free(it);

  if (matchc == 0)
    die("Error: Empty pathspec");

  return 0;
}
