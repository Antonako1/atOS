#include <STD/MATH.h>
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
I32 ceil_to_multiple(I32 x, I32 n) {
    return ((x + n - 1) / n) * n;
}


BOOLEAN range_overlap(U32 a_start, U32 a_len, U32 b_start, U32 b_len) {
    U32 a_end = a_start + a_len;
    U32 b_end = b_start + b_len;
    return !(a_end <= b_start || b_end <= a_start);
}
