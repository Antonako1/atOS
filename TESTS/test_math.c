#include "harness/test.h"
#include <STD/MATH.h>

/* ============================================================
   ipow
   ============================================================ */
static int test_ipow_basic(void) {
    TEST_ASSERT(ipow(2, 10) == 1024);
    TEST_ASSERT(ipow(3, 0) == 1);
    TEST_ASSERT(ipow(5, 1) == 5);
    TEST_ASSERT(ipow(10, 3) == 1000);
    return 0;
}

static int test_ipow_negative_exp(void) {
    /* Per implementation: negative exponent returns 0 for integers */
    TEST_ASSERT(ipow(2, -1) == 0);
    return 0;
}

/* ============================================================
   isqrt
   ============================================================ */
static int test_isqrt_perfect_squares(void) {
    TEST_ASSERT(isqrt(0) == 0);
    TEST_ASSERT(isqrt(1) == 1);
    TEST_ASSERT(isqrt(4) == 2);
    TEST_ASSERT(isqrt(9) == 3);
    TEST_ASSERT(isqrt(100) == 10);
    TEST_ASSERT(isqrt(144) == 12);
    return 0;
}

static int test_isqrt_non_perfect(void) {
    TEST_ASSERT(isqrt(2) == 1);
    TEST_ASSERT(isqrt(3) == 1);
    TEST_ASSERT(isqrt(5) == 2);
    TEST_ASSERT(isqrt(10) == 3);
    TEST_ASSERT(isqrt(15) == 3);
    return 0;
}

/* ============================================================
   gcd / lcm
   ============================================================ */
static int test_gcd_basic(void) {
    TEST_ASSERT(gcd(48, 18) == 6);
    TEST_ASSERT(gcd(54, 24) == 6);
    TEST_ASSERT(gcd(0, 5) == 5);
    TEST_ASSERT(gcd(5, 0) == 5);
    return 0;
}

static int test_lcm_basic(void) {
    TEST_ASSERT(lcm(4, 6) == 12);
    TEST_ASSERT(lcm(21, 6) == 42);
    TEST_ASSERT(lcm(0, 5) == 0);
    return 0;
}

/* ============================================================
   is_power_of_two / next_power_of_two
   ============================================================ */
static int test_is_power_of_two(void) {
    TEST_ASSERT(is_power_of_two(1) == TRUE);
    TEST_ASSERT(is_power_of_two(2) == TRUE);
    TEST_ASSERT(is_power_of_two(4) == TRUE);
    TEST_ASSERT(is_power_of_two(1024) == TRUE);
    TEST_ASSERT(is_power_of_two(0) == FALSE);
    TEST_ASSERT(is_power_of_two(3) == FALSE);
    TEST_ASSERT(is_power_of_two(1023) == FALSE);
    return 0;
}

static int test_next_power_of_two(void) {
    TEST_ASSERT(next_power_of_two(0) == 1);
    TEST_ASSERT(next_power_of_two(1) == 1);
    TEST_ASSERT(next_power_of_two(2) == 2);
    TEST_ASSERT(next_power_of_two(3) == 4);
    TEST_ASSERT(next_power_of_two(5) == 8);
    TEST_ASSERT(next_power_of_two(1025) == 2048);
    return 0;
}

/* ============================================================
   sqrtf
   ============================================================ */
static int test_sqrtf_basic(void) {
    F32 r = sqrtf(4.0f);
    TEST_ASSERT(r > 1.99f && r < 2.01f);
    r = sqrtf(2.0f);
    TEST_ASSERT(r > 1.41f && r < 1.42f);
    r = sqrtf(0.0f);
    TEST_ASSERT(r == 0.0f);
    return 0;
}

/* ============================================================
   floorf / ceilf / roundf
   ============================================================ */
static int test_floorf_basic(void) {
    TEST_ASSERT(floorf(3.7f) == 3.0f);
    TEST_ASSERT(floorf(3.1f) == 3.0f);
    TEST_ASSERT(floorf(-3.1f) == -4.0f);
    TEST_ASSERT(floorf(-3.7f) == -4.0f);
    TEST_ASSERT(floorf(3.0f) == 3.0f);
    return 0;
}

