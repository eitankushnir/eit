#include "tree.h"
#include "object.h"
#include "sha256.h"
#include "strbuf.h"
#include "wrappers.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
void init_root(tree_node *node) {
    node->name = NULL;
    node->children = xmalloc(1, tree_node);
    node->hex_oid = NULL;
    node->mode = 0;
    node->child_count = 0;
}

static void increase_array(tree_node* node) {
    node->children = realloc(node->children, ++node->child_count);
}

static char* cpy(const char* src) {
    char* copy = xmalloc(strlen(src) + 1, char);
    strncpy(copy, src, strlen(src));
    copy[strlen(copy)] = '\0';
    return copy;
}

void add_leaf(tree_node *root, const char *path, mode_t mode, const char *hex_oid) {
    char* slashptr = strchr(path, '/');
    if (!slashptr) {
        increase_array(root);
        tree_node* new_node = xmalloc(1, tree_node);
        new_node->name = cpy(path);
        new_node->hex_oid = cpy(hex_oid);
        new_node->mode = mode;
        root->children[root->child_count - 1] = new_node;
        return;
    }

    int index = slashptr - path;
    char* dirname = xmalloc(index + 1, char);
    strncpy(dirname, path, index);
    dirname[index] = '\0';

    int found_child = 0;
    for (int i = 0; i < root->child_count; i++) {
        if (strcmp(dirname, root->children[i]->name) == 0) {
            add_leaf(root->children[i], path + index + 1, mode, hex_oid);
            found_child = 1;
        }
    }

    if (!found_child) {
        increase_array(root);
        tree_node* new_node = xmalloc(1, tree_node);
        new_node->name = cpy(dirname);
        new_node->hex_oid = NULL;
        struct stat st;
        stat(dirname, &st);
        new_node->mode = st.st_mode;
        root->children[root->child_count - 1] = new_node;
        add_leaf(root->children[root->child_count - 1], path + index + 1, mode, hex_oid);
    }
}

int is_hashable(tree_node *node) {
    for (int i = 0; i < node->child_count; i++) {
        tree_node* child = node->children[i];
        if (!child->hex_oid || !child->mode || !child->name) return 0;
    }

    return 1;
}

static void write_node(tree_node *node, repository *repo) {
    // Assume hashability from write_tree.
    FILE* buffer = tmpfile();

    // Get the size of the list.
    for (int i = 0; i < node->child_count; i++) {
        tree_node* child = node->children[i];
        object_type type;
        if (child->child_count != 0) type = OBJ_TREE;
        else type = OBJ_BLOB;

        fprintf(buffer, "%06o %s %s %s\n", child->mode, type_name(type), child->hex_oid, child->name);
    }
    // Hash and write object.
    object_id oid;

    write_object(OBJ_TREE, buffer, repo, &oid);
    node->hex_oid = oid_tostring(&oid);
}

void write_tree(tree_node *root, repository *repo) {
    if (is_hashable(root)) {
        write_node(root, repo);
        return;
    }

    for (int i = 0; i < root->child_count; i++) {
        tree_node* child = root->children[i];
        if (child->child_count != 0) write_tree(child, repo);
    }

    if (!is_hashable(root)) die("Something went wrong when writing the tree");
    write_node(root, repo);
}

void free_tree(tree_node *root) {
    if (root->name) free(root->name);
    if (root->hex_oid) free(root->hex_oid);

    for (int i = 0; i < root->child_count; i++) {
        free_tree(root->children[i]);
    }

    free(root->children);
}
