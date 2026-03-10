#ifndef COMMIT_H
#define COMMIT_H

#include "repository.h"
#include "sha256.h"
typedef struct commit {
    object_id oid;
    object_id* parent_oids;
    unsigned int parent_count;

    char* author_name;
    char* author_email;
    unsigned int author_time;
    

    char* committer_name;
    char* committer_email;
    unsigned int commit_time;

    char* commit_message;

    object_id tree_oid;
} commit;

void init_commit(commit* c);
void add_author(commit* c, char* name, char* email, unsigned int time);
void add_commiter(commit* c, char* name, char* email, unsigned int time);
void add_parent(commit* c, object_id* oid);
void set_tree(commit* c, object_id* oid);
void set_message(commit* c, const char* msg);

void free_commit(commit* c);

void write_commit(commit* c, repository* repo);
void parse_commit(const char* hex, commit* out, repository* repo);
void print_commit(commit* c);

void hash_commit(commit* c);

#endif
