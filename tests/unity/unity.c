/* unity.c — minimal Unity-compatible test framework implementation */

#include "unity.h"
#include <stdio.h>
#include <stdlib.h>

jmp_buf     unity_jmp;
int         unity_failed  = 0;
int         unity_tests   = 0;
int         unity_asserts = 0;
const char *unity_cur_test = "(unknown)";

void unity_fail(const char *file, int line, const char *msg) {
    fprintf(stderr, "FAIL: %s:%d: %s: %s\n",
            file, line, unity_cur_test, msg);
    longjmp(unity_jmp, 1);
}
