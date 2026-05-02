#include <LIBRARIES/ATHASH/SHA1.h>
#include <STD/MEM.h>

/* ---- SHA-1 (FIPS 180-4) ---- */

#define ROL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define BLK0(i) (W[i] = (((U32)(data[i*4])<<24)|((U32)(data[i*4+1])<<16)|((U32)(data[i*4+2])<<8)|((U32)(data[i*4+3]))))
#define BLK(i)  (W[i&15] = ROL32(W[(i+13)&15]^W[(i+8)&15]^W[(i+2)&15]^W[i&15],1))

#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+BLK0(i)+0x5A827999+ROL32(v,5); w=ROL32(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+BLK(i) +0x5A827999+ROL32(v,5); w=ROL32(w,30);
#define R2(v,w,x,y,z,i) z+=(w^x^y)          +BLK(i) +0x6ED9EBA1+ROL32(v,5); w=ROL32(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+BLK(i) +0x8F1BBCDC+ROL32(v,5); w=ROL32(w,30);
#define R4(v,w,x,y,z,i) z+=(w^x^y)          +BLK(i) +0xCA62C1D6+ROL32(v,5); w=ROL32(w,30);

static VOID sha1_transform(U32 state[5], CONST U8 data[64]) {
    U32 W[16];
    U32 a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];

    R0(a,b,c,d,e, 0) R0(e,a,b,c,d, 1) R0(d,e,a,b,c, 2) R0(c,d,e,a,b, 3)
    R0(b,c,d,e,a, 4) R0(a,b,c,d,e, 5) R0(e,a,b,c,d, 6) R0(d,e,a,b,c, 7)
    R0(c,d,e,a,b, 8) R0(b,c,d,e,a, 9) R0(a,b,c,d,e,10) R0(e,a,b,c,d,11)
    R0(d,e,a,b,c,12) R0(c,d,e,a,b,13) R0(b,c,d,e,a,14) R0(a,b,c,d,e,15)
    R1(e,a,b,c,d,16) R1(d,e,a,b,c,17) R1(c,d,e,a,b,18) R1(b,c,d,e,a,19)
    R2(a,b,c,d,e,20) R2(e,a,b,c,d,21) R2(d,e,a,b,c,22) R2(c,d,e,a,b,23)
    R2(b,c,d,e,a,24) R2(a,b,c,d,e,25) R2(e,a,b,c,d,26) R2(d,e,a,b,c,27)
    R2(c,d,e,a,b,28) R2(b,c,d,e,a,29) R2(a,b,c,d,e,30) R2(e,a,b,c,d,31)
    R2(d,e,a,b,c,32) R2(c,d,e,a,b,33) R2(b,c,d,e,a,34) R2(a,b,c,d,e,35)
    R2(e,a,b,c,d,36) R2(d,e,a,b,c,37) R2(c,d,e,a,b,38) R2(b,c,d,e,a,39)
    R3(a,b,c,d,e,40) R3(e,a,b,c,d,41) R3(d,e,a,b,c,42) R3(c,d,e,a,b,43)
    R3(b,c,d,e,a,44) R3(a,b,c,d,e,45) R3(e,a,b,c,d,46) R3(d,e,a,b,c,47)
    R3(c,d,e,a,b,48) R3(b,c,d,e,a,49) R3(a,b,c,d,e,50) R3(e,a,b,c,d,51)
    R3(d,e,a,b,c,52) R3(c,d,e,a,b,53) R3(b,c,d,e,a,54) R3(a,b,c,d,e,55)
    R3(e,a,b,c,d,56) R3(d,e,a,b,c,57) R3(c,d,e,a,b,58) R3(b,c,d,e,a,59)
    R4(a,b,c,d,e,60) R4(e,a,b,c,d,61) R4(d,e,a,b,c,62) R4(c,d,e,a,b,63)
    R4(b,c,d,e,a,64) R4(a,b,c,d,e,65) R4(e,a,b,c,d,66) R4(d,e,a,b,c,67)
    R4(c,d,e,a,b,68) R4(b,c,d,e,a,69) R4(a,b,c,d,e,70) R4(e,a,b,c,d,71)
    R4(d,e,a,b,c,72) R4(c,d,e,a,b,73) R4(b,c,d,e,a,74) R4(a,b,c,d,e,75)
    R4(e,a,b,c,d,76) R4(d,e,a,b,c,77) R4(c,d,e,a,b,78) R4(b,c,d,e,a,79)

    state[0] += a; state[1] += b; state[2] += c; state[3] += d; state[4] += e;
}

VOID SHA1_INIT(SHA1_CTX *ctx) {
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->count[0] = 0;
    ctx->count[1] = 0;
}

VOID SHA1_UPDATE(SHA1_CTX *ctx, CONST U8 *data, U32 len) {
    U32 i, j;
    j = (ctx->count[0] >> 3) & 63;
    if ((ctx->count[0] += len << 3) < (len << 3)) ctx->count[1]++;
    ctx->count[1] += (len >> 29);
    if ((j + len) > 63) {
        MEMCPY(&ctx->buffer[j], data, (i = 64 - j));
        sha1_transform(ctx->state, ctx->buffer);
        for (; i + 63 < len; i += 64)
            sha1_transform(ctx->state, &data[i]);
        j = 0;
    } else {
        i = 0;
    }
    MEMCPY(&ctx->buffer[j], &data[i], len - i);
}

VOID SHA1_FINAL(U8 digest[SHA1_DIGEST_SIZE], SHA1_CTX *ctx) {
    U32 i;
    U8 finalcount[8];
    for (i = 0; i < 8; i++)
        finalcount[i] = (U8)((ctx->count[(i >= 4 ? 0 : 1)] >> ((3-(i & 3)) * 8)) & 0xFF);
    SHA1_UPDATE(ctx, (U8 *)"\200", 1);
    while ((ctx->count[0] & 504) != 448)
        SHA1_UPDATE(ctx, (U8 *)"\0", 1);
    SHA1_UPDATE(ctx, finalcount, 8);
    for (i = 0; i < SHA1_DIGEST_SIZE; i++)
        digest[i] = (U8)((ctx->state[i>>2] >> ((3-(i & 3)) * 8)) & 0xFF);
}

VOID SHA1_HASH(CONST U8 *data, U32 len, U8 digest[SHA1_DIGEST_SIZE]) {
    SHA1_CTX ctx;
    SHA1_INIT(&ctx);
    SHA1_UPDATE(&ctx, data, len);
    SHA1_FINAL(digest, &ctx);
}
