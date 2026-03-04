#include "config.h"
#include "repository.h"
#include "strbuf.h"
#include "wrappers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void add_config(enum scope scope, char* category, char* key, char* value)
{

    strbuf path = STRBUF_INIT;
    if (scope == GLOBAL) {
        strbuf_addf(&path, "%s/%s", getenv("HOME"), CONFIG_FILE);
    } else if (scope == LOCAL) {
        char* reporoot = find_repository_root();
        strbuf_addf(&path, "%s/%s", reporoot, CONFIG_FILE);
        free(reporoot);
    }

    // 1. find if cat exits
    // 2. if so add the key = value at the last line
    // 3. if not add the category and the key value.

    strbuf lines = STRBUF_INIT;
    strbuf category_header = STRBUF_INIT;
    strbuf_addf(&category_header, "[%s]\n", category);
    int found_cat_flag = 0;
    int last_line_read = 0;
    FILE* conffile = fopen(path.buf, "rb");
    if (conffile) {
        while (strbuf_read_file_line(&lines, conffile) == 0) {
            if (found_cat_flag && lines.buf[0] == '[') {
                break;
            }

            if (strcmp(lines.buf, category_header.buf) == 0) {
                found_cat_flag = 1;
            }

            // Reached the category after.

            last_line_read++;
            strbuf_free(&lines);
            strbuf_init(&lines);
        }
        fclose(conffile);
    }

    // Add the cat at the end.
    strbuf keyvalue = STRBUF_INIT;
    strbuf_addf(&keyvalue, "\t%s = %s\n", key, value);
    if (!found_cat_flag) {
        FILE* conffile = fopen(path.buf, "ab");
        if (!conffile) {
            perror("Failed to open config file\n");
            die("");
        }
        fwrite(category_header.buf, sizeof(char), category_header.len, conffile);
        fwrite(keyvalue.buf, sizeof(char), keyvalue.len, conffile);
        fclose(conffile);
    } else {
        writeline(keyvalue.buf, last_line_read, path.buf);
    }

    strbuf_free(&keyvalue);
    strbuf_free(&category_header);
    strbuf_free(&lines);
}

static char* find_value(char* path, char* category, char* key)
{
    int found_cat_flag = 0;
    strbuf category_header = STRBUF_INIT;
    strbuf lines = STRBUF_INIT;
    strbuf value = STRBUF_INIT;
    strbuf_addf(&category_header, "[%s]\n", category);

    FILE* conffile = fopen(path, "rb");
    if (conffile) {
        while (strbuf_read_file_line(&lines, conffile) == 0) {
            if (found_cat_flag && lines.buf[0] == '[') {
                break;
            }

            if (found_cat_flag) {
                char* delim = " = ";
                char* token = strtok(lines.buf, delim);
                while (token != NULL) {
                    // ignore the leading tab.
                    if (strcmp(token + 1, key) == 0) {
                        strbuf_free(&value);
                        strbuf_init(&value);
                        token = strtok(NULL, delim);
                        strbuf_addstr(&value, token);
                    }
                    token = NULL;
                }
            }

            if (strcmp(lines.buf, category_header.buf) == 0) {
                found_cat_flag = 1;
            }

            // Reached the category after.

            strbuf_free(&lines);
            strbuf_init(&lines);
        }
        fclose(conffile);
    }
    strbuf_free(&lines);
    strbuf_free(&category_header);
    if (value.len == 0) {
        strbuf_free(&value);
        return NULL;
    }

    char* value_str = strbuf_detach(&value, 0);
    value_str[strlen(value_str) - 1] = '\0'; // remove trailing newline.
    return value_str;
}

char* read_config_str(char* category, char* key)
{
    // 0. first try local - if not found try global.
    // 1. find the cat
    // 2. read the entire cat
    // 3. return the last value set for the key.

    strbuf path = STRBUF_INIT;
    char* reporoot = find_repository_root();
    strbuf_addf(&path, "%s/%s", reporoot, CONFIG_FILE);
    free(reporoot);

    char* value = find_value(path.buf, category, key);
    if (value) {
        return value;
    }

    strbuf_free(&path);
    strbuf_init(&path);
    strbuf_addf(&path, "~/%s", CONFIG_FILE);

    value = find_value(path.buf, category, key);
    return value;
}

int read_config_bool(char* category, char* key)
{
    char* raw = read_config_str(category, key);
    if (!raw)
        return -1;

    if (strcmp(raw, "true") == 0) {
        free(raw);
        return 1;
    } else if (strcmp(raw, "false") == 0) {
        free(raw);
        return 0;
    } else {
        return -2;
    }
}
