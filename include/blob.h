#ifndef BLOB_H
#define BLOB_H

#include "repository.h"
#include "sha256.h"
#include <stdio.h>

/** 
 * Generate the object id (SHA-256) hash for a blob object with the contents of the file.
 * Must be a seekable file.
 * If write_to_store is true, object is saved but program will die if we are not in a repo.
 */
void hash_blob_from_file(FILE* f, object_id* out_oid, int write_to_store, repository* repo);

/** 
 * Generate the object id (SHA-256) hash for a blob object with the contents of stdin.
 * If write_to_store is true, object is saved but program will die if we are not in a repo.
 */
void hash_blob_from_stdin(object_id *out_oid, int write_to_store, repository* repo);

#endif
