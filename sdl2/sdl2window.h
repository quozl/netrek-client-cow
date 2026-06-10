/* sdl2window.h — internal types for the SDL2 Wlib backend */

#ifndef _h_sdl2window
#define _h_sdl2window

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

/* W_Window is typedef char* in Wlib.h; forward-declare so this header
 * can be parsed standalone (e.g. by clangd / unit tests). */
#ifndef _h_Wlib
typedef char *W_Window;
#endif

/* Window type tags — mirror x11window.c constants */
#define SDL2WIN_GRAPH   1
#define SDL2WIN_TEXT    2
#define SDL2WIN_MENU    3
#define SDL2WIN_SCROLL  4

/* Number of logical colours (indices into sdl2_colortable) */
#ifdef RACE_COLORS
#define SDL2_NCOLORS 12
#else
#define SDL2_NCOLORS 7
#endif

/* Number of logical fonts (0=Big, 1=Regular, 2=Bold/Highlight, 3=Underline) */
#define SDL2_NFONTS 4

/* Per-glyph entry in the texture atlas (covers all printable ASCII) */
#define SDL2_FIRST_GLYPH 32
#define SDL2_LAST_GLYPH  126
#define SDL2_NGLYPH (SDL2_LAST_GLYPH - SDL2_FIRST_GLYPH + 1)

struct sdl2_glyph {
    SDL_Rect src;    /* source rect in atlas texture */
    int      advance;/* horizontal advance in pixels */
};

/* Per-font rendering state */
struct sdl2_font {
    TTF_Font          *ttf;
    int                height;   /* line height in pixels */
    int                ascent;   /* pixels above baseline */
    int                advance;  /* character cell advance (monospace) */
    /* atlas: SDL2_NCOLORS textures, each contains SDL2_NGLYPH glyphs */
    SDL_Texture       *atlas[SDL2_NCOLORS];
    struct sdl2_glyph  glyphs[SDL2_NGLYPH];
};

/* Scrolling window back-buffer linked list */
#define SDL2_MAX_TEXT_WIDTH 100
struct sdl2_string {
    char             text[SDL2_MAX_TEXT_WIDTH];
    int              color;
    int              font;       /* font index */
    struct sdl2_string *next, *prev;
};

struct sdl2_scroll {
    int                 lines;   /* total lines stored */
    int                 topline; /* index of first visible line */
    struct sdl2_string *head;    /* newest line */
    struct sdl2_string *tail;    /* oldest line */
};

/* Menu item */
struct sdl2_menuitem {
    int   column;
    char *text;     /* malloc'd */
    int   color;
};

/* Core window struct — W_Window is a typedef char* cast of this pointer */
struct sdl2_window {
    SDL_Texture         *texture;    /* off-screen render target */
    int                  x, y;      /* position in master SDL_Window (pixels) */
    int                  width;      /* pixel width */
    int                  height;     /* pixel height */
    int                  char_w;     /* logical columns (TEXT/MENU/SCROLL) */
    int                  char_h;     /* logical rows  (TEXT/MENU/SCROLL) */
    int                  type;       /* SDL2WIN_* */
    int                  mapped;     /* 1 = visible in W_Flush composite */
    char                *name;       /* resource name (malloc'd) */
    void                *data;       /* sdl2_scroll* or sdl2_menuitem* */
    /* callbacks */
    void               (*handle_keydown)(void *ev);
    void               (*handle_keyup)(void *ev);
    void               (*handle_button)(void *ev);
    void               (*handle_expose)(void *ev);
};

/* Hash table: maps arbitrary pointer → sdl2_window* (101 buckets) */
#define SDL2WIN_HASHSIZE 101
struct sdl2_winlist {
    struct sdl2_window  *win;
    struct sdl2_winlist *next;
};

/* Exposed globals used by sdl2sprite.c */
extern SDL_Renderer  *sdl2_renderer;
extern struct sdl2_font sdl2_fonts[SDL2_NFONTS];
extern SDL_Color      sdl2_colortable[SDL2_NCOLORS];

/* Internal helpers called by sdl2sprite.c */
struct sdl2_window *sdl2_find_window(W_Window w);
void                sdl2_set_target(struct sdl2_window *win);

/* W_Icon is typedef char* cast of sdl2_icon* */
struct sdl2_icon {
    SDL_Texture        *texture;
    int                 width, height;
    struct sdl2_window *target;  /* window to render into (set by W_StoreBitmap) */
};

#endif /* _h_sdl2window */
