#include "object_store.h"
#include "helper.h"
#include "object.h"
#include "sha256.h"
#include "strbuf.h"
#include <linux/limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
  strbuf_release(&dir_path);
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
  if (lstat(path, &st) != 0)
    return -1;

  struct strbuf header = STRBUF_INIT;
  strbuf_addf(&header, "%s %zu", object_type_to_string(type), st.st_size);

  SHA256_CTX ctx;
  sha256_init(&ctx);

  // Hash the header (plus the null terminator).
  sha256_update(&ctx, (uint8_t *)header.buf, header.len + 1);

  // hash the file.
  if (S_ISLNK(st.st_mode)) {
    uint8_t buf[PATH_MAX];
    ssize_t len = readlink(path, (char *)buf, sizeof(buf));
    if (len < 0) {
      strbuf_release(&header);
      return -1;
    }

    sha256_update(&ctx, buf, len);
  } else if (!S_ISREG(st.st_mode)) {
    strbuf_release(&header);
    return -1;
  } else {
    FILE *f = fopen(path, "rb");
    if (!f) {
      strbuf_release(&header);
      return -1;
    }

    size_t bytes_read;
    uint8_t buf[PATH_MAX];
    while ((bytes_read = fread(buf, sizeof(uint8_t), sizeof(buf), f))) {
      sha256_update(&ctx, buf, bytes_read);
    }
    fclose(f);
  }

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
  if (!objfile) {
    strbuf_release(&header);
    strbuf_release(&dir_path);
    return -1;
  }

  size_t result = fwrite(header.buf, sizeof(char), header.len + 1, objfile);

  if (S_ISLNK(st.st_mode)) {
    uint8_t buf[PATH_MAX];
    ssize_t len = readlink(path, (char *)buf, sizeof(buf));
    if (len < 0) {
      strbuf_release(&header);
      strbuf_release(&dir_path);
      fclose(objfile);
      return -1;
    }

    result += fwrite(buf, sizeof(char), len, objfile);
  } else {
    uint8_t buf[PATH_MAX];
    size_t bytes_read;
    FILE *sourcefile = fopen(path, "rb");
    if (!sourcefile) {
      strbuf_release(&header);
      strbuf_release(&dir_path);
      fclose(objfile);
      return -1;
    }
    while (
        (bytes_read = fread(buf, sizeof(uint8_t), sizeof(buf), sourcefile))) {
      result += fwrite(buf, sizeof(uint8_t), bytes_read, objfile);
    }

    fclose(sourcefile);
  }

  int headerlen = header.len;
  strbuf_release(&header);
  strbuf_release(&dir_path);
  free(disk_path);
  fclose(objfile);

  size_t total = st.st_size + headerlen + 1;
  if (result == total) {
    return 0;
  } else {
    return -1;
  }
}

void *object_store_read_raw(struct object_store *store, struct object_id *oid,
                            size_t *out_size, enum object_type *out_type) {
  char *path = oid_to_path(store, oid);
  FILE *objfile = fopen(path, "rb");

  if (!objfile) {
    struct hex_oid hex;
    die("Error: failed to read object with id %s", oid_to_hex(oid, &hex));
  }

  char type_name[10]; // no type name is larger than 9 chars.

  fscanf(objfile, "%s %zu", type_name, out_size);
  *out_type = string_to_object_type(type_name);
  fgetc(objfile); // get past the null-terminator of the header.

  void *data = xmalloc(*out_size, uint8_t);
  size_t result = fread(data, sizeof(uint8_t), *out_size, objfile);

  if (result < *out_size) {

    struct hex_oid hex;
    die("Error: object length in header does not match file size for object "
        "%s",
        oid_to_hex(oid, &hex));
  }

  return data;
}
