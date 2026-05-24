#include "command.h"
#include "helper.h"
#include "object.h"
#include "object_store.h"
#include "parse-options.h"
#include "repository.h"
#include "sha256.h"
#include "stage.h"
#include "strbuf.h"
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>

int cmd_ls_files(int argc, char **argv, struct repository *repo) {
  if (!repo->repodir) {
    die("Cannot run ls-files outside of a repository");
  }

  int show_cached = 0;
  int show_del = 0;
  int show_mod = 0;
  int show_others = 0;
  int show_stage = 0;

  struct option opts[] = {
      MKOPT_BOOL_F("cached", 'c', show_cached, "Show currently staged files",
                   OPT_NONEG),

      MKOPT_BOOL_F(
          "deleted", 'd', show_del,
          "Show staged files that have been deleted from the working tree",
          OPT_NONEG),
      MKOPT_BOOL_F(
          "modified", 'm', show_mod,
          "Show staged files that have been modified on the working tree",
          OPT_NONEG),
      MKOPT_BOOL_F("others", 'o', show_others, "Show untracked files",
                   OPT_NONEG),
      MKOPT_BOOL_F("stage", 's', show_stage,
                   "Show more detailed information about the files.",
                   OPT_NONEG),
      MKOPT_END,
  };

  char *usage[] = {
      "Usage: eit ls-files [flag]",
      "       List stage files",
      NULL,
  };

  argc = parse_options(argc, argv, opts, usage);
  struct stage *s = repo_get_stage(repo);

  int opt_count = show_cached + show_del + show_mod + show_others + show_stage;
  if (opt_count == 0) {
    for (size_t i = 0; i < s->entries_nr; i++) {
      printf("%s\n", s->entries[i]->path);
    }
    return 0;
  }
  if (opt_count > 1) {
    fprintf(stderr, "Error: Only one option can be used at a time\n");
    return 1;
  }

  if (show_stage) {
    for (size_t i = 0; i < s->entries_nr; i++) {
      struct stage_entry *ent = s->entries[i];
      printf("%06o", ent->mode);
      struct hex_oid hex;
      printf(" %s", oid_to_hex(&ent->oid, &hex));
      printf(" %d", ent->flags);
      printf(" %s\n", ent->path);
    }
    return 0;
  }

  for (size_t i = 0; i < s->entries_nr; i++) {
    int print = 0;
    if (show_cached == 1)
      print = 1;

    else if (show_mod == 1) {
      struct strbuf fullpath = STRBUF_INIT;
      strbuf_addf(&fullpath, "%s/%s", repo->worktree, s->entries[i]->path);
      struct stat st;
      if (lstat(fullpath.buf, &st) != 0)
        print = 0;
      else {
        if (st.st_mtim.tv_sec > s->entries[i]->st.st_mtim.tv_sec ||
            (st.st_mtim.tv_sec == s->entries[i]->st.st_mtim.tv_sec &&
             st.st_mtim.tv_nsec > s->entries[i]->st.st_mtim.tv_nsec)) {
          struct object_id oid;
          object_store_write_file(repo_get_object_store(repo), OBJ_BLOB,
                                  fullpath.buf, &oid, 0);
          print = !oideq(&s->entries[i]->oid, &oid);
        }
      }

      strbuf_release(&fullpath);
    } else if (show_del == 1) {
      struct strbuf fullpath = STRBUF_INIT;
      strbuf_addf(&fullpath, "%s/%s", repo->worktree, s->entries[i]->path);
      struct stat st;
      if (lstat(fullpath.buf, &st) != 0)
        print = errno == ENOENT;
      strbuf_release(&fullpath);
    }

    if (print)
      printf("%s\n", s->entries[i]->path);
  }

  return 0;
}
