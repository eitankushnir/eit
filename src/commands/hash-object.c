#include "command.h"
#include "object.h"
#include "object_store.h"
#include "parse-options.h"
#include "repository.h"
#include "sha256.h"
#include "strbuf.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

int cmd_hash_object(int argc, char **argv, struct repository *repo) {
  int use_stdin = 0;
  int write_disk = 0;
  char *type = "blob";

  char *usage[] = {
      "Usage: eit hash-object [-t <type>] [-w] <path>",
      "",
      "Or:    eit hash-object [-t <type>] [-w] --stdin",
      "",
      "Generate hash for an object with content from a file or stdin",
      NULL,
  };

  struct option opts[] = {
      MKOPT_BOOL_F(NULL, 'w', write_disk, "Write object in the object store",
                   OPT_NONEG),
      MKOPT_STRING(NULL, 't', type, "(blob|tree|commit)", "Type of the object"),
      MKOPT_BOOL("stdin", 0, use_stdin,
                 "Recieve content from stdin instead of a file"),
      MKOPT_END,
  };

  argc = parse_options(argc, argv, opts, usage);
  if (argc < 2 && !use_stdin) {
    fprintf(stderr, "Missing path to hash\n");
    return 1;
  }

  enum object_type actual_type = string_to_object_type(type);
  if (actual_type == OBJ_UNKNOWN) {
    fprintf(stderr, "Invalid type: %s\n", type);
    return 1;
  }

  struct object_id oid;
  struct hex_oid hex;

  struct object_store *store = repo_get_object_store(repo);
  int write_result;
  if (use_stdin) {
    char buf[4096];
    size_t bytes_read;
    struct strbuf content = STRBUF_INIT;

    while ((bytes_read = fread(buf, sizeof(char), sizeof(buf), stdin))) {
      strbuf_addraw(&content, buf, bytes_read);
    }

    write_result = object_store_write_memory(store, actual_type, content.buf,
                                             content.len, &oid, write_disk);
  } else {
    write_result =
        object_store_write_file(store, actual_type, argv[1], &oid, write_disk);
  }

  if (write_result != 0) {
    fprintf(stderr, "Failed to hash content");
    return 1;
  }

  printf("%s\n", oid_to_hex(&oid, &hex));
  return 0;
}
