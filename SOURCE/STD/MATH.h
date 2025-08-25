/*+++
    SOURCE/STD/MATH.h - Standard Math Library

    Part of atOS-RT

    Licensed under the MIT License. See LICENSE file in the project root for full license information.
---*/
#ifndef MATH_H
#define MATH_H
#include "./ATOSMINDEF.h"

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define abs(x) ((x) < 0 ? -(x) : (x))
#define sign(x) ((x) < 0 ? -1 : 1)
#define clamp(x, min, max) (max(min, min(x, max)))
#define is_even(x) ((x) % 2 == 0)
#define is_odd(x) ((x) % 2 != 0)

I32 pow(I32 base, I32 exp) {
    I32 result = 1;
    for (I32 i = 0; i < exp; i++) {
        result *= base;
    }
    return result;
}

F32 powf(F32 base, F32 exp) {
    F32 result = 1.0f;
    for (I32 i = 0; i < exp; i++) {
        result *= base;
    }
    return result;
}

#define pow2(x) (x*x)
#define pow3(x) (x*x*x)

I32 sqrt(I32 x) {
    I32 left = 0;
    I32 right = x;
    while (left < right) {
        I32 mid = (left + right + 1) / 2;
        if (mid <= x / mid) {
            left = mid;
        } else {
            right = mid - 1;
        }
    }
    return left;
}

F32 sqrtf(F32 x) {
    F32 left = 0.0f;
    F32 right = x;
    while (left < right) {
        F32 mid = (left + right + 1.0f) / 2.0f;
        if (mid <= x / mid) {
            left = mid;
        } else {
            right = mid - 1.0f;
        }
    }
    return left;
}

I32 round_up(I32 x) {
    return (x + 1) & ~1;
}
I32 round_down(I32 x) {
    return x & ~1;
}
I32 round_to_nearest(I32 x) {
    return (x + 1) & ~1;
}
#endif // MATH_H