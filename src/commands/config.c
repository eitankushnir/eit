#include "commands.h"
#include "repository.h"
#include "config.h"
#include <getopt.h>
#include <stdio.h>
#include <string.h>

int cmd_config(char **argv, int argc, repository* repo) {

    struct option opts[] = {
        { "scope", required_argument, 0, 's' },
        {0, 0, 0, 0},
    };

    int option_val = 0;
    int opindex = 0;
    enum scope scope = LOCAL;

    // Get the scope from the args
    while (( option_val = getopt_long(argc, argv, "s", opts, &opindex)) != -1) {
        switch (option_val) {
            case 's':
                if (strcmp(optarg, "global") == 0) scope = GLOBAL;
                else if (strcmp(optarg, "local") == 0) scope = LOCAL;
                else { printf("Error: Scope can either be 'local' or 'global'.\n"); return 1; }
        }
    }

    char* delim = ".";
    char* cat_key = argv[argc - 2]; // 2nd to last arg.
    if (cat_key[0] == '-') {
        printf("Error: Not enough arguments.\n");
        return 1;
    }
    char* cat = strtok(cat_key, delim);
    char* key = strtok(NULL, delim);
    if (!cat || !key) {
        printf("Error: Must include category and key pair seperated by a '.'.\n");
        return 1;
    }
    if (cat[0] == '-' || key[0] == '-') {
        printf("Error: category and keys cannot begin with a dash.\n");
        return 1;
    }

    char* val = argv[argc - 1]; // last arg.
    if (val[0] == '-') {
        printf("Error: values cannot begin with a dash.\n");
        return 1;
    }

    add_config(scope, cat, key, val);
    return 0;
}
