#ifndef REPOSITORY_H
#define REPOSITORY_H

#define REPO_DIR ".eit"

typedef struct {
    char* repo_dir; // path to the .eit folder.
} repository;

// Find the root of the current repository and return the absolute path to it.
// If no repository is initialized (i.e no .eit folder is present) then NULL is returned.
// Result is malloc'd, make sure to free it when done.

void init_current_repository_info(repository* repo);
char* find_repository_root(void);

#endif
