#ifndef ATHASH_SHA1_H
#define ATHASH_SHA1_H

#include <STD/TYPEDEF.h>

#define SHA1_DIGEST_SIZE 20

typedef struct {
    U32 state[5];
    U32 count[2];
    U8  buffer[64];
} SHA1_CTX;

VOID SHA1_INIT(SHA1_CTX *ctx);
VOID SHA1_UPDATE(SHA1_CTX *ctx, CONST U8 *data, U32 len);
VOID SHA1_FINAL(U8 digest[SHA1_DIGEST_SIZE], SHA1_CTX *ctx);

/* Convenience: hash a buffer in one call, writes 20-byte digest */
VOID SHA1_HASH(CONST U8 *data, U32 len, U8 digest[SHA1_DIGEST_SIZE]);

#endif // ATHASH_SHA1_H
