/* test_render_png.c — render bitmaps/shapes to PNG for visual inspection.
 * Run from the project root (font path must resolve).
 * Output files: /tmp/netrek_*.png
 */

#include "unity/unity.h"
#include "../Wlib.h"
#include "../sdl2/sdl2window.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string.h>
#include <stdio.h>

void setUp(void)    {}
void tearDown(void) {}

/* 8x8 checkerboard XBM: alternating pixels each row */
#define CHK_W 8
#define CHK_H 8
static char chk_data[8] = { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 };

/* 16x16 diamond XBM */
#define DIAM_W 16
#define DIAM_H 16
static char diam_data[32] = {
    0x00,0x00, 0x01,0x80, 0x03,0xC0, 0x07,0xE0,
    0x0F,0xF0, 0x1F,0xF8, 0x3F,0xFC, 0x7F,0xFE,
    0x7F,0xFE, 0x3F,0xFC, 0x1F,0xF8, 0x0F,0xF0,
    0x07,0xE0, 0x03,0xC0, 0x01,0x80, 0x00,0x00
};

/* Save the named W_Window's texture to a PNG file */
static int save_window_png(W_Window wv, const char *path) {
    struct sdl2_window *w = sdl2_find_window(wv);
    if (!w || !w->texture) return -1;

    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(
        0, w->width, w->height, 32, SDL_PIXELFORMAT_RGBA8888);
    if (!surf) return -1;

    SDL_SetRenderTarget(sdl2_renderer, w->texture);
    SDL_RenderReadPixels(sdl2_renderer, NULL,
                         SDL_PIXELFORMAT_RGBA8888,
                         surf->pixels, surf->pitch);
    SDL_SetRenderTarget(sdl2_renderer, NULL);

    int rc = IMG_SavePNG(surf, path);
    if (rc != 0) fprintf(stderr, "IMG_SavePNG(%s): %s\n", path, SDL_GetError());
    SDL_FreeSurface(surf);
    return rc;
}

/* ------------------------------------------------------------------ */

static void test_bitmap_composite(void) {
    W_Window win = W_MakeWindow("render_test", 0, 0, 200, 200, NULL, 0, W_Black);
    TEST_ASSERT_NOT_NULL(win);
    W_MapWindow(win);
    W_ClearWindow(win);

    W_Icon chk = W_StoreBitmap(CHK_W, CHK_H, chk_data, win);
    TEST_ASSERT_NOT_NULL(chk);

    /* Row of WriteBitmap in different colors — each must show solid-colored
     * checkerboard with black filling the 0-bits (not transparent). */
    W_WriteBitmap(10, 10, chk, W_White);
    W_WriteBitmap(30, 10, chk, W_Red);
    W_WriteBitmap(50, 10, chk, W_Green);
    W_WriteBitmap(70, 10, chk, W_Yellow);
    W_WriteBitmap(90, 10, chk, W_Cyan);

    /* Both W_WriteBitmap and W_OverlayBitmap are pure overlays (GXor semantics):
     * 0-bits leave destination unchanged; 1-bits draw in foreground color. */
    W_Icon diam = W_StoreBitmap(DIAM_W, DIAM_H, diam_data, win);
    TEST_ASSERT_NOT_NULL(diam);
    W_FillArea(win, 10, 30, 20, 20, W_Red);
    W_OverlayBitmap(10, 30, diam, W_White);   /* red must show through 0-bits */

    W_FillArea(win, 40, 30, 20, 20, W_Red);
    W_WriteBitmap(40, 30, diam, W_White);     /* same: red shows through 0-bits */

    /* Circle: bounding-box convention (x,y=topleft, r=diameter) */
    W_WriteCircle(win, 10, 70, 40, W_Green);  /* center=(30,90) r=20 */

    /* Triangles: t=0 downward (▽), t=1 upward (△) */
    W_WriteTriangle(win, 110, 20, 10, 0, W_Cyan);   /* t=0: tip at (110,20) base at y-10 */
    W_WriteTriangle(win, 140, 20, 10, 1, W_Yellow);  /* t=1: tip at (140,20) base at y+10 */

    /* Tractor-beam dashed line: 1px on / 8px off */
    W_MakeTractLine(win, 110, 50, 190, 50, W_Red);
    W_MakeTractLine(win, 110, 60, 190, 80, W_Red);

    /* Lines */
    W_MakeLine(win, 0, 120, 199, 120, W_White);
    W_MakeLine(win, 100, 0,  100, 119, W_Grey);

    /* Text */
    W_WriteText(win, 5, 130, W_White,  "WriteBitmap: black bg", 21, W_RegularFont);
    W_WriteText(win, 5, 150, W_Yellow, "Overlay: red shows thru", 23, W_RegularFont);
    W_WriteText(win, 5, 170, W_Cyan,   "Circle bounding-box conv", 24, W_RegularFont);

    save_window_png(win, "/tmp/netrek_render_test.png");
    printf("Saved /tmp/netrek_render_test.png\n");
    TEST_PASS();
}

