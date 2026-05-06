#include "command.h"
#include "helper.h"
#include "parse-options.h"
#include "repository.h"
#include "strbuf.h"
#include <stdio.h>
#include <stdlib.h>

static void remake_dir(struct strbuf *sb, char *d) {
  strbuf_release(sb);
  strbuf_init(sb);
  strbuf_addf(sb, "%s/%s", REPO_DIR_NAME, d);
}

int cmd_init(int argc, char **argv, struct repository *repo) {

  char *usage[] = {
      "usage: eit init",
      "       Initialize an empty repository at the current directory", NULL};

  argc = parse_options(argc, argv, NULL, usage);

  if (repo->repodir) {
    fprintf(stderr, "Repository already initilized at: %s\n", repo->repodir);
    return 1;
  }

  create_directory(REPO_DIR_NAME);
  struct strbuf dir = STRBUF_INIT;
  remake_dir(&dir, "objects");
  create_directory(dir.buf);
  remake_dir(&dir, "refs");
  create_directory(dir.buf);
  remake_dir(&dir, "refs/heads");
  create_directory(dir.buf);
  remake_dir(&dir, "refs/tags");
  create_directory(dir.buf);

  char *path = normalize_path(REPO_DIR_NAME);
  printf("Initialized empty repository at: %s\n", path);
  free(path);

  return 0;
}
