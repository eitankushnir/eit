#ifndef COMMANDS_H
#define COMMANDS_H

#include "repository.h"
typedef int (*command_fn)(char** argv, int argc, repository* repo);

typedef struct {
    char* name;
    command_fn fn;
} command;

int cmd_init(char** argv, int argc, repository* repo);
int cmd_help(char** argv, int argc, repository* repo);
int cmd_config(char** argv, int argc, repository* repo);
int cmd_hash_object(char** argv, int argc, repository* repo);
int cmd_update_stage(char** argv, int argc, repository* repo);
int cmd_write_tree(char** argv, int argc, repository* repo);
int cmd_cat_file(char** argv, int argc, repository* repo);
int cmd_ls_files(char** argv, int argc, repository* repo);
int cmd_commit_tree(char** argv, int argc, repository* repo);

#endif
