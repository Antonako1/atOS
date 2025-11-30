#include <STD/MEM.h>
#include <CPU/SYSCALL/SYSCALL.h>

U0 *MEMCPY(U0* dest, CONST U0* src, U32 size) {
    for (U32 i = 0; i < size; i++) {
        ((U8*)dest)[i] = ((U8*)src)[i];
    }
    return dest;
}

U0 *MEMSET(U0* dest, U8 value, U32 size) {
    U32 *p32 = (U32*)dest;
    U32 v32 = value | (value << 8) | (value << 16) | (value << 24);

    while (size >= 4) {
        *p32++ = v32;
        size -= 4;
    }

    U8 *p8 = (U8*)p32;
    while (size--) {
        *p8++ = value;
    }

    return dest;
}

U0 *MEMZERO(U0* dest, U32 size) {
    asm volatile("rep stosb"
                 : "+D"(dest), "+c"(size)
                 : "a"(0)
                 : "memory");
    return dest;
}

U0 *MEMCPY_OPT(U0* dest, CONST U0* src, U32 size) {
    asm volatile("rep movsb"
                 : "+D"(dest), "+S"(src), "+c"(size)
                 :
                 : "memory");
    return dest;
}

BOOLEAN MEMCMP(CONST U0* ptr1, CONST U0* ptr2, U32 size) {
    for (U32 i = 0; i < size; i++) {
        if (((U8*)ptr1)[i] != ((U8*)ptr2)[i])
            return TRUE;
    }
    return FALSE;
}

U0 *MEMMOVE(U0* dest, CONST U0* src, U32 size) {
    if (dest < src || dest >= src + size) {
        MEMCPY(dest, src, size);
    } else {
        for (U32 i = size; i > 0; i--) {
            ((U8*)dest)[i - 1] = ((U8*)src)[i - 1];
        }
    }
    return dest;
}

/* ===========================================================
   Optimized versions
   =========================================================== */

U0 *MEMSET_OPT(U0* dest, U8 value, U32 size) {
    asm volatile(
        "rep stosb"
        : "+D"(dest), "+c"(size)
        : "a"(value)
        : "memory"
    );
    return dest;
}

U0 *MEMMOVE_OPT(U0* dest, CONST U0* src, U32 size) {
    if (dest == src || size == 0)
        return dest;

    if (dest < src) {
        asm volatile(
            "cld\n\trep movsb"
            : "+D"(dest), "+S"(src), "+c"(size)
            :
            : "memory"
        );
    } else {
        dest = (U8*)dest + size - 1;
        src  = (U8*)src  + size - 1;
        asm volatile(
            "std\n\trep movsb\n\tcld"
            : "+D"(dest), "+S"(src), "+c"(size)
            :
            : "memory"
        );
    }
    return dest;
}

/**
 * @brief Optimized memset using REP STOSL (32-bit words)
 */
U0 *MEMSET32_OPT(U0* dest, U32 value, U32 dwordCount) {
    asm volatile(
        "rep stosl"
        : "+D"(dest), "+c"(dwordCount)
        : "a"(value)
        : "memory"
    );
    return dest;
}

/**
 * @brief Optimized memmove using REP MOVSL (32-bit words)
 */
U0 *MEMMOVE32_OPT(U0* dest, CONST U0* src, U32 dwordCount) {
    if (dest == src || dwordCount == 0)
        return dest;

    if (dest < src) {
        asm volatile(
            "cld\n\trep movsl"
            : "+D"(dest), "+S"(src), "+c"(dwordCount)
            :
            : "memory"
        );
    } else {
        dest = (U8*)dest + dwordCount * 4 - 4;
        src  = (U8*)src  + dwordCount * 4 - 4;
        asm volatile(
            "std\n\trep movsl\n\tcld"
            : "+D"(dest), "+S"(src), "+c"(dwordCount)
            :
            : "memory"
        );
    }
    return dest;
}

/* ===========================================================
   Kernel heap wrappers
   =========================================================== */

U0 *MAlloc(U32 size) {
    return (U0 *)SYSCALL(SYSCALL_KMALLOC, size, 0, 0, 0, 0);
}
U0 *CAlloc(U32 num, U32 size) {
    return (U0 *)SYSCALL(SYSCALL_KCALLOC, num, size, 0, 0, 0);
}
U0 *ReAlloc(U0* ptr, U32 newSize) {
    return (U0 *)SYSCALL(SYSCALL_KREALLOC, ptr, newSize, 0, 0, 0);
}
VOID MFree(U0* ptr) {
    SYSCALL(SYSCALL_KFREE, ptr, 0, 0, 0, 0);
}
