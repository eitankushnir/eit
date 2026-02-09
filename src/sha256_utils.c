#include "sha256_utils.h"
#include "sha256.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void print_hash(uint8_t hash[32]) {
  for (int i = 0; i < 32; i++) {
    printf("%02x", hash[i]);
  }
}

void println_hash(uint8_t hash[32]) {
  print_hash(hash);
  printf("\n");
}

void sha256_hash_string(const char *str, sha256_oid *out) {
  SHA256_CTX ctx;
  sha256_init(&ctx);
  sha256_update(&ctx, (uint8_t *)str, strlen(str));
  sha256_final(&ctx, out->bytes);
}

void sha256_hash_file(const char *path, sha256_oid *out) {
  FILE *file = fopen(path, "rb");
  if (!file) {
    perror("Failed to open file");
    return;
  }

  SHA256_CTX ctx;
  sha256_init(&ctx);
  uint8_t buffer[4096];
  size_t bytes_read;

  while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
    sha256_update(&ctx, buffer, bytes_read);
  }

  sha256_final(&ctx, out->bytes);
}

void sha256_to_hex(const sha256_oid *oid, sha256_hex *out) {
  static const char hex_chars[] = "0123456789abcdef";

  for (int i = 0; i < 32; i++) {
    out->str[i * 2] = hex_chars[(oid->bytes[i] >> 4) & 0xF];
    out->str[i * 2 + 1] = hex_chars[oid->bytes[i] & 0xF];
  }
  out->str[64] = '\0';
}
