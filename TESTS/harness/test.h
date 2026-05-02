#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

/* Forward declare printf to avoid pulling in host size_t before TYPEDEF.h */
extern int printf(const char *format, ...);

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("  FAIL at %s:%d  %s\n", __FILE__, __LINE__, #cond); \
            return 1; \
        } \
    } while(0)

#define RUN_TEST(fn) \
    do { \
        printf("  %-45s ", #fn); \
        if (fn() == 0) { \
            printf("PASS\n"); \
            passed++; \
        } else { \
            printf("FAIL\n"); \
            failed++; \
        } \
    } while(0)

#define TEST_MAIN(name) \
    int main(void) { \
        int passed = 0, failed = 0; \
        printf("=== " name " ===\n");

#define TEST_RETURN \
    printf("\nResults: %d passed, %d failed\n", passed, failed); \
    return failed; \
}

#endif
