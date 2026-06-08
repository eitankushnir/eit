#include "command.h"
#include "commit.h"
#include "helper.h"
#include "object_pool.h"
#include "parse-options.h"
#include "ref_store.h"
#include "repository.h"
#include "sha256.h"
#include "stage.h"
#include <stddef.h>
#include <string.h>

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
  commit_list_free(bases);

  struct stage *our_stage = repo_get_stage(repo);
  struct stage *their_stage = repo_construct_stage(repo, c_merge->tree);
  struct stage *base_stage = repo_construct_stage(repo, merge_base->tree);

  struct stage *our_dels = xcalloc(1, struct stage);
  struct stage *our_mods = xcalloc(1, struct stage);
  size_t i = 0;
  size_t j = 0;
  while (i < base_stage->entries_nr && j < our_stage->entries_nr) {
    struct stage_entry *a_ent = base_stage->entries[i];
    struct stage_entry *b_ent = our_stage->entries[j];
    int cmp = strcmp(a_ent->path, b_ent->path);
    if (cmp == 0) {
      if (!oideq(&a_ent->oid, &b_ent->oid)) {
        our_mods->entries = xrealloc(our_mods->entries, ++our_mods->entries_nr, struct stage_entry);
        our_mods->entries[our_mods->entries_nr - 1] = b_ent;
      }
      i++;
      j++;
    } else if (cmp < 0) {
      our_dels->entries = xrealloc(our_dels->entries, ++our_dels->entries_nr, struct stage_entry);
      our_dels->entries[our_dels->entries_nr - 1] = b_ent;
      i++;
    } else {
      j++;
    }
  }

  struct stage *their_dels = xcalloc(1, struct stage);
  struct stage *their_mods = xcalloc(1, struct stage);
  i = 0;
  j = 0;
  while (i < base_stage->entries_nr && j < their_stage->entries_nr) {
    struct stage_entry *a_ent = base_stage->entries[i];
    struct stage_entry *b_ent = their_stage->entries[j];
    int cmp = strcmp(a_ent->path, b_ent->path);
    if (cmp == 0) {
      if (!oideq(&a_ent->oid, &b_ent->oid)) {
        their_mods->entries = xrealloc(their_mods->entries, ++their_mods->entries_nr, struct stage_entry);
        their_mods->entries[their_mods->entries_nr - 1] = b_ent;
      }
      i++;
      j++;
    } else if (cmp < 0) {
      their_dels->entries = xrealloc(their_dels->entries, ++their_dels->entries_nr, struct stage_entry);
      their_dels->entries[their_dels->entries_nr - 1] = b_ent;
      i++;
    } else {
      j++;
    }
  }

  // CHECK MOD-DEL conflicts
  i = 0;
  j = 0;
  while (i < our_dels->entries_nr && j < their_stage->entries_nr) {
    struct stage_entry *a_ent = our_dels->entries[i];
    struct stage_entry *b_ent = their_mods->entries[j];
    int cmp = strcmp(a_ent->path, b_ent->path);
    if (cmp == 0) {
      die("CONFLICT %s was deleted in current branch but modified in %s", a_ent->path, merge_condidate);
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
      die("CONFLICT %s was deleted in %s but modified in current branch", a_ent->path, merge_condidate);
    } else if (cmp < 0) {
      i++;
    } else {
      j++;
    }
  }

  return 0;
}
