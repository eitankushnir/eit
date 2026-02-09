#ifndef SHA256_UTILS_H
#define SHA256_UTILS_H

#include <stdint.h>

typedef struct {
  uint8_t bytes[32];
} sha256_oid;

typedef struct {
  char str[65];
} sha256_hex;

void sha256_to_hex(const sha256_oid *oid, sha256_hex *out);
void print_hash(uint8_t hash[32]);
void sha256_hash_string(const char *str, sha256_oid *out);
void sha256_hash_file(const char *path, sha256_oid *out);
void println_hash(uint8_t hash[32]);

#endif
