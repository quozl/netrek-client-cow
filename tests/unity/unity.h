/* unity.h — minimal Unity-compatible test framework.
 * API compatible with ThrowTheSwitch/Unity (MIT license).
 */

#ifndef UNITY_H
#define UNITY_H

#include <stdio.h>
#include <setjmp.h>
#include <string.h>

extern jmp_buf  unity_jmp;
extern int      unity_failed;
extern int      unity_tests;
extern int      unity_asserts;
extern const char *unity_cur_test;

void unity_fail(const char *file, int line, const char *msg);

static inline void UnityBegin(const char *filename) {
    (void)filename;
    unity_failed  = 0;
    unity_tests   = 0;
    unity_asserts = 0;
    printf("---\n");
}

static inline int UnityEnd(void) {
    printf("%d Tests %d Failures %d Ignored\n",
           unity_tests, unity_failed, 0);
    if (unity_failed) printf("FAIL\n");
    else              printf("OK\n");
    return unity_failed ? 1 : 0;
}

#define UNITY_BEGIN()     UnityBegin(__FILE__)
#define UNITY_END()       UnityEnd()

#define RUN_TEST(fn) do { \
    unity_tests++; \
    unity_cur_test = #fn; \
    if (setjmp(unity_jmp) == 0) { (fn)(); printf("PASS: " #fn "\n"); } \
    else { unity_failed++; } \
} while (0)

#define UNITY_TEST_ASSERT(cond, line, msg) do { \
    unity_asserts++; \
    if (!(cond)) unity_fail(__FILE__, line, msg); \
} while (0)

#define TEST_ASSERT_TRUE(c)  UNITY_TEST_ASSERT((c),  __LINE__, #c " is not TRUE")
#define TEST_ASSERT_FALSE(c) UNITY_TEST_ASSERT(!(c), __LINE__, #c " is not FALSE")

#define TEST_ASSERT_NULL(p)     UNITY_TEST_ASSERT((p) == NULL, __LINE__, #p " is not NULL")
#define TEST_ASSERT_NOT_NULL(p) UNITY_TEST_ASSERT((p) != NULL, __LINE__, #p " is NULL")

#define TEST_ASSERT_EQUAL_INT(e,a) UNITY_TEST_ASSERT((e)==(a), __LINE__, \
    "Expected " #e " but was " #a)

#define TEST_ASSERT_GREATER_THAN(t,a) UNITY_TEST_ASSERT((a)>(t), __LINE__, \
    #a " is not greater than " #t)

#define TEST_ASSERT_EQUAL_STRING(e,a) UNITY_TEST_ASSERT(strcmp((e),(a))==0, \
    __LINE__, "String mismatch: " #e " vs " #a)

#define TEST_FAIL_MESSAGE(m) unity_fail(__FILE__, __LINE__, (m))
#define TEST_PASS() do {} while (0)

#endif /* UNITY_H */
