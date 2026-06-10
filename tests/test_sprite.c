/* test_sprite.c — tests for W_StoreBitmap / W_WriteBitmap / W_OverlayBitmap */

#include "unity/unity.h"
#include "../Wlib.h"

/* A minimal 8×8 XBM bitmap (filled pattern) */
#define BM_W 8
#define BM_H 8
static char bm_data[8] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static W_Window win = NULL;

static void test_store_bitmap_returns_non_null(void) {
    if (!win) win = W_MakeWindow("sprite_win", 0, 0, 200, 200, NULL, 0, W_Black);
    W_Icon icon = W_StoreBitmap(BM_W, BM_H, bm_data, win);
    TEST_ASSERT_NOT_NULL(icon);
}

static void test_write_bitmap_no_crash(void) {
    if (!win) win = W_MakeWindow("sprite_win2", 0, 0, 200, 200, NULL, 0, W_Black);
    W_MapWindow(win);
    W_Icon icon = W_StoreBitmap(BM_W, BM_H, bm_data, win);
    TEST_ASSERT_NOT_NULL(icon);
    W_WriteBitmap(10, 10, icon, W_White);
    TEST_PASS();
}

static void test_overlay_bitmap_no_crash(void) {
    if (!win) win = W_MakeWindow("sprite_win3", 0, 0, 200, 200, NULL, 0, W_Black);
    W_MapWindow(win);
    W_Icon icon = W_StoreBitmap(BM_W, BM_H, bm_data, win);
    TEST_ASSERT_NOT_NULL(icon);
    W_OverlayBitmap(20, 20, icon, W_Red);
    TEST_PASS();
}

static void test_store_bitmap_zero_size_handled(void) {
    /* Don't crash on edge case */
    if (!win) win = W_MakeWindow("sprite_win4", 0, 0, 200, 200, NULL, 0, W_Black);
    /* We just check it doesn't segfault — result may be NULL */
    W_StoreBitmap(0, 0, bm_data, win);
    TEST_PASS();
}

int main(void) {
    W_Initialize(NULL);

    UNITY_BEGIN();
    RUN_TEST(test_store_bitmap_returns_non_null);
    RUN_TEST(test_write_bitmap_no_crash);
    RUN_TEST(test_overlay_bitmap_no_crash);
    RUN_TEST(test_store_bitmap_zero_size_handled);
    return UNITY_END();
}
