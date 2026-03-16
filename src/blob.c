#include "blob.h"
#include "object.h"
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
    if (write_to_store) {
        write_object(OBJ_BLOB, f, repo, out_oid);
        return;
    }

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
    strbuf_free(&header);
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

void print_blob(const char *hex, repository *repo) {
    object_type type = get_type(hex, repo);
    if (type != OBJ_BLOB) die("%s is not a valid blob object\n", hex);

    FILE* objfile = open_object(hex, repo);
    if (!objfile) die("Failed to read object %s\n", hex);

    // Get past the header.
    while (fgetc(objfile) != '\0') { }

    char* buf[4096];
    int len;
    while (( len = fread(buf, 1, sizeof(buf) - 1, objfile))) {
        buf[len - 1] = '\0';
        printf("%s", buf);
    }
}

void writeout_blob(const char *hex, repository *repo, const char *dest_path) {
    FILE* objfile = open_object(hex, repo);
    FILE* dest = fopen(dest_path, "wb");

    // Get past the header.
    while (fgetc(objfile) != '\0') { }

    char* buf[4096];
    int len;
    while (( len = fread(buf, 1, sizeof(buf), objfile))) {
        fwrite(buf, 1, len, dest);
    }
}
