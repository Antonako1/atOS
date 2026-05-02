#include <LIBRARIES/ATZP/ATZP.h>
#include <STD/STRING.h>
#include <STD/MEM.h>

U32 LZ4_DECOMPRESS(PU8 src, U32 src_len, PU8 dst, U32 dst_capacity) {
    PU8 ip = src;
    PU8 ip_end = src + src_len;
    PU8 op = dst;
    PU8 op_end = dst + dst_capacity;

    while (ip < ip_end) {
        /* Step 1: read token */
        U8 token = *ip++;

        /* Step 2: decode literal length */
        U32 lit_len = (token >> 4) & 0x0F;
        if (lit_len == 15) {
            U8 extra;
            do {
                if (ip >= ip_end) return 0;
                extra = *ip++;
                lit_len += extra;
            } while (extra == 255);
        }

        /* Step 3: bounds check then copy literals verbatim */
        if (op + lit_len > op_end) return 0;
        MEMCPY_OPT(op, ip, lit_len);
        ip += lit_len;
        op += lit_len;

        /* Step 4: end-of-block — last sequence has no match section */
        if (ip >= ip_end) break;

        /* Step 5: read 2-byte little-endian offset */
        if (ip + 2 > ip_end) return 0;
        U16 offset = (U16)(ip[0]) | ((U16)(ip[1]) << 8);
        ip += 2;
        if (offset == 0 || op - dst < (U32)offset) return 0; /* invalid offset */
        PU8 match_ptr = op - offset;

        /* Step 6: decode match length (raw nibble checked before +4) */
        U8 match_nibble = token & 0x0F;
        U32 match_len = (U32)match_nibble + 4;
        if (match_nibble == 15) {
            U8 extra;
            do {
                if (ip >= ip_end) return 0;
                extra = *ip++;
                match_len += extra;
            } while (extra == 255);
        }

        /* Step 7: copy match byte-by-byte to handle overlapping regions */
        if (op + match_len > op_end) return 0;
        U32 m;
        for (m = 0; m < match_len; m++)
            *op++ = *match_ptr++;
    }

    return (U32)(op - dst);
}

/* Hash table: maps 12-bit hash → last seen position in src. Zeroed each call. */
static U32 hash_table[4096];

static U32 lz4_hash(PU8 p) {
    U32 v = (U32)p[0] | ((U32)p[1] << 8) | ((U32)p[2] << 16) | ((U32)p[3] << 24);
    return (v * 2654435761UL) >> 20; /* Knuth multiplicative hash → 12-bit index */
}

/* Emit extra-length extension bytes for lengths >= 15 */
static PU8 emit_extra(PU8 op, U32 remainder) {
    while (remainder >= 255) { *op++ = 255; remainder -= 255; }
    *op++ = (U8)remainder;
    return op;
}

U32 LZ4_COMPRESS(PU8 src, U32 src_len, PU8 dst, U32 dst_capacity) {
    if (src_len < 4) {
        /* Too small to compress — emit as literals only */
        if (dst_capacity < src_len + 1) return 0;
        PU8 op = dst;
        *op++ = (U8)(src_len << 4); /* token: lit_len nibble, match nibble = 0 */
        MEMCPY_OPT(op, src, src_len);
        return (U32)(op + src_len - dst);
    }

    /* Zero hash table for this call */
    MEMZERO(hash_table, sizeof(hash_table));

    PU8 op           = dst;
    PU8 op_end       = dst + dst_capacity;
    U32 i            = 0;                  /* current input position */
    U32 literal_start = 0;                 /* start of accumulated literals */

    /* Leave last 5 bytes as mandatory literals (LZ4 spec) */
    U32 limit = src_len > 5 ? src_len - 5 : 0;

    while (i < limit) {
        /* Step 1: hash current 4 bytes, look up candidate */
        U32 h         = lz4_hash(src + i);
        U32 candidate = hash_table[h];
        hash_table[h] = i;

        /* Step 2: check for a valid match (min 4 bytes, offset fits in U16) */
        if (candidate < i &&
            (i - candidate) <= 65535 &&
            MEMCMP(src + candidate, src + i, 4) == 0)
        {
            /* Step 3: extend match as far as possible */
            U32 match_len = 4;
            while (i + match_len < src_len - 1 &&
                   src[candidate + match_len] == src[i + match_len]) {
                match_len++;
            }

            U32 lit_len = i - literal_start;
            U32 offset  = i - candidate;

            /* Bounds guard (worst case: token + extras + lits + 2 + extras) */
            if (op + 1 + (lit_len / 255) + 1 + lit_len + 2 + (match_len / 255) + 1 > op_end)
                return 0;

            /* Step 4a: write token */
            U8 lit_nibble   = (lit_len   >= 15) ? 15 : (U8)lit_len;
            U8 match_nibble = (match_len - 4 >= 15) ? 15 : (U8)(match_len - 4);
            *op++ = (U8)((lit_nibble << 4) | match_nibble);

            /* Step 4b: write extra literal length bytes */
            if (lit_len >= 15) op = emit_extra(op, lit_len - 15);

            /* Step 4c: write literals */
            MEMCPY_OPT(op, src + literal_start, lit_len);
            op += lit_len;

            /* Step 4d: write 2-byte little-endian offset */
            *op++ = (U8)(offset & 0xFF);
            *op++ = (U8)((offset >> 8) & 0xFF);

            /* Step 4e: write extra match length bytes */
            if (match_len - 4 >= 15) op = emit_extra(op, match_len - 4 - 15);

            /* Advance past the match */
            i += match_len;
            literal_start = i;
        } else {
            /* No match — accumulate literal */
            i++;
        }
    }

    /* Step 5: flush all remaining bytes (including mandatory last-5) as literals */
    U32 lit_len = src_len - literal_start;
    if (op + 1 + (lit_len / 255) + 1 + lit_len > op_end) return 0;

    U8 lit_nibble = (lit_len >= 15) ? 15 : (U8)lit_len;
    *op++ = (U8)(lit_nibble << 4); /* low nibble = 0: no match */
    if (lit_len >= 15) op = emit_extra(op, lit_len - 15);
    MEMCPY_OPT(op, src + literal_start, lit_len);
    op += lit_len;

    return (U32)(op - dst);
}