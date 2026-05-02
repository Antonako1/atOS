#include "harness/test.h"
#include <LIBRARIES/ARGHAND/ARGHAND.h>

/* ============================================================
   ARGHAND_INIT / IS_PRESENT / GET_VALUE / FREE
   ============================================================ */
static int test_arghand_basic_flags(void) {
    U8 *argv[] = {(U8*)"prog", (U8*)"-h", (U8*)"--verbose"};
    U32 argc = 3;

    PU8 help_aliases[] = {(U8*)"-h", (U8*)"--help"};
    PU8 verbose_aliases[] = {(U8*)"-v", (U8*)"--verbose"};

    PPU8 all_aliases[] = {help_aliases, verbose_aliases};
    U32 alias_counts[] = {2, 2};

    ARGHANDLER ah;
    ARGHAND_INIT(&ah, argc, (PPU8)argv, all_aliases, alias_counts, 2);

    TEST_ASSERT(ARGHAND_IS_PRESENT(&ah, (U8*)"-h") == TRUE);
    TEST_ASSERT(ARGHAND_IS_PRESENT(&ah, (U8*)"--help") == TRUE);
    TEST_ASSERT(ARGHAND_IS_PRESENT(&ah, (U8*)"-v") == TRUE);  /* alias of --verbose which is present */
    TEST_ASSERT(ARGHAND_IS_PRESENT(&ah, (U8*)"--verbose") == TRUE);

    ARGHAND_FREE(&ah);
    return 0;
}

static int test_arghand_value_option(void) {
    U8 *argv[] = {(U8*)"prog", (U8*)"-n", (U8*)"Alice"};
    U32 argc = 3;

    PU8 name_aliases[] = {(U8*)"-n", (U8*)"--name"};
    PPU8 all_aliases[] = {name_aliases};
    U32 alias_counts[] = {2};

    ARGHANDLER ah;
    ARGHAND_INIT(&ah, argc, (PPU8)argv, all_aliases, alias_counts, 1);

    TEST_ASSERT(ARGHAND_IS_PRESENT(&ah, (U8*)"-n") == TRUE);
    TEST_ASSERT(ARGHAND_IS_PRESENT(&ah, (U8*)"--name") == TRUE);
    PU8 val = ARGHAND_GET_VALUE(&ah, (U8*)"-n");
    TEST_ASSERT(val != NULLPTR);
    TEST_ASSERT(STRCMP(val, "Alice") == 0);

    ARGHAND_FREE(&ah);
    return 0;
}

static int test_arghand_value_with_flag_after(void) {
    /* Value should not be picked up if next arg starts with '-' */
    U8 *argv[] = {(U8*)"prog", (U8*)"-n", (U8*)"-v"};
    U32 argc = 3;

    PU8 name_aliases[] = {(U8*)"-n"};
    PU8 verbose_aliases[] = {(U8*)"-v"};
    PPU8 all_aliases[] = {name_aliases, verbose_aliases};
    U32 alias_counts[] = {1, 1};

    ARGHANDLER ah;
    ARGHAND_INIT(&ah, argc, (PPU8)argv, all_aliases, alias_counts, 2);

    TEST_ASSERT(ARGHAND_IS_PRESENT(&ah, (U8*)"-n") == TRUE);
    /* -n has no value because -v starts with '-' */
    TEST_ASSERT(ARGHAND_GET_VALUE(&ah, (U8*)"-n") == NULLPTR);
    TEST_ASSERT(ARGHAND_IS_PRESENT(&ah, (U8*)"-v") == TRUE);

    ARGHAND_FREE(&ah);
    return 0;
}

static int test_arghand_missing_option(void) {
    U8 *argv[] = {(U8*)"prog"};
    U32 argc = 1;

    PU8 help_aliases[] = {(U8*)"-h"};
    PPU8 all_aliases[] = {help_aliases};
    U32 alias_counts[] = {1};

    ARGHANDLER ah;
    ARGHAND_INIT(&ah, argc, (PPU8)argv, all_aliases, alias_counts, 1);

    TEST_ASSERT(ARGHAND_IS_PRESENT(&ah, (U8*)"-h") == FALSE);
    TEST_ASSERT(ARGHAND_GET_VALUE(&ah, (U8*)"-h") == NULLPTR);

    ARGHAND_FREE(&ah);
    return 0;
}

static int test_arghand_multiple_values_first_wins(void) {
    /* If same flag appears multiple times, first occurrence wins */
    U8 *argv[] = {(U8*)"prog", (U8*)"-n", (U8*)"Alice", (U8*)"-n", (U8*)"Bob"};
    U32 argc = 5;

    PU8 name_aliases[] = {(U8*)"-n"};
    PPU8 all_aliases[] = {name_aliases};
    U32 alias_counts[] = {1};

    ARGHANDLER ah;
    ARGHAND_INIT(&ah, argc, (PPU8)argv, all_aliases, alias_counts, 1);

    TEST_ASSERT(ARGHAND_IS_PRESENT(&ah, (U8*)"-n") == TRUE);
    PU8 val = ARGHAND_GET_VALUE(&ah, (U8*)"-n");
    TEST_ASSERT(val != NULLPTR);
    TEST_ASSERT(STRCMP(val, "Alice") == 0);

    ARGHAND_FREE(&ah);
    return 0;
}

static int test_arghand_unknown_name_lookup(void) {
    U8 *argv[] = {(U8*)"prog", (U8*)"-h"};
    U32 argc = 2;

    PU8 help_aliases[] = {(U8*)"-h"};
    PPU8 all_aliases[] = {help_aliases};
    U32 alias_counts[] = {1};

    ARGHANDLER ah;
    ARGHAND_INIT(&ah, argc, (PPU8)argv, all_aliases, alias_counts, 1);

    TEST_ASSERT(ARGHAND_IS_PRESENT(&ah, (U8*)"--unknown") == FALSE);

    ARGHAND_FREE(&ah);
    return 0;
}

/* ============================================================
   MAIN
   ============================================================ */
TEST_MAIN("ARGHAND TESTS")
    RUN_TEST(test_arghand_basic_flags);
    RUN_TEST(test_arghand_value_option);
    RUN_TEST(test_arghand_value_with_flag_after);
    RUN_TEST(test_arghand_missing_option);
    RUN_TEST(test_arghand_multiple_values_first_wins);
    RUN_TEST(test_arghand_unknown_name_lookup);
TEST_RETURN
