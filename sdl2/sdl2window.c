/* sdl2window.c — SDL2 implementation of the Wlib windowing interface.
 *
 * Implements all functions declared in Wlib.h using SDL2 + SDL_ttf.
 * Architecture: one SDL_Window / SDL_Renderer for the whole game;
 * each W_Window is an SDL_Texture render-target composited in W_Flush().
 *
 * Event loop compatibility: W_Socket() returns the read end of a self-pipe.
 * W_EventsQueuedCk() calls SDL_PumpEvents() and signals the pipe when
 * SDL events are queued.  input.c's select() loop works unchanged.
 */

#include "config.h"

#ifdef HAVE_SDL2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#ifdef SOUND
#include <SDL2/SDL_mixer.h>
#endif

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "sdl2window.h"
#include "sdl2sprite.h"
#include "defaults.h"

/* =========================================================================
 * Global state
 * ========================================================================= */

static SDL_Window   *sdl2_window_handle = NULL;
SDL_Renderer        *sdl2_renderer      = NULL;

/* Self-pipe for select() compatibility */
static int pipe_read_fd  = -1;
static int pipe_write_fd = -1;
static volatile int pipe_signaled = 0;

/* Last known mouse position (window-relative), updated by motion and button
 * events.  Key events use these to aim directional commands at the cursor,
 * mirroring X11 which carries the pointer location in every key event. */
static W_Window sdl2_last_mwin = NULL;
static int      sdl2_last_mx   = 0;
static int      sdl2_last_my   = 0;

#ifdef SOUND
/* Pre-built 880 Hz beep chunk for W_Beep(); NULL if audio unavailable. */
static Mix_Chunk *sdl2_beep_chunk = NULL;
static Sint16    *sdl2_beep_buf   = NULL;
#endif

/* Window hash table */
static struct sdl2_winlist *wintable[SDL2WIN_HASHSIZE];

/* Ordered list of all windows (for W_Flush composite pass) */
#define SDL2_MAX_WINDOWS 256
static struct sdl2_window *winorder[SDL2_MAX_WINDOWS];
static int winorder_count = 0;

/* Fonts: 0=Big(40pt), 1=Regular(10pt), 2=Bold(10pt), 3=Underline(10pt) */
struct sdl2_font sdl2_fonts[SDL2_NFONTS];

/* Buffered event for W_EventsPending lookahead */
static W_Event pending_event;
static int     have_pending = 0;

/* Colour table (indexed by W_Color integer constants) */
SDL_Color sdl2_colortable[SDL2_NCOLORS] = {
    {255, 255, 255, 255},  /* 0 WHITE  */
    {  0,   0,   0, 255},  /* 1 BLACK  */
    {255,   0,   0, 255},  /* 2 RED    */
    {  0, 255,   0, 255},  /* 3 GREEN  */
    {255, 255,   0, 255},  /* 4 YELLOW */
    {  0, 255, 255, 255},  /* 5 CYAN   */
    {100, 100, 100, 255},  /* 6 GREY   */
#ifdef RACE_COLORS
    {255, 100, 100, 255},  /* 7  ROM   */
    {100, 100, 255, 255},  /* 8  KLI   */
    {100, 150, 255, 255},  /* 9  FED   */
    {255, 165,   0, 255},  /* 10 ORI   */
    {200, 200, 200, 255},  /* 11 IND   */
#endif
};

/* Use the game's global backColor (data.h extern) for window clearing. */

/* SDL2 uses whole-window clears each frame (W_FastClear=1) because partial
 * clearzone tracking assumes the same pixel can be at a different position than
 * the circle/line was drawn (e.g. det circle has a pre-existing xy-swap bug). */

/* =========================================================================
 * Wlib global variables (declared extern in Wlib.h)
 * ========================================================================= */

/* Font handles are integer indices encoded as pointers to static ints */
static int _fi0=0, _fi1=1, _fi2=2, _fi3=3;
W_Font W_BigFont       = (W_Font)&_fi0;
W_Font W_RegularFont   = (W_Font)&_fi1;
W_Font W_HighlightFont = (W_Font)&_fi2;
W_Font W_UnderlineFont = (W_Font)&_fi3;
W_Font W_IndyFont      = (W_Font)&_fi1;
W_Font W_MyPlanetFont, W_FriendlyPlanetFont, W_EnemyPlanetFont;

int W_BigTextwidth, W_BigTextheight;
int W_Textwidth, W_Textheight;
int W_FastClear = 1;

W_Color W_White  = 0;
W_Color W_Black  = 1;
W_Color W_Red    = 2;
W_Color W_Green  = 3;
W_Color W_Yellow = 4;
W_Color W_Cyan   = 5;
W_Color W_Grey   = 6;
#ifdef RACE_COLORS
W_Color W_Rom = 7;
W_Color W_Kli = 8;
W_Color W_Fed = 9;
W_Color W_Ori = 10;
W_Color W_Ind = 11;
#endif

/* =========================================================================
 * Hash table helpers
 * ========================================================================= */

static int winhash(struct sdl2_window *w) {
    return (int)((unsigned long)w % SDL2WIN_HASHSIZE);
}

static void win_register(struct sdl2_window *w) {
    int h = winhash(w);
    struct sdl2_winlist *e = malloc(sizeof(*e));
    e->win  = w;
    e->next = wintable[h];
    wintable[h] = e;
    if (winorder_count < SDL2_MAX_WINDOWS)
        winorder[winorder_count++] = w;
}

static void win_deregister(struct sdl2_window *w) {
    int h = winhash(w);
    struct sdl2_winlist **pp = &wintable[h];
    while (*pp) {
        if ((*pp)->win == w) {
            struct sdl2_winlist *e = *pp;
            *pp = e->next;
            free(e);
            break;
        }
        pp = &(*pp)->next;
    }
    for (int i = 0; i < winorder_count; i++) {
        if (winorder[i] == w) {
            memmove(&winorder[i], &winorder[i+1],
                    (winorder_count - i - 1) * sizeof(winorder[0]));
            winorder_count--;
            break;
        }
    }
}

struct sdl2_window *sdl2_find_window(W_Window wv) {
    return (struct sdl2_window *)wv;
}

/* =========================================================================
 * Self-pipe helpers
 * ========================================================================= */

static void pipe_signal(void) {
    if (!pipe_signaled && pipe_write_fd >= 0) {
        char b = 1;
        write(pipe_write_fd, &b, 1);
        pipe_signaled = 1;
    }
}

static void pipe_drain(void) {
    if (pipe_read_fd < 0) return;
    char buf[64];
    while (read(pipe_read_fd, buf, sizeof(buf)) > 0)
        ;
    pipe_signaled = 0;
}

/* =========================================================================
 * Font loading and glyph atlas
 * ========================================================================= */

static const char *font_path(void) {
    /* Look for fonts/ relative to the binary, then in common system paths */
    static char path[512];
    const char *candidates[] = {
        "sdl2/fonts/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/DejaVu/DejaVuSansMono.ttf",
        "/Library/Fonts/Andale Mono.ttf",
        NULL
    };
    for (int i = 0; candidates[i]; i++) {
        FILE *f = fopen(candidates[i], "r");
        if (f) { fclose(f); return candidates[i]; }
    }
    snprintf(path, sizeof(path), "sdl2/fonts/DejaVuSansMono.ttf");
    return path;
}

