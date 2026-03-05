#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>
#include <stdint.h>
typedef struct {
    uint8_t data[64]; // The current 512-bit chunk.
    uint32_t datalen; // How many bytes currently in data.
    uint64_t bitlen; // Total length of the message in bits.
    uint32_t state[8]; // Cool magic variables.
} SHA256_CTX;

typedef struct {
    uint32_t hash[32];
} object_id;

void sha256_init(SHA256_CTX* ctx);
void sha256_update(SHA256_CTX* ctx, const uint8_t *data, size_t len);
void sha256_final(SHA256_CTX *ctx, uint8_t hash[32]);
void println_hash(uint8_t hash[32]);
#endif
