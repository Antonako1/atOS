#ifndef ATHASH_BASE64_H
#define ATHASH_BASE64_H

#include <STD/TYPEDEF.h>

void BASE64_ENCODE(PU8 input, U32 input_len, PU8 output);
U32 BASE64_DECODE(PU8 input, PU8 output);

#endif // ATHASH_BASE_64_H