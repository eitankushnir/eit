#include "branch.h"
#include "commands.h"
#include "commit.h"
#include "head.h"
#include "object.h"
#include "repository.h"
#include "sha256.h"
#include "stage.h"
#include "tree.h"
#include "wrappers.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct options {
    int detach;
    int create;
    int force;
};

static void switch_commit(char* hex, repository* repo)
{
    commit c;
    tree_node root;
    stage new_stage;
    parse_commit(hex, &c, repo);
    char* tree_hex = oid_tostring(&c.tree_oid);
    parse_tree_recursive(tree_hex, &root, repo);
    reconstruct_stage_from_tree(&new_stage, &root);
    swap_stage(repo, &new_stage);
    write_stage(repo);

    free(hex);
}

int cmd_switch(char** argv, int argc, repository* repo)
{
    struct option opts[] = {
        { "create", no_argument, 0, 'c' },
        { "detach", no_argument, 0, 'd' },
        { "force", no_argument, 0, 'f' },
        { 0, 0, 0, 0 },
    };

    int option_val = 0;
    int opindex = 0;
    extern int optind;
    struct options cmd_opts = { 0 };

    while ((option_val = getopt_long(argc, argv, "cdCf", opts, &opindex)) != -1) {
        switch (option_val) {
        case 'c':
            cmd_opts.create = 1;
            break;
        case 'd':
            cmd_opts.detach = 1;
            break;
        case 'C':
            cmd_opts.force = 1;
            cmd_opts.create = 1;
            break;
        case 'f':
            cmd_opts.force = 1;
        default:
            return 1;
        }
    }
    if (cmd_opts.create + cmd_opts.detach > 1) {
        die("Error: --create, --detach and --orphan are all mutually exclusive\n");
    }

    argc = non_opt_count(argv, argc, optind);
    if (cmd_opts.create && (argc < 1 || argc > 2)) {
        fprintf(stderr, "Usage: eit switch (-c|-C) <new-branch> [<starting-point>]\n");
        return 1;
    }
    if (cmd_opts.detach && argc != 1) {
        fprintf(stderr, "Usage: eit switch --detach <starting-point>\n");
        return 1;
    }

    object_id current_commit_id;
    if (repo->head->mode == DETACHED)
        current_commit_id = repo->head->current_commit;
    else if (repo->head->mode == NORMAL) {
        branch h;
        find_branch(repo->head->current_branch, &h, repo);
        if (!h.name && !cmd_opts.detach) die("Error: Repo's head branch doesn't currently point to any commit, please make some before using switch\n");
        current_commit_id = h.commit_id;
        free_branch(&h);
    }
    if (cmd_opts.create) {
        char* branch_name = argv[optind];
        char* starting_point = NULL;
        if (argc == 2)
            starting_point = argv[optind + 1];
        if (starting_point) {
            printf("Given point");
            char* hex = complete_hash_hex(starting_point, repo);
            oid_from_hex(&current_commit_id, hex);
            free(hex);
        }
        branch b;
        find_branch(branch_name, &b, repo);
        if (b.name && !cmd_opts.force)
            die("Fatal: Branch %s already exists, run with --force to procceed anyways\n", branch_name);

        if (!b.name)
            b.name = strdup(branch_name);
        oidcpy(&b.commit_id, &current_commit_id);

        write_branch(&b, repo);
        free_branch(&b);

        repo->head->current_branch = strdup(branch_name);
        repo->head->mode = NORMAL;
        write_head(repo->head, repo);
    } else if (cmd_opts.detach) {
        char* starting_point = argv[optind];
        char* hex = complete_hash_hex(starting_point, repo);
        oid_from_hex(&repo->head->current_commit, hex);
        repo->head->mode = DETACHED;
        write_head(repo->head, repo);
        switch_commit(hex, repo);
    } else {
        char* branch_name = argv[optind];
        branch b;
        find_branch(branch_name, &b, repo);
        if (!b.name)
            die("Fatal: No branch named %s\n", branch_name);

        repo->head->current_branch = strdup(branch_name);
        repo->head->mode = NORMAL;
        write_head(repo->head, repo);
        char* hex = oid_tostring(&b.commit_id);
        switch_commit(hex, repo);
    }

    return 0;
}