static int test_ceilf_basic(void) {
    TEST_ASSERT(ceilf(3.1f) == 4.0f);
    TEST_ASSERT(ceilf(3.7f) == 4.0f);
    TEST_ASSERT(ceilf(-3.1f) == -3.0f);
    TEST_ASSERT(ceilf(-3.7f) == -3.0f);
    TEST_ASSERT(ceilf(3.0f) == 3.0f);
    return 0;
}

static int test_roundf_basic(void) {
    TEST_ASSERT(roundf(3.4f) == 3.0f);
    TEST_ASSERT(roundf(3.5f) == 4.0f);
    TEST_ASSERT(roundf(3.6f) == 4.0f);
    TEST_ASSERT(roundf(-3.4f) == -3.0f);
    TEST_ASSERT(roundf(-3.5f) == -4.0f);
    TEST_ASSERT(roundf(-3.6f) == -4.0f);
    return 0;
}

/* ============================================================
   sinf / cosf / tanf
   ============================================================ */
static int test_sinf_basic(void) {
    F32 r = sinf(0.0f);
    TEST_ASSERT(fabsf(r) < 0.001f);
    r = sinf(HALF_PI);
    TEST_ASSERT(r > 0.99f && r < 1.01f);
    r = sinf(PI);
    TEST_ASSERT(fabsf(r) < 0.01f);
    return 0;
}

static int test_cosf_basic(void) {
    F32 r = cosf(0.0f);
    TEST_ASSERT(r > 0.99f && r < 1.01f);
    r = cosf(HALF_PI);
    TEST_ASSERT(fabsf(r) < 0.1f); /* cos(PI/2) should be ~0 */
    r = cosf(PI);
    TEST_ASSERT(r < -0.99f);
    return 0;
}

static int test_tanf_basic(void) {
    F32 r = tanf(0.0f);
    TEST_ASSERT(fabsf(r) < 0.001f);
    r = tanf(PI / 4.0f);
    TEST_ASSERT(r > 0.99f && r < 1.01f);
    return 0;
}

static int test_tanf_near_singularity(void) {
    /* tanf(PI/2) should avoid divide-by-zero and return 0 per implementation */
    F32 r = tanf(HALF_PI);
    TEST_ASSERT(r == 0.0f);
    return 0;
}

/* ============================================================
   range_overlap
   ============================================================ */
static int test_range_overlap_basic(void) {
    TEST_ASSERT(range_overlap(0, 10, 5, 10) == TRUE);
    TEST_ASSERT(range_overlap(0, 10, 10, 10) == FALSE); /* [0,10) and [10,20) don't overlap */
    TEST_ASSERT(range_overlap(0, 10, 15, 10) == FALSE);
    TEST_ASSERT(range_overlap(5, 10, 0, 10) == TRUE);
    return 0;
}

/* ============================================================
   powf_i
   ============================================================ */
static int test_powf_i_basic(void) {
    TEST_ASSERT(powf_i(2.0f, 10) == 1024.0f);
    TEST_ASSERT(powf_i(2.0f, 0) == 1.0f);
    TEST_ASSERT(powf_i(2.0f, -1) == 0.5f);
    return 0;
}

/* ============================================================
   MAIN
   ============================================================ */
TEST_MAIN("MATH TESTS")
    RUN_TEST(test_ipow_basic);
    RUN_TEST(test_ipow_negative_exp);
    RUN_TEST(test_isqrt_perfect_squares);
    RUN_TEST(test_isqrt_non_perfect);
    RUN_TEST(test_gcd_basic);
    RUN_TEST(test_lcm_basic);
    RUN_TEST(test_is_power_of_two);
    RUN_TEST(test_next_power_of_two);
    RUN_TEST(test_sqrtf_basic);
    RUN_TEST(test_floorf_basic);
    RUN_TEST(test_ceilf_basic);
    RUN_TEST(test_roundf_basic);
    RUN_TEST(test_sinf_basic);
    RUN_TEST(test_cosf_basic);
    RUN_TEST(test_tanf_basic);
    RUN_TEST(test_tanf_near_singularity);
    RUN_TEST(test_range_overlap_basic);
    RUN_TEST(test_powf_i_basic);
TEST_RETURN
