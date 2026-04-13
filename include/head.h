#ifndef HEAD_H
#define HEAD_H

#include "branch.h"
#include "repository.h"
#include "sha256.h"
typedef enum {
    NORMAL = 0,
    DETACHED = 1,
} head_mode;

typedef struct head {
    char* current_branch; // for when in normal mode.
    object_id current_commit; // for when in detached mode.

    head_mode mode;
} head;

void parse_head(head* out, repository* repo);
void write_head(head* head, repository* repo);
void discard_head(head* head);

#endif
