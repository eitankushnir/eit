#include "stage.h"
#include "helper.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

int write_stage_disk(struct stage *s) {
  FILE *file = fopen(s->disk_location, "wb");
  if (!file)
    return -1;

  uint32_t out_size = htonl(s->entries_nr);
  fwrite(&out_size, 1, sizeof(uint32_t), file);
  for (size_t i = 0; i < s->entries_nr; i++) {
    struct stage_entry *ent = s->entries[i];
    uint32_t network_order_data[12];
    network_order_data[0] = htonl(ent->st.st_ctim.tv_sec);
    network_order_data[1] = htonl(ent->st.st_ctim.tv_nsec);
    network_order_data[2] = htonl(ent->st.st_mtim.tv_sec);
    network_order_data[3] = htonl(ent->st.st_mtim.tv_nsec);
    network_order_data[4] = htonl(ent->st.st_dev);
    network_order_data[5] = htonl(ent->st.st_ino);
    network_order_data[6] = htonl(ent->st.st_size);
    network_order_data[7] = htonl(ent->st.st_gid);
    network_order_data[8] = htonl(ent->st.st_uid);
    network_order_data[9] = htonl(ent->flags);
    network_order_data[10] = htonl(ent->mode);
    network_order_data[11] = htonl(ent->path_len);
    fwrite(network_order_data, sizeof(uint32_t), 12, file);

    fwrite(ent->oid.hash, sizeof(uint8_t), 32, file);
    fwrite(ent->path, sizeof(char), ent->path_len, file);
  }

  fclose(file);
  return 0;
}

struct stage *parse_stage_disk(const char *path) {
  struct stage *s = xcalloc(1, struct stage);
  s->disk_location = strdup(path);

  FILE *file = fopen(path, "rb");
  if (!file) {
    return s;
  }

  uint32_t in_size;
  fread(&in_size, sizeof(uint32_t), 1, file);
  s->entries_nr = ntohl(in_size);
  s->entries = xmalloc(s->entries_nr, struct stage_entry *);

  for (size_t i = 0; i < s->entries_nr; i++) {
    uint32_t network_order_data[12];
    fread(network_order_data, sizeof(uint32_t), 12, file);
    struct stage_entry *ent = xmalloc(1, struct stage_entry);

    ent->st.st_ctim.tv_sec = ntohl(network_order_data[0]);
    ent->st.st_ctim.tv_nsec = ntohl(network_order_data[1]);
    ent->st.st_mtim.tv_sec = ntohl(network_order_data[2]);
    ent->st.st_mtim.tv_nsec = ntohl(network_order_data[3]);
    ent->st.st_dev = ntohl(network_order_data[4]);
    ent->st.st_ino = ntohl(network_order_data[5]);
    ent->st.st_size = ntohl(network_order_data[6]);
    ent->st.st_gid = ntohl(network_order_data[7]);
    ent->st.st_uid = ntohl(network_order_data[8]);
    ent->flags = ntohl(network_order_data[9]);
    ent->mode = ntohl(network_order_data[10]);
    ent->path_len = ntohl(network_order_data[11]);

    fread(ent->oid.hash, sizeof(uint8_t), 32, file);
    ent->path = xmalloc(ent->path_len + 1, char);
    fread(ent->path, sizeof(char), ent->path_len, file);
    ent->path[ent->path_len] = '\0';

    s->entries[i] = ent;
  }

  fclose(file);
  return s;
}

void stage_free(struct stage *s) {
  for (size_t i = 0; i < s->entries_nr; i++) {
    free(s->entries[i]->path);
    free(s->entries[i]);
  }

  if (s->disk_location)
    free(s->disk_location);
  free(s->entries);
  free(s);
}

static int index_on_stage(struct stage *s, const char *path, int *is_on_stage) {
  int l = 0;
  int r = s->entries_nr - 1;

  while (l <= r) {
    int m = l + ((r - l) / 2);
    int cmp = strcmp(path, s->entries[m]->path);
    if (cmp == 0) {
      *is_on_stage = 1;
      return m;
    }
    if (cmp < 0)
      r = m - 1;
    else
      l = m + 1;
  }

  *is_on_stage = 0;
  return l;
}

int stage_add_path(struct stage *s, const char *path, struct stat st,
                   struct object_id *oid) {
  int is_on_stage;
  size_t i = index_on_stage(s, path, &is_on_stage);
  if (is_on_stage) {
    s->entries[i]->st = st;
    s->entries[i]->mode = normalize_mode(st.st_mode);
    s->entries[i]->oid = *oid;
    return 0;
  }

  s->entries = xrealloc(s->entries, ++s->entries_nr, struct stage_entry *);
  for (size_t j = s->entries_nr - 1; j > i; j--) {
    s->entries[j] = s->entries[j - 1];
  }
  struct stage_entry *new_ent = xmalloc(1, struct stage_entry);
  new_ent->path = strdup(path);
  new_ent->oid = *oid;
  new_ent->st = st;
  new_ent->path_len = strlen(path);
  new_ent->mode = normalize_mode(st.st_mode);
  new_ent->flags = 0;
  s->entries[i] = new_ent;
  return 0;
}

int stage_remove_path(struct stage *s, const char *path) {
  int is_on_stage;
  size_t i = index_on_stage(s, path, &is_on_stage);

  if (!is_on_stage)
    return -1;

  free(s->entries[i]->path);
  free(s->entries[i]);

  for (size_t j = i; j < s->entries_nr - 1; j++) {
    s->entries[j] = s->entries[j + 1];
  }
  if (s->entries_nr - 1 == 0) {
    free(s->entries);
    s->entries = NULL;
    s->entries_nr = 0;
  } else {
    s->entries = xrealloc(s->entries, --s->entries_nr, struct stage_entry *);
  }
  return 0;
}

int stage_has_path(struct stage *s, const char *path) {
  int is_on_stage;
  index_on_stage(s, path, &is_on_stage);

  return is_on_stage;
}
