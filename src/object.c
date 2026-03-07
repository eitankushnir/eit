#include "object.h"
#include "common.h"
#include "sha256.h"
#include "strbuf.h"
#include "wrappers.h"
#include <stdio.h>
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

    die("Invalid type name: %s", name);
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
    if (stat(object_path.buf, &check) == 0) {
        // skip writing an exisiting object.
        return;
    }

    FILE* objfile = fopen(object_path.buf, "wb");
    if (!objfile)
        die("Fatal: failed to create the new object.");
    fwrite(header.buf, 1, header.len + 1, objfile);
    fseek(src, 0, SEEK_SET);
    while ((len = fread(buf, 1, sizeof(buf), src))) {
        fwrite(buf, 1, len, objfile);
    }

    strbuf_free(&header);
    strbuf_free(&object_path);
    fclose(objfile);
}
