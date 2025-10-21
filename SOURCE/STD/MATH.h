/*+++
    SOURCE/STD/MATH.h - Standard Math Library

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.
---*/
#ifndef MATH_H
#define MATH_H
#include "./TYPEDEF.h"

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define abs(x) ((x) < 0 ? -(x) : (x))
#define sign(x) ((x) < 0 ? -1 : 1)
#define clamp(x, min, max) (max(min, min(x, max)))
#define is_even(x) ((x) % 2 == 0)
#define is_odd(x) ((x) % 2 != 0)

I32 pow(I32 base, I32 exp);

F32 powf(F32 base, F32 exp);

#define pow2(x) (x*x)
#define pow3(x) (x*x*x)

I32 sqrt(I32 x);

F32 sqrtf(F32 x);

I32 round_up(I32 x);
I32 round_down(I32 x);
I32 round_to_nearest(I32 x);
I32 ceil_to_multiple(I32 x, I32 n);
BOOLEAN range_overlap(U32 a_start, U32 a_len, U32 b_start, U32 b_len);
#endif // MATH_H