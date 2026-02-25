#include "config.h"
#include "strbuf.h"
#include "wrappers.h"
#include <stdio.h>
#include <string.h>

void add_config(enum scope scope, char* category, char* key, char* value)
{

    strbuf path = STRBUF_INIT;
    if (scope == GLOBAL) {
        strbuf_addf(&path, "~/%s", CONFIG_FILE);
    } else if (scope == LOCAL) {
        // TODO: make it be the repo root.
        strbuf_addf(&path, "./%s", CONFIG_FILE);
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

            printf("%s", lines.buf);

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
        fwrite(category_header.buf, sizeof(char), category_header.len, conffile);
        fwrite(keyvalue.buf, sizeof(char), keyvalue.len, conffile);
        fclose(conffile);
    } else {
        writeline(keyvalue.buf, last_line_read, path.buf);
    }
}
