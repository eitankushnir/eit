#include "tree.h"
#include "object.h"
#include "repository.h"
#include "sha256.h"
#include "strbuf.h"
#include "wrappers.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
void init_root(tree_node* node)
{
    node->name = NULL;
    node->children = xmalloc(1, tree_node);
    node->mode = 0;
    node->child_count = 0;
    node->parsed = 0;
}

static void increase_array(tree_node* node)
{
    node->children = realloc(node->children, ++node->child_count);
}

static char* cpy(const char* src)
{
    char* copy = xmalloc(strlen(src) + 1, char);
    strncpy(copy, src, strlen(src));
    copy[strlen(copy)] = '\0';
    return copy;
}

void add_leaf(tree_node* root, const char* path, unsigned int mode, object_id* oid)
{
    char* slashptr = strchr(path, '/');
    if (!slashptr) {
        increase_array(root);
        tree_node* new_node = xmalloc(1, tree_node);
        new_node->name = cpy(path);
        oidcpy(&new_node->oid, oid);
        new_node->mode = mode;
        new_node->parsed = 1;
        root->children[root->child_count - 1] = new_node;
        return;
    }

    int index = slashptr - path;
    char* dirname = substr(path, index);

    int found_child = 0;
    for (int i = 0; i < root->child_count; i++) {
        if (strcmp(dirname, root->children[i]->name) == 0) {
            // Go into the directory.
            add_leaf(root->children[i], path + index + 1, mode, oid);
            found_child = 1;
        }
    }

    // Create a child for the directory.
    if (!found_child) {
        increase_array(root);
        tree_node* new_node = xmalloc(1, tree_node);
        new_node->name = substr(dirname, -1);
        new_node->mode = 0040000;
        new_node->parsed = 0;
        root->children[root->child_count - 1] = new_node;

        // Go into the newly created directory.
        add_leaf(root->children[root->child_count - 1], path + index + 1, mode, oid);
    }
}

int is_hashable(tree_node* node)
{
    for (int i = 0; i < node->child_count; i++) {
        tree_node* child = node->children[i];
        if (!child->parsed)
            return 0;
    }

    return 1;
}

static void write_node(tree_node* node, repository* repo)
{
    // Assume hashability from write_tree.
    FILE* buffer = tmpfile();

    // Get the size of the list.
    for (int i = 0; i < node->child_count; i++) {
        tree_node* child = node->children[i];

        fprintf(buffer, "%06o", child->mode);
        fputc(' ', buffer);
        fwrite(child->oid.hash, sizeof(uint8_t), 32, buffer);
        fputc(' ', buffer);
        fprintf(buffer, "%s", child->name);
        fputc('\0', buffer);
    }
    // Hash and write object.
    write_object(OBJ_TREE, buffer, repo, &node->oid);
}

void write_tree(tree_node* root, repository* repo)
{
    if (is_hashable(root)) {
        write_node(root, repo);
        return;
    }

    for (int i = 0; i < root->child_count; i++) {
        tree_node* child = root->children[i];
        if (child->child_count != 0)
            write_tree(child, repo);
        child->parsed = 1;
    }

    if (!is_hashable(root))
        die("Something went wrong when writing the tree");
    write_node(root, repo);
}

void free_tree(tree_node* root)
{
    if (root->name)
        free(root->name);

    for (int i = 0; i < root->child_count; i++) {
        free_tree(root->children[i]);
    }

    free(root->children);
}

void print_tree(tree_node* node, repository* repo)
{
    if (!is_hashable(node))
        die("Tree canno't be printed");

    for (int i = 0; i < node->child_count; i++) {
        tree_node* child = node->children[i];
        char* hex = oid_tostring(&child->oid);
        char* type = type_name(get_type(hex, repo));

        printf("%06o %s %s    %s\n", child->mode, type, hex, child->name);
        free(hex);
    }
}

void parse_tree(const char* hex, tree_node* out_node, repository* repo)
{
    object_type type = get_type(hex, repo);
    if (type != OBJ_TREE)
        die("%s is not a valid tree object\n", hex);

    FILE* objfile = open_object(hex, repo);
    if (!objfile)
        die("Failed to read object %s\n", hex);

    init_root(out_node);

    // Get past the header.
    while (fgetc(objfile) != '\0') { }

    while (1) {
        tree_node* child = xmalloc(1, tree_node);
        int res = fscanf(objfile, "%o", &child->mode);
        if (res != 1) {
            free(child);
            break;
        }
        char a = fgetc(objfile);
        fread(child->oid.hash, sizeof(uint8_t), 32, objfile);
        char b = fgetc(objfile); // the space;
        strbuf name = STRBUF_INIT;
        char c;
        while ((c = fgetc(objfile)) != '\0') {
            strbuf_addchr(&name, c);
        }
        child->name = strbuf_detach(&name, 0);
        child->parsed = 1;
        out_node->child_count++;
        out_node->children[out_node->child_count - 1] = child;
    }
}

void parse_tree_recursive(const char* hex, tree_node* out_node, repository* repo)
{
    parse_tree(hex, out_node, repo);
    for (int i = 0; i < out_node->child_count; i++) {
        tree_node* child = out_node->children[i];
        char* child_hex = oid_tostring(&child->oid);

        object_type type = get_type(child_hex, repo);
        if (type == OBJ_TREE) {
            tree_node node;
            parse_tree_recursive(child_hex, &node, repo);
            child->children = node.children;
            child->child_count = node.child_count;
        }

        free(child_hex);
    }
}
