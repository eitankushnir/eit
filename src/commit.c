#include "commit.h"
#include "sha256.h"
#include "strbuf.h"
#include <endian.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void *write_commit(struct commit_info *info, size_t *out_size) {
  struct strbuf buf = STRBUF_INIT;

  strbuf_addraw(&buf, info->tree_oid->hash, 32);
  uint32_t stored_parent_nr = htonl(info->parent_nr);
  strbuf_addraw(&buf, &stored_parent_nr, sizeof(uint32_t));
  strbuf_addraw(&buf, info->parent_oids, 32 * info->parent_nr);

  uint32_t author_len = htonl(strlen(info->author));
  uint32_t author_elen = htonl(strlen(info->author_email));
  uint32_t author_tlen = htonl(strlen(info->author_tz));
  uint64_t author_time = htobe64(info->author_time);

  uint32_t committer_len = htonl(strlen(info->committer));
  uint32_t committer_elen = htonl(strlen(info->committer_email));
  uint32_t committer_tlen = htonl(strlen(info->committer_tz));
  uint64_t committer_time = htobe64(info->commit_time);

  uint32_t message_len = htonl(strlen(info->message));

  strbuf_addraw(&buf, &author_time, sizeof(uint64_t));
  strbuf_addraw(&buf, &committer_time, sizeof(uint64_t));

  strbuf_addraw(&buf, &author_len, sizeof(uint32_t));
  strbuf_addraw(&buf, info->author, strlen(info->author) + 1);
  strbuf_addraw(&buf, &author_elen, sizeof(uint32_t));
  strbuf_addraw(&buf, info->author_email, strlen(info->author_email) + 1);
  strbuf_addraw(&buf, &author_tlen, sizeof(uint32_t));
  strbuf_addraw(&buf, info->author_tz, strlen(info->author_tz) + 1);

  strbuf_addraw(&buf, &committer_len, sizeof(uint32_t));
  strbuf_addraw(&buf, info->committer, strlen(info->committer) + 1);
  strbuf_addraw(&buf, &committer_elen, sizeof(uint32_t));
  strbuf_addraw(&buf, info->committer_email, strlen(info->committer_email) + 1);
  strbuf_addraw(&buf, &committer_tlen, sizeof(uint32_t));
  strbuf_addraw(&buf, info->committer_tz, strlen(info->committer_tz) + 1);

  strbuf_addraw(&buf, &message_len, sizeof(uint32_t));
  strbuf_addraw(&buf, info->message, strlen(info->message) + 1);

  *out_size = buf.len;
  return buf.buf;
}

void hydrate_commit_info(struct commit_info *info, void *buf) {
  char *ptr = (char *)buf;
  // Tree oid
  info->tree_oid = (struct object_id *)ptr;
  ptr += 32;
  memcpy(&info->parent_nr, ptr, sizeof(uint32_t));
  info->parent_nr = ntohl(info->parent_nr);
  ptr += sizeof(uint32_t);

  info->parent_oids = (struct object_id *)ptr;
  ptr += 32 * info->parent_nr;

  // TIMESTAMPS
  memcpy(&info->author_time, buf, sizeof(uint64_t));
  info->author_time = htole64(info->author_time);
  ptr += sizeof(uint64_t);

  memcpy(&info->commit_time, buf, sizeof(uint64_t));
  info->commit_time = htole64(info->author_time);
  ptr += sizeof(uint64_t);

  // AUTHOR - NAME, EMAIL, TZ
  uint32_t len;
  memcpy(&len, ptr, sizeof(uint32_t));
  len = ntohl(len);
  ptr += sizeof(uint32_t);
  info->author = ptr;
  ptr += len + 1;

  memcpy(&len, ptr, sizeof(uint32_t));
  len = ntohl(len);
  ptr += sizeof(uint32_t);
  info->author_email = ptr;
  ptr += len + 1;

  memcpy(&len, ptr, sizeof(uint32_t));
  len = ntohl(len);
  ptr += sizeof(uint32_t);
  info->author_tz = ptr;
  ptr += len + 1;

  // COMMITTER - NAME, EMAIL, TZ
  memcpy(&len, ptr, sizeof(uint32_t));
  len = ntohl(len);
  ptr += sizeof(uint32_t);
  info->committer = ptr;
  ptr += len + 1;

  memcpy(&len, ptr, sizeof(uint32_t));
  len = ntohl(len);
  ptr += sizeof(uint32_t);
  info->committer_email = ptr;
  ptr += len + 1;

  memcpy(&len, ptr, sizeof(uint32_t));
  len = ntohl(len);
  ptr += sizeof(uint32_t);
  info->committer_tz = ptr;
  ptr += len + 1;

  // MESSAGE
  memcpy(&len, ptr, sizeof(uint32_t));
  len = ntohl(len);
  ptr += sizeof(uint32_t);
  info->message = ptr;
  ptr += len + 1;
}

void pretty_print_commit(struct commit_info *info) {
  struct hex_oid hex;
  printf("tree %s\n", oid_to_hex(info->tree_oid, &hex));

  if (info->parent_nr == 0) {
    printf("no parents\n");
  } else if (info->parent_nr == 1) {
    printf("parent %s\n", oid_to_hex(info->parent_oids, &hex));
  } else {
    printf("parents ");
    for (size_t i = 0; i < info->parent_nr; i++) {
      printf("%s ", oid_to_hex(info->parent_oids + i, &hex));
    }
    printf("\n");
  }

  printf("author %s <%s> %llu %s\n", info->author, info->author_email, info->author_time, info->author_tz);
  printf("committer %s <%s> %llu %s\n", info->committer, info->committer_email, info->commit_time, info->committer_tz);

  printf("\n%s\n", info->message);
}
