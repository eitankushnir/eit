#include "parse-options.h"
#include "helper.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

enum option_form {
  LONG_STUCK,
  LONG_UNSTUCK,
  SHORT_UNSTUCK,
  SHORT_MULTIPLE,
  DASH_DASH,
  NOT_AN_OPTION,
};

static enum option_form get_option_form(char *arg) {
  if (!arg)
    return NOT_AN_OPTION;
  size_t len = strlen(arg);
  if (len <= 1)
    return NOT_AN_OPTION;

  if (arg[0] == '-' && arg[1] != '-') {
    return len > 2 ? SHORT_MULTIPLE : SHORT_UNSTUCK;
  }

  if (arg[0] == '-' && arg[1] == '-') {
    if (len == 2)
      return DASH_DASH;
    return strchr(arg, '=') ? LONG_STUCK : LONG_UNSTUCK;
  }

  return NOT_AN_OPTION;
}

static char *get_option_argument(int argc, char **argv, int argi) {
  char *arg = argv[argi];
  enum option_form form = get_option_form(arg);

  if (form == NOT_AN_OPTION)
    return NULL;
  if (form == DASH_DASH)
    return NULL;
  if (form == SHORT_UNSTUCK || form == LONG_UNSTUCK || form == SHORT_MULTIPLE) {
    char *optarg = argv[argi + 1];
    argv[argi + 1] = NULL;
    return argi + 1 > argc ? NULL : optarg;
  }
  if (form == LONG_STUCK)
    return strchr(arg, '=') + 1;

  return NULL;
}

static void handle_optarg(struct option *opt, char *optname, int argc,
                          char **argv, int argi) {
  char *optarg = get_option_argument(argc, argv, argi);
  if (!optarg) {
    if (opt->flags & OPT_HASARG)
      die("Error: option %s is missing required argument", optname);
    else
      memcpy(opt->value, &opt->defval, sizeof(int));
  }

  if (opt->type == OPT_INT) {
    char *endptr;
    int a = strtol(optarg, &endptr, 10);
    if (*endptr != '\0')
      die("Error: option: %s expects an integer value", optname);

    memcpy(opt->value, &a, sizeof(int));
  } else if (opt->type == OPT_STRING) {
    *(char **)opt->value = strdup(optarg);
  }
}

static int cleanup_argv(int argc, char **argv) {
  int i, j = 0;
  for (i = 0; i < argc; i++) {
    if (argv[i])
      argv[j++] = argv[i];
  }

  return j;
}

static struct option *find_short_option(char c, struct option *options) {
  int i = 0;
  while (options[i].type != OPT_END) {
    if (c == options[i].short_name)
      return &options[i];
    i++;
  }

  return NULL;
}

static struct option *find_long_option(char *arg, struct option *options,
                                       int *is_neg) {
  int i = 0;
  char *eq = strchr(arg, '=');
  if (eq)
    *eq = '\0';

  char *neg = strstr(arg, "no-");
  if (neg)
    *is_neg = 1;
  else
    *is_neg = 0;

  char *name = neg ? neg + 3 : arg + 2;
  while (options[i].type != OPT_END) {
    if (strcmp(name, options[i].long_name) == 0) {
      if (eq)
        *eq = '=';
      return &options[i];
    }
    i++;
  }

  if (eq)
    *eq = '=';
  return NULL;
}

// e.g -a [arg]
// argi - index of the option in the vector.
static void parse_short_unstuck(int argc, char **argv, int argi,
                                struct option *opts) {
  char *arg = argv[argi];
  struct option *opt = find_short_option(arg[1], opts);
  if (!opt)
    die("Error: Unrecognized option -- %c", arg[1]);
  if (opt->flags & OPT_NOARG) {
    memcpy(opt->value, &opt->defval, sizeof(int));
  } else {
    handle_optarg(opt, &opt->short_name, argc, argv, argi);
  }

  argv[argi] = NULL;
}

// e.g -abcd [arg]
// argi - index of the option in the vector.
static void parse_short_multiple(int argc, char **argv, int argi,
                                 struct option *opts) {
  char *arg = argv[argi];
  for (size_t i = 1; i < strlen(arg) - 1; i++) {
    struct option *opt = find_short_option(arg[i], opts);
    if (!opt)
      die("Error: Unrecognized option -- %c", arg[i]);
    if (opt->flags & OPT_HASARG) {
      die("Error: option %c is missing required argument", opt->short_name);
    }
    memcpy(opt->value, &opt->defval, sizeof(int));
  }

  struct option *last_opt = find_short_option(arg[strlen(arg) - 1], opts);
  if (!last_opt)
    die("Error: Unrecognized option -- %c", arg[strlen(arg) - 1]);

  if (last_opt->flags & OPT_NOARG) {
    memcpy(last_opt->value, &last_opt->defval, sizeof(int));
  } else {
    handle_optarg(last_opt, &last_opt->short_name, argc, argv, argi);
  }
  argv[argi] = NULL;
}

static void parse_long(int argc, char **argv, int argi, struct option *opts) {
  char *arg = argv[argi];
  int is_neg;
  struct option *opt = find_long_option(arg, opts, &is_neg);
  if (!opt)
    die("Error: Unrecognized option -- %s", arg + 2);

  if ((opt->flags & OPT_NONEG) && is_neg)
    die("Error: Option %s cannot be negated.", opt->long_name);

  if (opt->flags & OPT_NOARG) {
    int val = is_neg ? 0 : opt->defval;
    memcpy(opt->value, &val, sizeof(int));
  } else {
    handle_optarg(opt, opt->long_name, argc, argv, argi);
  }

  argv[argi] = NULL;
}

int parse_options(int argc, char **argv, struct option *opts, char **usagestr) {

  for (int i = 0; i < argc; i++) {
    enum option_form form = get_option_form(argv[i]);
    if (form == NOT_AN_OPTION)
      continue;
    if (form == DASH_DASH) {
      argv[i] = NULL;
      return cleanup_argv(argc, argv);
    }
    if (form == SHORT_UNSTUCK)
      parse_short_unstuck(argc, argv, i, opts);
    if (form == SHORT_MULTIPLE)
      parse_short_multiple(argc, argv, i, opts);
    if (form == LONG_UNSTUCK || form == LONG_STUCK)
      parse_long(argc, argv, i, opts);
  }

  return cleanup_argv(argc, argv);
}
