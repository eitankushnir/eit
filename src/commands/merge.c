#include "command.h"
#include "commit.h"
#include "diff.h"
#include "helper.h"
#include "object.h"
#include "object_pool.h"
#include "object_store.h"
#include "parse-options.h"
#include "ref_store.h"
#include "repository.h"
#include "sha256.h"
#include "stage.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void stage_add(struct stage *s, struct stage_entry *ent) {
  s->entries = xrealloc(s->entries, ++s->entries_nr, struct stage_entry *);
  s->entries[s->entries_nr - 1] = ent;
}

static void stage_partial_free(struct stage *s) {
  free(s->entries);
  free(s);
}

static void get_mods_dels_adds(struct stage *base, struct stage *new, struct stage *out_mods, struct stage *out_dels, struct stage *out_adds) {
  size_t i = 0;
  size_t j = 0;

  while (i < base->entries_nr && j < new->entries_nr) {
    struct stage_entry *base_ent = base->entries[i];
    struct stage_entry *new_ent = new->entries[j];

    int cmp = strcmp(base_ent->path, new_ent->path);
    if (cmp == 0) {
      if (!oideq(&base_ent->oid, &new_ent->oid)) {
        stage_add(out_mods, new_ent);
      }
      i++;
      j++;
    } else if (cmp < 0) {
      stage_add(out_dels, base_ent);
      i++;
    } else {
      stage_add(out_adds, new_ent);
      j++;
    }
  }

  while (i < base->entries_nr) {
    stage_add(out_dels, base->entries[i]);
    i++;
  }

  while (j < new->entries_nr) {
    stage_add(out_adds, new->entries[j]);
    j++;
  }
}

