#include "command.h"
#include "helper.h"
#include "object.h"
#include "object_store.h"
#include "parse-options.h"
#include "repository.h"
#include "sha256.h"
#include "tree.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int cmd_cat_file(int argc, char **argv, struct repository *repo) {

  if (!repo->repodir) {
    die("Cannot run cat-file outside of a repository");
  }
  int pretty_print = 0;
  int size_print = 0;
  int type_print = 0;
  char *usage[] = {
      "Usage: eit cat-file <type> <object_id>",
      "       or",
      "       eit cat-file [flag] <object_id>",
      NULL,
  };

  struct option opts[] = {
      MKOPT_BOOL_F("pretty", 'p', pretty_print, "Print in human readable format",
                   OPT_NONEG),

      MKOPT_BOOL_F(
          "size", 's', size_print,
          "Print the size of the object in bytes",
          OPT_NONEG),

      MKOPT_BOOL_F(
          "type", 't', type_print,
          "Print the type of the object",
          OPT_NONEG),
      MKOPT_END,
  };

  argc = parse_options(argc, argv, opts, usage);
  int opt_count = pretty_print + size_print + type_print;
  if (opt_count > 1) {
    fprintf(stderr, "Error: Cannot use multiple flags together.\n");
    return 1;
  }

  if ((argc != 3 && opt_count == 0) || (argc > 2 && opt_count == 1)) {
    fprintf(stderr, "Error: Either too little or to many arguments.\n");
    return 1;
  }

  struct object_id oid;
  enum object_type expected_type;
  enum autocomplete_error e;

  struct object_store *store = repo_get_object_store(repo);
  char *partial = opt_count == 0 ? argv[2] : argv[1];
  e = complete_hex(store, partial, &oid);
  if (opt_count == 0) {
    expected_type = string_to_object_type(argv[1]);
    if (expected_type == OBJ_UNKNOWN) {
      fprintf(stderr, "Error: Type can be blob|tree|commit.\n");
      return 1;
    }
  }

  if (e == NO_SUCH_HEX) {
    fprintf(stderr, "Error: %s does not match any object id\n", partial);
    return 1;
  }
  if (e == PARTIAL_TOO_SHORT) {
    fprintf(stderr, "Error: Partial hex must be at least 3 characters long\n");
    return 1;
  }
  if (e == AMBIGOUS_HEX) {
    fprintf(stderr, "Error: %s is ambigous\n", partial);
    return 1;
  }

  size_t size;
  enum object_type type;
  void *buf = object_store_read_raw(store, &oid, &size, &type);

  if (opt_count == 0) {
    if (expected_type != type) {
      const char *expected_name = object_type_to_string(expected_type);
      const char *name = object_type_to_string(type);
      fprintf(stderr, "Error: Expected %s but object is of type %s.\n", expected_name, name);
      free(buf);
      return 1;
    } else {
      fwrite(buf, sizeof(char), size, stdout);
      free(buf);
      return 0;
    }
  }

  if (type_print) {
    printf("%s\n", object_type_to_string(type));
    free(buf);
    return 0;
  }

  if (size_print) {
    printf("%zu\n", size);
    free(buf);
    return 0;
  }

  // Only option left is to pretty print
  if (type == OBJ_BLOB) {
    fwrite(buf, sizeof(char), size, stdout);
  } else if (type == OBJ_TREE) {
    struct tree t;
    t.buf = buf;
    t.size = size;
    tree_pretty_print(&t);
  }

  free(buf);
  return 0;
}
