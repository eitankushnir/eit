#include "config.h"
#include "command.h"
#include "helper.h"
#include "parse-options.h"
#include "repository.h"
#include <stdio.h>
#include <string.h>
int cmd_config(int argc, char **argv, struct repository *repo) {

  int local = 1;
  int global = 0;
  int read = 0;
  int unset = 0;
  int unset_all = 0;
  int list = 0;

  struct option opts[] = {
      MKOPT_BOOL_F("local", 0, local, "Use local configs (default)", OPT_NONEG),
      MKOPT_BOOL_F("global", 0, global, "Use global configs (override --local)", OPT_NONEG),
      MKOPT_BOOL_F("get", 0, read, "Print the values of a configuration", OPT_NONEG),
      MKOPT_BOOL_F("unset", 0, unset, "Remove a configuration with a single value", OPT_NONEG),
      MKOPT_BOOL_F("unset-all", 0, unset_all, "Remove all values of a configuration", OPT_NONEG),
      MKOPT_BOOL_F("list", 0, list, "List all configurations", OPT_NONEG),
      MKOPT_END,
  };

  char *usage[] = {
      "Usage: eit config [--global] [--local] category.key value",
      "       or",
      "       eit config --get [--global] [--local] category.key",
      NULL,
  };

  argc = parse_options(argc, argv, opts, usage);
  if ((unset || unset_all) && read) {
    die("Error: Cannot use --get with --unset or --unset-all");
  }

  int need_3_arg = !(read || unset || unset_all || list);
  int need_2_arg = !need_3_arg && !list;

  if ((need_3_arg && argc < 3) || (need_2_arg && argc < 2))
    die("Error: Not enough arguments");

  if (need_2_arg || need_3_arg)
    if (argv[1][0] == '.' || argv[1][strlen(argv[1]) - 1] == '.')
      die("Error: Must specify category and key");

  config *c;
  if (global) {
    c = repo_get_global_config(repo);
  } else {
    c = repo_get_local_config(repo);
  }

  if (list) {
    config_print(c);
    return 0;
  }

  char *dot = strchr(argv[1], '.');
  if (!dot)
    die("Error: Missing . delimiter for category and key seperation");

  *dot = '\0';
  char *cat = argv[1];
  char *key = dot + 1;
  char *val;
  if (!read)
    val = argv[2];

  if (read) {
    struct string_list *l = config_get_multi(c, cat, key);
    if (!l) {
      printf("No values stored for %s.%s\n", cat, key);
    } else {
      for (size_t i = 0; i < l->nr; i++) {
        printf("%s\n", l->values[i]);
      }
    }
  } else if (unset || unset_all) {
    struct string_list *l = config_get_multi(c, cat, key);
    if (!l) {
      printf("No values stored for %s.%s\n", cat, key);
    } else if (l->nr > 1 && !unset_all) {
      printf("%s.%s has more than 1 value. Use --unset-all\n", cat, key);
    } else {
      config_remove_all(c, cat, key);
    }
  } else {
    config_add(c, cat, key, val);
  }

  return 0;
}
