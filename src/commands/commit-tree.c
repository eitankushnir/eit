#include "commands.h"
#include "commit.h"
#include "config.h"
#include "object.h"
#include "sha256.h"
#include "wrappers.h"
#include <bits/getopt_core.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct validation {
    int parents;
    int message;
    int author;
    int committer;
    int tree;
};

int cmd_commit_tree(char** argv, int argc, repository* repo)
{
    struct option opts[] = {
        { 0, 0, 0, 0 }
    };

    int option_val = 0;
    int opindex = 0;
    extern int optind;
    struct validation valid = { 0 };

    int parents, message;
    commit c;
    init_commit(&c);
    while ((option_val = getopt_long(argc, argv, "p:m:", opts, &opindex)) != -1) {
        switch (option_val) {
        case 'p':
            if (!optarg)
                break;

            char* phex = strtok(optarg, " ");
            while (phex) {
                char* full_hex = complete_hash_hex(phex, repo);
                if (get_type(full_hex, repo) != OBJ_COMMIT)
                    die("%s is not a valid commit hash\n", phex);
                object_id pid;
                oid_from_hex(&pid, full_hex);
                add_parent(&c, &pid);
                phex = strtok(NULL, " ");
                free(full_hex);
            }
            parents = 1;
            break;
        case 'm':
            if (!optarg)
                die("Must provide a commit message\n");
            set_message(&c, optarg);
            message = 1;
            break;
        default:
            return 1;
        }
    }

    if (!message) {
        die("Please provide parents and message using the -p and -m options\n");
    }

    argc = non_opt_count(argv, argc, optind);
    if (argc != 1) {
        fprintf(stderr, "Error: Must provide exactly on argument which is the tree hash\n");
        return 1;
    }

    char* thex = argv[optind];
    char* full_hex = complete_hash_hex(thex, repo);
    if (get_type(full_hex, repo) != OBJ_TREE)
        die("%s is not a valid tree hash\n", thex);

    object_id toid;
    oid_from_hex(&toid, full_hex);
    set_tree(&c, &toid);

    unsigned int ctime;
    time((time_t*)&ctime);
    char* email = read_config_str("user", "email");
    char* name = read_config_str("user", "name");

    if (!name) die("Fatal: must specify user.name in the config\n");
    if (!email) die("Fatal: must specify user.email in the config\n");

    add_author(&c, name, email, ctime);
    add_commiter(&c, name, email, ctime);

    write_commit(&c, repo);
    char* final_hex = oid_tostring(&c.oid);
    printf("%s\n", final_hex);

    free(final_hex);
    free(name);
    free(email);

    return 0;
}
