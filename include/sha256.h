#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
typedef struct {
    uint8_t data[64]; // The current 512-bit chunk.
    uint32_t datalen; // How many bytes currently in data.
    uint64_t bitlen; // Total length of the message in bits.
    uint32_t state[8]; // Cool magic variables.
} SHA256_CTX;

typedef struct {
    uint8_t hash[32];
} object_id;


void sha256_init(SHA256_CTX* ctx);
void sha256_update(SHA256_CTX* ctx, const uint8_t *data, size_t len);
void sha256_final(SHA256_CTX *ctx, object_id* out);

void oidcpy(object_id* dest, object_id* src);
void println_oid(object_id* oid);
char* oid_tostring(object_id* oid);

void hash_file(FILE* f, object_id* out);
void hash_stdin(object_id* out);

#endif
