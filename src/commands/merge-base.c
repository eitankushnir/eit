#include "command.h"
#include "commit.h"
#include "object_pool.h"
#include "ref_store.h"
#include "repository.h"
#include "sha256.h"
#include <stdio.h>
int cmd_merge_base(int argc, char **argv, struct repository *repo) {

  struct ref_store *refs = repo_get_ref_store(repo);
  struct object_id o1, o2;

  ref_store_read_branch(refs, argv[1], &o1);
  ref_store_read_branch(refs, argv[2], &o2);

  struct commit *c1 = object_pool_lookup_commit(repo_get_pool(repo), &o1);
  struct commit *c2 = object_pool_lookup_commit(repo_get_pool(repo), &o2);
  repo_parse_commit(repo, c1);
  repo_parse_commit(repo, c2);
  struct commit_list *bases = commit_merge_bases(c1, c2);

  struct hex_oid hex;
  printf("BASE: %s\n", oid_to_hex(&bases->item->obj.oid, &hex));
  commit_list_free(bases);
  return 0;
}
