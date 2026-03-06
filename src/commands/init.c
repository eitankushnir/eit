#include "commands.h"
#include "repository.h"
#include "strbuf.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int cmd_init(char** argv, int argc, repository* repo)
{

    char* check = repo->repo_dir;
    strbuf repo_dir = STRBUF_INIT;
    strbuf object_store_path = STRBUF_INIT;
    strbuf refs_path = STRBUF_INIT;
    strbuf refs_heads_path = STRBUF_INIT;
    strbuf refs_remotes_path = STRBUF_INIT;
    strbuf refs_tags_path = STRBUF_INIT;

    if (check) {
        strbuf_addf(&repo_dir, "%s", check);
        strbuf_addf(&object_store_path, "%s/objects", check);
        strbuf_addf(&refs_path, "%s/refs", check);
        strbuf_addf(&refs_heads_path, "%s/refs/heads", check);
        strbuf_addf(&refs_remotes_path, "%s/refs/remotes", check);
        strbuf_addf(&refs_tags_path, "%s/refs/tags", check);
    } else {
        strbuf_addf(&repo_dir, "./%s", REPO_DIR);
        strbuf_addf(&object_store_path, "./%s/objects", REPO_DIR);
        strbuf_addf(&refs_path, "./%s/refs", REPO_DIR);
        strbuf_addf(&refs_heads_path, "./%s/refs/heads", REPO_DIR);
        strbuf_addf(&refs_remotes_path, "./%s/refs/remotes", REPO_DIR);
        strbuf_addf(&refs_tags_path, "./%s/refs/tags", REPO_DIR);
    }

    int mode = S_IRUSR | S_IXUSR | S_IWUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    mkdir(repo_dir.buf, mode);
    mkdir(object_store_path.buf, mode);
    mkdir(refs_path.buf, mode);
    mkdir(refs_heads_path.buf, mode);
    mkdir(refs_remotes_path.buf, mode);
    mkdir(refs_tags_path.buf, mode);

    if (check) {
        printf("Reinitializing repository at: %s\n", check);
        free(check);
    } else {
        char* root = find_repository_root();
        printf("Initialized empty repository at: %s\n", root);
        free(root);
    }
    strbuf_free(&repo_dir);
    strbuf_free(&object_store_path);
    strbuf_free(&refs_path);
    strbuf_free(&refs_heads_path);
    strbuf_free(&refs_remotes_path);
    strbuf_free(&refs_tags_path);

    return 0;
}
