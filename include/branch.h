#ifndef BRANCH_H
#define BRANCH_H

#include "repository.h"
#include "sha256.h"

typedef struct branch {
    char* name;
    object_id commit_id;
} branch;

void find_branch(const char* name, branch* out, repository* repo);
void write_branch(branch* b, repository* repo);
void free_branch(branch* b);

#endif
