#include "head.h"
#include "repository.h"
#include "sha256.h"
#include "strbuf.h"
#include "wrappers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* get_head_path(repository* repo)
{
    strbuf path = STRBUF_INIT;
    strbuf_addf(&path, "%s/HEAD", repo->repo_dir);
    return strbuf_detach(&path, 0);
}

void parse_head(head* out, repository* repo)
{
    char* path = get_head_path(repo);
    FILE* headfile = fopen(path, "rb");
    if (!headfile)
        die("Failed to open head for parsing\n");
    strbuf line = STRBUF_INIT;

    char c;
    while ((c = fgetc(headfile)) != EOF) {
        strbuf_addchr(&line, c);
    }

    char* colon_ptr = strchr(line.buf, ':');
    if (colon_ptr) {
        out->current_branch = substr(colon_ptr + 2, -1);
        out->mode = NORMAL;
    } else {
        oid_hex hex;
        strncpy(hex.hex, line.buf, 64);
        hex.hex[64] = '\0';
        out->current_commit = oid_from_hex(&hex);
        out->mode = DETACHED;
    }

    free(path);
    strbuf_free(&line);
}

void write_head(head* head, repository* repo)
{
    char* path = get_head_path(repo);
    FILE* headfile = fopen(path, "wb");

    if (head->mode == NORMAL) {
        fprintf(headfile, "ref: %s", head->current_branch);
    } else {
        oid_hex hex = oid_tostring(&head->current_commit);
        fprintf(headfile, "%s", hex.hex);
    }

    free(path);
}

void discard_head(head* head)
{
    if (head->mode == NORMAL)
        free(head->current_branch);
}
