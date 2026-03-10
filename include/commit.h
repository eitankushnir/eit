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
void write_commit(commit* c, repository* repo);

void parse_commit(const char* hex, commit* out, repository* repo);
void print_commit(commit* c, repository* repo);

#endif
