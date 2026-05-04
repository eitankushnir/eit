#include "object_store.h"
#include "helper.h"
#include "object.h"
#include "sha256.h"
#include "strbuf.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

void object_store_free(struct object_store *store) {
  free(store->objectsdir);
  free(store);
}

struct object_store *object_store_new(const char *objectsdir) {
  struct object_store *store = xmalloc(1, struct object_store);
  store->objectsdir = normalize_path(objectsdir);

  return store;
}

char *oid_to_path(struct object_store *store, struct object_id *oid) {

  struct hex_oid hex;
  oid_to_hex(oid, &hex);

  struct strbuf path = STRBUF_INIT;
  strbuf_addf(&path, "%s/%.2s/%s", store->objectsdir, hex.hex, hex.hex + 2);

  return path.buf;
}

int object_exists(struct object_store *store, struct object_id *oid) {
  char *objpath = oid_to_path(store, oid);

  struct stat st;
  int result = stat(objpath, &st);

  free(objpath);
  return result == 0;
}

int object_store_write_memory(struct object_store *store, enum object_type type,
                              void *buf, size_t len, struct object_id *out_oid,
                              int write_to_disk) {
  struct strbuf header = STRBUF_INIT;
  strbuf_addf(&header, "%s %zu", object_type_to_string(type), len);

  SHA256_CTX ctx;
  sha256_init(&ctx);

  // Hash the header (plus the null terminator) and buf.
  sha256_update(&ctx, (uint8_t *)header.buf, header.len + 1);
  sha256_update(&ctx, (uint8_t *)buf, len);
  sha256_final(&ctx, out_oid);

  if (!write_to_disk || object_exists(store, out_oid))
    return 0;

  char *disk_path = oid_to_path(store, out_oid);
  // Create the directory
  struct strbuf dir_path = STRBUF_INIT;
  strbuf_addstr(&dir_path, disk_path);
  strbuf_truncate(&dir_path, last_index_of(disk_path, '/'));
  mkdir(dir_path.buf, 0755);

  FILE *objfile = fopen(disk_path, "wb");

  size_t result = fwrite(header.buf, sizeof(char), header.len + 1, objfile);
  result += fwrite(buf, sizeof(uint8_t), len, objfile);

  int headerlen = header.len;
  strbuf_release(&header);
  free(disk_path);
  fclose(objfile);

  if (result == len + headerlen + 1) {
    return 0;
  } else {
    return -1;
  }
}

int object_store_write_file(struct object_store *store, enum object_type type,
                            const char *path, struct object_id *out_oid,
                            int write_to_disk) {
  struct stat st;
  if (stat(path, &st) != 0)
    return -1;

  struct strbuf header = STRBUF_INIT;
  strbuf_addf(&header, "%s %zu", object_type_to_string(type), st.st_size);

  SHA256_CTX ctx;
  sha256_init(&ctx);

  // Hash the header (plus the null terminator).
  sha256_update(&ctx, (uint8_t *)header.buf, header.len + 1);

  // hash the file.
  size_t bytes_read;
  uint8_t buf[4096];
  FILE *f = fopen(path, "rb");
  if (!f) {
    strbuf_release(&header);
    return -1;
  }

  while ((bytes_read = fread(buf, sizeof(uint8_t), sizeof(buf), f))) {
    sha256_update(&ctx, buf, bytes_read);
  }
  sha256_final(&ctx, out_oid);
  fclose(f);

  if (!write_to_disk || object_exists(store, out_oid))
    return 0;

  char *disk_path = oid_to_path(store, out_oid);

  // Create the directory
  struct strbuf dir_path = STRBUF_INIT;
  strbuf_addstr(&dir_path, disk_path);
  strbuf_truncate(&dir_path, last_index_of(disk_path, '/'));

  mkdir(dir_path.buf, 0755);
  FILE *objfile = fopen(disk_path, "wb");
  FILE *sourcefile = fopen(path, "rb");

  size_t result = fwrite(header.buf, sizeof(char), header.len + 1, objfile);

  while ((bytes_read = fread(buf, sizeof(uint8_t), sizeof(buf), sourcefile))) {
    result += fwrite(buf, sizeof(uint8_t), bytes_read, objfile);
  }

  int headerlen = header.len;
  strbuf_release(&header);
  free(disk_path);
  fclose(sourcefile);
  fclose(objfile);

  size_t total = st.st_size + headerlen + 1;
  if (result == total) {
    return 0;
  } else {
    return -1;
  }
}
