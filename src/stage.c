#include "stage.h"
#include "blob.h"
#include "repository.h"
#include "sha256.h"
#include "strbuf.h"
#include "wrappers.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void add_to_stage(const char* path, repository* repo)
{
    strbuf stage_path = STRBUF_INIT;
    strbuf_addf(&stage_path, "%s/%s", repo->repo_dir, STAGE_DIR);
    FILE* create_stage = fopen(stage_path.buf, "a");
    if (!create_stage)
        die("Failed to open stage file\n");
    fclose(create_stage);

    struct stat check_for_dir;
    if (stat(path, &check_for_dir) != 0) {
        perror("Failed to stat");
        die("");
    }
    if (S_ISDIR(check_for_dir.st_mode)) {
        die("Error: Cannot add %s to the stage since it is a directory\n", path);
    }

    char* repo_path = path_in_repo(path, repo);

    int index = index_on_stage(path, repo);

    strbuf new_entry = STRBUF_INIT;
    FILE* file_to_add = fopen(path, "rb");
    if (!file_to_add)
        die("Fatal: failed to open file for addition");

    object_id oid;
    hash_blob_from_file(file_to_add, &oid, 1, repo);
    fclose(file_to_add);

    char* oid_hex = oid_tostring(&oid);
    // TOOD: figure out modes and these merge statuses.
    strbuf_addf(&new_entry, "100644 %s 0    %s\n", oid_hex, repo_path);

    // Insert new file sorted by path.
    if (index == -1) {
        FILE* stage_file = fopen(stage_path.buf, "rb+");
        if (!stage_file)
            die("Fatal: Failed to open stage file");

        size_t len = maxlinelen(stage_file);
        fseek(stage_file, 0, SEEK_SET);
        char* path_buf = xmalloc(len, char);
        int line_count = 0;
        while (1) {
            int res = fscanf(stage_file, "%*s %*s %*s %s", path_buf);
            if (res == EOF || strcmp(path_buf, repo_path) > 0) {
                writeline(new_entry.buf, line_count, stage_path.buf);
                break;
            }
            line_count++;
        }
        fclose(stage_file);
    } else {
        replaceline(new_entry.buf, index, stage_path.buf);
    }

    strbuf_free(&new_entry);
    strbuf_free(&stage_path);
    free(repo_path);
}

int index_on_stage(const char* path, repository* repo)
{
    strbuf stage_path = STRBUF_INIT;
    strbuf_addf(&stage_path, "%s/%s", repo->repo_dir, STAGE_DIR);

    FILE* stage_file = fopen(stage_path.buf, "rb");
    if (!stage_file)
        die("Fatal: Failed to open stage file\n");

    char* repo_path = path_in_repo(path, repo);
    char* buf = xmalloc(strlen(repo_path) + 1, char);

    int line_count = 0;
    while (fscanf(stage_file, "%*s %*s %*s %s", buf) != EOF) {
        if (strcmp(repo_path, buf) == 0) {
            free(repo_path);
            free(buf);
            fclose(stage_file);
            return line_count;
        }
        line_count++;
    }

    free(repo_path);
    free(buf);
    fclose(stage_file);
    return -1;
}

int is_on_stage(const char* path, repository* repo)
{
    return index_on_stage(path, repo) != -1;
}

void remove_from_stage(const char* path, repository* repo)
{
    strbuf stage_path = STRBUF_INIT;
    strbuf_addf(&stage_path, "%s/%s", repo->repo_dir, STAGE_DIR);

    int index = index_on_stage(path, repo);
    if (index == -1)
        die("Error: %s is not on the stage\n", path);

    printf("%d\n", index);
    removeline(index, stage_path.buf);
    strbuf_free(&stage_path);
}

int stage_can_be_written(repository* repo)
{
    strbuf stage_path = STRBUF_INIT;
    strbuf_addf(&stage_path, "%s/%s", repo->repo_dir, STAGE_DIR);

    FILE* stage_file = fopen(stage_path.buf, "rb");
    if (!stage_file)
        die("Fatal: Failed to open stage file\n");

    int state;
    while (fscanf(stage_file, "%*s %*s %d %*s", &state) != EOF) {
        if (state != 0) {
            fclose(stage_file);
            return 0;
        }
    }

    fclose(stage_file);
    return 1;
}
