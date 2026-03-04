#ifndef COMMANDS_H
#define COMMANDS_H

typedef int (*command_fn)(char** argv, int argc);

typedef struct {
    char* name;
    command_fn fn;
} command;

int cmd_init(char** argv, int argc);
int cmd_help(char** argv, int argc);
int cmd_config(char** argv, int argc);

#endif
