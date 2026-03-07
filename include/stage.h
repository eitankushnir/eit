#ifndef STAGE_H
#define STAGE_H

#include "repository.h"
#include "tree.h"

#define STAGE_DIR "stage"

/** 
 * Add a file to the stage, creating a blob for it in the process 
 * If the file is already on the stage, that hash will be replaced.
 */
void add_to_stage(const char* path, repository* repo);

/** Remove a file from the stage */
void remove_from_stage(const char* path, repository* repo);

/** Get the index of the file on the stage */
int index_on_stage(const char* path, repository* repo);

/** Find out if a file is already on the stage (might be a different version) */
int is_on_stage(const char* path, repository* repo);

/**
 * Returns 1 if all files in the stage are not in a conflicted state
 */
int stage_can_be_written(repository* repo);

void construct_stage_tree(tree_node* out_root, repository* repo);

#endif
