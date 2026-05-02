#include "harness/test.h"
#include <STD/MEM.h>

/* ============================================================
   MEMCPY
   ============================================================ */
static int test_memcpy_basic(void) {
    U8 src[] = "hello world";
    U8 dst[32] = {0};
    MEMCPY(dst, src, 12);
    TEST_ASSERT(dst[0] == 'h');
    TEST_ASSERT(dst[11] == '\0');
    TEST_ASSERT(dst[5] == ' ');
    return 0;
}

static int test_memcpy_zero_size(void) {
    U8 src[] = "abc";
    U8 dst[] = "xyz";
    MEMCPY(dst, src, 0);
    TEST_ASSERT(dst[0] == 'x');
    return 0;
}

static int test_memcpy_return_value(void) {
    U8 dst[8];
    U8 *r = MEMCPY(dst, "test", 5);
    TEST_ASSERT(r == dst);
    return 0;
}

/* ============================================================
   MEMSET
   ============================================================ */
static int test_memset_basic(void) {
    U8 buf[16];
    MEMSET(buf, 0xAB, 16);
    for (U32 i = 0; i < 16; i++) {
        TEST_ASSERT(buf[i] == 0xAB);
    }
    return 0;
}

static int test_memset_zero_size(void) {
    U8 buf[] = "unchanged";
    MEMSET(buf, 'X', 0);
    TEST_ASSERT(buf[0] == 'u');
    return 0;
}

static int test_memset_return_value(void) {
    U8 buf[4];
    U8 *r = MEMSET(buf, 0, 4);
    TEST_ASSERT(r == buf);
    return 0;
}

/* ============================================================
   MEMCMP
   ============================================================ */
static int test_memcmp_equal(void) {
    U8 a[] = {1, 2, 3, 4};
    U8 b[] = {1, 2, 3, 4};
    TEST_ASSERT(MEMCMP(a, b, 4) == 0);
    return 0;
}

static int test_memcmp_less(void) {
    U8 a[] = {1, 2, 3, 4};
    U8 b[] = {1, 2, 4, 4};
    TEST_ASSERT(MEMCMP(a, b, 4) == -1);
    return 0;
}

static int test_memcmp_greater(void) {
    U8 a[] = {1, 2, 4, 4};
    U8 b[] = {1, 2, 3, 4};
    TEST_ASSERT(MEMCMP(a, b, 4) == 1);
    return 0;
}

static int test_memcmp_first_byte(void) {
    U8 a[] = {0, 2, 3};
    U8 b[] = {1, 2, 3};
    TEST_ASSERT(MEMCMP(a, b, 3) == -1);
    return 0;
}

static int test_memcmp_zero_size(void) {
    U8 a[] = {1, 2, 3};
    U8 b[] = {9, 8, 7};
    TEST_ASSERT(MEMCMP(a, b, 0) == 0);
    return 0;
}

/* ============================================================
   MEMMOVE
   ============================================================ */
static int test_memmove_basic(void) {
    U8 src[] = "hello world";
    U8 dst[32] = {0};
    MEMMOVE(dst, src, 12);
    TEST_ASSERT(dst[0] == 'h');
    TEST_ASSERT(dst[11] == '\0');
    return 0;
}

static int test_memmove_overlap_forward(void) {
    /* Overlapping: copy from beginning to later position */
    U8 buf[] = "abcdef";
    MEMMOVE(buf + 2, buf, 4); /* copy "abcd" to position 2 */
    TEST_ASSERT(buf[2] == 'a');
    TEST_ASSERT(buf[3] == 'b');
    TEST_ASSERT(buf[4] == 'c');
    TEST_ASSERT(buf[5] == 'd');
    return 0;
}

static int test_memmove_overlap_backward(void) {
    /* Overlapping: copy from later to earlier position */
    U8 buf[] = "abcdef";
    MEMMOVE(buf, buf + 2, 4); /* copy "cdef" to position 0 */
    TEST_ASSERT(buf[0] == 'c');
    TEST_ASSERT(buf[1] == 'd');
    TEST_ASSERT(buf[2] == 'e');
    TEST_ASSERT(buf[3] == 'f');
    return 0;
}

static int test_memmove_return_value(void) {
    U8 dst[8];
    U8 *r = MEMMOVE(dst, "test", 5);
    TEST_ASSERT(r == dst);
    return 0;
}

/* ============================================================
   MAlloc / MFree / ReAlloc / CAlloc
   (via SYSCALL stubs -> host malloc)
   ============================================================ */
static int test_malloc_basic(void) {
    U8 *p = MAlloc(16);
    TEST_ASSERT(p != NULLPTR);
    for (U32 i = 0; i < 16; i++) p[i] = (U8)i;
    for (U32 i = 0; i < 16; i++) TEST_ASSERT(p[i] == (U8)i);
    MFree(p);
    return 0;
}

static int test_calloc_zeroes(void) {
    U32 *p = CAlloc(4, sizeof(U32));
    TEST_ASSERT(p != NULLPTR);
    for (U32 i = 0; i < 4; i++) TEST_ASSERT(p[i] == 0);
    MFree(p);
    return 0;
}

static int test_realloc_grow(void) {
    U8 *p = MAlloc(8);
    TEST_ASSERT(p != NULLPTR);
    for (U32 i = 0; i < 8; i++) p[i] = (U8)i;
    U8 *q = ReAlloc(p, 16);
    TEST_ASSERT(q != NULLPTR);
    for (U32 i = 0; i < 8; i++) TEST_ASSERT(q[i] == (U8)i);
    MFree(q);
    return 0;
}

/* ============================================================
   MAIN
   ============================================================ */
TEST_MAIN("MEM TESTS")
    RUN_TEST(test_memcpy_basic);
    RUN_TEST(test_memcpy_zero_size);
    RUN_TEST(test_memcpy_return_value);
    RUN_TEST(test_memset_basic);
    RUN_TEST(test_memset_zero_size);
    RUN_TEST(test_memset_return_value);
    RUN_TEST(test_memcmp_equal);
    RUN_TEST(test_memcmp_less);
    RUN_TEST(test_memcmp_greater);
    RUN_TEST(test_memcmp_first_byte);
    RUN_TEST(test_memcmp_zero_size);
    RUN_TEST(test_memmove_basic);
    RUN_TEST(test_memmove_overlap_forward);
    RUN_TEST(test_memmove_overlap_backward);
    RUN_TEST(test_memmove_return_value);
    RUN_TEST(test_malloc_basic);
    RUN_TEST(test_calloc_zeroes);
    RUN_TEST(test_realloc_grow);
TEST_RETURN