static int build_glyph_atlas(struct sdl2_font *f, int font_idx) {
    /* Build a texture atlas per colour containing all printable ASCII glyphs
     * laid out horizontally: [space][!][""]...[~] */
    int g_w = f->advance;
    int g_h = f->height;
    int atlas_w = g_w * SDL2_NGLYPH;
    int atlas_h = g_h;

    /* Measure each glyph and fill sdl2_glyph table once (colour-independent) */
    for (int c = SDL2_FIRST_GLYPH; c <= SDL2_LAST_GLYPH; c++) {
        int idx = c - SDL2_FIRST_GLYPH;
        f->glyphs[idx].src.x = idx * g_w;
        f->glyphs[idx].src.y = 0;
        f->glyphs[idx].src.w = g_w;
        f->glyphs[idx].src.h = g_h;
        f->glyphs[idx].advance = g_w;
    }

    for (int ci = 0; ci < SDL2_NCOLORS; ci++) {
        SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(
            0, atlas_w, atlas_h, 32, SDL_PIXELFORMAT_RGBA8888);
        if (!surf) return 0;
        SDL_FillRect(surf, NULL, SDL_MapRGBA(surf->format, 0, 0, 0, 0));

        SDL_Color col = sdl2_colortable[ci];
        char glyph[2] = {0, 0};
        for (int c = SDL2_FIRST_GLYPH; c <= SDL2_LAST_GLYPH; c++) {
            glyph[0] = (char)c;
            SDL_Surface *gs = TTF_RenderText_Blended(f->ttf, glyph, col);
            if (gs) {
                int idx = c - SDL2_FIRST_GLYPH;
                SDL_Rect dst = { idx * g_w, f->height - f->ascent - gs->h, gs->w, gs->h };
                /* align to baseline */
                dst.y = f->ascent - TTF_FontAscent(f->ttf);
                if (dst.y < 0) dst.y = 0;
                SDL_BlitSurface(gs, NULL, surf, &dst);
                SDL_FreeSurface(gs);
            }
        }
        f->atlas[ci] = SDL_CreateTextureFromSurface(sdl2_renderer, surf);
        SDL_FreeSurface(surf);
        if (!f->atlas[ci]) return 0;
        SDL_SetTextureBlendMode(f->atlas[ci], SDL_BLENDMODE_BLEND);
    }
    return 1;
}

static int load_fonts(void) {
    const char *fp = font_path();
    int sizes[SDL2_NFONTS] = {40, 10, 10, 10};

    for (int i = 0; i < SDL2_NFONTS; i++) {
        int style = TTF_STYLE_NORMAL;
        if (i == 2) style = TTF_STYLE_BOLD;
        if (i == 3) style = TTF_STYLE_UNDERLINE;

        sdl2_fonts[i].ttf = TTF_OpenFont(fp, sizes[i]);
        if (!sdl2_fonts[i].ttf) {
            fprintf(stderr, "sdl2window: TTF_OpenFont(%s, %d): %s\n",
                    fp, sizes[i], TTF_GetError());
            return 0;
        }
        TTF_SetFontStyle(sdl2_fonts[i].ttf, style);
        TTF_SetFontHinting(sdl2_fonts[i].ttf, TTF_HINTING_MONO);

        sdl2_fonts[i].height  = TTF_FontHeight(sdl2_fonts[i].ttf);
        sdl2_fonts[i].ascent  = TTF_FontAscent(sdl2_fonts[i].ttf);
        /* For monospace fonts advance == glyph width; use 'M' as reference */
        int minx, maxx, miny, maxy, advance;
        TTF_GlyphMetrics(sdl2_fonts[i].ttf, 'M',
                         &minx, &maxx, &miny, &maxy, &advance);
        sdl2_fonts[i].advance = advance;

        if (!build_glyph_atlas(&sdl2_fonts[i], i)) {
            fprintf(stderr, "sdl2window: glyph atlas failed for font %d\n", i);
            return 0;
        }
    }

    /* Set Wlib global metrics from regular font (index 1) */
    W_Textwidth     = sdl2_fonts[1].advance;
    W_Textheight    = sdl2_fonts[1].height;
    W_BigTextwidth  = sdl2_fonts[0].advance;
    W_BigTextheight = sdl2_fonts[0].height;

    /* Planet fonts all map to regular */
    W_MyPlanetFont       = W_RegularFont;
    W_FriendlyPlanetFont = W_RegularFont;
    W_EnemyPlanetFont    = W_RegularFont;

    return 1;
}

/* font index from opaque handle */
static inline int font_idx(W_Font f) {
    if (!f) return 1;
    return *((int *)f);
}

/* =========================================================================
 * W_Initialize / W_Deinitialize
 * ========================================================================= */

