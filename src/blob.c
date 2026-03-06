#include "blob.h"
#include "repository.h"
#include "sha256.h"
#include "strbuf.h"
#include "wrappers.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

void hash_blob_from_file(FILE* f, object_id* out_oid, int write_to_store, repository* repo)
{
    char buf[4096];
    size_t size;

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);

    strbuf header = STRBUF_INIT;
    strbuf_addf(&header, "blob %zu", size);

    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, (uint8_t*)header.buf, header.len + 1);

    int len;
    while ((len = fread(buf, 1, sizeof(buf), f))) {
        sha256_update(&ctx, (uint8_t*)buf, len);
    }

    sha256_final(&ctx, out_oid);

    if (write_to_store && !repo->repo_dir)
        die("Error: Cannot write object since we are not in a repository.\n");

    if (!write_to_store) {
        strbuf_free(&header);
    }

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
    if (stat(object_path.buf, &check) == 0) {
        // skip writing an exisiting object.
        return;
    }

    FILE* objfile = fopen(object_path.buf, "wb");
    if (!objfile) die("Fatal: failed to create the new object.");
    fwrite(header.buf, 1, header.len + 1, objfile);
    fseek(f, 0, SEEK_SET);
    while (( len = fread(buf, 1, sizeof(buf), f))) {
        fwrite(buf, 1, len, objfile);
    }

    strbuf_free(&header);
    strbuf_free(&object_path);
    fclose(objfile);
}

void hash_blob_from_stdin(object_id *out_oid, int write_to_store, repository *repo) {
    // Copy stdin to tmpfile and use it to hash.
    FILE* tmp = tmpfile();
    char buf[4096];
    size_t len;

    while (( len = fread(buf, 1, sizeof(buf), stdin))) {
        fwrite(buf, 1, len, tmp);
    }

    hash_blob_from_file(tmp, out_oid, write_to_store, repo);
    fclose(tmp);
}
