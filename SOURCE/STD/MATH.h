#ifndef MATH_H
#define MATH_H

#ifdef KERNEL_ENTRY
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define abs(x) ((x) < 0 ? -(x) : (x))
#define sign(x) ((x) < 0 ? -1 : 1)
#define sqr(x) ((x) * (x))

#else
#include <STD/TYPEDEF.h>

#define PI 3.14159265358979323846f
#define TAU (2.0f * PI)
#define HALF_PI (PI * 0.5f)
#define EPSILON 1.19209290e-7f

static inline I32 iabs(I32 x) { return x < 0 ? -x : x; }
static inline I32 imin(I32 a, I32 b) { return a < b ? a : b; }
static inline I32 imax(I32 a, I32 b) { return a > b ? a : b; }
static inline I32 isign(I32 x) { return (x < 0) ? -1 : 1; }
static inline I32 isqr(I32 x) { return x * x; }

#ifdef __RTOS__
#define abs(x) iabs(x)
#endif // __RTOS__

/* Bitwise checks */
static inline BOOLEAN is_even(U32 x) { return (x & 1) == 0; }
static inline BOOLEAN is_odd(U32 x)  { return (x & 1) != 0; }

/* Float inlines */
static inline F32 fabsf(F32 x) { return x < 0.0f ? -x : x; }
static inline F32 fminf(F32 a, F32 b) { return a < b ? a : b; }
static inline F32 fmaxf(F32 a, F32 b) { return a > b ? a : b; }
static inline F32 fsqr(F32 x) { return x * x; }

/* Clamping */
static inline I32 clampi(I32 x, I32 lo, I32 hi) { return imax(lo, imin(x, hi)); }
static inline U32 clampu(U32 x, U32 lo, U32 hi) { 
    return (x < lo) ? lo : ((x > hi) ? hi : x); 
}
static inline F32 clampf(F32 x, F32 lo, F32 hi) { return fmaxf(lo, fminf(x, hi)); }

/* Integer math declarations */
I32 ipow(I32 base, I32 exp);
I32 isqrt(I32 x);

U32 gcd(U32 a, U32 b);
U32 lcm(U32 a, U32 b);

BOOLEAN is_power_of_two(U32 x);
U32 next_power_of_two(U32 x);

/* Floating math declarations */
F32 powf_i(F32 base, I32 exp); // Renamed to reflect it only handles int exponents
F32 sqrtf(F32 x);

F32 floorf(F32 x);
F32 ceilf(F32 x);
F32 roundf(F32 x);

F32 sinf(F32 x);
F32 cosf(F32 x);
F32 tanf(F32 x);

/* Utilities */
static inline F32 DEG_TO_RAD(F32 deg) { return deg * (PI / 180.0f); }
static inline F32 RAD_TO_DEG(F32 rad) { return rad * (180.0f / PI); }

static inline I32 iround(F32 x) {
    return (I32)(x + (x >= 0.0f ? 0.5f : -0.5f));
}

BOOLEAN range_overlap(U32 a_start, U32 a_len, U32 b_start, U32 b_len);
#endif // KERNEL_ENTRY

#endif // MATH_H