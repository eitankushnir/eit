#ifndef REPOSITORY_H
#define REPOSITORY_H

#define REPO_DIR_NAME ".eit"

struct repository {
  char *repodir;  // Absolute path to the .eit directory.
  char *worktree; // Absolute path to the root of the project.
};

void repository_init(struct repository *repo);
void repository_release(struct repository *repo);

char *repo_find_repo_dir(void);
char *repo_find_repo_worktree(void);

#endif
