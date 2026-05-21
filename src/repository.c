#include "repository.h"
#include "helper.h"
#include "ignore.h"
#include "object.h"
#include "object_store.h"
#include "pathspec.h"
#include "sha256.h"
#include "stage.h"
#include "strbuf.h"
#include <dirent.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void repository_init(struct repository *repo) {
  repo->repodir = repo_find_repo_dir();
  repo->worktree = repo_find_repo_worktree();
  repo->objects = NULL;
  repo->stage = NULL;
  repo->ignores = NULL;
}

void repository_release(struct repository *repo) {
  if (repo->repodir)
    free(repo->repodir);

  if (repo->worktree)
    free(repo->worktree);
}

char *repo_find_repo_dir() {

  char *repo_dir_path = NULL;
  struct strbuf current_path = STRBUF_INIT;
  strbuf_addstr(&current_path, ".");

  while (true) {

    strbuf_addf(&current_path, "/%s", REPO_DIR_NAME);
    struct stat st_repodir;
    if (stat(current_path.buf, &st_repodir) == 0 &&
        S_ISDIR(st_repodir.st_mode)) {

      repo_dir_path = normalize_path(current_path.buf);
      break;
    }

    int last_slash = last_index_of(current_path.buf, '/');
    strbuf_truncate(&current_path, last_slash);

    struct stat current, parent;
    if (stat(current_path.buf, &current) != 0)
      break;
    strbuf_addstr(&current_path, "/..");
    if (stat(current_path.buf, &parent) != 0)
      break;

    if (current.st_dev == parent.st_dev && current.st_ino == parent.st_ino)
      break;
  }

  strbuf_release(&current_path);
  return repo_dir_path;
}

char *repo_find_repo_worktree() {
  char *repodir = repo_find_repo_dir();
  if (!repodir)
    return NULL;

  struct strbuf worktree = STRBUF_INIT;

  strbuf_addstr(&worktree, repodir);
  free(repodir);
  return strbuf_truncate(&worktree, last_index_of(worktree.buf, '/'));
}

const char *repo_relative_path(struct repository *repo, const char *path) {
  if (strstr(path, repo->worktree) != path)
    return NULL;

  return path + strlen(repo->worktree) + 1;
}

struct object_store *repo_get_object_store(struct repository *repo) {
  if (repo->objects)
    return repo->objects;

  // allocate (lazy init)

  struct strbuf object_storage_loc = STRBUF_INIT;
  strbuf_addf(&object_storage_loc, "%s/%s", repo->repodir, "objects");
  repo->objects = object_store_new(object_storage_loc.buf);
  strbuf_release(&object_storage_loc);
  return repo->objects;
}

struct stage *repo_get_stage(struct repository *repo) {
  if (repo->stage)
    return repo->stage;

  struct strbuf stage_loc = STRBUF_INIT;
  strbuf_addf(&stage_loc, "%s/%s", repo->repodir, "stage");
  repo->stage = parse_stage_disk(stage_loc.buf);
  strbuf_release(&stage_loc);
  return repo->stage;
}

static int dont_go_into_repo_dir(const char *path, void *_) {
  if (strstr(path, REPO_DIR_NAME) || strstr(path, ".git"))
    return 1;

  return 0;
}

struct ignores **repo_get_ignores(struct repository *repo) {
  if (repo->ignores)
    return repo->ignores;

  // allocate (lazy init)
  struct strbuf ignores_spec = STRBUF_INIT;
  strbuf_addf(&ignores_spec, "%s/%s", repo->worktree, "*.eitignore");
  struct resolved_pathspec *ignores =
      resolve_pathspec(ignores_spec.buf, dont_go_into_repo_dir, NULL);

  strbuf_release(&ignores_spec);

  repo->ignores = xmalloc(ignores->nr + 1, struct ignores *);
  for (size_t i = 0; i < ignores->nr; i++) {
    repo->ignores[i] = parse_ignores(ignores->matching_paths[i]);
  }
  repo->ignores[ignores->nr] = NULL;

  return repo->ignores;
}

enum staging_error repo_stage_file(struct repository *repo, const char *path) {
  struct object_id oid;
  struct stat st;
  if (lstat(path, &st) != 0)
    return NO_SUCH_FILE;

  const char *relpath = repo_relative_path(repo, path);
  if (!relpath) {
    return PATH_OUTSIDE_REPO;
  }

  struct object_store *store = repo_get_object_store(repo);
  struct stage *stage = repo_get_stage(repo);

  int res = object_store_write_file(store, OBJ_BLOB, path, &oid, 1);
  if (res != 0)
    return STAGING_FAILED;

  res = stage_add_path(stage, relpath, st, &oid);
  if (res != 0)
    return STAGING_FAILED;

  return SUCCESS;
}

static int filter_ignored(const char *path, void *ignores) {
  if (strstr(path, ".eit") || strstr(path, ".git"))
    return 1;

  struct ignores **ig = (struct ignores **)ignores;
  size_t i = 0;
  while (ig[i]) {
    if (ignores_is_ignored(ig[i], path))
      return 1;
    i++;
  }

  return 0;
}

struct resolved_pathspec *
repo_resolve_pathspec_with_ignore(struct repository *repo,
                                  const char *pathspec) {
  return resolve_pathspec(pathspec, filter_ignored, repo_get_ignores(repo));
}
