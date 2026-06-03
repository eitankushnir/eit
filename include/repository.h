#ifndef REPOSITORY_H
#define REPOSITORY_H

#include "config.h"
#include "ignore.h"
#include "object_pool.h"
#include "object_store.h"
#include "pathspec.h"
#include "sha256.h"
#include "stage.h"
#include "tree.h"
#include <stddef.h>
#define REPO_DIR_NAME ".eit"

struct repository {
  char *repodir;  // Absolute path to the .eit directory.
  char *worktree; // Absolute path to the root of the project.

  struct object_store *objects;
  struct stage *stage;
  struct ignores **ignores; // NULL terminated array of all ignore files in the repo.

  struct object_pool *parsed_object_pool; // Where all objects that where looked up and parsed live.

  struct ref_store *refs;

  config *local_config;
  config *global_config;
};

enum staging_error {
  STAGE_SUCCESS,
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
struct object_pool *repo_get_pool(struct repository *repo);
struct ref_store *repo_get_ref_store(struct repository *repo);

struct resolved_pathspec *repo_resolve_pathspec_with_ignore(struct repository *repo,
                                                            const char *pathspec);

enum write_tree_error {
  WRITE_TREE_SUCCESS,
  NOT_MERGED,
  STAGE_EMPTY,
};

enum write_tree_error repo_write_stage_as_tree(struct repository *repo, struct object_id *out_oid);
size_t repo_count_worktree_changes(struct repository *repo);

// Remake a stage using the paths from a given tree.
struct stage *repo_construct_stage(struct repository *repo, struct tree *tree);
size_t repo_count_stage_changes(struct repository *repo, struct stage *ver_b);
void repo_swap_stage(struct repository *repo, struct stage *new_stage);

void repo_pull_blob(struct repository *repo, const char *path, struct object_id *blob_id);
void repo_delete_file(struct repository *repo, const char *path);

config *repo_get_local_config(struct repository *repo);
config *repo_get_global_config(struct repository *repo);

// Wrappers - Check local first then global
int repo_config_get_bool(struct repository *repo, const char *catergory, const char *key, int default_value);
char *repo_config_get_string(struct repository *repo, const char *catergory, const char *key, char *default_value);

// PASRING FUNCTIONS.
// Takes a pointer (usually generated via a lookup function).
// Hydrates the struct with information from the disk. sets parsed flag to 1.
// If there is a type mismatch -1 is returned. else 0.
int repo_parse_tree(struct repository *repo, struct tree *tree);
int repo_parse_commit(struct repository *repo, struct commit *commit);

#endif
