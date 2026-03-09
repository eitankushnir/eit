#include "commands.h"
#include "sha256.h"
#include "stage.h"
#include "tree.h"
#include <stdio.h>
#include <stdlib.h>

int cmd_write_tree(char **argv, int argc, repository *repo) {

    if (!stage_can_be_written(repo->stage)) {
        fprintf(stderr, "Error: The stage is not in a fully merged state. Cannot write a tree");
        return 1;
    }

    tree_node tree;
    construct_stage_tree(&tree, repo->stage);
    write_tree(&tree, repo);
    char* hex_str = oid_tostring(&tree.oid);
    printf("%s\n", hex_str);
    free_tree(&tree);
    free(hex_str);
    return 0;
}
