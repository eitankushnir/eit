#include "object.h"
#include "common.h"
#include "repository.h"
#include "sha256.h"
#include "strbuf.h"
#include "wrappers.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

const char* type_name(object_type type)
{
    unsigned int idx = (unsigned int)type;
    return type_names[idx];
}

object_type type_from_name(char* name)
{
    unsigned int n = ARRAY_SIZE(type_names);
    for (unsigned int i = 1; i < n; i++) {
        if (strcmp(name, type_names[i]) == 0) {
            return (object_type)i;
        }
    }

    return OBJ_BAD;
}

void write_object(object_type type, FILE* src, repository* repo, object_id* out_oid)
{
    char buf[4096];
    size_t size;

    fseek(src, 0, SEEK_END);
    size = ftell(src);
    fseek(src, 0, SEEK_SET);

    strbuf header = STRBUF_INIT;
    strbuf_addf(&header, "%s %zu", type_name(type), size);

    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, (uint8_t*)header.buf, header.len + 1);

    int len;
    while ((len = fread(buf, 1, sizeof(buf), src))) {
        sha256_update(&ctx, (uint8_t*)buf, len);
    }

    sha256_final(&ctx, out_oid);

    if (!repo->repo_dir)
        die("Error: Cannot write object since we are not in a repository.\n");

    strbuf object_path = STRBUF_INIT;
    const char* hex = oid_tostring(out_oid);
    char prefix[3];
    strncpy(prefix, hex, 2);
    prefix[2] = '\0';

    // First create the subdir (first two leters of the hash.
    strbuf_addf(&object_path, "%s/objects/%s", repo->repo_dir, prefix);
    mkdir(object_path.buf, 0755);
    const char* objname = hex + 2;
    strbuf_addf(&object_path, "/%s", objname);

    struct stat check;
    // skip writing an exisiting object.
    if (stat(object_path.buf, &check) != 0) {
        FILE* objfile = fopen(object_path.buf, "wb");
        if (!objfile)
            die("Fatal: failed to create the new object.");
        fwrite(header.buf, 1, header.len + 1, objfile);
        fseek(src, 0, SEEK_SET);
        while ((len = fread(buf, 1, sizeof(buf), src))) {
            fwrite(buf, 1, len, objfile);
        }
        fclose(objfile);
    }

    strbuf_free(&header);
    strbuf_free(&object_path);
    free(hex);
}

FILE* open_object(const char* hex_oid, repository* repo)
{
    char prefix[3];
    strncpy(prefix, hex_oid, 2);
    prefix[2] = '\0';
    char* objfile_name = hex_oid + 2;

    strbuf objfile_path = STRBUF_INIT;
    strbuf_addf(&objfile_path, "%s/objects/%s/%s", repo->repo_dir, prefix, objfile_name);

    FILE* objfile = fopen(objfile_path.buf, "rb");
    strbuf_free(&objfile_path);
    return objfile;
}

object_type get_type(const char* hex_oid, repository* repo)
{
    FILE* objfile = open_object(hex_oid, repo);
    if (!objfile) {
        printf("Failed to open: %s\n", hex_oid);
        return OBJ_NONE;
    }

    strbuf header = STRBUF_INIT;
    char c;
    while ((c = fgetc(objfile)) != '\0') {
        strbuf_addchr(&header, c);
    }

    const char* space_ptr = strchr(header.buf, ' ');
    if (!space_ptr)
        return OBJ_BAD;

    const int space_idx = space_ptr - header.buf;
    const char* type_name = substr(header.buf, space_idx);

    object_type type = type_from_name(type_name);
    free(type_name);
    strbuf_free(&header);
    fclose(objfile);
    return type;
}

char* complete_hash_hex(const char* short_hash, repository* repo)
{
    if (strlen(short_hash) < 4) {
        die("A short hash must be atleast 4 characters long.\n");
    }

    char* prefix = substr(short_hash, 2);
    strbuf path = STRBUF_INIT;
    strbuf_addf(&path, "%s/objects/%s", repo->repo_dir, prefix);
    DIR* hash_dir = opendir(path.buf);
    if (!hash_dir) {
        die("Error: %s SHA-256 short hash has not found any matches\n", short_hash);
    }

    struct dirent* ent;
    int len = strlen(short_hash) - 2;
    int matches = 0;
    strbuf match = STRBUF_INIT;
    while ((ent = readdir(hash_dir))) {
        char* match_try = substr(ent->d_name, len);
        if (strcmp(match_try, short_hash + 2) == 0) {
            strbuf_free(&match);
            strbuf_init(&match);
            strbuf_addf(&match, "%s%s", prefix, ent->d_name);
            matches++;
        }
    }

    closedir(hash_dir);
    strbuf_free(&path);
    free(prefix);

    if (matches == 0) {
        die("Error: %s SHA-256 short hash has not found any matches\n", short_hash);
    }
    if (matches > 1) {
        die("Error: %s SHA-256 short hash is ambigious.\n", short_hash);
    }

    return strbuf_detach(&match, 0);
}
