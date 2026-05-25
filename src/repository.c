#include "repository.h"
#include "helper.h"
#include "ignore.h"
#include "object.h"
#include "object_store.h"
#include "pathspec.h"
#include "sha256.h"
#include "stage.h"
#include "strbuf.h"
#include "tree.h"
#include <dirent.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
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

  if (repo->objects)
    object_store_free(repo->objects);

  if (repo->stage)
    stage_free(repo->stage);

  if (repo->ignores) {
    size_t i = 0;
    while (repo->ignores[i]) {
      ignores_free(repo->ignores[i]);
      i++;
    }
    free(repo->ignores);
  }
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
  if (ends_with(path, REPO_DIR_NAME) || ends_with(path, ".git"))
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

  resolved_pathspec_free(ignores);
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

  return STAGE_SUCCESS;
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

struct resolved_pathspec *repo_resolve_pathspec_with_ignore(
    struct repository *repo,
    const char *pathspec) {

  return resolve_pathspec(pathspec, filter_ignored, repo_get_ignores(repo));
}

static enum write_tree_error repo_write_tree_helper(
    struct repository *repo,
    struct object_id *out_oid,
    const char *prefix,
    size_t start,
    size_t *out_end) {

  struct tree t = {0};
  struct stage *s = repo_get_stage(repo);
  struct object_store *store = repo_get_object_store(repo);

  size_t i = start;
  while (i < s->entries_nr) {
    struct stage_entry *ent = s->entries[i];
    const char *basename;

    if (skip_prefix(ent->path, prefix, &basename)) {
      if (index_of(basename, '/') == -1) {
        // File
        struct tree_entry new_ent = {
            .filename = basename,
            .filename_len = strlen(basename),
            .mode = ent->mode,
            .oid = ent->oid,
        };

        tree_add_entry(&t, &new_ent);
        i++;
      } else {
        struct tree_entry new_ent;
        size_t slash_idx = index_of(basename, '/');

        struct strbuf filename = STRBUF_INIT, new_loc = STRBUF_INIT;
        strbuf_addraw(&filename, basename, slash_idx);
        strbuf_addf(&new_loc, "%s%s/", prefix, filename.buf);

        new_ent.filename_len = slash_idx;
        new_ent.filename = filename.buf;
        new_ent.mode = 0040000;

        repo_write_tree_helper(repo, &new_ent.oid, new_loc.buf, i, &i);
        tree_add_entry(&t, &new_ent);

        strbuf_release(&filename);
        strbuf_release(&new_loc);
      }
      // Went outside of location.
    } else {
      break;
    }
  }

  *out_end = i;
  object_store_write_memory(store, OBJ_TREE, t.buf, t.size, out_oid, 1);
  free(t.buf);
  return WRITE_TREE_SUCCESS;
}

enum write_tree_error repo_write_stage_as_tree(
    struct repository *repo,
    struct object_id *out_oid) {

  size_t end;
  return repo_write_tree_helper(repo, out_oid, "", 0, &end);
}
