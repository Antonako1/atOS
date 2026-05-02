#include "harness/test.h"
#include <STD/BITMAP.h>

/* ============================================================
   BITMAP_CREATE / BITMAP_SET / BITMAP_GET
   ============================================================ */
static int test_bitmap_create_and_get(void) {
    U8 buf[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    BITMAP bm;
    BITMAP_CREATE(4, buf, &bm);
    TEST_ASSERT(bm.data == buf);
    TEST_ASSERT(bm.size == 4);
    /* MEMZERO should have cleared the buffer */
    for (U32 i = 0; i < 4; i++) TEST_ASSERT(buf[i] == 0);
    return 0;
}

static int test_bitmap_set_and_get_single_bit(void) {
    U8 buf[4] = {0};
    BITMAP bm;
    BITMAP_CREATE(4, buf, &bm);

    TEST_ASSERT(BITMAP_GET(&bm, 0) == 0);
    TEST_ASSERT(BITMAP_SET(&bm, 0, 1) == TRUE);
    TEST_ASSERT(BITMAP_GET(&bm, 0) == 1);

    TEST_ASSERT(BITMAP_SET(&bm, 0, 0) == TRUE);
    TEST_ASSERT(BITMAP_GET(&bm, 0) == 0);
    return 0;
}

static int test_bitmap_set_and_get_various_bits(void) {
    U8 buf[4] = {0};
    BITMAP bm;
    BITMAP_CREATE(4, buf, &bm);

    /* Set bits 0, 7, 8, 15, 31 */
    BITMAP_SET(&bm, 0, 1);
    BITMAP_SET(&bm, 7, 1);
    BITMAP_SET(&bm, 8, 1);
    BITMAP_SET(&bm, 15, 1);
    BITMAP_SET(&bm, 31, 1);

    TEST_ASSERT(BITMAP_GET(&bm, 0) == 1);
    TEST_ASSERT(BITMAP_GET(&bm, 7) == 1);
    TEST_ASSERT(BITMAP_GET(&bm, 8) == 1);
    TEST_ASSERT(BITMAP_GET(&bm, 15) == 1);
    TEST_ASSERT(BITMAP_GET(&bm, 31) == 1);
    TEST_ASSERT(BITMAP_GET(&bm, 1) == 0);
    TEST_ASSERT(BITMAP_GET(&bm, 30) == 0);
    return 0;
}

static int test_bitmap_out_of_bounds(void) {
    U8 buf[1] = {0};
    BITMAP bm;
    BITMAP_CREATE(1, buf, &bm);

    /* 1 byte = 8 bits, index 8 is out of bounds */
    TEST_ASSERT(BITMAP_GET(&bm, 8) == 0);
    TEST_ASSERT(BITMAP_SET(&bm, 8, 1) == FALSE);
    return 0;
}

static int test_bitmap_null_bitmap(void) {
    TEST_ASSERT(BITMAP_GET(NULLPTR, 0) == 0);
    TEST_ASSERT(BITMAP_SET(NULLPTR, 0, 1) == FALSE);
    return 0;
}

static int test_bitmap_all_bits_in_byte(void) {
    U8 buf[1] = {0};
    BITMAP bm;
    BITMAP_CREATE(1, buf, &bm);

    for (U32 i = 0; i < 8; i++) {
        TEST_ASSERT(BITMAP_SET(&bm, i, 1) == TRUE);
    }
    for (U32 i = 0; i < 8; i++) {
        TEST_ASSERT(BITMAP_GET(&bm, i) == 1);
    }
    TEST_ASSERT(buf[0] == 0xFF);
    return 0;
}

static int test_bitmap_cross_byte_boundary(void) {
    U8 buf[2] = {0};
    BITMAP bm;
    BITMAP_CREATE(2, buf, &bm);

    BITMAP_SET(&bm, 7, 1);  /* last bit of first byte */
    BITMAP_SET(&bm, 8, 1);  /* first bit of second byte */

    TEST_ASSERT(buf[0] == 0x80);
    TEST_ASSERT(buf[1] == 0x01);
    return 0;
}

/* ============================================================
   MAIN
   ============================================================ */
TEST_MAIN("BITMAP TESTS")
    RUN_TEST(test_bitmap_create_and_get);
    RUN_TEST(test_bitmap_set_and_get_single_bit);
    RUN_TEST(test_bitmap_set_and_get_various_bits);
    RUN_TEST(test_bitmap_out_of_bounds);
    RUN_TEST(test_bitmap_null_bitmap);
    RUN_TEST(test_bitmap_all_bits_in_byte);
    RUN_TEST(test_bitmap_cross_byte_boundary);
TEST_RETURN
