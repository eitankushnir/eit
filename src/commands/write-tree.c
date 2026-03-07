#include "commands.h"
#include "stage.h"
#include "tree.h"
#include <stdio.h>

int cmd_write_tree(char **argv, int argc, repository *repo) {

    if (!stage_can_be_written(repo)) {
        fprintf(stderr, "Error: The stage is not in a fully merged state. Cannot write a tree");
        return 1;
    }

    tree_node tree;
    construct_stage_tree(&tree, repo);
    write_tree(&tree, repo);
    free_tree(&tree);
    return 0;
}
