/* test_window_mgmt.c — tests for window lifecycle management */

#include "unity/unity.h"
#include "../Wlib.h"

static void test_make_window_not_null(void) {
    W_Window w2 = W_MakeWindow("mgmt_test", 0, 0, 100, 80, NULL, 0, W_Black);
    TEST_ASSERT_NOT_NULL(w2);
    W_DestroyWindow(w2);
}

static void test_map_unmap_cycle(void) {
    W_Window w2 = W_MakeWindow("map_test", 0, 0, 50, 50, NULL, 0, W_Black);
    TEST_ASSERT_NOT_NULL(w2);
    TEST_ASSERT_FALSE(W_IsMapped(w2));
    W_MapWindow(w2);
    TEST_ASSERT_TRUE(W_IsMapped(w2));
    W_UnmapWindow(w2);
    TEST_ASSERT_FALSE(W_IsMapped(w2));
    W_DestroyWindow(w2);
}

static void test_resize_window(void) {
    W_Window w2 = W_MakeWindow("resize_test", 0, 0, 100, 100, NULL, 0, W_Black);
    TEST_ASSERT_NOT_NULL(w2);
    W_ResizeWindow(w2, 200, 150);
    TEST_ASSERT_EQUAL_INT(200, W_WindowWidth(w2));
    TEST_ASSERT_EQUAL_INT(150, W_WindowHeight(w2));
    W_DestroyWindow(w2);
}

static void test_make_text_window_pixel_size(void) {
    /* Text window: pixel width = cols * W_Textwidth + 2*WIN_EDGE */
    W_Window tw = W_MakeTextWindow("text_test", 0, 0, 10, 5, NULL, 0);
    TEST_ASSERT_NOT_NULL(tw);
    int expected_w = 10 * W_Textwidth + 5 * 2;   /* WIN_EDGE=5 */
    TEST_ASSERT_EQUAL_INT(expected_w, W_WindowWidth(tw));
    W_DestroyWindow(tw);
}

static void test_make_scrolling_window(void) {
    W_Window sw = W_MakeScrollingWindow("scroll_test", 0, 0, 40, 10, NULL, 0);
    TEST_ASSERT_NOT_NULL(sw);
    W_DestroyWindow(sw);
}

static void test_make_menu(void) {
    W_Window mw = W_MakeMenu("menu_test", 0, 0, 20, 5, NULL, 0);
    TEST_ASSERT_NOT_NULL(mw);
    W_DestroyWindow(mw);
}

int main(void) {
    W_Initialize(NULL);

    UNITY_BEGIN();
    RUN_TEST(test_make_window_not_null);
    RUN_TEST(test_map_unmap_cycle);
    RUN_TEST(test_resize_window);
    RUN_TEST(test_make_text_window_pixel_size);
    RUN_TEST(test_make_scrolling_window);
    RUN_TEST(test_make_menu);
    return UNITY_END();
}
