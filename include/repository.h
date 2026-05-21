#ifndef REPOSITORY_H
#define REPOSITORY_H

#include "ignore.h"
#include "object_store.h"
#include "pathspec.h"
#include "stage.h"
#define REPO_DIR_NAME ".eit"

struct repository {
  char *repodir;  // Absolute path to the .eit directory.
  char *worktree; // Absolute path to the root of the project.

  struct object_store *objects;
  struct stage *stage;
  struct ignores *
      *ignores; // NULL terminated array of all ignore files in the repo.
};

enum staging_error {
  SUCCESS,
  PATH_OUTSIDE_REPO,
  NO_SUCH_FILE,
  STAGING_FAILED,
};

void repository_init(struct repository *repo);
void repository_release(struct repository *repo);

char *repo_find_repo_dir(void);
char *repo_find_repo_worktree(void);

enum staging_error repo_stage_file(struct repository *repo, const char *path);

/*
 * Returns NULL if an *absolute* path is outside the repo.
 * Otherwise returns a pointer (in place) to the path relative to the repo.
 */
const char *repo_relative_path(struct repository *repo, const char *path);

struct object_store *repo_get_object_store(struct repository *repo);
struct stage *repo_get_stage(struct repository *repo);
struct ignores **repo_get_ignores(struct repository *repo);

struct resolved_pathspec *
repo_resolve_pathspec_with_ignore(struct repository *repo,
                                  const char *pathspec);

#endif
