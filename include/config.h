#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>

struct config_bucket {
  struct string_list *list;
  char *cat;
  char *key;
  struct config_bucket *next;
};

typedef struct {
  size_t buckets_nr;
  struct config_bucket **buckets;

  char *disk_location;
} config;

config *config_read_disk(const char *path);
void config_free(config *c);

char *config_get_string(config *c, const char *category, const char *key);
int config_get_bool(config *c, const char *catergory, const char *key);
struct string_list *config_get_multi(config *c, const char *category, const char *key);

void config_insert(config *c, const char *category, const char *key, const char *value);

void config_add(config *c, const char *category, const char *key, const char *value);
void config_remove_all(config *c, const char *category, const char *key);

void config_print(config *c);
#endif
