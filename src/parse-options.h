#ifndef PARSE_OPTIONS_H
#define PARSE_OPTIONS_H

#include <stddef.h>
#include <stdint.h>
enum option_type {
  OPT_END = 0, // Signals the end of an options array.
  OPT_SET_INT, // No arguement, will set *value to an integer

  OPT_INT,
  OPT_STRING,
};

enum option_flags {
  OPT_NOARG = 1 << 0,  // Option has no argument.
  OPT_OPTARG = 1 << 1, // Option has optional argument.
  OPT_HASARG = 1 << 2, // Mandetory argument.
  OPT_NONEG = 1 << 3,  // (Boolean) Option cannot be negated.
};

struct option {
  char *long_name; // Long name --something.
  char short_name; // short name -s.
  enum option_type type;
  void *value; // Where the optarg will be stored.
  int defval;  // A default value when no optarg.

  uint8_t flags;
  char *argh; // Short explanation of what the option expects. e.g name.
  char *help; // Help message of what the option does.
};

#define MKOPT_STRING(ln, sn, val, ah, h)                                       \
  {.long_name = (ln),                                                          \
   .short_name = (sn),                                                         \
   .type = OPT_STRING,                                                         \
   .value = &(val),                                                            \
   .defval = 0,                                                                \
   .flags = OPT_NONEG | OPT_HASARG,                                            \
   .argh = (ah),                                                               \
   .help = (h)}

#define MKOPT_END                                                              \
  {.long_name = NULL,                                                          \
   .short_name = 0,                                                            \
   .type = OPT_END,                                                            \
   .value = NULL,                                                              \
   .defval = 0,                                                                \
   .flags = 0,                                                                 \
   .argh = NULL,                                                               \
   .help = NULL}

#define MKOPT_INT(ln, sn, val, ah, h)                                          \
  {.long_name = (ln),                                                          \
   .short_name = (sn),                                                         \
   .type = OPT_INT,                                                            \
   .value = &(val),                                                            \
   .defval = 0,                                                                \
   .flags = OPT_NONEG | OPT_HASARG,                                            \
   .argh = (ah),                                                               \
   .help = (h)}

#define MKOPT_SET_INT_F(ln, sn, val, def, h, f)                                \
  {.long_name = (ln),                                                          \
   .short_name = (sn),                                                         \
   .type = OPT_SET_INT,                                                        \
   .value = &(val),                                                            \
   .defval = (def),                                                            \
   .flags = OPT_NOARG | (f),                                                   \
   .argh = NULL,                                                               \
   .help = (h)}

#define MKOPT_BOOL_F(ln, sn, val, h, f) MKOPT_SET_INT_F(ln, sn, val, 1, h, f)
#define MKOPT_BOOL(ln, sn, val, h) MKOPT_BOOL_F(ln, sn, val, h, 0)

/*
 * Parse the argument vector using the provided options.
 * usagestr is a NULL-terminated array of string to print out usage incase of
 * help or no args.
 *
 * A [--] that everything past it will not be concidered an option.
 *
 * argv will have all option and their arguments removed. the amount of remaning
 * arguments is returned.
 *
 * Will die on any error.
 */
int parse_options(int argc, char **argv, struct option *opts, char **usagestr);

#endif
