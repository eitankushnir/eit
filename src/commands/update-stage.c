#include "commands.h"
#include "repository.h"
#include "stage.h"
#include <getopt.h>
#include <stdio.h>
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
        struct stat st;
        if (stat(path, &st) != 0) {
            if (!cmd_opts.remove) {
                fprintf(stderr, "Warning: failed to stat %s -- ignoring. If meant to be removed, run with --remove.\n", path);
            }
            else {
                printf("%s was found on stage but is missing -- removing\n", path);
                remove_from_stage(path, repo);
            }
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            fprintf(stderr, "Warning: %s is a directory -- ignoring\n", path);
            continue;
        }

        int idx = index_on_stage(path, repo);
        if (idx == -1 && cmd_opts.add) {
            add_to_stage(path, repo);
            continue;
        }
        if (idx == -1 && !cmd_opts.add) {
            fprintf(stderr, "Warning: %s is not on the stage. to add it run with --add\n", path);
            continue;
        }
        if (cmd_opts.force_remove) {
            remove_from_stage(path, repo);
            printf("%s was found on stage -- removing\n", path);
            continue;
        }

        if (cmd_opts.remove) {
            fprintf(stderr, "Warning: %s is not missing, to remove it from the stage regardless run with --force-remove.\n", path);
        }
        add_to_stage(path, repo);
    }

    write_stage(repo);
    return 0;
}
