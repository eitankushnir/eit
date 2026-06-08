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
int cmd_ls_files(int argc, char **argv, struct repository *repo);
int cmd_add(int argc, char **argv, struct repository *repo);
int cmd_write_tree(int argc, char **argv, struct repository *repo);
int cmd_cat_file(int argc, char **argv, struct repository *repo);
int cmd_commit_tree(int argc, char **argv, struct repository *repo);
int cmd_switch(int argc, char **argv, struct repository *repo);
int cmd_commit(int argc, char **argv, struct repository *repo);
int cmd_config(int argc, char **argv, struct repository *repo);
int cmd_diff(int argc, char **argv, struct repository *repo);

#endif
