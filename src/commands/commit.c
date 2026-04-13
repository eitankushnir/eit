#include "commit.h"
#include "commands.h"
#include "config.h"
#include "repository.h"
#include "sha256.h"
#include "stage.h"
#include "tree.h"
#include "wrappers.h"
#include <getopt.h>
#include <stdlib.h>
#include <time.h>

int cmd_commit(char** argv, int argc, repository* repo)
{
    struct option opts[] = {
        { "message", required_argument, 0, 'm' },
        { 0, 0, 0, 0 }
    };

    int option_val = 0;
    int opindex = 0;
    extern int optind;

    commit c;
    init_commit(&c);
    while ((option_val = getopt_long(argc, argv, "m:", opts, &opindex)) != -1) {
        switch (option_val) {
        case 'm':
            if (!optarg)
                die("Must provide a commit message\n");
            set_message(&c, optarg);
            break;
        default:
            return 1;
        }
    }

    tree_node tree;
    construct_stage_tree(&tree, repo->stage);
    write_tree(&tree, repo);
    set_tree(&c, &tree.oid);

    object_id latest_commit;
    if (get_latest_commit_oid(repo, &latest_commit) == 0) {
        add_parent(&c, &latest_commit);
    }

    unsigned int ctime;
    time((time_t*)&ctime);
    char* email = read_config_str("user", "email");
    char* name = read_config_str("user", "name");

    if (!name)
        die("Fatal: must specify user.name in the config\n");
    if (!email)
        die("Fatal: must specify user.email in the config\n");

    add_author(&c, name, email, ctime);
    add_commiter(&c, name, email, ctime);

    free(name);
    free(email);

    write_commit(&c, repo);
    update_head(repo, &c.oid);
    free_commit(&c);
    free_tree(&tree);

    return 0;
}
