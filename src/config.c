#include "config.h"
#include "helper.h"
#include "strbuf.h"
#include <linux/limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
unsigned long hash(const char *cat, const char *key) {
  struct strbuf final_key = STRBUF_INIT;
  strbuf_addf(&final_key, "%s.%s", cat, key);
  unsigned long hash = 5381;
  int c;

  char *str = final_key.buf;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }

  strbuf_release(&final_key);
  return hash;
}

void config_free(config *c) {
  for (size_t i = 0; i < c->buckets_nr; i++) {
    struct config_bucket *b = c->buckets[i];
    while (b) {
      for (size_t j = 0; j < b->list->nr; j++) {
        free(b->list->values[j]);
      }
      free(b->list->values);
      free(b->list);
      free(b->cat);
      free(b->key);
      struct config_bucket *tmp = b;
      b = b->next;
      free(tmp);
    }
  }

  free(c->buckets);
  free(c->disk_location);
  free(c);
}

config *config_read_disk(const char *path) {
  FILE *f = fopen(path, "rb");
  config *c = xcalloc(1, config);
  c->buckets_nr = 20;
  c->buckets = xcalloc(c->buckets_nr, struct config_bucket *);
  c->disk_location = strdup(path);

  if (!f)
    return c;

  char buf[4096];
  char current_cat[256];
  int in_cat = 0;
  while (fgets(buf, sizeof(buf), f)) {
    char *newline = strchr(buf, '\n');
    if (newline)
      *newline = '\0';

    if (buf[0] == '[') {
      char *last_bracket = strrchr(buf, ']');
      *last_bracket = '\0';
      snprintf(current_cat, sizeof(current_cat), "%s", buf + 1);
      in_cat = 1;

    } else if (in_cat && buf[0] == '\t') {
      char key[256];
      char value[256];
      char *eq = strchr(buf, '=');
      *(eq - 1) = '\0';
      eq = eq + 2;

      snprintf(key, sizeof(key), "%s", buf + 1);
      snprintf(value, sizeof(value), "%s", eq);
      config_insert(c, current_cat, key, value);
    }
  }

  return c;
}

void config_insert(config *c, const char *category, const char *key, const char *value) {
  struct string_list *l = config_get_multi(c, category, key);
  if (l) {
    l->values = xrealloc(l->values, ++l->nr, char *);
    l->values[l->nr - 1] = strdup(value);
    return;
  }

  size_t index = hash(category, key) % c->buckets_nr;
  struct config_bucket *newb = xmalloc(1, struct config_bucket);
  newb->list = xmalloc(1, struct string_list);
  newb->list->nr = 1;
  newb->list->values = xmalloc(1, char *);
  newb->list->values[0] = strdup(value);
  newb->cat = strdup(category);
  newb->key = strdup(key);
  newb->next = c->buckets[index];
  c->buckets[index] = newb;
}

static struct config_bucket *get_bucket(config *c, const char *category, const char *key) {
  size_t index = hash(category, key) % c->buckets_nr;
  return c->buckets[index];
}

struct string_list *config_get_multi(config *c, const char *category, const char *key) {
  struct config_bucket *b = get_bucket(c, category, key);
  while (b) {
    if (strcmp(b->cat, category) == 0 && strcmp(b->key, key) == 0)
      return b->list;
    b = b->next;
  }

  return NULL;
}

char *config_get_string(config *c, const char *category, const char *key) {
  struct string_list *lst = config_get_multi(c, category, key);
  if (!lst)
    return NULL;

  return lst->values[lst->nr - 1];
}

int config_get_bool(config *c, const char *catergory, const char *key) {
  char *word = config_get_string(c, catergory, key);
  if (!word)
    return -1;

  if (strcmp(word, "true") == 0)
    return 1;
  if (strcmp(word, "false") == 0)
    return 0;

  return -1;
}

void config_add(config *c, const char *category, const char *key, const char *value) {
  FILE *f = fopen(c->disk_location, "rb");
  char lockpath[PATH_MAX];
  snprintf(lockpath, sizeof(lockpath), "%s.lock", c->disk_location);

  FILE *tmp = fopen(lockpath, "wb");

  char buf[4096];
  char current_cat[256];
  int found_cat = 0;
  int added = 0;
  while (f && fgets(buf, sizeof(buf), f)) {
    if (buf[0] == '[' && found_cat && !added) {
      fprintf(tmp, "\t%s = %s\n", key, value);
      added = 1;

    } else if (buf[0] == '[' && !found_cat) {
      char *last_bracket = strrchr(buf, ']');
      *last_bracket = '\0';
      snprintf(current_cat, sizeof(current_cat), "%s", buf + 1);
      *last_bracket = ']';
      if (strcmp(current_cat, category) == 0)
        found_cat = 1;
    }

    fprintf(tmp, "%s", buf);
  }

  if (!found_cat) {
    fprintf(tmp, "[%s]\n", category);
    fprintf(tmp, "\t%s = %s\n", key, value);
  }
  if (found_cat && !added) {
    fprintf(tmp, "\t%s = %s\n", key, value);
  }

  rename(lockpath, c->disk_location);
  config_insert(c, category, key, value);
}

void config_remove_all(config *c, const char *category, const char *key) {
  FILE *f = fopen(c->disk_location, "rb");
  char lockpath[PATH_MAX];
  snprintf(lockpath, sizeof(lockpath), "%s.lock", c->disk_location);

  FILE *tmp = fopen(lockpath, "wb");

  char buf[4096];
  char current_cat[256];
  int found_cat = 0;
  while (f && fgets(buf, sizeof(buf), f)) {
    if (found_cat && buf[0] == '\t') {
      char key_test[256];
      char *eq = strchr(buf, '=');
      *(eq - 1) = '\0';
      snprintf(key_test, sizeof(key), "%s", buf + 1);
      *(eq - 1) = ' ';
      if (strcmp(key, key_test) == 0)
        continue;
    } else if (buf[0] == '[' && found_cat) {
      found_cat = 0;
    } else if (buf[0] == '[' && !found_cat) {
      char *last_bracket = strrchr(buf, ']');
      *last_bracket = '\0';
      snprintf(current_cat, sizeof(current_cat), "%s", buf + 1);
      *last_bracket = ']';
      if (strcmp(current_cat, category) == 0)
        found_cat = 1;
    }

    fprintf(tmp, "%s", buf);
  }

  rename(lockpath, c->disk_location);
}

void config_print(config *c) {
  for (size_t i = 0; i < c->buckets_nr; i++) {

    struct config_bucket *b = c->buckets[i];
    while (b) {
      struct strbuf keystr = STRBUF_INIT;
      strbuf_addf(&keystr, "%s.%s", b->cat, b->key);
      for (size_t j = 0; j < b->list->nr; j++) {
        printf("%s = %s\n", keystr.buf, b->list->values[j]);
      }
      b = b->next;
      strbuf_setlen(&keystr, 0);
    }
  }
}
