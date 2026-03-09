#include "commands.h"
#include "sha256.h"
#include "stage.h"
#include "wrappers.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

struct options {
    int show_cached;
    int show_del;
    int show_mod;
    int show_stage;
};

int cmd_ls_files(char** argv, int argc, repository* repo)
{
    struct option opts[] = {
        { "deleted", no_argument, 0, 'd' },
        { "modified", no_argument, 0, 'm' },
        { "cached", no_argument, 0, 'c' },
        { "stage", no_argument, 0, 's' },
        { 0, 0, 0, 0 }
    };

    int option_val = 0;
    int opindex = 0;
    extern int optind;
    struct options cmd_opts = { 0 };

    while ((option_val = getopt_long(argc, argv, "cdm", opts, &opindex)) != -1) {
        switch (option_val) {
        case 'c':
            cmd_opts.show_cached = 1;
            break;
        case 'd':
            cmd_opts.show_del = 1;
            break;
        case 'm':
            cmd_opts.show_mod = 1;
            break;
        case 's':
            cmd_opts.show_stage = 1;
            break;
        default:
            return 1;
        }
    }

    if (!cmd_opts.show_del && !cmd_opts.show_mod) cmd_opts.show_cached = 1;
    argc = non_opt_count(argv, argc);
    if (argc != 0) {
        fprintf(stderr, "Warning: Ingnoring all other non-option arguments\n");
    }

    if (cmd_opts.show_stage) {
        stage* s = repo->stage;
        for (int i = 0; i < s->entry_count; i++) {
            stage_entry* ent = s->entries[i];
            char* hex = oid_tostring(&ent->oid);
            
            printf("%06o %s %d    %s\n", ent->mode, hex, ent->flags & 0b11, ent->path);
            free(hex);
        }
        return 0;
    }
    
    if (cmd_opts.show_cached) {
        stage* s = repo->stage;
        for (int i = 0; i < s->entry_count; i++) {
            stage_entry* ent = s->entries[i];
            
            printf("%s\n", ent->path);
        }
        return 0;
    }

    if (cmd_opts.show_mod) {
        stage mod_stage;
        get_modified_entries(&mod_stage, repo);
        for (int i = 0; i < mod_stage.entry_count; i++) {
            stage_entry* ent = mod_stage.entries[i];
            
            printf("%s\n", ent->path);
        }
    }


    if (cmd_opts.show_del) {
        stage del_stage;
        get_deleted_entries(&del_stage, repo);
        for (int i = 0; i < del_stage.entry_count; i++) {
            stage_entry* ent = del_stage.entries[i];
            
            printf("%s\n", ent->path);
        }
    }

    return 0;
}
