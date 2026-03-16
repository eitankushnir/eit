#ifndef REPOSITORY_H
#define REPOSITORY_H

#define REPO_DIR ".eit"

typedef struct repository {
    const char* repo_root; // path to the parent of .eit folder.
    const char* repo_dir; // path to the .eit folder.

    struct stage* stage;
    struct head* head;
} repository;

// Find the root of the current repository and return the absolute path to it.
// If no repository is initialized (i.e no .eit folder is present) then NULL is returned.
// Result is malloc'd, make sure to free it when done.

void init_current_repository_info(repository* repo);
char* find_repository_root(void);

// Returns NULL is the path is invalid.
// Retuns a malloc'd path that begins in the repo.
char* path_in_repo(const char* path, repository* repo);

void swap_stage(repository* repo, struct stage* new_stage);

int mkpath(repository* repo, const char* path);
int rmpath(repository* repo, const char* path);

#endif
