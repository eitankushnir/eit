#include "parse-options.h"
#include "helper.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum option_form {
  LONG_STUCK,
  LONG_UNSTUCK,
  SHORT_UNSTUCK,
  SHORT_MULTIPLE,
  DASH_DASH,
  NOT_AN_OPTION,
  HELP,
};

static enum option_form get_option_form(char *arg) {
  if (!arg)
    return NOT_AN_OPTION;
  size_t len = strlen(arg);
  if (len <= 1)
    return NOT_AN_OPTION;

  if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
    return HELP;
  }

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
    if (argi + 1 > argc || argv[argi + 1][0] == '-')
      return NULL;

    char *optarg = argv[argi + 1];
    argv[argi + 1] = NULL;
    return optarg;
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
    *(char **)opt->value = optarg;
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

static int get_longest_name(struct option *opts) {
  int i = 0;
  int max = 0;
  while (opts[i].type != OPT_END) {
    int len = 0;
    if (opts[i].long_name)
      len += strlen(opts[i].long_name);
    if (opts[i].short_name)
      len += 1;

    if (opts[i].flags & OPT_NONEG)
      len += 9; // all the puncuation and spaces.
    else
      len += 12;

    if (opts[i].argh)
      len += strlen(opts[i].argh) + 3;

    max = len > max ? len : max;
    i++;
  }

  return max;
}

static void print_option(struct option *opt, int help_offset) {
  // Print name(s).
  printf("    ");
  int total_print = 4;
  if (opt->short_name && opt->long_name) {
    if (opt->flags & OPT_NONEG) {
      printf("-%c, --%s", opt->short_name, opt->long_name);
      total_print += 6 + strlen(opt->long_name);
    } else {
      printf("-%c, --[no-]%s", opt->short_name, opt->long_name);
      total_print += 11 + strlen(opt->long_name);
    }
  } else if (opt->short_name) {
    printf("-%c", opt->short_name);
    total_print += 2;
  } else if (opt->long_name) {
    if (opt->flags & OPT_NONEG) {
      printf("--%s", opt->long_name);
      total_print += 2 + strlen(opt->long_name);
    } else {
      printf("--[no-]%s", opt->long_name);
      total_print += 7 + strlen(opt->long_name);
    }
  } else {
    // Should not happen ever. purely here to catch developer mistakes.
    die("Option without any name??");
  }

  // Printf argument help.
  if (opt->argh) {
    if (strpbrk(opt->argh, "<>[]()|")) {
      printf(" %s", opt->argh);
      total_print += 1 + strlen(opt->argh);
    } else if (opt->flags & OPT_OPTARG) {
      printf(" [<%s>]", opt->argh);
      total_print += 5 + strlen(opt->argh);
    } else {
      printf(" <%s>", opt->argh);
      total_print += 3 + strlen(opt->argh);
    }
  }

  for (int i = 0; i < help_offset - total_print; i++) {
    printf(" ");
  }

  size_t helplen = strlen(opt->help);
  for (size_t i = 0; i < helplen; i++) {
    printf("%c", opt->help[i]);
    if (opt->help[i] == '\n') {
      for (int i = 0; i < help_offset; i++) {
        printf(" ");
      }
    }
  }

  printf("\n\n");
}

static void print_help(struct option *opts, char **usagestr) {

  int i = 0;
  while (usagestr[i]) {
    printf("%s\n", usagestr[i]);
    i++;
  }

  i = 0;
  int off = get_longest_name(opts) + 8;
  while (opts[i].type != OPT_END) {
    print_option(&opts[i], off);
    i++;
  }
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
    else if (form == SHORT_MULTIPLE)
      parse_short_multiple(argc, argv, i, opts);
    else if (form == LONG_UNSTUCK || form == LONG_STUCK)
      parse_long(argc, argv, i, opts);
    else if (form == HELP) {
      print_help(opts, usagestr);
      exit(0);
    }
  }

  return cleanup_argv(argc, argv);
}
