/* sdl2sprite.h — sprite/image API for the SDL2 Wlib backend */

#ifndef _h_sdl2sprite
#define _h_sdl2sprite

#include <SDL2/SDL.h>

/* Forward-declare internal window type without pulling in all of sdl2window.h */
struct sdl2_window;

/* W_Window forward declaration (defined as typedef char* in Wlib.h) */
#ifndef _h_Wlib
typedef char *W_Window;
#endif

/* Sprite: an SDL_Texture sprite-sheet (all rotation frames stacked vertically).
 * Mirrors the role of struct S_Object in x11sprite.c. */
struct sdl2_sprite {
    SDL_Texture        *texture;  /* RGBA sprite sheet */
    int                 width;    /* per-frame width  */
    int                 height;   /* per-frame height */
    int                 nviews;   /* rotation frames  */
    int                 view;     /* current frame    */
    int                 cloak;    /* cloaking phase   */
    struct sdl2_window *target;   /* destination window for W_DrawSprite */
};

/* Called from W_GetPixmaps() in sdl2window.c */
void sdl2_GetPixmaps(W_Window tactical, W_Window galactic);

/* Functions mirroring x11sprite.c's public interface */
int  W_DrawSprite(void *sprite_v, int x, int y, int winside);
void W_DrawSpriteAbsolute(void *sprite_v, int x, int y);
void W_ClearSpriteAbsolute(void *sprite_v, int x, int y);

/* Game-facing sprite selectors (same signatures as x11sprite.c) */
void *S_Ship(int playerno);
void *S_mPlanet(int planetno);
void *S_mArmy(int planetno);
void *S_mRepair(int planetno);
void *S_mFuel(int planetno);
void *S_mOwner(int planetno);
void *S_Torp(int torpno);
void *S_Plasma(int plasmatorpno);

/* W_ReadImage / W_DrawImage / W_DropImage (declared in Wlib.h, implemented here) */

#endif /* _h_sdl2sprite */
