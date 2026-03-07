#ifndef STAGE_H
#define STAGE_H

#include "repository.h"

#define STAGE_DIR "stage"

void add_to_stage(const char* path, repository* repo);
void remove_from_stage(const char* path, repository* repo);
int index_on_stage(const char* path, repository* repo);
int is_on_stage(const char* path, repository* repo);
int stage_can_be_written(repository* repo);

#endif
