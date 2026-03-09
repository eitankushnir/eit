#ifndef TREE_H
#define TREE_H

#include "repository.h"
#include "sha256.h"
#include <sys/types.h>

typedef struct tree_node {
    const char* name;
    unsigned int mode;
    object_id oid;
    struct tree_node** children;
    int child_count;
    int parsed;
} tree_node;

void init_root(tree_node* node);
void add_leaf(tree_node* root, const char* path, unsigned int mode, object_id* oid);
int is_hashable(tree_node* node);
void write_tree(tree_node* root, repository* repo);
void free_tree(tree_node* root);

#endif
