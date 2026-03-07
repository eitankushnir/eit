#include "commands.h"
#include "repository.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static command commands[] = {
    { "init", cmd_init },
    { "config", cmd_config },
    { "hash-object", cmd_hash_object },
    { "update-stage", cmd_update_stage }
};

void print_help(char* arg) {
    printf("Usage: %s <command> [<arguments>]\n", arg);
}

int main(int argc, char* argv[])
{
    if (argc == 1 || strcmp(argv[1], "--help") == 0) {
        print_help(argv[0]);
        return 0;
    }

    repository repo = { 0 };
    init_current_repository_info(&repo);

    size_t command_count = sizeof(commands) / sizeof(command);
    for (size_t i = 0; i < command_count; i++) {
        if (strcmp(argv[1], commands[i].name) == 0) {
            return commands[i].fn(argv + 1, argc - 1, &repo);
        }
    }

    printf("Unkown command: %s\n", argv[1]);

    return 1;
}
