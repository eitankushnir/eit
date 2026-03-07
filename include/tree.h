#ifndef TREE_H
#define TREE_H

#include "repository.h"
#include <sys/types.h>

typedef struct tree_node {
    const char* name;
    mode_t mode;
    const char* hex_oid; // only there for leaves, since they are blobs which already have an id.
    struct tree_node** children;
    int child_count;
} tree_node;

void init_root(tree_node* node);
void add_leaf(tree_node* root, const char* path, mode_t mode, const char* hex_oid);
int is_hashable(tree_node* node);
void write_tree(tree_node* root, repository* repo);
void free_tree(tree_node* root);

#endif