void W_Initialize(char *display_name) {
    (void)display_name; /* SDL2 doesn't need a display string */

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "sdl2window: SDL_Init: %s\n", SDL_GetError());
        exit(1);
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "sdl2window: TTF_Init: %s\n", TTF_GetError());
        exit(1);
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        fprintf(stderr, "sdl2window: IMG_Init PNG: %s\n", IMG_GetError());
        exit(1);
    }

    /* Initial window size — will grow as W_MakeWindow creates windows */
    sdl2_window_handle = SDL_CreateWindow(
        "Netrek",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1100, 700,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!sdl2_window_handle) {
        fprintf(stderr, "sdl2window: SDL_CreateWindow: %s\n", SDL_GetError());
        exit(1);
    }

    sdl2_renderer = SDL_CreateRenderer(
        sdl2_window_handle, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    if (!sdl2_renderer) {
        /* Fallback to software renderer */
        sdl2_renderer = SDL_CreateRenderer(
            sdl2_window_handle, -1,
            SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
    }
    if (!sdl2_renderer) {
        fprintf(stderr, "sdl2window: SDL_CreateRenderer: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_SetRenderDrawBlendMode(sdl2_renderer, SDL_BLENDMODE_BLEND);

    /* Self-pipe for select() compatibility */
    int fds[2];
    if (pipe(fds) == 0) {
        pipe_read_fd  = fds[0];
        pipe_write_fd = fds[1];
        /* Make read end non-blocking so drain doesn't block */
        fcntl(pipe_read_fd,  F_SETFL, O_NONBLOCK);
        fcntl(pipe_write_fd, F_SETFL, O_NONBLOCK);
        /* Close on exec */
        fcntl(pipe_read_fd,  F_SETFD, FD_CLOEXEC);
        fcntl(pipe_write_fd, F_SETFD, FD_CLOEXEC);
    }

    memset(wintable, 0, sizeof(wintable));
    winorder_count = 0;

#ifdef SOUND
    /* Build a short 880 Hz sine wave for W_Beep().  Mix_OpenAudio has not been
     * called yet (that happens in Init_Sound later), so we just pre-allocate the
     * buffer here.  Mix_QuickLoad_RAW borrows the pointer — do not free buf. */
    {
        const int rate = 44100, ms = 200, freq = 880;
        int n = rate * ms / 1000;           /* mono samples */
        sdl2_beep_buf = SDL_malloc(n * 2 * sizeof(Sint16));  /* stereo */
        if (sdl2_beep_buf) {
            for (int i = 0; i < n; i++) {
                double t   = (double)i / rate;
                double env = (i < n * 9 / 10) ? 1.0
                           : (double)(n - i) / (n / 10.0);  /* 10% fade-out */
                Sint16 v = (Sint16)(10000 * env * sin(2.0 * M_PI * freq * t));
                sdl2_beep_buf[i*2]     = v;
                sdl2_beep_buf[i*2 + 1] = v;
            }
            sdl2_beep_chunk = Mix_QuickLoad_RAW(
                (Uint8 *)sdl2_beep_buf, (Uint32)(n * 2 * sizeof(Sint16)));
        }
    }
#endif

    if (!load_fonts()) {
        fprintf(stderr, "sdl2window: font load failed\n");
        exit(1);
    }

    SDL_SetRenderDrawColor(sdl2_renderer, 0, 0, 0, 255);
    SDL_RenderClear(sdl2_renderer);
    SDL_RenderPresent(sdl2_renderer);
}

void W_Deinitialize(void) {
    for (int i = 0; i < SDL2_NFONTS; i++) {
        for (int c = 0; c < SDL2_NCOLORS; c++) {
            if (sdl2_fonts[i].atlas[c])
                SDL_DestroyTexture(sdl2_fonts[i].atlas[c]);
        }
        if (sdl2_fonts[i].ttf)
            TTF_CloseFont(sdl2_fonts[i].ttf);
    }

    for (int i = 0; i < winorder_count; i++) {
        struct sdl2_window *w = winorder[i];
        if (w->texture) SDL_DestroyTexture(w->texture);
        free(w->name);
        /* free type-specific data */
        if (w->type == SDL2WIN_SCROLL && w->data) {
            struct sdl2_scroll *sc = (struct sdl2_scroll *)w->data;
            struct sdl2_string *s = sc->head;
            while (s) { struct sdl2_string *n = s->next; free(s); s = n; }
            free(sc);
        } else if (w->type == SDL2WIN_MENU && w->data) {
            struct sdl2_menuitem *items = (struct sdl2_menuitem *)w->data;
            for (int j = 0; j < w->char_h; j++)
                if (items[j].text) free(items[j].text);
            free(items);
        }
        free(w);
    }
    winorder_count = 0;

    if (pipe_read_fd  >= 0) { close(pipe_read_fd);  pipe_read_fd  = -1; }
    if (pipe_write_fd >= 0) { close(pipe_write_fd); pipe_write_fd = -1; }

#ifdef SOUND
    if (sdl2_beep_chunk) { Mix_FreeChunk(sdl2_beep_chunk); sdl2_beep_chunk = NULL; }
    if (sdl2_beep_buf)   { SDL_free(sdl2_beep_buf);        sdl2_beep_buf   = NULL; }
#endif

    IMG_Quit();
    TTF_Quit();
    if (sdl2_renderer)     SDL_DestroyRenderer(sdl2_renderer);
    if (sdl2_window_handle) SDL_DestroyWindow(sdl2_window_handle);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

/* =========================================================================
 * Window creation helpers
 * ========================================================================= */

#define WIN_EDGE  5
#define MENU_PAD  6
#define MENU_BAR  1

static struct sdl2_window *make_win(const char *name, int x, int y,
                                    int px_w, int px_h, int type) {
    struct sdl2_window *w = calloc(1, sizeof(*w));
    w->name    = strdup(name);
    w->x       = x;
    w->y       = y;
    w->width   = px_w;
    w->height  = px_h;
    w->type    = type;
    w->mapped  = 0;

    w->texture = SDL_CreateTexture(
        sdl2_renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        px_w, px_h);
    if (w->texture) {
        SDL_SetTextureBlendMode(w->texture, SDL_BLENDMODE_BLEND);
        /* Clear to black */
        SDL_SetRenderTarget(sdl2_renderer, w->texture);
        SDL_SetRenderDrawColor(sdl2_renderer, 0, 0, 0, 255);
        SDL_RenderClear(sdl2_renderer);
        SDL_SetRenderTarget(sdl2_renderer, NULL);
    }
    win_register(w);
    return w;
}

/* =========================================================================
 * W_Make* — window creation
 * ========================================================================= */

W_Window W_MakeWindow(char *name, int x, int y, int width, int height,
                      W_Window parent, int border, W_Color color) {
    (void)parent; (void)border; (void)color;
    return (W_Window)make_win(name, x, y, width, height, SDL2WIN_GRAPH);
}

W_Window W_MakeTextWindow(char *name, int x, int y,
                           int cols, int rows,
                           W_Window parent, int border) {
    (void)parent; (void)border;
    int px_w = cols * W_Textwidth  + WIN_EDGE * 2;
    int px_h = rows * W_Textheight + MENU_PAD * 2;
    struct sdl2_window *w = make_win(name, x, y, px_w, px_h, SDL2WIN_TEXT);
    w->char_w = cols;
    w->char_h = rows;
    return (W_Window)w;
}

W_Window W_MakeScrollingWindow(char *name, int x, int y,
                                int cols, int rows,
                                W_Window parent, int border) {
    (void)parent; (void)border;
    int px_w = cols * W_Textwidth  + WIN_EDGE * 2;
    int px_h = rows * W_Textheight + MENU_PAD * 2;
    struct sdl2_window *w = make_win(name, x, y, px_w, px_h, SDL2WIN_SCROLL);
    w->char_w = cols;
    w->char_h = rows;
    struct sdl2_scroll *sc = calloc(1, sizeof(*sc));
    w->data = sc;
    return (W_Window)w;
}

W_Window W_MakeMenu(char *name, int x, int y,
                    int cols, int rows,
                    W_Window parent, int border) {
    (void)parent; (void)border;
    int px_w = cols * W_Textwidth  + WIN_EDGE * 2;
    int px_h = rows * (W_Textheight + MENU_PAD * 2) + (rows - 1) * MENU_BAR;
    struct sdl2_window *w = make_win(name, x, y, px_w, px_h, SDL2WIN_MENU);
    w->char_w = cols;
    w->char_h = rows;
    struct sdl2_menuitem *items = calloc(rows, sizeof(*items));
    w->data = items;
    return (W_Window)w;
}

/* =========================================================================
 * Window lifecycle
 * ========================================================================= */

void W_MapWindow(W_Window wv) {
    struct sdl2_window *w = sdl2_find_window(wv);
    if (w) w->mapped = 1;
}

void W_UnmapWindow(W_Window wv) {
    struct sdl2_window *w = sdl2_find_window(wv);
    if (w) w->mapped = 0;
}

int W_IsMapped(W_Window wv) {
    struct sdl2_window *w = sdl2_find_window(wv);
    return w ? w->mapped : 0;
}

void W_DestroyWindow(W_Window wv) {
    struct sdl2_window *w = sdl2_find_window(wv);
    if (!w) return;
    win_deregister(w);
    if (w->texture) SDL_DestroyTexture(w->texture);
    free(w->name);
    if (w->type == SDL2WIN_SCROLL && w->data) {
        struct sdl2_scroll *sc = (struct sdl2_scroll *)w->data;
        struct sdl2_string *s = sc->head;
        while (s) { struct sdl2_string *n = s->next; free(s); s = n; }
        free(sc);
    } else if (w->type == SDL2WIN_MENU && w->data) {
        struct sdl2_menuitem *items = (struct sdl2_menuitem *)w->data;
        for (int j = 0; j < w->char_h; j++)
            if (items[j].text) free(items[j].text);
        free(items);
    }
    free(w);
}

void W_ResizeWindow(W_Window wv, int neww, int newh) {
    struct sdl2_window *w = sdl2_find_window(wv);
    if (!w) return;
    if (w->texture) SDL_DestroyTexture(w->texture);
    w->width  = neww;
    w->height = newh;
    w->texture = SDL_CreateTexture(
        sdl2_renderer, SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET, neww, newh);
    if (w->texture) SDL_SetTextureBlendMode(w->texture, SDL_BLENDMODE_BLEND);
}

void W_ResizeTextWindow(W_Window wv, int cols, int rows) {
    struct sdl2_window *w = sdl2_find_window(wv);
    if (!w) return;
    w->char_w = cols;
    w->char_h = rows;
    W_ResizeWindow(wv,
        cols * W_Textwidth  + WIN_EDGE * 2,
        rows * W_Textheight + MENU_PAD * 2);
}

int W_WindowWidth(W_Window wv) {
    struct sdl2_window *w = sdl2_find_window(wv);
    return w ? w->width : 0;
}

int W_WindowHeight(W_Window wv) {
    struct sdl2_window *w = sdl2_find_window(wv);
    return w ? w->height : 0;
}

void W_RenameWindow(W_Window wv, char *str) {
    (void)wv; SDL_SetWindowTitle(sdl2_window_handle, str);
}

void W_SetWindowName(W_Window wv, char *name) {
    (void)wv; SDL_SetWindowTitle(sdl2_window_handle, name);
}

void W_SetIconWindow(W_Window main, W_Window icon) { (void)main; (void)icon; }
void W_SetSensitive(W_Window w, int v) { (void)w; (void)v; }

void W_ReinitMenu(W_Window wv, int neww, int newh) {
    struct sdl2_window *w = sdl2_find_window(wv);
    if (!w || w->type != SDL2WIN_MENU) return;
    if (w->data) {
        struct sdl2_menuitem *items = (struct sdl2_menuitem *)w->data;
        for (int j = 0; j < w->char_h; j++)
            if (items[j].text) { free(items[j].text); items[j].text = NULL; }
        free(items);
    }
    w->char_w = neww;
    w->char_h = newh;
    w->data   = calloc(newh, sizeof(struct sdl2_menuitem));
    W_ResizeWindow(wv,
        neww * W_Textwidth  + WIN_EDGE * 2,
        newh * (W_Textheight + MENU_PAD * 2) + (newh - 1) * MENU_BAR);
}

void W_DestroyMenu(W_Window wv) { W_DestroyWindow(wv); }

void W_ResizeMenu(W_Window wv, int neww, int newh) {
    W_ReinitMenu(wv, neww, newh);
}

/* Background / tile — stub (no bg image support in SDL2 backend yet) */
void W_SetBackgroundPixmap(W_Window w, unsigned long bg) { (void)w; (void)bg; }
void W_UnsetBackgroundPixmap(W_Window w) { (void)w; }
void W_TileWindow(W_Window w, W_Icon bit) { (void)w; (void)bit; }
void W_UnTileWindow(W_Window w) { (void)w; }
void W_ChangeBorder(W_Window w, int color) { (void)w; (void)color; }
void W_GalacticBgd(int which) { (void)which; }
void W_LocalBgd(int which) { (void)which; }
void W_SetBackground(W_Window w, int which) { (void)w; (void)which; }
void W_GetPixmaps(W_Window t, W_Window g) { sdl2_GetPixmaps(t, g); }

/* =========================================================================
 * Render target helpers
 * ========================================================================= */

void sdl2_set_target(struct sdl2_window *w) {
    SDL_SetRenderTarget(sdl2_renderer, w ? w->texture : NULL);
}

static inline void set_color(int color) {
    if (color < 0 || color >= SDL2_NCOLORS) color = 0;
    SDL_Color c = sdl2_colortable[color];
    SDL_SetRenderDrawColor(sdl2_renderer, c.r, c.g, c.b, c.a);
}

/* =========================================================================
 * Drawing — clear
 * ========================================================================= */

void W_ClearWindow(W_Window wv) {
    struct sdl2_window *w = sdl2_find_window(wv);
    if (!w || !w->texture) return;
    SDL_SetRenderTarget(sdl2_renderer, w->texture);
    SDL_Color c = sdl2_colortable[(int)backColor < SDL2_NCOLORS ? (int)backColor : 1];
    SDL_SetRenderDrawColor(sdl2_renderer, c.r, c.g, c.b, 255);
    SDL_RenderClear(sdl2_renderer);
    SDL_SetRenderTarget(sdl2_renderer, NULL);
}

void W_ClearArea(W_Window wv, int x, int y, int width, int height) {
    struct sdl2_window *w = sdl2_find_window(wv);
    if (!w || !w->texture) return;
    SDL_SetRenderTarget(sdl2_renderer, w->texture);
    set_color((int)backColor < SDL2_NCOLORS ? (int)backColor : 1);
    SDL_Rect r = {x, y, width, height};
    SDL_RenderFillRect(sdl2_renderer, &r);
    SDL_SetRenderTarget(sdl2_renderer, NULL);
}

void W_CacheClearArea(W_Window wv, int x, int y, int w2, int h2) {
    W_ClearArea(wv, x, y, w2, h2);
}

void W_FlushClearAreaCache(W_Window wv) { (void)wv; }

/* =========================================================================
 * Drawing — fill and lines
 * ========================================================================= */

void W_FillArea(W_Window wv, int x, int y, int width, int height, W_Color color) {
    struct sdl2_window *w = sdl2_find_window(wv);
    if (!w || !w->texture) return;
    SDL_SetRenderTarget(sdl2_renderer, w->texture);
    if (w->type == SDL2WIN_TEXT || w->type == SDL2WIN_MENU) {
        /* Character-grid coords → pixels */
        x      = x * W_Textwidth  + WIN_EDGE;
        y      = y * W_Textheight + MENU_PAD;
        width  = width  * W_Textwidth;
        height = height * W_Textheight;
    }
    set_color(color);
    SDL_Rect r = {x, y, width, height};
    SDL_RenderFillRect(sdl2_renderer, &r);
    SDL_SetRenderTarget(sdl2_renderer, NULL);
}

void W_MakeLine(W_Window wv, int x0, int y0, int x1, int y1, W_Color color) {
    struct sdl2_window *w = sdl2_find_window(wv);
    if (!w || !w->texture) return;
    SDL_SetRenderTarget(sdl2_renderer, w->texture);
    set_color(color);
    SDL_RenderDrawLine(sdl2_renderer, x0, y0, x1, y1);
    SDL_SetRenderTarget(sdl2_renderer, NULL);
}

/* Dashed line for tractor beams */
void W_MakeTractLine(W_Window wv, int x0, int y0, int x1, int y1, W_Color color) {
    struct sdl2_window *w = sdl2_find_window(wv);
    if (!w || !w->texture) return;
    SDL_SetRenderTarget(sdl2_renderer, w->texture);
    set_color(color);
    /* Match X11 LineOnOffDash {1, 8}: 1px on, 8px off → one point per 9px */
    int dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf((float)(dx*dx + dy*dy));
    if (len < 1.0f) { SDL_SetRenderTarget(sdl2_renderer, NULL); return; }
    float ux = dx / len, uy = dy / len;
    for (float t = 0; t < len; t += 9.0f) {
        SDL_RenderDrawPoint(sdl2_renderer,
            (int)(x0 + ux * t), (int)(y0 + uy * t));
    }
    SDL_SetRenderTarget(sdl2_renderer, NULL);
}

void W_MakePhaserLine(W_Window wv, int x0, int y0, int x1, int y1, W_Color color) {
    W_MakeLine(wv, x0, y0, x1, y1, color);
}

/* Cached lines: we just draw immediately (no deferred cache needed) */
void W_CacheLine(W_Window wv, int x0, int y0, int x1, int y1, int color) {
    W_MakeLine(wv, x0, y0, x1, y1, (W_Color)color);
}

void W_FlushLineCaches(W_Window wv) { (void)wv; }

/* =========================================================================
 * Drawing — shapes
 * ========================================================================= */

/* W_WriteCircle matches XDrawArc convention: (x,y) is top-left of bounding
 * box, r is the diameter (not radius).  Center = (x+r/2, y+r/2), radius = r/2. */
void W_WriteCircle(W_Window wv, int bx, int by, int diam, W_Color color) {
    struct sdl2_window *w = sdl2_find_window(wv);
    if (!w || !w->texture) return;
    SDL_SetRenderTarget(sdl2_renderer, w->texture);
    set_color(color);
    int rad = diam / 2;
    int ocx = bx + rad;
    int ocy = by + rad;
    int x = rad, y = 0, err = 0;
    while (x >= y) {
        SDL_RenderDrawPoint(sdl2_renderer, ocx+x, ocy+y);
        SDL_RenderDrawPoint(sdl2_renderer, ocx+y, ocy+x);
        SDL_RenderDrawPoint(sdl2_renderer, ocx-y, ocy+x);
        SDL_RenderDrawPoint(sdl2_renderer, ocx-x, ocy+y);
        SDL_RenderDrawPoint(sdl2_renderer, ocx-x, ocy-y);
        SDL_RenderDrawPoint(sdl2_renderer, ocx-y, ocy-x);
        SDL_RenderDrawPoint(sdl2_renderer, ocx+y, ocy-x);
        SDL_RenderDrawPoint(sdl2_renderer, ocx+x, ocy-y);
        y++;
        err += 2*y + 1;
        if (err > x) { x--; err -= 2*x; }
    }
    SDL_SetRenderTarget(sdl2_renderer, NULL);
}

void W_WriteTriangle(W_Window wv, int x, int y, int s, int t, W_Color color) {
    struct sdl2_window *w = sdl2_find_window(wv);
    if (!w || !w->texture) return;
    SDL_SetRenderTarget(sdl2_renderer, w->texture);
    set_color(color);
    SDL_Point pts[4];
    /* Match X11: t=0 → ▽ tip at (x,y) base above at y-s; t=1 → △ base below */
    if (t == 0) {
        pts[0].x = x;     pts[0].y = y;
        pts[1].x = x + s; pts[1].y = y - s;
        pts[2].x = x - s; pts[2].y = y - s;
    } else {
        pts[0].x = x;     pts[0].y = y;
        pts[1].x = x + s; pts[1].y = y + s;
        pts[2].x = x - s; pts[2].y = y + s;
    }
    pts[3] = pts[0];
    SDL_RenderDrawLines(sdl2_renderer, pts, 4);
    SDL_SetRenderTarget(sdl2_renderer, NULL);
}

void W_Halo(int x, int y, W_Color color) { (void)x; (void)y; (void)color; }

/* =========================================================================
 * Drawing — text
 * ========================================================================= */

#define TEXT_XOFF WIN_EDGE
#define TEXT_YOFF MENU_PAD

/* fill_bg=1: XDrawImageString — fill background before drawing glyphs (W_WriteText).
 * fill_bg=0: XDrawString — transparent, no background fill (W_MaskText). */
static void draw_string(struct sdl2_window *w, int px, int py,
                        int color, const char *str, int len, int fidx, int fill_bg) {
    if (!str || len == 0) return;
    if (len < 0) len = (int)strlen(str);
    if (color < 0 || color >= SDL2_NCOLORS) color = 0;
    if (fidx < 0 || fidx >= SDL2_NFONTS) fidx = 1;

    struct sdl2_font *f = &sdl2_fonts[fidx];
    SDL_Texture *atlas = f->atlas[color];
    if (!atlas) return;

    if (fill_bg) {
        SDL_SetRenderDrawColor(sdl2_renderer, 0, 0, 0, 255);
        SDL_Rect bg = { px, py, len * f->advance, f->height };
        SDL_RenderFillRect(sdl2_renderer, &bg);
    }

    int x = px;
    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
        if (c < SDL2_FIRST_GLYPH || c > SDL2_LAST_GLYPH) { x += f->advance; continue; }
        int idx = c - SDL2_FIRST_GLYPH;
        SDL_Rect src = f->glyphs[idx].src;
        SDL_Rect dst = { x, py, f->advance, f->height };
        SDL_RenderCopy(sdl2_renderer, atlas, &src, &dst);
        x += f->advance;
    }
}

void W_WriteText(W_Window wv, int x, int y, W_Color color,
                 char *str, int len, W_Font font) {
    struct sdl2_window *w = sdl2_find_window(wv);
    if (!w || !w->texture) return;
    int fidx = font_idx(font);

    SDL_SetRenderTarget(sdl2_renderer, w->texture);

    int px, py;
    if (w->type == SDL2WIN_GRAPH) {
        px = x; py = y;
    } else if (w->type == SDL2WIN_TEXT || w->type == SDL2WIN_MENU) {
        px = x * W_Textwidth  + TEXT_XOFF;
        py = y * W_Textheight + TEXT_YOFF;
    } else if (w->type == SDL2WIN_SCROLL) {
        /* For scrolling windows x/y are pixel offsets from top-left */
        px = TEXT_XOFF;
        struct sdl2_scroll *sc = (struct sdl2_scroll *)w->data;
        /* Append to scroll buffer and redraw the last visible rows */
        if (len < 0) len = (int)strlen(str);
        struct sdl2_string *s = calloc(1, sizeof(*s));
        strncpy(s->text, str, SDL2_MAX_TEXT_WIDTH - 1);
        s->color = color;
        s->font  = fidx;
        s->prev  = NULL;
        s->next  = sc->head;
        if (sc->head) sc->head->prev = s;
        sc->head = s;
        if (!sc->tail) sc->tail = s;
        sc->lines++;
        /* Trim old lines */
        while (sc->lines > 100) {
            struct sdl2_string *old = sc->tail;
            sc->tail = old->prev;
            if (sc->tail) sc->tail->next = NULL;
            free(old);
            sc->lines--;
        }
        SDL_SetRenderTarget(sdl2_renderer, NULL);
        W_FlushScrollingWindow(wv);
        return;
    } else {
        px = x; py = y;
    }

    draw_string(w, px, py, (int)color, str, len, fidx, 1 /* fill_bg */);
    SDL_SetRenderTarget(sdl2_renderer, NULL);
}

void W_MaskText(W_Window wv, int x, int y, W_Color color,
                char *str, int len, W_Font font) {
    /* XDrawString: draws glyphs only, no background fill (unlike W_WriteText). */
    struct sdl2_window *w = sdl2_find_window(wv);
    if (!w || !w->texture) return;
    int fidx = font_idx(font);
    SDL_SetRenderTarget(sdl2_renderer, w->texture);
    int px, py;
    if (w->type == SDL2WIN_TEXT || w->type == SDL2WIN_MENU) {
        px = x * W_Textwidth  + TEXT_XOFF;
        py = y * W_Textheight + TEXT_YOFF;
    } else {
        px = x; py = y;
    }
    draw_string(w, px, py, (int)color, str, len, fidx, 0 /* no fill_bg */);
    SDL_SetRenderTarget(sdl2_renderer, NULL);
}

void W_FlushScrollingWindow(W_Window wv) {
    struct sdl2_window *w = sdl2_find_window(wv);
    if (!w || w->type != SDL2WIN_SCROLL || !w->texture) return;
    struct sdl2_scroll *sc = (struct sdl2_scroll *)w->data;

    SDL_SetRenderTarget(sdl2_renderer, w->texture);
    SDL_SetRenderDrawColor(sdl2_renderer, 0, 0, 0, 255);
    SDL_RenderClear(sdl2_renderer);

    int rows = w->char_h;
    int row_h = W_Textheight;
    /* Collect the last `rows` lines from the linked list */
    struct sdl2_string *lines[200];
    int n = 0;
    for (struct sdl2_string *s = sc->head; s && n < rows; s = s->next)
        lines[n++] = s;
    /* Draw from bottom (oldest visible at top) */
    for (int i = n - 1; i >= 0; i--) {
        int py = (n - 1 - i) * row_h + TEXT_YOFF;
        draw_string(w, TEXT_XOFF, py,
                    lines[i]->color, lines[i]->text, -1, lines[i]->font, 1);
    }
    SDL_SetRenderTarget(sdl2_renderer, NULL);
}

/* TTS text (beeplite) — use regular font metrics */
int W_TTSTextHeight(void) { return W_Textheight; }

int W_TTSTextWidth(char *s, int l) {
    if (!s) return 0;
    if (l < 0) l = (int)strlen(s);
    return l * W_Textwidth;
}

void W_EraseTTSText(W_Window w, int max_width, int y, int width) {
    W_ClearArea(w, 0, y, max_width, W_Textheight);
    (void)width;
}

void W_WriteTTSText(W_Window w, int max_width, int y, int width,
                    char *str, int len) {
    (void)max_width; (void)width;
    W_WriteText(w, 0, y, W_White, str, len, W_RegularFont);
}

/* =========================================================================
 * Bitmaps (monochrome icons, W_StoreBitmap / W_WriteBitmap)
 * ========================================================================= */

W_Icon W_StoreBitmap(int width, int height, char *data, W_Window wv) {
    /* Convert XBM 1-bit data to an RGBA surface: set=white on transparent */
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(
        0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
    if (!surf) return NULL;
    SDL_FillRect(surf, NULL, SDL_MapRGBA(surf->format, 0, 0, 0, 0));

    int row_bytes = (width + 7) / 8;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int byte_idx = y * row_bytes + x / 8;
            int bit      = (data[byte_idx] >> (x & 7)) & 1;
            if (bit) {
                /* We store as white; W_WriteBitmap applies color via ColorMod */
                Uint32 *px = (Uint32 *)((Uint8 *)surf->pixels
                             + y * surf->pitch + x * 4);
                *px = SDL_MapRGBA(surf->format, 255, 255, 255, 255);
            }
        }
    }

    struct sdl2_icon *icon = malloc(sizeof(*icon));
    icon->width   = width;
    icon->height  = height;
    icon->target  = sdl2_find_window(wv);
    icon->texture = SDL_CreateTextureFromSurface(sdl2_renderer, surf);
    SDL_FreeSurface(surf);
    if (icon->texture)
        SDL_SetTextureBlendMode(icon->texture, SDL_BLENDMODE_BLEND);
    return (W_Icon)icon;
}

static void write_bitmap_impl(int x, int y, W_Icon bit,
                               W_Color color, int overlay) {
    struct sdl2_icon   *icon = (struct sdl2_icon *)bit;
    struct sdl2_window *w    = icon ? icon->target : NULL;
    if (!icon || !icon->texture || !w || !w->texture) return;

    /* X11 BITGC uses GXor — 0-bits leave destination unchanged, 1-bits draw
     * the foreground color.  Both W_WriteBitmap and W_OverlayBitmap behave the
     * same: pure overlay with BLEND.  No background fill — that creates cutouts
     * when a bitmap is drawn over other content (e.g. explosion over a planet). */
    (void)overlay;
    SDL_Color c = sdl2_colortable[(int)color < SDL2_NCOLORS ? (int)color : 0];
    SDL_SetTextureColorMod(icon->texture, c.r, c.g, c.b);
    SDL_SetTextureAlphaMod(icon->texture, 255);
    SDL_SetTextureBlendMode(icon->texture, SDL_BLENDMODE_BLEND);

    SDL_SetRenderTarget(sdl2_renderer, w->texture);
    SDL_Rect dst = {x, y, icon->width, icon->height};
    SDL_RenderCopy(sdl2_renderer, icon->texture, NULL, &dst);
    SDL_SetRenderTarget(sdl2_renderer, NULL);

    SDL_SetTextureColorMod(icon->texture, 255, 255, 255);
}

void W_WriteBitmap(int x, int y, W_Icon bit, W_Color color) {
    write_bitmap_impl(x, y, bit, color, 0);
}

void W_OverlayBitmap(int x, int y, W_Icon bit, W_Color color) {
    write_bitmap_impl(x, y, bit, color, 1);
}

/* =========================================================================
 * Flush / present
 * ========================================================================= */

void W_FlushWindow(W_Window wv) {
    /* In the SDL2 composite model, we can't present just one window — we must
     * composite all mapped windows together to avoid partial-frame flicker.
     * Defer to W_Flush so every SDL_RenderPresent reflects the full scene. */
    (void)wv;
    W_Flush();
}

void W_Flush(void) {
    SDL_SetRenderTarget(sdl2_renderer, NULL);
    SDL_SetRenderDrawColor(sdl2_renderer, 0, 0, 0, 255);
    SDL_RenderClear(sdl2_renderer);
    for (int i = 0; i < winorder_count; i++) {
        struct sdl2_window *win = winorder[i];
        if (!win->mapped || !win->texture) continue;
        SDL_Rect dst = {win->x, win->y, win->width, win->height};
        SDL_RenderCopy(sdl2_renderer, win->texture, NULL, &dst);
    }
    SDL_RenderPresent(sdl2_renderer);
}

void W_ProbeLatency(W_Window w) { (void)w; }
void W_Beep(void) {
#ifdef SOUND
    /* Play pre-built 880 Hz tone through SDL2_mixer (Init_Sound opens the device). */
    if (sdl2_beep_chunk && Mix_PlayChannel(-1, sdl2_beep_chunk, 0) >= 0)
        return;
#endif
    /* Fallback: silent (no terminal bell — has no effect in a GUI app). */
}
int  W_Mono(void) { return 0; }

/* =========================================================================
 * Event system — self-pipe + SDL_PollEvent translation
 * ========================================================================= */

int W_Socket(void) { return pipe_read_fd; }

int W_EventsQueuedCk(void) {
    SDL_PumpEvents();
    int n = SDL_PeepEvents(NULL, 0, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
    if (n > 0) pipe_signal();
    return n;
}

int W_EventsQueued(void) { return W_EventsQueuedCk(); }

/* Map SDL window → W_Window handle by walking the order list */
static W_Window sdl_window_to_wwin(Uint32 sdl_win_id) {
    /* All our game windows share one SDL_Window, so return the base window.
     * The game will dispatch events based on mouse coordinates. */
    (void)sdl_win_id;
    if (winorder_count > 0)
        return (W_Window)winorder[0];
    return NULL;
}

/* Return the W_Window that contains the given SDL screen coordinates */
static W_Window win_at(int sx, int sy) {
    /* Walk in reverse order (topmost first) */
    for (int i = winorder_count - 1; i >= 0; i--) {
        struct sdl2_window *w = winorder[i];
        if (!w->mapped) continue;
        if (sx >= w->x && sx < w->x + w->width &&
            sy >= w->y && sy < w->y + w->height)
            return (W_Window)w;
    }
    return winorder_count > 0 ? (W_Window)winorder[0] : NULL;
}

static int translate_sdl_event(SDL_Event *e, W_Event *we) {
    memset(we, 0, sizeof(*we));
    switch (e->type) {
    case SDL_KEYDOWN: {
        SDL_Keycode k = e->key.keysym.sym;
        we->type   = W_EV_KEY;
        /* Use the last tracked mouse position so directional key commands
         * (phaser, torpedo, set-course) aim toward the cursor.  This mirrors
         * X11, which includes the pointer location in every key event.  We
         * track from MOUSEMOTION/MOUSEBUTTONDOWN rather than calling
         * SDL_GetMouseState, because SDL_GetMouseState may return coordinates
         * in a different scale under HiDPI on macOS. */
        we->Window = sdl2_last_mwin ? sdl2_last_mwin
                                    : sdl_window_to_wwin(e->key.windowID);
        we->x = sdl2_last_mx;
        we->y = sdl2_last_my;
        /* Arrow keys */
        if (k == SDLK_UP)    { we->key = W_Key_Up;   return 1; }
        if (k == SDLK_DOWN)  { we->key = W_Key_Down; return 1; }
        /* Control characters that match their ASCII values */
        if (k == SDLK_BACKSPACE) { we->key = 8;   return 1; }
        if (k == SDLK_DELETE)    { we->key = 127; return 1; }
        if (k == SDLK_RETURN || k == SDLK_KP_ENTER) { we->key = 13; return 1; }
        if (k == SDLK_ESCAPE)    { we->key = 27;  return 1; }
        if (k == SDLK_TAB)       { we->key = 9;   return 1; }
        /* Printable ASCII */
        if (k >= 32 && k <= 126) { we->key = (unsigned char)k; return 1; }
        /* Ctrl modifier: produce control character */
        if ((e->key.keysym.mod & KMOD_CTRL) && k >= SDLK_a && k <= SDLK_z) {
            we->key = (unsigned char)(k - SDLK_a + 1);
            return 1;
        }
        return 0; }

#ifdef AUTOKEY
    case SDL_KEYUP: {
        we->type   = W_EV_KEY_OFF;
        we->Window = sdl_window_to_wwin(e->key.windowID);
        SDL_Keycode k = e->key.keysym.sym;
        if (k >= 32 && k <= 126) { we->key = (unsigned char)k; return 1; }
        return 0; }
#endif

    case SDL_MOUSEMOTION: {
        /* Track position so key events can aim directional commands at cursor */
        int sx = e->motion.x, sy = e->motion.y;
        sdl2_last_mwin = win_at(sx, sy);
        if (sdl2_last_mwin) {
            struct sdl2_window *mv = (struct sdl2_window *)sdl2_last_mwin;
            sdl2_last_mx = sx - mv->x;
            sdl2_last_my = sy - mv->y;
        } else {
            sdl2_last_mx = sx;
            sdl2_last_my = sy;
        }
        return 0; }

    case SDL_MOUSEBUTTONDOWN: {
        we->type   = W_EV_BUTTON;
        we->x      = e->button.x;
        we->y      = e->button.y;
        we->Window = win_at(e->button.x, e->button.y);
        if (we->Window) {
            struct sdl2_window *win = (struct sdl2_window *)we->Window;
            we->x -= win->x;
            we->y -= win->y;
        }
        /* Keep tracking in sync with button events too */
        sdl2_last_mwin = we->Window;
        sdl2_last_mx   = we->x;
        sdl2_last_my   = we->y;
        switch (e->button.button) {
        case SDL_BUTTON_LEFT:   we->key = W_LBUTTON; break;
        case SDL_BUTTON_MIDDLE: we->key = W_MBUTTON; break;
        case SDL_BUTTON_RIGHT:  we->key = W_RBUTTON; break;
        default:                we->key = W_LBUTTON; break;
        }
        return 1; }

    case SDL_MOUSEWHEEL: {
        we->type   = W_EV_BUTTON;
        we->Window = winorder_count > 0 ? (W_Window)winorder[0] : NULL;
        we->key    = e->wheel.y > 0 ? W_WUBUTTON : W_WDBUTTON;
        return 1; }

    case SDL_WINDOWEVENT:
        if (e->window.event == SDL_WINDOWEVENT_EXPOSED) {
            we->type   = W_EV_EXPOSE;
            we->Window = sdl_window_to_wwin(e->window.windowID);
            return 1;
        }
        if (e->window.event == SDL_WINDOWEVENT_CLOSE) {
            we->type   = W_EV_CLOSED;
            we->Window = sdl_window_to_wwin(e->window.windowID);
            return 1;
        }
        return 0;

    case SDL_QUIT:
        we->type = W_EV_CLOSED;
        we->Window = winorder_count > 0 ? (W_Window)winorder[0] : NULL;
        return 1;

    default:
        return 0;
    }
}

int W_SpNextEvent(W_Event *ev) {
    pipe_drain();
    SDL_PumpEvents();

    /* Return buffered event first */
    if (have_pending) {
        *ev = pending_event;
        have_pending = 0;
        return 1;
    }

    SDL_Event sdl_ev;
    while (SDL_PollEvent(&sdl_ev)) {
        if (translate_sdl_event(&sdl_ev, ev))
            return 1;
    }
    return 0;
}

void W_NextEvent(W_Event *ev) {
    while (!W_SpNextEvent(ev))
        SDL_Delay(1);
}

int W_EventsPending(void) {
    if (have_pending) return 1;
    SDL_PumpEvents();
    W_Event tmp;
    while (W_SpNextEvent(&tmp)) {
        pending_event = tmp;
        have_pending  = 1;
        return 1;
    }
    return 0;
}

/* Callback-style event handler registration (Wlib.h) */
W_Callback W_GetWindowKeyDownHandler(W_Window wv) {
    struct sdl2_window *w = sdl2_find_window(wv);
    return w ? (W_Callback)w->handle_keydown : NULL;
}
W_Callback W_GetWindowKeyUpHandler(W_Window wv) {
    struct sdl2_window *w = sdl2_find_window(wv);
    return w ? (W_Callback)w->handle_keyup : NULL;
}
W_Callback W_GetWindowButtonHandler(W_Window wv) {
    struct sdl2_window *w = sdl2_find_window(wv);
    return w ? (W_Callback)w->handle_button : NULL;
}
W_Callback W_GetWindowExposeHandler(W_Window wv) {
    struct sdl2_window *w = sdl2_find_window(wv);
    return w ? (W_Callback)w->handle_expose : NULL;
}
void W_SetWindowKeyDownHandler(W_Window wv, W_Callback c) {
    struct sdl2_window *w = sdl2_find_window(wv);
    if (w) w->handle_keydown = (void(*)(void*))c;
}
void W_SetWindowKeyUpHandler(W_Window wv, W_Callback c) {
    struct sdl2_window *w = sdl2_find_window(wv);
    if (w) w->handle_keyup = (void(*)(void*))c;
}
void W_SetWindowButtonHandler(W_Window wv, W_Callback c) {
    struct sdl2_window *w = sdl2_find_window(wv);
    if (w) w->handle_button = (void(*)(void*))c;
}
void W_SetWindowExposeHandler(W_Window wv, W_Callback c) {
    struct sdl2_window *w = sdl2_find_window(wv);
    if (w) w->handle_expose = (void(*)(void*))c;
}

/* =========================================================================
 * Fullscreen
 * ========================================================================= */

void W_FullScreenInitialise(void) {}

void W_FullScreenOn(W_Window w) {
    (void)w;
    SDL_SetWindowFullscreen(sdl2_window_handle, SDL_WINDOW_FULLSCREEN_DESKTOP);
}

void W_FullScreenOff(W_Window w) {
    (void)w;
    SDL_SetWindowFullscreen(sdl2_window_handle, 0);
}

int W_FullScreenToggle(W_Window w) {
    Uint32 flags = SDL_GetWindowFlags(sdl2_window_handle);
    if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
        W_FullScreenOff(w);
    else
        W_FullScreenOn(w);
    return FULLSCREEN_OK;
}

void W_FullScreenBegin(W_Window w) { (void)w; }
void W_FullScreen(W_Window w)      { W_FullScreenOn(w); }

/* =========================================================================
 * Cursors — SDL2 cursor creation from XBM data
 * ========================================================================= */

static void set_cursor_from_xbm(int width, int height,
                                  const char *bits, const char *mask,
                                  int xhot, int yhot) {
    SDL_Cursor *cur = SDL_CreateCursor(
        (const Uint8 *)bits, (const Uint8 *)mask,
        width, height, xhot, yhot);
    if (cur) SDL_SetCursor(cur);
}

void W_DefineCursor(W_Window w, int width, int height,
                    char *bits, char *mask, int xhot, int yhot) {
    (void)w;
    set_cursor_from_xbm(width, height, bits, mask, xhot, yhot);
}

/* All named cursors use the default arrow for now */
void W_DefineMapcursor(W_Window w)     { (void)w; }
void W_DefineLocalcursor(W_Window w)   { (void)w; }
void W_DefineFedCursor(W_Window w)     { (void)w; }
void W_DefineRomCursor(W_Window w)     { (void)w; }
void W_DefineKliCursor(W_Window w)     { (void)w; }
void W_DefineOriCursor(W_Window w)     { (void)w; }
void W_DefineTrekCursor(W_Window w)    { (void)w; }
void W_DefineWarningCursor(W_Window w) { (void)w; }
void W_DefineTextCursor(W_Window w)    { (void)w; }
void W_DefineArrowCursor(W_Window w)   { (void)w; }

/* =========================================================================
 * Mouse position
 * ========================================================================= */

int findMouseInWin(int *x, int *y, W_Window wv) {
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    struct sdl2_window *w = sdl2_find_window(wv);
    if (!w) { *x = mx; *y = my; return 1; }
    *x = mx - w->x;
    *y = my - w->y;
    return (*x >= 0 && *x < w->width && *y >= 0 && *y < w->height);
}

/* =========================================================================
 * Screenshot
 * ========================================================================= */

void W_CameraSnap(W_Window wv) {
    (void)wv;
    static int snap_n = 0;
    char fname[64];
    snprintf(fname, sizeof(fname), "netrek-snap-%03d.png", snap_n++);
    int w, h;
    SDL_GetWindowSize(sdl2_window_handle, &w, &h);
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(
        0, w, h, 32, SDL_PIXELFORMAT_RGBA8888);
    if (surf) {
        SDL_RenderReadPixels(sdl2_renderer, NULL,
                             SDL_PIXELFORMAT_RGBA8888,
                             surf->pixels, surf->pitch);
        IMG_SavePNG(surf, fname);
        SDL_FreeSurface(surf);
        fprintf(stderr, "sdl2window: screenshot saved to %s\n", fname);
    }
}

/* W_NextScreenShot / W_DrawScreenShot / W_SetBackgroundImage implemented in sdl2sprite.c */

/* =========================================================================
 * Window preference helpers (mirror of x11window.c / gnu_win32.c versions)
 * ========================================================================= */

extern int booleanDefault(char *def, int preferred);

int checkMapped(char *name) {
    char buf[100];
    snprintf(buf, sizeof(buf), "%s.mapped", name);
    return booleanDefault(buf, 0);
}

int checkMappedPref(char *name, int preferred) {
    char buf[100];
    snprintf(buf, sizeof(buf), "%s.mapped", name);
    return booleanDefault(buf, preferred);
}

void pastebuffer(void) { /* SDL2: clipboard paste not yet implemented */ }

/* Test hook: translate an SDL_Event directly to a W_Event without touching the queue. */
#ifdef UNIT_TEST
int sdl2_test_translate(SDL_Event *e, W_Event *we) { return translate_sdl_event(e, we); }
#endif

#endif /* HAVE_SDL2 */
