#include "blob.h"
#include "commands.h"
#include "object.h"
#include "repository.h"
#include "sha256.h"
#include "strbuf.h"
#include "wrappers.h"
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

struct options {
    int use_stdin;
    int write;
    object_type type;
};

int cmd_hash_object(char** argv, int argc, repository* repo)
{
    struct option opts[] = {
        { "stdin", no_argument, 0, 's' },
        { 0, 0, 0, 0 }
    };

    int option_val = 0;
    int opindex = 0;
    struct options cmd_opts = { 0 };
    cmd_opts.type = OBJ_BLOB; //set default.

    while ((option_val = getopt_long(argc, argv, "sw", opts, &opindex)) != -1) {

        switch (option_val) {
        case 's':
            cmd_opts.use_stdin = 1;
            break;
        case 'w':
            cmd_opts.write = 1;
            break;
        default:
            return 1;
        }
    }

    object_id oid;
    if (cmd_opts.use_stdin) {
        hash_blob_from_stdin(&oid, cmd_opts.write, repo);
    } else {
        const char* path = argv[argc - 1];
        if (path[0] == '-') die("Error: Not enough arguments");

        FILE* f = fopen(argv[argc - 1], "rb");
        if (!f) die("Fatal: failed to open file at: %s", argv[argc - 1]);

        hash_blob_from_file(f, &oid, cmd_opts.write, repo);
        fclose(f);
    }

    println_oid(&oid);

    return 0;
}
