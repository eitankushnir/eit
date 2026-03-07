#include "sha256.h"
#include "strbuf.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// BITWISE MACROS

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - n)))
#define SHFTR(x, n) ((x) >> (n))

#define MAJ(a, b, c) (((a) & (b)) ^ ((a) & (c)) ^ ((c) & (b)))
#define CH(e, f, g) (((e) & (f)) ^ ((~(e)) & (g)))

#define sigma0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ SHFTR(x, 3))
#define sigma1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ SHFTR(x, 10))

#define SIGMA0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define SIGMA1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))

// Cool magic constants
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// Proccess the 64-bytes(512-bit) chunk of data and update the state
static void sha256_transform(SHA256_CTX* ctx, const uint8_t* data)
{

    uint32_t w[64] = { 0 };
    for (size_t i = 0; i < 16; i++) {
        // convert to big endian
        w[i] = (((uint32_t)data[i * 4] << 24) | ((uint32_t)data[i * 4 + 1] << 16) | ((uint32_t)data[i * 4 + 2] << 8) | ((uint32_t)data[i * 4 + 3]));
    }

    // remaning words
    for (size_t i = 16; i < 64; i++) {
        w[i] = w[i - 16] + sigma0(w[i - 15]) + w[i - 7] + sigma1(w[i - 2]);
    }

    uint32_t a = ctx->state[0];
    uint32_t b = ctx->state[1];
    uint32_t c = ctx->state[2];
    uint32_t d = ctx->state[3];
    uint32_t e = ctx->state[4];
    uint32_t f = ctx->state[5];
    uint32_t g = ctx->state[6];
    uint32_t h = ctx->state[7];
    uint32_t T1, T2;

    for (size_t i = 0; i < 64; i++) {
        T1 = h + SIGMA1(e) + CH(e, f, g) + K[i] + w[i];
        T2 = SIGMA0(a) + MAJ(a, b, c);

        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

// PUBLIC FUNCTIONS

void sha256_init(SHA256_CTX* ctx)
{
    ctx->datalen = 0;
    ctx->bitlen = 0;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
}

void sha256_update(SHA256_CTX* ctx, const uint8_t* data, size_t len)
{

    // Copy over the data, if we fill up the chunk we process it.
    // We update bitlen accordingly for the final padding.
    for (size_t i = 0; i < len; i++) {
        ctx->data[ctx->datalen] = data[i];
        ctx->datalen++;

        if (ctx->datalen == 64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

void sha256_final(SHA256_CTX* ctx, object_id* out)
{
    // Capture the length of the final remaining chunk.
    ctx->bitlen += ctx->datalen * 8;

    // Append a single 1.
    ctx->data[ctx->datalen] = 0x80;
    ctx->datalen++;

    // We need 56 bits exactly to append the length, so if we have more just pad
    // 0s and process.
    if (ctx->datalen > 56) {
        while (ctx->datalen < 64) {
            ctx->data[ctx->datalen] = 0x00;
            ctx->datalen++;
        }
        sha256_transform(ctx, ctx->data);
        ctx->datalen = 0;
    }

    // Finish the padding of 0s till we have exactly 8 bytes left.
    while (ctx->datalen < 56) {
        ctx->data[ctx->datalen] = 0x00;
        ctx->datalen++;
    }

    // Add the length of the message at big endian at the end.
    ctx->data[56] = (ctx->bitlen >> 56) & 0xFF;
    ctx->data[57] = (ctx->bitlen >> 48) & 0xFF;
    ctx->data[58] = (ctx->bitlen >> 40) & 0xFF;
    ctx->data[59] = (ctx->bitlen >> 32) & 0xFF;
    ctx->data[60] = (ctx->bitlen >> 24) & 0xFF;
    ctx->data[61] = (ctx->bitlen >> 16) & 0xFF;
    ctx->data[62] = (ctx->bitlen >> 8) & 0xFF;
    ctx->data[63] = (ctx->bitlen) & 0xFF;

    // Process one last time.
    sha256_transform(ctx, ctx->data);

    // Encode the hash (which are big endian) into the integer array.
    for (uint32_t i = 0; i < 8; i++) {
        out->hash[i * 4] = (ctx->state[i] >> 24) & 0xFF;
        out->hash[i * 4 + 1] = (ctx->state[i] >> 16) & 0xFF;
        out->hash[i * 4 + 2] = (ctx->state[i] >> 8) & 0xFF;
        out->hash[i * 4 + 3] = (ctx->state[i]) & 0xFF;
    }
}

void oidcpy(object_id* dest, object_id* src)
{
    for (int i = 0; i < 32; i++) {
        dest->hash[i] = src->hash[i];
    }
}

void println_oid(object_id* oid)
{
    for (int i = 0; i < 32; i++) {
        printf("%02x", oid->hash[i]); // Prints each byte as a two-digit hexadecimal number
    }
    printf("\n");
}

void hash_file(FILE* f, object_id* out)
{
    char buf[4096];

    SHA256_CTX ctx;
    sha256_init(&ctx);

    while (fread(buf, 1, sizeof(buf), f)) {
        sha256_update(&ctx, (uint8_t*)buf, strlen(buf));
    }

    fclose(f);
    sha256_final(&ctx, out);
}

void hash_stdin(object_id* out)
{
    char buf[4096];

    SHA256_CTX ctx;
    sha256_init(&ctx);

    while (fread(buf, 1, sizeof(buf), stdin)) {
        sha256_update(&ctx, (uint8_t*)buf, strlen(buf));
    }

    sha256_final(&ctx, out);
}

char* oid_tostring(object_id* oid)
{
    strbuf str = STRBUF_INIT;
    for (int i = 0; i < 32; i++) {
        strbuf_addf(&str, "%02x", oid->hash[i]); 
    }
    return strbuf_detach(&str, 0);
}
