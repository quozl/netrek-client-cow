/* test_drawing.c — tests for drawing primitives (headless SDL2) */

#include "unity/unity.h"
#include "../Wlib.h"

static W_Window win = NULL;

static void ensure_window(void) {
    if (!win) {
        win = W_MakeWindow("test", 10, 10, 200, 200, NULL, 0, W_Black);
        W_MapWindow(win);
    }
}

static void test_make_window_not_null(void) {
    ensure_window();
    TEST_ASSERT_NOT_NULL(win);
}

static void test_window_width(void) {
    ensure_window();
    TEST_ASSERT_EQUAL_INT(200, W_WindowWidth(win));
}

static void test_window_height(void) {
    ensure_window();
    TEST_ASSERT_EQUAL_INT(200, W_WindowHeight(win));
}

static void test_is_mapped(void) {
    ensure_window();
    TEST_ASSERT_TRUE(W_IsMapped(win));
}

static void test_clear_window_no_crash(void) {
    ensure_window();
    W_ClearWindow(win);
    TEST_PASS();
}

static void test_make_line_no_crash(void) {
    ensure_window();
    W_MakeLine(win, 0, 0, 100, 100, W_White);
    TEST_PASS();
}

static void test_fill_area_no_crash(void) {
    ensure_window();
    W_FillArea(win, 10, 10, 50, 50, W_Red);
    TEST_PASS();
}

static void test_write_circle_no_crash(void) {
    ensure_window();
    W_WriteCircle(win, 100, 100, 30, W_Green);
    TEST_PASS();
}

static void test_write_triangle_no_crash(void) {
    ensure_window();
    W_WriteTriangle(win, 100, 100, 10, 0, W_Yellow);
    TEST_PASS();
}

static void test_flush_no_crash(void) {
    ensure_window();
    W_Flush();
    TEST_PASS();
}

static void test_flush_window_no_crash(void) {
    ensure_window();
    W_FlushWindow(win);
    TEST_PASS();
}

int main(void) {
    W_Initialize(NULL);

    UNITY_BEGIN();
    RUN_TEST(test_make_window_not_null);
    RUN_TEST(test_window_width);
    RUN_TEST(test_window_height);
    RUN_TEST(test_is_mapped);
    RUN_TEST(test_clear_window_no_crash);
    RUN_TEST(test_make_line_no_crash);
    RUN_TEST(test_fill_area_no_crash);
    RUN_TEST(test_write_circle_no_crash);
    RUN_TEST(test_write_triangle_no_crash);
    RUN_TEST(test_flush_no_crash);
    RUN_TEST(test_flush_window_no_crash);
    return UNITY_END();
}
