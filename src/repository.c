#include "repository.h"
#include "strbuf.h"
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

char* find_repository_root(void)
{

    strbuf current_path = STRBUF_INIT;
    strbuf_addstr(&current_path, "./");

    while (1) {
        DIR* current_dir = opendir(current_path.buf);
        if (!current_dir) {
            strbuf_free(&current_path);
            return NULL;
        }

        // Search dir for the repo directory.
        struct dirent* ent;
        while ((ent = readdir(current_dir))) {
            if (strcmp(ent->d_name, REPO_DIR) == 0) {
                closedir(current_dir);
                char* relpath = strbuf_detach(&current_path, 0);
                char* abspath = realpath(relpath, NULL);
                free(relpath);
                return abspath;
            }
        }
        closedir(current_dir);

        // Make sure we arent in a root.
        struct stat parent, current;
        if (stat(current_path.buf, &current) != 0) {
            strbuf_free(&current_path);
            return NULL;
        }
        // Go back a dir
        strbuf_addstr(&current_path, "../");
        if (stat(current_path.buf, &parent) != 0) {
            strbuf_free(&current_path);
            return NULL;
        }


        if (parent.st_ino == current.st_ino && current.st_dev == parent.st_dev) {
            // break at root.
            break;
        }
    }

    strbuf_free(&current_path);
    return NULL;
}

void init_current_repository_info(repository *repo) {
    strbuf root = STRBUF_INIT;
    char* repo_parent = find_repository_root();
    strbuf_addf(&root, "%s/%s", repo_parent, REPO_DIR);
    repo->repo_dir = strbuf_detach(&root, 0);
    free(repo_parent);
}