static void test_writeBitmap_overlay_preserves_background(void) {
    /* Confirm: W_WriteBitmap is a pure overlay (GXor semantics).
     * 0-bits must NOT erase the red background — red shows through. */
    W_Window w2 = W_MakeWindow("blit_bg", 0, 0, 50, 50, NULL, 0, W_Black);
    TEST_ASSERT_NOT_NULL(w2);
    W_MapWindow(w2);
    W_FillArea(w2, 0, 0, 50, 50, W_Red);
    W_Icon chk = W_StoreBitmap(CHK_W, CHK_H, chk_data, w2);
    TEST_ASSERT_NOT_NULL(chk);
    W_WriteBitmap(5, 5, chk, W_White);  /* red must show through 0-bits */

    save_window_png(w2, "/tmp/netrek_blit_bg_test.png");
    printf("Saved /tmp/netrek_blit_bg_test.png\n");
    TEST_PASS();
}

static void test_overlay_preserves_background(void) {
    /* Confirm: W_OverlayBitmap does NOT paint 0-bits — red shows through. */
    W_Window w3 = W_MakeWindow("overlay_bg", 0, 0, 50, 50, NULL, 0, W_Black);
    TEST_ASSERT_NOT_NULL(w3);
    W_MapWindow(w3);
    W_FillArea(w3, 0, 0, 50, 50, W_Red);
    W_Icon chk = W_StoreBitmap(CHK_W, CHK_H, chk_data, w3);
    TEST_ASSERT_NOT_NULL(chk);
    W_OverlayBitmap(5, 5, chk, W_White);       /* 0-bits stay red */

    save_window_png(w3, "/tmp/netrek_overlay_test.png");
    printf("Saved /tmp/netrek_overlay_test.png\n");
    TEST_PASS();
}

static void test_masktext_no_background_fill(void) {
    /* W_MaskText must not fill the black background rectangle beneath the text.
     * Fill a window with red, draw a W_MaskText string on top.
     * A pixel BESIDE the text (but within its bounding box) must remain red. */
    W_Window wm = W_MakeWindow("mask_text", 0, 0, 200, 30, NULL, 0, W_Black);
    TEST_ASSERT_NOT_NULL(wm);
    W_MapWindow(wm);
    W_FillArea(wm, 0, 0, 200, 30, W_Red);
    /* Draw text; if W_MaskText fills background, the bg rect will be black. */
    W_MaskText(wm, 5, 5, W_White, "TEST", 4, W_RegularFont);

    /* Read back pixels: find a pixel that was red before but would be black
     * if W_MaskText filled the background.  We sample (5, 5+h) just below
     * the text baseline — still inside the glyph cell but likely in the
     * descender gap which W_WriteText would fill black. */
    struct sdl2_window *sw = sdl2_find_window(wm);
    TEST_ASSERT_NOT_NULL(sw);
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(
        0, 200, 30, 32, SDL_PIXELFORMAT_RGBA8888);
    TEST_ASSERT_NOT_NULL(surf);
    SDL_SetRenderTarget(sdl2_renderer, sw->texture);
    SDL_RenderReadPixels(sdl2_renderer, NULL, SDL_PIXELFORMAT_RGBA8888,
                         surf->pixels, surf->pitch);
    SDL_SetRenderTarget(sdl2_renderer, NULL);

    /* Sample a pixel at (5, 28): well below any text, full background area */
    Uint32 *pixels = (Uint32 *)surf->pixels;
    Uint32 pix = pixels[28 * 200 + 5];
    Uint8 r, g, b, a;
    SDL_GetRGBA(pix, surf->format, &r, &g, &b, &a);
    SDL_FreeSurface(surf);
    save_window_png(wm, "/tmp/netrek_masktext_test.png");
    printf("Saved /tmp/netrek_masktext_test.png  pixel(5,28)=(%d,%d,%d)\n", r, g, b);
    /* Background must be red, not black */
    TEST_ASSERT_GREATER_THAN(128, (int)r);  /* red channel bright */
    TEST_ASSERT_TRUE((int)g < 64);          /* not green */
    TEST_ASSERT_TRUE((int)b < 64);          /* not blue */
}

int main(void) {
    W_Initialize(NULL);

    UNITY_BEGIN();
    RUN_TEST(test_bitmap_composite);
    RUN_TEST(test_writeBitmap_overlay_preserves_background);
    RUN_TEST(test_overlay_preserves_background);
    RUN_TEST(test_masktext_no_background_fill);
    return UNITY_END();
}
