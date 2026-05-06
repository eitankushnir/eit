#ifndef COMMAND_H
#define COMMAND_H

struct repository;
typedef int (*command_fn)(int argc, char **argv, struct repository *repo);

struct command {
  char *name;
  command_fn fn;
};

int cmd_init(int argc, char **argv, struct repository *repo);
int cmd_hash_object(int argc, char **argv, struct repository *repo);

#endif
