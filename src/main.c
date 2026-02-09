#include "sha256_utils.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  sha256_oid oid;
  sha256_hex hex;
  sha256_hash_file("./text", &oid);
  sha256_to_hex(&oid, &hex);
  printf("%s", hex.str);
  return EXIT_SUCCESS;
}
