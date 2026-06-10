/* test_text.c — tests for text rendering and font metrics */

#include "unity/unity.h"
#include "../Wlib.h"

static W_Window win = NULL;

static void test_textwidth_nonzero(void) {
    TEST_ASSERT_GREATER_THAN(0, W_Textwidth);
}

static void test_textheight_nonzero(void) {
    TEST_ASSERT_GREATER_THAN(0, W_Textheight);
}

static void test_bigtextwidth_nonzero(void) {
    TEST_ASSERT_GREATER_THAN(0, W_BigTextwidth);
}

static void test_bigtextheight_nonzero(void) {
    TEST_ASSERT_GREATER_THAN(0, W_BigTextheight);
}

static void test_regular_font_not_null(void) {
    TEST_ASSERT_NOT_NULL(W_RegularFont);
}

static void test_big_font_not_null(void) {
    TEST_ASSERT_NOT_NULL(W_BigFont);
}

static void test_write_text_no_crash(void) {
    if (!win) {
        win = W_MakeWindow("text_win", 0, 0, 400, 300, NULL, 0, W_Black);
        W_MapWindow(win);
    }
    W_WriteText(win, 10, 10, W_White, "Hello", 5, W_RegularFont);
    TEST_PASS();
}

static void test_write_text_big_font_no_crash(void) {
    if (!win) {
        win = W_MakeWindow("text_win2", 0, 0, 400, 300, NULL, 0, W_Black);
        W_MapWindow(win);
    }
    W_WriteText(win, 10, 10, W_Yellow, "BIG", 3, W_BigFont);
    TEST_PASS();
}

static void test_tts_text_height_matches_textheight(void) {
    TEST_ASSERT_EQUAL_INT(W_Textheight, W_TTSTextHeight());
}

static void test_tts_text_width_proportional(void) {
    int w4 = W_TTSTextWidth("abcd", 4);
    int w2 = W_TTSTextWidth("ab",   2);
    TEST_ASSERT_EQUAL_INT(w4, w2 * 2);
}

static void test_mask_text_no_crash(void) {
    if (!win) {
        win = W_MakeWindow("text_win3", 0, 0, 400, 300, NULL, 0, W_Black);
        W_MapWindow(win);
    }
    W_MaskText(win, 5, 5, W_Cyan, "Mask", 4, W_RegularFont);
    TEST_PASS();
}

int main(void) {
    W_Initialize(NULL);

    UNITY_BEGIN();
    RUN_TEST(test_textwidth_nonzero);
    RUN_TEST(test_textheight_nonzero);
    RUN_TEST(test_bigtextwidth_nonzero);
    RUN_TEST(test_bigtextheight_nonzero);
    RUN_TEST(test_regular_font_not_null);
    RUN_TEST(test_big_font_not_null);
    RUN_TEST(test_write_text_no_crash);
    RUN_TEST(test_write_text_big_font_no_crash);
    RUN_TEST(test_tts_text_height_matches_textheight);
    RUN_TEST(test_tts_text_width_proportional);
    RUN_TEST(test_mask_text_no_crash);
    return UNITY_END();
}
