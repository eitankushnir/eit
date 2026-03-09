#include "repository.h"
#include "stage.h"
#include "strbuf.h"
#include "wrappers.h"
#include <asm-generic/errno-base.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
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

void init_current_repository_info(repository* repo)
{
    strbuf root = STRBUF_INIT;
    char* repo_parent = find_repository_root();
    if (!repo_parent) {
        repo->repo_root = NULL;
        repo->repo_dir = NULL;
        return;
    }
    strbuf_addf(&root, "%s/%s", repo_parent, REPO_DIR);
    repo->repo_dir = strbuf_detach(&root, 0);
    repo->repo_root = repo_parent;
    repo->stage = xmalloc(1, stage);
    load_stage(repo);
}

char* path_in_repo(const char* path, repository* repo)
{
    char* abspath = realpath(path, NULL);
    if (!abspath) {
        int should_die = 0;
        if (errno != ENOENT) {
            should_die = 1;
        } else {
            FILE* create = fopen(path, "a");
            if (!create)
                should_die = 1;
            abspath = realpath(path, NULL);
            if (!abspath)
                should_die = 1;
            remove(path);
        }

        if (should_die) {
            strbuf err = STRBUF_INIT;
            strbuf_addf(&err, "Failed to resolve %s", path);
            perror(err.buf);
            strbuf_free(&err);
            die("");
        }
    }
    char* buf = malloc(strlen(repo->repo_root) + 1);
    int min = strlen(abspath) < strlen(repo->repo_root) ? strlen(abspath) : strlen(repo->repo_root);
    strncpy(buf, abspath, min);
    buf[min] = '\0';
    if (strcmp(buf, repo->repo_root) != 0) {
        die("Fatal: Path is not inside repository at %s\n", repo->repo_root);
        return NULL;
    }
    int len = strlen(abspath) - strlen(repo->repo_root);
    char* repo_path = xmalloc(len, char);
    strncpy(repo_path, abspath + strlen(repo->repo_root) + 1, len);
    free(abspath);
    return repo_path;
}
