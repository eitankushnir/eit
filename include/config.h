#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_FILE ".eitconfig"

enum scope {
    GLOBAL,
    LOCAL
};

char* read_config_str(char* category, char* key);
int read_config_bool(char* category, char* key);
int read_config_int(char* category, char* key);

void add_config(enum scope scope, char* category, char* key, char* value);

#endif
