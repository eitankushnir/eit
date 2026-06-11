#include "command.h"
#include "parse-options.h"
#include "repository.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int cmd_rm(int argc, char **argv, struct repository *repo) {
  if (!repo->repodir)
    die("Cannot run add outside of a repository");

  int cached = 0;

  struct option opts[] = {
      MKOPT_BOOL_F("cached", 0, cached, "Only unstage the file", OPT_NONEG),
      MKOPT_END,
  };

  char *usage[] = {
      "Usage: git rm [--cached] [--] [<pathspec>...]",
      "       Remove a file and unstage it",
      NULL,
  };

  argc = parse_options(argc, argv, opts, usage);

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

      const char *repo_path = repo_relative_path(repo, path);
      stage_remove_path(repo_get_stage(repo), repo_path);
      struct stat st;
      if (stat(path, &st) == 0 && !cached) {
        repo_delete_file(repo, repo_path);
      }
    }
    free(path);
  }
  pathspec_release(&spec);

  if (matchc == 0)
    die("Error: Empty pathspec");

  return 0;
}
