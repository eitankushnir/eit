#ifndef SHA256_H
#define SHA256_H
#include <stddef.h>
#include <stdint.h>

#define HASH_SIZE 32
typedef struct {
  uint8_t data[64];  // The current 512-bit chunk.
  uint32_t datalen;  // How many bytes currently in the data.
  uint64_t bitlen;   // Total number of bits in the message being hashed.
  uint32_t state[8]; // State variables for the hash.
} SHA256_CTX;

// Stores the raw bytes of the sha256 hash.
struct object_id {
  uint8_t hash[32];
};

// Stores the human-readable hexidecimal rep. of a sha256 hash.
struct hex_oid {
  char hex[65];
};

// Initialize the context struct.
void sha256_init(SHA256_CTX *ctx);

// Feed the hash engine raw bytes of memory.
void sha256_update(SHA256_CTX *ctx, const uint8_t *data, size_t len);

// Compute the sha256 hash with all the data fed via sha256_update.
void sha256_final(SHA256_CTX *ctx, struct object_id *out);

// Conversion between raw bytes to human readable string.
// Returns a pointer to the string inside the given hex_oid struct.
char *oid_to_hex(struct object_id *oid, struct hex_oid *out);

// Conversion between human readable string to raw bytes.
// Returns -1 on invalid strings.
// Returns 0 on success.
int hex_to_oid(struct hex_oid *hex, struct object_id *out);

int oideq(struct object_id *a, struct object_id *b);
#endif
