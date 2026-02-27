#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_FILE ".eitconfig"


enum scope {
    GLOBAL,
    LOCAL
};

char* read_config_str(char* category, char* key);
// 1 - true, 0 - false, -1 - no such key. -2 - not a boolean.
int read_config_bool(char* category, char* key);
int read_config_int(char* category, char* key);

void add_config(enum scope scope, char* category, char* key, char* value);

#endif
