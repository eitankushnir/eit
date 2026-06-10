#include "repository.h"
#include "commit.h"
#include "config.h"
#include "helper.h"
#include "ignore.h"
#include "object.h"
#include "object_pool.h"
#include "object_store.h"
#include "pathspec.h"
#include "ref_store.h"
#include "sha256.h"
#include "stage.h"
#include "strbuf.h"
#include "tree.h"
#include <dirent.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

void repository_init(struct repository *repo) {
  memset(repo, 0, sizeof(struct repository));
  repo->repodir = repo_find_repo_dir();
  repo->worktree = repo_find_repo_worktree();
}

void repository_release(struct repository *repo) {
  if (repo->repodir)
    free(repo->repodir);

  if (repo->worktree)
    free(repo->worktree);

  if (repo->objects)
    object_store_free(repo->objects);

  if (repo->stage) {
    write_stage_disk(repo->stage);
    stage_free(repo->stage);
  }

  if (repo->ignores) {
    size_t i = 0;
    while (repo->ignores[i]) {
      ignores_free(repo->ignores[i]);
      i++;
    }
    free(repo->ignores);
  }

  if (repo->parsed_object_pool)
    object_pool_free(repo->parsed_object_pool);

  if (repo->refs)
    ref_store_free(repo->refs);

  if (repo->local_config)
    config_free(repo->local_config);

  if (repo->global_config)
    config_free(repo->global_config);
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
    strbuf_setlen(&current_path, last_slash);

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
  return strbuf_setlen(&worktree, last_index_of(worktree.buf, '/'));
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

  // // allocate (lazy init)
  struct strbuf ignores_spec = STRBUF_INIT;
  strbuf_addf(&ignores_spec, "%s/%s", repo->worktree, "*.eitignore");
  struct path_iterator *fs = fs_iterator_create(repo->worktree);
  ((struct fs_iterator *)fs)->filter_func = dont_go_into_repo_dir;

  struct pathspec spec = PATHSPEC_INIT;
  pathspec_add(&spec, ignores_spec.buf);

  const char *path;
  size_t nr = 0;
  while (fs->next(fs, &path) == 0) {
    if (pathspec_match(&spec, path)) {
      repo->ignores = xrealloc(repo->ignores, ++nr, struct ignores);
      repo->ignores[nr - 1] = parse_ignores(path);
    }
  }

  repo->ignores = xrealloc(repo->ignores, ++nr, struct ignores);
  repo->ignores[nr - 1] = NULL;
  return repo->ignores;
}

struct object_pool *repo_get_pool(struct repository *repo) {
  if (repo->parsed_object_pool)
    return repo->parsed_object_pool;

  repo->parsed_object_pool = object_pool_new(100);
  return repo->parsed_object_pool;
}

struct ref_store *repo_get_ref_store(struct repository *repo) {
  if (repo->refs)
    return repo->refs;

  repo->refs = ref_store_new(repo->repodir);
  return repo->refs;
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

  res = stage_add_path(stage, relpath, st, &oid, 0);
  if (res != 0)
    return STAGING_FAILED;

  return STAGE_SUCCESS;
}

