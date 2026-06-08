#include "commit.h"
#include "helper.h"
#include "object.h"
#include "sha256.h"
#include "strbuf.h"
#include <endian.h>
#include <limits.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
  memcpy(&info->author_time, ptr, sizeof(uint64_t));
  info->author_time = be64toh(info->author_time);
  ptr += sizeof(uint64_t);

  memcpy(&info->commit_time, ptr, sizeof(uint64_t));
  info->commit_time = be64toh(info->commit_time);
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

  printf("author %s <%s> %ld %s\n", info->author, info->author_email, info->author_time, info->author_tz);
  printf("committer %s <%s> %ld %s\n", info->committer, info->committer_email, info->commit_time, info->committer_tz);

  printf("\n%s\n", info->message);
}

void commit_list_free(struct commit_list *head) {
  while (head) {
    struct commit_list *tmp = head;
    head = head->next;
    free(tmp);
  }
}

struct commit_list *commit_list_push(struct commit_list *l, struct commit *item) {
  struct commit_list *n = xmalloc(1, struct commit_list);
  n->item = item;
  n->next = l;
  return n;
}

struct commit_list *commit_list_push_sorted(struct commit_list *l, struct commit *item) {
  struct commit_list *n = xmalloc(1, struct commit_list);
  n->item = item;
  if (!l || item->date > l->item->date) {
    n->next = l;
    return n;
  }

  struct commit_list *it = l;
  while (it->next && it->next->item->date > item->date) {
    it = it->next;
  }

  n->next = it->next;
  it->next = n;

  return l;
}

static struct commit_list *filter_sol(struct commit_list *sol) {

  if (!sol || !sol->next)
    return sol;

  struct commit_list *q;
  struct commit_list *curr = sol;

  time_t floor_time = ULONG_MAX;
  while (curr) {
    struct commit *item = curr->item;
    if (item->date < floor_time)
      floor_time = item->date;

    struct commit_list *p = item->parents;
    while (p) {
      if (!(p->item->obj.flags & REDUNDANT)) {
        p->item->obj.flags |= REDUNDANT;
        q = commit_list_push_sorted(q, p->item);
      }
      p = p->next;
    }

    curr = curr->next;
  }

  while (q) {
    struct commit *c = q->item;
    struct commit_list *tmp = q;
    q = q->next;
    free(tmp);

    if (c->date < floor_time)
      continue;

    if (c->obj.flags & SOLUTION)
      c->obj.flags &= ~SOLUTION;

    struct commit_list *p = c->parents;
    while (p) {
      if (!(p->item->obj.flags & REDUNDANT)) {
        p->item->obj.flags |= REDUNDANT;
        q = commit_list_push_sorted(q, p->item);
      }
      p = p->next;
    }
  }

  struct commit_list *final_sol = NULL;
  curr = sol;
  while (curr) {
    struct commit_list *tmp = curr;
    curr = curr->next;
    if (tmp->item->obj.flags & SOLUTION)
      final_sol = commit_list_push_sorted(final_sol, tmp->item);

    free(tmp);
  }

  return final_sol;
}

static void clear_commit_marks(struct commit *c, int clear_mask) {
  struct commit_list *q = NULL;

  q = commit_list_push(q, c);
  while (q) {
    struct commit *curr = q->item;
    struct commit_list *tmp = q;
    q = q->next;
    free(tmp);

    if (!(curr->obj.flags & clear_mask))
      continue;

    curr->obj.flags &= ~clear_mask;

    struct commit_list *p = curr->parents;
    while (p) {
      q = commit_list_push(q, p->item);
      p = p->next;
    }
  }
}

struct commit_list *commit_merge_bases(struct commit *c1, struct commit *c2) {
  struct commit_list *q = NULL;
  struct commit_list *sol = NULL;

  c1->obj.flags |= (PARENT1 | SEEN);
  q = commit_list_push_sorted(q, c1);

  c2->obj.flags |= (PARENT2 | SEEN);
  q = commit_list_push_sorted(q, c2);

  struct commit *current;
  while (q) {
    current = q->item;
    struct commit_list *tmp = q;
    q = q->next;
    free(tmp);

    if ((current->obj.flags & COMMON) == COMMON) {
      if (!(current->obj.flags & STALE) && !(current->obj.flags & SOLUTION)) {
        current->obj.flags |= SOLUTION;
        sol = commit_list_push(sol, current);
      }
      current->obj.flags |= STALE;
    }

    struct commit_list *p = current->parents;
    while (p) {

      struct commit *parent = p->item;
      parent->obj.flags |= (current->obj.flags & (COMMON | STALE));

      if (!(parent->obj.flags & SEEN)) {
        parent->obj.flags |= SEEN;
        q = commit_list_push_sorted(q, parent);
      }
      p = p->next;
    }
  }

  sol = filter_sol(sol);
  clear_commit_marks(c1, PARENT1 | PARENT2 | COMMON | SOLUTION | STALE | REDUNDANT | SEEN);
  clear_commit_marks(c2, PARENT1 | PARENT2 | COMMON | SOLUTION | STALE | REDUNDANT | SEEN);
  return sol;
}