int cmd_merge(int argc, char **argv, struct repository *repo) {

  if (!repo->repodir)
    die("Cannot run merge outside of a repository");

  char *usage[] = {
      "Usage: eit merge <branch>",
      NULL,
  };

  argc = parse_options(argc, argv, NULL, usage);
  if (argc < 2)
    die("Error: missing merge cadidate branch");
  struct ref_store *refs = repo_get_ref_store(repo);
  ref_store_parse_head(refs);

  char *merge_condidate = argv[1];
  if (!ref_store_has_branch(refs, merge_condidate))
    die("Error: No branch named %s", merge_condidate);

  if (refs->head_mode == HEAD_NORMAL && strcmp(refs->head_branch, merge_condidate) == 0)
    die("Error: Cannot merge current branch with itself");

  struct object_id merge_commit_id;
  ref_store_read_branch(refs, merge_condidate, &merge_commit_id);

  struct commit *c_head, *c_merge;
  c_head = object_pool_lookup_commit(repo_get_pool(repo), &refs->head_id);
  c_merge = object_pool_lookup_commit(repo_get_pool(repo), &merge_commit_id);
  repo_parse_commit(repo, c_head);
  repo_parse_commit(repo, c_merge);

  struct commit_list *bases = commit_merge_bases(c_head, c_merge);
  if (!bases)
    die("Error: No merge base found");

  struct commit *merge_base = bases->item;

  struct stage *our_stage = repo_construct_stage(repo, c_head->tree);
  struct stage *their_stage = repo_construct_stage(repo, c_merge->tree);
  struct stage *base_stage = repo_construct_stage(repo, merge_base->tree);

  struct stage *our_dels = xcalloc(1, struct stage);
  struct stage *our_mods = xcalloc(1, struct stage);
  struct stage *our_adds = xcalloc(1, struct stage);
  struct stage *their_dels = xcalloc(1, struct stage);
  struct stage *their_mods = xcalloc(1, struct stage);
  struct stage *their_adds = xcalloc(1, struct stage);

  get_mods_dels_adds(base_stage, our_stage, our_mods, our_dels, our_adds);
  get_mods_dels_adds(base_stage, their_stage, their_mods, their_dels, their_adds);

  struct object_store *objs = repo_get_object_store(repo);
  // CHECK MOD-DEL conflicts
  size_t i = 0;
  size_t j = 0;
  while (i < our_dels->entries_nr && j < their_mods->entries_nr) {
    struct stage_entry *a_ent = our_dels->entries[i];
    struct stage_entry *b_ent = their_mods->entries[j];
    int cmp = strcmp(a_ent->path, b_ent->path);
    if (cmp == 0) {
      repo_stage_mod_del_conflict(repo, b_ent);
      printf("Conflict: %s was deleted in current branch but modified in %s. Keep using eit add, remove using eit rm.\n", b_ent->path, merge_condidate);
      i++;
      j++;
    } else if (cmp < 0) {
      i++;
    } else {
      j++;
    }
  }

  i = 0;
  j = 0;
  while (i < their_dels->entries_nr && j < our_mods->entries_nr) {
    struct stage_entry *a_ent = their_dels->entries[i];
    struct stage_entry *b_ent = our_mods->entries[j];
    int cmp = strcmp(a_ent->path, b_ent->path);
    if (cmp == 0) {
      repo_stage_mod_del_conflict(repo, b_ent);
      printf("Conflict: %s was deleted in current branch but modified in %s. Keep using eit add, remove using eit rm.\n", b_ent->path, merge_condidate);
      i++;
      j++;
    } else if (cmp < 0) {
      i++;
    } else {
      j++;
    }
  }

  // MOD-MOD conflicts
  i = 0;
  j = 0;
  size_t k = 0;
  while (i < our_mods->entries_nr && j < their_mods->entries_nr) {
    struct stage_entry *a_ent = our_mods->entries[i];
    struct stage_entry *b_ent = their_mods->entries[j];
    int cmp = strcmp(a_ent->path, b_ent->path);
    if (cmp == 0) {
      struct stage_entry *base_ent = base_stage->entries[k];
      while (k < base_stage->entries_nr && strcmp(a_ent->path, base_ent->path) != 0) {
        k++;
      }
      // die("CONFLICT %s was modified in %s and current branch", a_ent->path, merge_condidate);
      size_t a_size, b_size, base_size;
      enum object_type t;
      void *a_raw = object_store_read_raw(objs, &a_ent->oid, &a_size, &t);
      void *b_raw = object_store_read_raw(objs, &b_ent->oid, &b_size, &t);
      void *base_raw = object_store_read_raw(objs, &base_ent->oid, &base_size, &t);

      struct string_list *a = raw_to_lines(a_raw, a_size);
      struct string_list *b = raw_to_lines(b_raw, b_size);
      struct string_list *base = raw_to_lines(base_raw, base_size);

      bool is_conflict;
      struct string_list *merge = merge_diff_3_way(base, a, b, "HEAD", merge_condidate, &is_conflict);
      if (is_conflict) {
        printf("Conflict found while merging file: %s\n. Conflict markers inserted. Edit to your liking and re-stage with eit add.", base_ent->path);
        repo_stage_3way_conflict(repo, base_ent, a_ent, b_ent, merge);
      }

      free(a_raw);
      free(b_raw);
      free(base_raw);
      string_list_free(a);
      string_list_free(b);
      string_list_free(base);
      string_list_free(merge);

      i++;
      j++;
    } else if (cmp < 0) {
      i++;
    } else {
      j++;
    }
  }

  // ADD-ADD conflicts
  i = 0;
  j = 0;
  while (i < our_adds->entries_nr && j < their_adds->entries_nr) {
    struct stage_entry *a_ent = our_adds->entries[i];
    struct stage_entry *b_ent = their_adds->entries[j];
    int cmp = strcmp(a_ent->path, b_ent->path);
    if (cmp == 0) {
      size_t a_size, b_size;
      enum object_type t;
      void *a_raw = object_store_read_raw(objs, &a_ent->oid, &a_size, &t);
      void *b_raw = object_store_read_raw(objs, &b_ent->oid, &b_size, &t);

      struct string_list *a = raw_to_lines(a_raw, a_size);
      struct string_list *b = raw_to_lines(b_raw, b_size);

      bool conflict;
      struct string_list *merge = merge_diff_2_way(a, b, "HEAD", merge_condidate, &conflict);
      if (conflict) {
        printf("Conflict found while merging file: %s\n. Conflict markers inserted. Edit to your liking and re-stage with eit add.", a_ent->path);
        repo_stage_2way_conflict(repo, a_ent, b_ent, merge);
      }
      i++;
      j++;
    } else if (cmp < 0) {
      i++;
    } else {
      j++;
    }
  }

  commit_list_free(bases);
  stage_free(our_stage);
  stage_free(their_stage);
  stage_free(base_stage);

  stage_partial_free(our_mods);
  stage_partial_free(our_adds);
  stage_partial_free(our_dels);

  stage_partial_free(their_mods);
  stage_partial_free(their_adds);
  stage_partial_free(their_dels);
  return 0;
}
