#ifndef STD_MEM_H
#define STD_MEM_H

#include <STD/TYPEDEF.h>

U0 *MEMCPY(U0* dest, CONST U0* src, U32 size);
U0 *MEMSET(U0* dest, U8 value, U32 size);
U0 *MEMZERO(U0* dest, U32 size);
U0 *MEMMOVE(U0* dest, CONST U0* src, U32 size);
I32 MEMCMP(CONST U0* ptr1, CONST U0* ptr2, U32 size);

U0 *MEMCPY_OPT(U0* dest, CONST U0* src, U32 size);
U0 *MEMSET_OPT(U0* dest, U8 value, U32 size);
U0 *MEMMOVE_OPT(U0* dest, CONST U0* src, U32 size);

// 32-bit optimized versions
U0 *MEMSET32_OPT(U0* dest, U32 value, U32 dwordCount);
U0 *MEMMOVE32_OPT(U0* dest, CONST U0* src, U32 dwordCount);

U0 *MAlloc(U32 size);
U0 *CAlloc(U32 num, U32 size);
U0 *ReAlloc(U0* ptr, U32 newSize);
VOID MFree(U0* ptr);

#endif // STD_MEM_H
