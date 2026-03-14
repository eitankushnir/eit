#ifndef STAGE_H
#define STAGE_H


#include "repository.h"
#include "sha256.h"
#include "tree.h"
#include <sys/stat.h>
#include <sys/types.h>
#define STAGE_DIR "stage"

typedef struct stage_entry {
    struct stat stat_data; // os-data
    unsigned int mode; // eit's own mode.
    unsigned int flags; // merge statuses and other stuff.

    object_id oid; // oid of the entry.
    char* path; // relative repo path of the entry.
    int path_len;
} stage_entry;

typedef struct stage {
    stage_entry** entries;
    int entry_count;
} stage;

int load_stage(struct repository* repo);
int write_stage(struct repository* repo);
/**
 * Add a path to the stage. provide the oid for it aswell.
 * If the file is already on the stage, that oid will be replaced.
 */
void add_to_stage(stage* stage, const char* path, object_id oid);

/** Remove a file from the stage */
void remove_from_stage(stage* stage, const char* path);

/** Get the index of the file on the stage */
int index_on_stage(stage* stage, const char* path);

/** Find out if a file is already on the stage (might be a different version) */
int is_on_stage(stage* stage, const char* path);

/**
 * Returns 1 if all files in the stage are not in a conflicted state
 */
int stage_can_be_written(stage* stage);

void construct_stage_tree(struct tree_node* out_root, stage* stage);

void get_modified_entries(stage* out, repository* repo);
void get_deleted_entries(stage* out, repository* repo);

#endif
