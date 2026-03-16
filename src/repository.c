#include "repository.h"
#include "blob.h"
#include "head.h"
#include "sha256.h"
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
#include <unistd.h>

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

    repo->head = xmalloc(1, head);
    parse_head(repo->head, repo);
}

char* path_in_repo(const char* path, repository* repo)
{
    char* abspath = realpath(path, NULL);
    if (!abspath) {
        int should_die = 0;
        if (errno != ENOENT) {
            should_die = 1;
        } else {
            int res = mkabspath(path);
            if (res != 0)
                should_die = 1;
            abspath = realpath(path, NULL);
            if (!abspath)
                should_die = 1;
            rmabspath(path, repo->repo_root);
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

int mkpath(repository* repo, const char* path)
{
    char* cwd = getcwd(NULL, 0);
    chdir(repo->repo_root);

    strbuf pathbuf = STRBUF_INIT;
    strbuf_addstr(&pathbuf, "./");
    char* path_copy = strdup(path);
    char* dir = strtok(path_copy, "/");
    while (dir) {
        char* next = strtok(NULL, "/");
        if (next) {
            strbuf_addf(&pathbuf, "%s/", dir);
            mkdir(pathbuf.buf, 0755);
        } else {
            strbuf_addf(&pathbuf, "%s", dir);
            FILE* final = fopen(pathbuf.buf, "a");
            if (!final) {
                free(path_copy);
                strbuf_free(&pathbuf);
                return -1;
            }
            fclose(final);
        }

        dir = next;
    }

    free(path_copy);
    strbuf_free(&pathbuf);
    chdir(cwd);
    free(cwd);
    return 0;
}

int rmpath(repository* repo, const char* path)
{
    char* cwd = getcwd(NULL, 0);
    chdir(repo->repo_root);

    strbuf abspath = STRBUF_INIT;
    strbuf_addf(&abspath, "%s/%s", repo->repo_root, path);

    if (remove(abspath.buf) != 0) {
        strbuf_free(&abspath);
        return -1;
    }
    char* slash_ptr = strrchr(path, '/');
    if (!slash_ptr)
        return 0;

    int slash_idx = slash_ptr - path;
    char* dirname = substr(path, slash_idx);
    int res = rmpath(repo, dirname);
    free(dirname);
    strbuf_free(&abspath);
    chdir(cwd);
    free(cwd);
    return res;
}

void swap_stage(repository* repo, struct stage* new_stage)
{
    int i = 0;
    int j = 0;
    while (i < repo->stage->entry_count && j < new_stage->entry_count) {
        stage_entry* curr_new = new_stage->entries[j];
        stage_entry* curr_repo = repo->stage->entries[i];

        int cmp = strcmp(curr_repo->path, curr_new->path);
        if (cmp < 0) {
            rmpath(repo, curr_repo->path);
            i++;
        } else if (cmp > 0) {
            mkpath(repo, curr_new->path);
            char* curr_new_hex = oid_tostring(&curr_new->oid);
            writeout_blob(curr_new_hex, repo, curr_new->path);
            j++;
        } else {
            char* curr_repo_hex = oid_tostring(&curr_repo->oid);
            char* curr_new_hex = oid_tostring(&curr_new->oid);
            if (strcmp(curr_repo_hex, curr_new_hex) != 0) {
                mkpath(repo, curr_new->path);
                writeout_blob(curr_new_hex, repo, curr_new->path);
            }

            free(curr_repo_hex);
            free(curr_new_hex);
            i++;
            j++;
        }
    }

    // add remaining entries.
    while (j < new_stage->entry_count) {
        stage_entry* curr_new = new_stage->entries[j];
        mkpath(repo, curr_new->path);
        char* curr_new_hex = oid_tostring(&curr_new->oid);
        writeout_blob(curr_new_hex, repo, curr_new->path);
        j++;
    }

    // remove remaining entries.
    while (i < repo->stage->entry_count) {
        stage_entry* curr_repo = repo->stage->entries[i];
        rmpath(repo, curr_repo->path);
        i++;
    }
    repo->stage = new_stage;
}
