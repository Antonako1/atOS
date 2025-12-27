#include <STD/MATH.h>

/* Integer math */

/* Optimized: Binary Exponentiation (O(log n)) */
I32 ipow(I32 base, I32 exp) {
    if (exp < 0) return 0; // Integer power of negative exp is mostly 0
    I32 result = 1;
    while (exp > 0) {
        if (exp & 1) result *= base;
        base *= base;
        exp >>= 1;
    }
    return result;
}

I32 isqrt(I32 x) {
    if (x <= 0) return 0;
    /* Standard Newton-Raphson for Integers */
    I32 r = x;
    I32 next = (r + x / r) / 2;
    while (next < r) {
        r = next;
        next = (r + x / r) / 2;
    }
    return r;
}

U32 gcd(U32 a, U32 b) {
    while (b != 0) {
        U32 t = b;
        b = a % b;
        a = t;
    }
    return a;
}

U32 lcm(U32 a, U32 b) {
    if (a == 0 || b == 0) return 0;
    return (a / gcd(a, b)) * b; // Divide first to prevent overflow
}

BOOLEAN is_power_of_two(U32 x) {
    return x && !(x & (x - 1));
}

U32 next_power_of_two(U32 x) {
    if (x <= 1) return 1;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

/* Floating math */

/* NOTE: This only calculates float^int. 
   True float^float requires exp() and log() functions.
*/
F32 powf_i(F32 base, I32 exp) {
    F32 result = 1.0f;
    I32 abs_exp = (exp < 0) ? -exp : exp;
    
    while (abs_exp > 0) {
        if (abs_exp & 1) result *= base;
        base *= base;
        abs_exp >>= 1;
    }
    
    return (exp < 0) ? (1.0f / result) : result;
}

F32 sqrtf(F32 x) {
    if (x <= 0.0f) return 0.0f;
    F32 r = x;
    /* 10 iterations is generally plenty for float precision */
    for (int i = 0; i < 10; i++) {
        r = 0.5f * (r + x / r);
    }
    return r;
}

/* FIX: Previous version cast to (I32). 
   Floats can represent values larger than I32_MAX.
   Also, large IEEE 754 floats have no fractional part anyway.
*/
#define FLOAT_PRECISION_LIMIT (1 << 23) // Mantissa limit

F32 floorf(F32 x) {
    // If number is huge, it has no decimals, so it equals itself.
    if (fabsf(x) >= FLOAT_PRECISION_LIMIT) return x;
    
    I32 i = (I32)x;
    // Fix truncation direction for negative numbers
    if (x < 0.0f && x != (F32)i) {
        return (F32)(i - 1);
    }
    return (F32)i;
}

F32 ceilf(F32 x) {
    if (fabsf(x) >= FLOAT_PRECISION_LIMIT) return x;

    I32 i = (I32)x;
    if (x > 0.0f && x != (F32)i) {
        return (F32)(i + 1);
    }
    return (F32)i;
}

F32 roundf(F32 x) {
    return (x >= 0.0f) ? floorf(x + 0.5f) : ceilf(x - 0.5f);
}

/* Trig (simple approximations) */

F32 sinf(F32 x) {
    // 1. Reduce to [-TAU, TAU] to prevent overflow
    if (x > TAU || x < -TAU) {
        x -= (F32)((I32)(x / TAU)) * TAU;
    }
    
    // 2. Reduce to [-PI, PI]
    if (x > PI)  x -= TAU;
    if (x < -PI) x += TAU;

    // 3. Mirror quadrants to [-PI/2, PI/2]
    // sin(PI - x) = sin(x), so if x > PI/2, we map it back
    if (x > HALF_PI) {
        x = PI - x;
    } else if (x < -HALF_PI) {
        x = -PI - x;
    }

    // 4. Taylor Series (Optimized for range [-1.57, 1.57])
    // x - x^3/6 + x^5/120 - x^7/5040
    F32 x2 = x * x;
    F32 result = x;
    F32 term = x;
    
    term *= -x2 * 0.1666666f; // /6
    result += term;
    
    term *= -x2 * 0.05f;      // /20 (since we already div by 6)
    result += term;
    
    term *= -x2 * 0.0238095f; // /42 (since we already div by 120)
    result += term;

    return result;
}

F32 cosf(F32 x) {
    return sinf(x + HALF_PI);
}

F32 tanf(F32 x) {
    F32 c = cosf(x);
    if (fabsf(c) < EPSILON) return 0.0f; // Avoid divide by zero
    return sinf(x) / c;
}

BOOLEAN range_overlap(U32 a_start, U32 a_len, U32 b_start, U32 b_len) {
    // FIX: Check for overflow before addition
    // The previous code `a_start + a_len` wraps around if close to UINT_MAX
    
    U32 a_end = a_start + a_len;
    U32 b_end = b_start + b_len;

    // Check if lengths cause overflow (invalid range)
    if (a_end < a_start || b_end < b_start) return 0; 

    return !(a_end <= b_start || b_end <= a_start);
}