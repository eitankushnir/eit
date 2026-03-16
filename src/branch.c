#include "branch.h"
#include "repository.h"
#include "sha256.h"
#include "strbuf.h"
#include "wrappers.h"
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static char* get_branch_path(const char* name, repository* repo)
{
    strbuf path = STRBUF_INIT;
    strbuf_addf(&path, "%s/refs/heads/%s", repo->repo_dir, name);
    return strbuf_detach(&path, 0);
}

void find_branch(const char* name, branch* out, repository* repo)
{
    char* path = get_branch_path(name, repo);
    FILE* branch_file = fopen(path, "rb+");

    if (!branch_file) {
        if (errno == ENOENT) {
            out->name = NULL;
            return;
        } else {
            die("%s: Failed to open branch file for reading\n", name);
        }
    }

    char hex[65];
    fscanf(branch_file, "%s", hex);
    out->name = substr(name, -1);
    oid_from_hex(&out->commit_id, hex);
    free(path);
}

void free_branch(branch* b)
{
    free(b->name);
}

void write_branch(branch* b, repository* repo)
{
    char* path = get_branch_path(b->name, repo);
    FILE* branch_file = fopen(path, "wb");
    if (!branch_file) {
        die("%s: Failed to open branch file for writing\n", b->name);
    }
    char* hex = oid_tostring(&b->commit_id);
    fprintf(branch_file, "%s", hex);
    free(hex);
    free(path);
}
