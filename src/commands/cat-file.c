#include "blob.h"
#include "commands.h"
#include "commit.h"
#include "object.h"
#include "sha256.h"
#include "strbuf.h"
#include "tree.h"
#include "wrappers.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct options {
    // Mutually exclusive modes.
    int type_mode;
    int size_mode;

    int pretty_print;
};

int cmd_cat_file(char** argv, int argc, repository* repo)
{
    struct option opts[] = {
        { 0, 0, 0, 0 }
    };

    int option_val = 0;
    int opindex = 0;
    extern int optind;
    struct options cmd_opts = { 0 };

    while ((option_val = getopt_long(argc, argv, "pst", opts, &opindex)) != -1) {
        switch (option_val) {
        case 'p':
            cmd_opts.pretty_print = 1;
            break;
        case 's':
            cmd_opts.size_mode = 1;
            break;
        case 't':
            cmd_opts.type_mode = 1;
            break;
        default:
            return 1;
        }
    }
    argc = non_opt_count(argv, argc, optind);
    if (argc < 1) {
        fprintf(stderr, "Error: Not enough arguments\n");
        return 1;
    }
    char* hex_arg;
    oid_hex hex;
    char* type = NULL;
    if (!cmd_opts.type_mode && !cmd_opts.pretty_print && !cmd_opts.size_mode) {
        if (argc != 2) {
            fprintf(stderr, "Error: Not enough arguments\n");
            return 1;
        }
        type = argv[optind];
        type_from_name(type);
        hex_arg = argv[optind];
    } else {
        hex_arg = argv[optind];
    }

    if (strlen(hex_arg) > 64) {
        die("Error: a SHA-256 hash cannot extends 64 characters.\n");
    } else if (strlen(hex_arg) <= 64) {
        hex = complete_hash_hex(hex_arg, repo);
    }

    if (cmd_opts.size_mode && cmd_opts.type_mode) {
        fprintf(stderr, "Error: -t and -s cannot be used together\n");
        return 1;
    }

    FILE* objfile = open_object(&hex, repo);
    if (!objfile)
        die("Failed to read object with hash: %s\n", hex.hex);
    strbuf header = STRBUF_INIT;
    char c;
    while ((c = fgetc(objfile)) != '\0') {
        strbuf_addchr(&header, c);
    }
    if (cmd_opts.size_mode) {
        char* space_ptr = strchr(header.buf, ' ');
        char* size = substr(space_ptr + 1, -1);
        printf("Size: %s bytes\n", size);
        free(size);
    }
    if (cmd_opts.type_mode) {
        int space_idx = strchr(header.buf, ' ') - header.buf;
        char* type_str = substr(header.buf, space_idx);
        printf("Type: %s\n", type_str);
        free(type_str);
    }
    if (cmd_opts.pretty_print) {
        object_type type = get_type(&hex, repo);
        tree_node node;
        commit c;
        switch (type) {
        case OBJ_TREE:
            parse_tree(&hex, &node, repo);
            print_tree(&node, repo);
            free_tree(&node);
            break;
        case OBJ_BLOB:
            print_blob(&hex, repo);
            break;
        case OBJ_COMMIT:
            parse_commit(&hex, &c, repo);
            print_commit(&c);
            free_commit(&c);
            break;
        default:
            die("%s is not a valid object hash\n", hex.hex);
        }
    }
    strbuf_free(&header);
    return 0;
}