static int filter_ignored(const char *path, void *ignores) {
  if (ends_with(path, REPO_DIR_NAME) || ends_with(path, ".git"))
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
    if (ent->flags != 0)
      return NOT_MERGED;
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

        enum write_tree_error e = repo_write_tree_helper(repo, &new_ent.oid, new_loc.buf, i, &i);
        if (e != WRITE_TREE_SUCCESS) {
          strbuf_release(&filename);
          strbuf_release(&new_loc);
          free(t.buf);
          return e;
        }
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

size_t repo_count_worktree_changes(struct repository *repo) {
  struct stage *s = repo_get_stage(repo);
  size_t count = 0;

  for (size_t i = 0; i < s->entries_nr; i++) {
    struct strbuf fullpath = STRBUF_INIT;
    strbuf_addf(&fullpath, "%s/%s", repo->worktree, s->entries[i]->path);
    struct stat st;
    if (lstat(fullpath.buf, &st) != 0)
      count++;
    else if (st.st_mtim.tv_sec > s->entries[i]->st.st_mtim.tv_sec ||
             (st.st_mtim.tv_sec == s->entries[i]->st.st_mtim.tv_sec &&
              st.st_mtim.tv_nsec > s->entries[i]->st.st_mtim.tv_nsec)) {
      struct object_id oid;
      object_store_write_file(repo_get_object_store(repo), OBJ_BLOB,
                              fullpath.buf, &oid, 0);
      if (!oideq(&s->entries[i]->oid, &oid))
        count += 1;
    }
    strbuf_release(&fullpath);
  }

  return count;
}

int repo_parse_tree(struct repository *repo, struct tree *tree) {
  if (tree->obj.parsed)
    return 0;

  struct object_store *store = repo_get_object_store(repo);
  size_t size;
  enum object_type type;
  void *buf = object_store_read_raw(store, &tree->obj.oid, &size, &type);
  if (!buf)
    return -1;

  if (type != OBJ_TREE) {
    free(buf);
    return -1;
  }

  tree->buf = buf;
  tree->size = size;
  tree->obj.parsed = 1;

  return 0;
}

int repo_parse_commit(struct repository *repo, struct commit *commit) {
  if (commit->obj.parsed)
    return 0;

  struct object_store *store = repo_get_object_store(repo);
  size_t size;
  enum object_type type;
  void *buf = object_store_read_raw(store, &commit->obj.oid, &size, &type);
  if (!buf)
    return -1;

  if (type != OBJ_COMMIT) {
    free(buf);
    return -1;
  }

  struct commit_info info;
  hydrate_commit_info(&info, buf);

  commit->obj.parsed = 1;
  commit->tree = object_pool_lookup_tree(repo_get_pool(repo), info.tree_oid);
  repo_parse_tree(repo, commit->tree);

  commit->date = info.commit_time;
  commit->parents = NULL;
  for (size_t i = 0; i < info.parent_nr; i++) {
    struct commit *parent = object_pool_lookup_commit(repo_get_pool(repo), &info.parent_oids[i]);
    repo_parse_commit(repo, parent);
    struct commit_list *new_node = xmalloc(1, struct commit_list);
    new_node->item = parent;
    new_node->next = commit->parents;
    commit->parents = new_node;
  }

  free(buf);
  return 0;
}

void construct_stage_helper(struct repository *repo, struct tree *tree, const char *current_location, struct stage *current_stage) {
  struct tree_iterator it;
  tree_get_iterator(tree, &it);
  struct stage *s = current_stage;

  struct tree_entry *ent;
  while ((ent = tree_iterate(&it))) {
    struct strbuf path = STRBUF_INIT;
    strbuf_addstr(&path, current_location);
    if (path.len == 0)
      strbuf_addstr(&path, ent->filename);
    else
      strbuf_addf(&path, "/%s", ent->filename);

    if (ent->mode == 0040000) {
      struct object_pool *pool = repo_get_pool(repo);
      struct tree *t = object_pool_lookup_tree(pool, &ent->oid);
      repo_parse_tree(repo, t);
      construct_stage_helper(repo, t, path.buf, current_stage);
      strbuf_release(&path);
    } else {
      s->entries = xrealloc(s->entries, ++s->entries_nr, struct stage_entry *);
      struct stage_entry *new_ent = xmalloc(1, struct stage_entry);
      new_ent->mode = ent->mode;
      new_ent->path = path.buf;
      new_ent->path_len = path.len;
      new_ent->flags = 0;
      new_ent->oid = ent->oid;
      memset(&new_ent->st, 0, sizeof(struct stat));
      s->entries[s->entries_nr - 1] = new_ent;
    }
  }
}

struct stage *repo_construct_stage(struct repository *repo, struct tree *tree) {
  struct stage *s = xcalloc(1, struct stage);
  construct_stage_helper(repo, tree, "", s);
  return s;
}

size_t repo_count_stage_changes(struct repository *repo, struct stage *ver_b) {
  size_t i = 0, j = 0;
  size_t count = 0;

  struct stage *ver_a = repo_get_stage(repo);

  while (i < ver_a->entries_nr && j < ver_b->entries_nr) {
    int cmp = strcmp(ver_a->entries[i]->path, ver_b->entries[j]->path);
    if (cmp == 0) {
      count = oideq(&ver_a->entries[i]->oid, &ver_b->entries[j]->oid) ? count : count + 1;
      i++;
      j++;
    } else if (cmp < 0) {
      i++;
      count++;
    } else {
      count++;
      j++;
    }
  }

  while (i < ver_a->entries_nr) {
    count++;
    i++;
  }

  while (j < ver_b->entries_nr) {
    count++;
    j++;
  }

  return count;
}

void repo_pull_blob(struct repository *repo, const char *path, struct object_id *blob_id) {

  struct strbuf loc = STRBUF_INIT;
  strbuf_addstr(&loc, repo->worktree);

  char *cpy = strdup(path);
  char *basename = basename_inplace(cpy);
  char *dirname = strtok(cpy, "/");

  while (dirname != basename) {
    strbuf_addf(&loc, "/%s", dirname);
    create_directory(loc.buf);
    dirname = strtok(NULL, "/");
  }

  struct object_store *s = repo_get_object_store(repo);
  object_store_stream_raw(s, blob_id, path);

  strbuf_release(&loc);
  free(cpy);
}

void repo_delete_file(struct repository *repo, const char *path) {
  size_t len = strlen(repo->worktree);
  struct strbuf fullpath = STRBUF_INIT;
  strbuf_addf(&fullpath, "%s/%s", repo->worktree, path);

  int res = 0;
  while (fullpath.len >= len && res != 0) {
    res = remove(fullpath.buf);
    strbuf_setlen(&fullpath, last_index_of(fullpath.buf, '/'));
  }

  strbuf_release(&fullpath);
}

void repo_swap_stage(struct repository *repo, struct stage *new_stage) {
  struct stage *current = repo_get_stage(repo);

  size_t i = 0;
  size_t j = 0;
  while (i < current->entries_nr && j < new_stage->entries_nr) {
    int cmp = strcmp(current->entries[i]->path, new_stage->entries[j]->path);
    if (cmp == 0) {
      if (!oideq(&current->entries[i]->oid, &new_stage->entries[j]->oid))
        repo_pull_blob(repo, new_stage->entries[j]->path, &new_stage->entries[j]->oid);
      i++;
      j++;
    } else if (cmp < 0) {
      repo_delete_file(repo, current->entries[i]->path);
      i++;
    } else {
      repo_pull_blob(repo, new_stage->entries[j]->path, &new_stage->entries[j]->oid);
      j++;
    }
  }

  while (i < current->entries_nr) {
    repo_delete_file(repo, current->entries[i]->path);
    i++;
  }

  while (j < new_stage->entries_nr) {
    repo_pull_blob(repo, new_stage->entries[j]->path, &new_stage->entries[j]->oid);
    j++;
  }

  size_t len = strlen(repo->worktree);
  struct strbuf abspath = STRBUF_INIT;
  strbuf_addstr(&abspath, repo->worktree);
  for (i = 0; i < new_stage->entries_nr; i++) {
    strbuf_addf(&abspath, "/%s", new_stage->entries[i]->path);
    lstat(abspath.buf, &new_stage->entries[i]->st);
    strbuf_setlen(&abspath, len);
  }
  strbuf_release(&abspath);

  repo->stage = new_stage;
  new_stage->disk_location = strdup(current->disk_location);
  stage_free(current);
}

config *repo_get_local_config(struct repository *repo) {
  if (repo->local_config)
    return repo->local_config;

  struct strbuf path = STRBUF_INIT;
  strbuf_addf(&path, "%s/%s", repo->repodir, "config");

  repo->local_config = config_read_disk(path.buf);
  strbuf_release(&path);
  return repo->local_config;
}

config *repo_get_global_config(struct repository *repo) {

  if (repo->global_config)
    return repo->global_config;

  struct strbuf path = STRBUF_INIT;
  strbuf_addf(&path, "%s/.eitconfig", getenv("HOME"));

  repo->global_config = config_read_disk(path.buf);
  strbuf_release(&path);
  return repo->global_config;
}

int repo_config_get_bool(struct repository *repo, const char *catergory, const char *key, int default_value) {
  config *c;
  c = repo_get_local_config(repo);
  int res = config_get_bool(c, catergory, key);
  if (res != -1)
    return res;

  c = repo_get_global_config(repo);
  res = config_get_bool(c, catergory, key);

  return res == -1 ? default_value : res;
}

char *repo_config_get_string(struct repository *repo, const char *catergory, const char *key, char *default_value) {
  config *c;
  c = repo_get_local_config(repo);
  char *res = config_get_string(c, catergory, key);
  if (res)
    return res;

  c = repo_get_global_config(repo);
  res = config_get_string(c, catergory, key);
  return res == NULL ? default_value : res;
}

struct path_iterator *repo_path_iterator_create(struct repository *repo) {
  struct repo_path_iterator *rp = xmalloc(1, struct repo_path_iterator);
  rp->repo = repo;
  rp->stage = repo_get_stage(repo);
  rp->fs = (struct fs_iterator *)fs_iterator_create(repo->worktree);
  rp->has_fs = rp->fs->iter.next(&rp->fs->iter, &rp->fs_preload) == 0;
  rp->fs->filter_func = filter_ignored;
  rp->fs->filter_data = repo_get_ignores(repo);
  rp->stage_idx = 0;

  rp->iter.next = repo_path_iterator_next;
  rp->iter.free = repo_path_iterator_free;

  return (struct path_iterator *)rp;
}

int repo_path_iterator_next(struct path_iterator *iter, const char **out_path) {
  struct repo_path_iterator *rp = (struct repo_path_iterator *)iter;
  if (rp->stage_idx < rp->stage->entries_nr) {
    struct strbuf fullpath = STRBUF_INIT;
    char *path = rp->stage->entries[rp->stage_idx++]->path;
    strbuf_addf(&fullpath, "%s/%s", rp->repo->worktree, path);
    *out_path = fullpath.buf;
    return 0;
  }

  while (rp->has_fs && stage_has_path(rp->stage, repo_relative_path(rp->repo, rp->fs_preload))) {
    rp->has_fs = rp->fs->iter.next(&rp->fs->iter, &rp->fs_preload) == 0;
  }

  if (rp->has_fs) {
    *out_path = rp->fs_preload;
    rp->has_fs = rp->fs->iter.next(&rp->fs->iter, &rp->fs_preload) == 0;
    return 0;
  } else {
    return -1;
  }
}

void repo_path_iterator_free(struct path_iterator *iter) {
  struct repo_path_iterator *rp = (struct repo_path_iterator *)iter;
  rp->fs->iter.free(&rp->fs->iter);
  free(rp);
}

void repo_stage_3way_conflict(struct repository *repo, struct stage_entry *base, struct stage_entry *a, struct stage_entry *b, struct string_list *conflicted_merge) {
  FILE *conflict = fopen(base->path, "wb");
  for (size_t i = 0; i < conflicted_merge->nr; i++) {
    fputs(conflicted_merge->values[i], conflict);
  }
  fclose(conflict);

  stage_remove_path(repo_get_stage(repo), base->path);
  base->st.st_mode = base->mode;
  a->st.st_mode = a->mode;
  b->st.st_mode = b->mode;

  stage_add_path(repo_get_stage(repo), base->path, base->st, &base->oid, 1);
  stage_add_path(repo_get_stage(repo), a->path, a->st, &a->oid, 2);
  stage_add_path(repo_get_stage(repo), b->path, b->st, &b->oid, 3);
}

void repo_stage_2way_conflict(struct repository *repo, struct stage_entry *a, struct stage_entry *b, struct string_list *conflicted_merge) {
  FILE *conflict = fopen(a->path, "wb");
  for (size_t i = 0; i < conflicted_merge->nr; i++) {
    fputs(conflicted_merge->values[i], conflict);
  }
  fclose(conflict);

  stage_remove_path(repo_get_stage(repo), a->path);
  a->st.st_mode = a->mode;
  b->st.st_mode = b->mode;

  stage_add_path(repo_get_stage(repo), a->path, a->st, &a->oid, 3);
  stage_add_path(repo_get_stage(repo), b->path, b->st, &b->oid, 4);
}

void repo_stage_mod_del_conflict(struct repository *repo, struct stage_entry *a) {
  repo_pull_blob(repo, a->path, &a->oid);
  stage_remove_path(repo_get_stage(repo), a->path);
  a->st.st_mode = a->mode;

  stage_add_path(repo_get_stage(repo), a->path, a->st, &a->oid, 5);
}
