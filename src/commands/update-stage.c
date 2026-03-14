#include "blob.h"
#include "commands.h"
#include "repository.h"
#include "sha256.h"
#include "stage.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

struct options {
    int add;
    int remove;
    int force_remove;
};

int cmd_update_stage(char** argv, int argc, repository* repo)
{
    struct option opts[] = {
        { "add", no_argument, 0, 'a' },
        { "remove", no_argument, 0, 'r' },
        { "force-remove", no_argument, 0, 'x' },
        { 0, 0, 0, 0 }
    };

    int option_val = 0;
    int opindex = 0;
    extern int optind;
    struct options cmd_opts = { 0 };

    while ((option_val = getopt_long(argc, argv, "arx", opts, &opindex)) != -1) {
        switch (option_val) {
        case 'a':
            cmd_opts.add = 1;
            break;
        case 'r':
            cmd_opts.remove = 1;
            break;
        case 'x':
            cmd_opts.force_remove = 1;
            break;
        default:
            return 1;
        }
    }

    int first_file_index = optind;
    for (int i = first_file_index; i < argc; i++) {
        char* path = argv[i];
        char* resolved_path = path_in_repo(path, repo);
        struct stat st;
        if (stat(path, &st) != 0) {
            if (!cmd_opts.remove) {
                fprintf(stderr, "Warning: failed to stat %s -- ignoring. If meant to be removed, run with --remove.\n", path);
            }
            else {
                printf("%s was found on stage but is missing -- removing\n", path);
                remove_from_stage(repo->stage, resolved_path);
            }
            free(resolved_path);
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            fprintf(stderr, "Warning: %s is a directory -- ignoring\n", path);
            free(resolved_path);
            continue;
        }

        int idx = index_on_stage(repo->stage, resolved_path);
        if (idx == -1 && !cmd_opts.add) {
            fprintf(stderr, "Warning: %s is not on the stage. to add it run with --add\n", path);
            free(resolved_path);
            continue;
        }
        if (cmd_opts.force_remove) {
            remove_from_stage(repo->stage, resolved_path);
            printf("%s was found on stage -- removing\n", path);
            free(resolved_path);
            continue;
        }

        if (cmd_opts.remove) {
            fprintf(stderr, "Warning: %s is not missing, to remove it from the stage regardless run with --force-remove.\n", path);
        }

        FILE* src = fopen(path, "r");
        if (!src) {
            fprintf(stderr, "Warning: Failed to open %s for hashing -- skipping\n", path);
        } else {
            object_id oid;
            hash_blob_from_file(src, &oid, 1, repo);
            add_to_stage(repo->stage, resolved_path, oid);
        }

        free(resolved_path);
    }

    write_stage(repo);
    return 0;
}
