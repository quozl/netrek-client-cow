/* sdl2sprite.c — SDL2_image sprite loader, replacing x11sprite.c.
 *
 * Implements the game-facing sprite API: GetPixmaps / S_Ship / S_Torp /
 * S_Plasma / S_mPlanet* / W_DrawSprite / W_ReadImage / W_DrawImage /
 * W_DropImage.
 *
 * Sprite sheets: same layout as X11 version — a PNG whose height is
 * (width * nviews).  SDL2 textures carry alpha natively so no shape mask.
 */

#include "config.h"

#ifdef HAVE_SDL2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/stat.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "defaults.h"
#include "sdl2window.h"
#include "sdl2sprite.h"

/* =========================================================================
 * Sprite table mirrors x11sprite.c layout
 * ========================================================================= */

#define NUM_BG_IMGS  5
#define NUM_PL_IMGS 11

#define PL_PIX_UKN    0
#define PL_PIX_ROCK   1
#define PL_PIX_AGRI   2
#define PL_PIX_ARMY   3
#define PL_PIX_REPAIR 4
#define PL_PIX_FUEL   5
#define PL_PIX_IND    6
#define PL_PIX_FED    7
#define PL_PIX_ROM    8
#define PL_PIX_KLI    9
#define PL_PIX_ORI   10

static const int reremap[5] = {
    NO_IND_PIX, NO_FED_PIX, NO_ROM_PIX, NO_KLI_PIX, NO_ORI_PIX
};

static struct sdl2_sprite mplanetImg[NUM_PL_IMGS];
static struct sdl2_sprite shipImg[NUMTEAM + 1][NUM_TYPES];
static struct sdl2_sprite torpImg[NUMTEAM + 1][2];
static struct sdl2_sprite plasmaImg[NUMTEAM + 1][2];
static struct sdl2_sprite cloakImg;
static struct sdl2_sprite explosionImg[2];

static const char teamnames[NUMTEAM + 1][4] = {
    "Ind", "Fed", "Rom", "Kli", "Ori"
};

static const char mplanetfiles[NUM_PL_IMGS][12] = {
    "UNKN.png", "ROCK.png", "AGRI.png", "army.png",
    "repair.png", "fuel.png", "Ind.png", "Fed.png",
    "Rom.png", "Kli.png", "Ori.png"
};
static const char shipfiles[NUM_TYPES][8] = {
    "SC.png", "DD.png", "CA.png", "BB.png",
    "AS.png", "SB.png", "GA.png", "AT.png"
};
static const char torpfiles[2][14]     = {"torp.png",   "torp_det.png"};
static const char plasmafiles[2][16]   = {"plasma.png", "plasma_det.png"};
static const char cloakfile[10]        = "cloak.png";
static const char explosionfiles[2][18]= {"explosion.png", "sbexplosion.png"};

/* =========================================================================
 * Pixmap directory helpers (mirrors x11sprite.c)
 * ========================================================================= */

static char pixmapDir[1024];

static void GetPixmapDir(void) {
    char *pd;
    if (pixmapDir[0] != '\0') return;

    pd = getdefault("pixmapDir");
    if (pd)
        strncpy(pixmapDir, pd, sizeof(pixmapDir) - 1);
    else
        strcpy(pixmapDir, "/usr/share/pixmaps/netrek-client-cow");

    if (strcmpi(pixmapDir, "None") == 0 || (pixMissing & NO_PIXMAPS)) {
        pixMissing = NO_IND_PIX | NO_FED_PIX | NO_ROM_PIX |
                     NO_KLI_PIX | NO_ORI_PIX | NO_WEP_PIX | NO_EXP_PIX |
                     NO_CLK_PIX | NO_MAP_PIX | NO_BG_PIX | NO_PIXMAPS;
        fprintf(stderr, "sdl2sprite: pixmaps turned off\n");
    } else {
        struct stat buf;
        if (stat(pixmapDir, &buf) || !S_ISDIR(buf.st_mode)) {
            if (stat("pixmaps", &buf) || !S_ISDIR(buf.st_mode)) {
                pixMissing = NO_IND_PIX | NO_FED_PIX | NO_ROM_PIX |
                             NO_KLI_PIX | NO_ORI_PIX | NO_WEP_PIX | NO_EXP_PIX |
                             NO_CLK_PIX | NO_MAP_PIX | NO_BG_PIX | NO_PIXMAPS;
                fprintf(stderr, "sdl2sprite: pixmaps not found\n");
            } else {
                strcpy(pixmapDir, "pixmaps");
            }
        }
    }
}

/* =========================================================================
 * Core image loader
 * ========================================================================= */

static int load_sprite(const char *path, struct sdl2_sprite *s,
                       struct sdl2_window *target) {
    if (access(path, R_OK) != 0) {
        fprintf(stderr, "sdl2sprite: not readable: %s\n", path);
        return 1;
    }

    SDL_Surface *surf = IMG_Load(path);
    if (!surf) {
        fprintf(stderr, "sdl2sprite: IMG_Load(%s): %s\n", path, IMG_GetError());
        return 1;
    }

    int w = surf->w, h = surf->h;
    int nviews = (w > 0) ? h / w : 1;
    if (nviews * w != h) nviews = 1;

    s->texture = SDL_CreateTextureFromSurface(sdl2_renderer, surf);
    SDL_FreeSurface(surf);
    if (!s->texture) {
        fprintf(stderr, "sdl2sprite: CreateTexture(%s): %s\n", path, SDL_GetError());
        return 1;
    }
    SDL_SetTextureBlendMode(s->texture, SDL_BLENDMODE_BLEND);

    s->width   = w;
    s->height  = (nviews > 0) ? h / nviews : h;
    s->nviews  = nviews;
    s->view    = 0;
    s->cloak   = 0;
    s->target  = target;

#ifdef DEBUG
    fprintf(stderr, "sdl2sprite: loaded %s nv=%d w=%d h=%d\n",
            path, nviews, s->width, s->height);
#endif
    return 0;
}

/* =========================================================================
 * W_GetPixmaps entry point (called from sdl2window.c's W_GetPixmaps)
 * ========================================================================= */

void sdl2_GetPixmaps(W_Window tactical, W_Window galactic) {
    int i, j, missing;
    char buf[2048];

    struct sdl2_window *tac = (struct sdl2_window *)tactical;
    struct sdl2_window *gal = (struct sdl2_window *)galactic;

    GetPixmapDir();

    /* Ships */
    for (i = 0; i <= NUMTEAM; i++) {
        missing = 0;
        for (j = 0; j < NUM_TYPES; j++) {
            snprintf(buf, sizeof(buf), "%s/%s/%s",
                     pixmapDir, teamnames[i], shipfiles[j]);
            missing += load_sprite(buf, &shipImg[i][j], tac);
        }
        if (missing == NUM_TYPES) {
            pixMissing |= reremap[i];
            if (!(pixMissing & NO_PIXMAPS))
                fprintf(stderr, "sdl2sprite: %s ship pixmaps unavailable\n",
                        teamnames[i]);
        }
    }

    /* Torpedoes & plasma */
    missing = 0;
    for (i = 0; i <= NUMTEAM; i++) {
        for (j = 0; j < 2; j++) {
            snprintf(buf, sizeof(buf), "%s/%s/%s",
                     pixmapDir, teamnames[i], torpfiles[j]);
            missing += load_sprite(buf, &torpImg[i][j], tac);
            snprintf(buf, sizeof(buf), "%s/%s/%s",
                     pixmapDir, teamnames[i], plasmafiles[j]);
            missing += load_sprite(buf, &plasmaImg[i][j], tac);
        }
    }
    if (missing == (NUMTEAM + 1) * 4) {
        pixMissing |= NO_WEP_PIX;
        if (!(pixMissing & NO_PIXMAPS))
            fprintf(stderr, "sdl2sprite: weapon pixmaps unavailable\n");
    }

    /* Explosions */
    missing = 0;
    for (i = 0; i < 2; i++) {
        snprintf(buf, sizeof(buf), "%s/Misc/%s", pixmapDir, explosionfiles[i]);
        missing += load_sprite(buf, &explosionImg[i], tac);
    }
    if (missing == 2) {
        pixMissing |= NO_EXP_PIX;
        if (!(pixMissing & NO_PIXMAPS))
            fprintf(stderr, "sdl2sprite: explosion pixmaps unavailable\n");
    }

    /* Map planets */
    missing = 0;
    for (i = 0; i < NUM_PL_IMGS; i++) {
        snprintf(buf, sizeof(buf), "%s/Planets/Map/%s",
                 pixmapDir, mplanetfiles[i]);
        if (i == PL_PIX_AGRI) {
            if (load_sprite(buf, &mplanetImg[i], gal) != 0) {
                snprintf(buf, sizeof(buf), "%s/Planets/%s",
                         pixmapDir, mplanetfiles[i - 1]);
                load_sprite(buf, &mplanetImg[i], gal);
            }
        } else {
            missing += load_sprite(buf, &mplanetImg[i], gal);
        }
    }
    if (missing) {
        pixMissing |= NO_MAP_PIX;
        if (!(pixMissing & NO_PIXMAPS))
            fprintf(stderr, "sdl2sprite: map planet pixmaps unavailable\n");
    }

    /* Cloak */
    snprintf(buf, sizeof(buf), "%s/Misc/%s", pixmapDir, cloakfile);
    if (load_sprite(buf, &cloakImg, tac)) {
        pixMissing |= NO_CLK_PIX;
        if (!(pixMissing & NO_PIXMAPS))
            fprintf(stderr, "sdl2sprite: cloak pixmap unavailable\n");
    }

    pixMissing &= ~NO_HALOS;
}

/* =========================================================================
 * Drawing functions
 * ========================================================================= */

int W_DrawSprite(void *sprite_v, int x, int y, int winside) {
    struct sdl2_sprite *s = (struct sdl2_sprite *)sprite_v;
    if (!s || !s->texture || !s->target) return 0;

    const int view = SCALE * winside / 2;
    if (x > view || x < -view || y > view || y < -view) return 0;
    if (s->view < 0 || s->view >= s->nviews) return 0;

    int dx = x - s->width  / 2;
    int dy = y - s->height / 2;

    /* Cloak: use cloakImg if available */
    struct sdl2_sprite *draw = s;
    if ((s->cloak > 0) && !(pixFlags & NO_CLK_PIX) && cloakImg.texture) {
        draw = &cloakImg;
        dy -= draw->view * draw->height;
    }

    SDL_Rect src = { 0, draw->view * draw->height, draw->width, draw->height };
    SDL_Rect dst = { draw->target->x + dx, draw->target->y + dy,
                     draw->width, draw->height };

    /* Render directly to the backbuffer (master window) */
    SDL_SetRenderTarget(sdl2_renderer, draw->target->texture);
    SDL_RenderCopy(sdl2_renderer, draw->texture, &src,
                   &(SDL_Rect){ dx, dy, draw->width, draw->height });
    SDL_SetRenderTarget(sdl2_renderer, NULL);
    (void)dst;
    return s->width;
}

void W_DrawSpriteAbsolute(void *sprite_v, int x, int y) {
    struct sdl2_sprite *s = (struct sdl2_sprite *)sprite_v;
    if (!s || !s->texture || !s->target) return;

    SDL_Rect src = { 0, s->view * s->height, s->width, s->height };
    SDL_Rect dst = { x, y, s->width, s->height };

    SDL_SetRenderTarget(sdl2_renderer, s->target->texture);
    SDL_RenderCopy(sdl2_renderer, s->texture, &src, &dst);
    SDL_SetRenderTarget(sdl2_renderer, NULL);
}

void W_ClearSpriteAbsolute(void *sprite_v, int x, int y) {
    struct sdl2_sprite *s = (struct sdl2_sprite *)sprite_v;
    if (!s || !s->target) return;

    SDL_SetRenderTarget(sdl2_renderer, s->target->texture);
    SDL_SetRenderDrawColor(sdl2_renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(sdl2_renderer,
        &(SDL_Rect){ x, y, s->width, s->height });
    SDL_SetRenderTarget(sdl2_renderer, NULL);
}

/* =========================================================================
 * Game-facing sprite selectors (same logic as x11sprite.c)
 * ========================================================================= */

void *S_Ship(int playerno) {
    struct sdl2_sprite *s;
    struct player *this;

    if (playerno < 0 || playerno >= MAXPLAYER) return NULL;
    this = &players[playerno];

    if (this->p_status != PALIVE && this->p_status != PEXPLODE) return NULL;
    if ((this->p_flags & PFOBSERV) && !(this->p_flags & PFPLOCK)) return NULL;
    if ((this->p_flags & PFOBSERV) && !(this->p_flags & PFCLOAK)) return NULL;

    s = &shipImg[remap[this->p_team]][this->p_ship.s_type];
    s->cloak = 0;
    s->view  = (((this->p_dir + 128 / s->nviews) / (256 / s->nviews)) % s->nviews);

    if (this->p_status == PEXPLODE) {
        if (pixFlags & NO_EXP_PIX) return NULL;
        if (this->p_ship.s_type == STARBASE) {
            int i = this->p_explode * 5 / server_ups;
            s = &explosionImg[1];
            if (i >= s->nviews) return NULL;
            s->view = i;
        } else {
            int i = this->p_explode * 10 / server_ups;
            s = &explosionImg[0];
            if (i >= s->nviews) return NULL;
            s->view = i;
        }
        this->p_explode++;
    } else if ((this->p_flags & PFCLOAK) || this->p_cloakphase > 0) {
        if (this->p_cloakphase == CLOAK_PHASES - 1) {
            if (this == me && !(pixFlags & NO_CLK_PIX)) {
                s = &cloakImg;
                s->view = cloakImg.nviews - 1;
            } else {
                return NULL;
            }
        } else {
            s->cloak = this->p_cloakphase;
        }
    }

    if (!s->texture || (pixFlags & reremap[remap[this->p_team]])) return NULL;
    return (void *)s;
}

void *S_mPlanet(int planetno) {
    struct sdl2_sprite *s = NULL;
    struct planet *this = &planets[planetno];

    if ((pixFlags & NO_MAP_PIX) || showgalactic != 1) return NULL;
    if ((this->pl_info & me->p_team)
#ifdef RECORDGAME
        || playback
#endif
        ) {
        s = &mplanetImg[PL_PIX_IND + remap[this->pl_owner]];
        s->view = 0;
    }
    return (void *)s;
}

void *S_mArmy(int planetno) {
    struct sdl2_sprite *s = NULL;
    struct planet *this = &planets[planetno];

    if ((pixFlags & NO_MAP_PIX) || showgalactic != 1) return NULL;
    if (((this->pl_info & me->p_team)
#ifdef RECORDGAME
        || playback
#endif
        ) && this->pl_armies > 0) {
        s = &mplanetImg[PL_PIX_ARMY];
        s->view = 0;
    }
    return (void *)s;
}

void *S_mRepair(int planetno) {
    struct sdl2_sprite *s = NULL;
    struct planet *this = &planets[planetno];

    if ((pixFlags & NO_MAP_PIX) || showgalactic != 1) return NULL;
    if (((this->pl_info & me->p_team)
#ifdef RECORDGAME
        || playback
#endif
        ) && (this->pl_flags & PLREPAIR)) {
        s = &mplanetImg[PL_PIX_REPAIR];
        s->view = 0;
    }
    return (void *)s;
}

void *S_mFuel(int planetno) {
    struct sdl2_sprite *s = NULL;
    struct planet *this = &planets[planetno];

    if ((pixFlags & NO_MAP_PIX) || showgalactic != 1) return NULL;
    if (((this->pl_info & me->p_team)
#ifdef RECORDGAME
        || playback
#endif
        ) && (this->pl_flags & PLFUEL)) {
        s = &mplanetImg[PL_PIX_FUEL];
        s->view = 0;
    }
    return (void *)s;
}

void *S_mOwner(int planetno) {
    struct sdl2_sprite *s = NULL;
    struct planet *this = &planets[planetno];

    if ((pixFlags & NO_MAP_PIX) || showgalactic != 0) return NULL;
    if ((this->pl_info & me->p_team)
#ifdef RECORDGAME
        || playback
#endif
        ) {
        s = &mplanetImg[PL_PIX_IND + remap[this->pl_owner]];
        s->view = 0;
    }
    return (void *)s;
}

void *S_Torp(int torpno) {
    struct sdl2_sprite *s;
    struct torp *this = &torps[torpno];
    int numdetframes, frame;

    if (this->t_status == TEXPLODE) {
        s = &torpImg[remap[players[this->t_owner].p_team]][1];
        this->t_fuse--;
        numdetframes = NUMDETFRAMES * server_ups / 5;
        frame = this->t_fuse * 5 / server_ups;
        if (this->t_fuse <= 0) {
            this->t_status = PTFREE;
            players[this->t_owner].p_ntorp--;
        } else if (this->t_fuse >= numdetframes) {
            this->t_fuse = numdetframes - 1;
        } else {
            s->view = frame;
        }
    } else {
        s = &torpImg[remap[players[this->t_owner].p_team]][0];
        s->view = (s->view + 1) % s->nviews;
    }

    if (!s->texture || (pixFlags & NO_WEP_PIX)) return NULL;
    return (void *)s;
}

void *S_Plasma(int plasmatorpno) {
    struct sdl2_sprite *s;
    struct plasmatorp *this = &plasmatorps[plasmatorpno];
    int numdetframes, frame;

    if (this->pt_status == PTEXPLODE) {
        s = &plasmaImg[remap[players[this->pt_owner].p_team]][1];
        this->pt_fuse--;
        numdetframes = NUMDETFRAMES * server_ups / 5;
        frame = this->pt_fuse * 5 / server_ups;
        if (this->pt_fuse <= 0) {
            this->pt_status = PTFREE;
            players[this->pt_owner].p_nplasmatorp--;
        } else if (this->pt_fuse >= numdetframes) {
            this->pt_fuse = numdetframes - 1;
        } else {
            s->view = frame;
        }
    } else {
        s = &plasmaImg[remap[players[this->pt_owner].p_team]][0];
        s->view = (s->view + 1) % s->nviews;
    }

    if (!s->texture || (pixFlags & NO_WEP_PIX)) return NULL;
    return (void *)s;
}

/* =========================================================================
 * W_ReadImage / W_DrawImage / W_DropImage
 * ========================================================================= */

void *W_ReadImage(W_Window wv, char *name) {
    GetPixmapDir();
    char *path = malloc(strlen(pixmapDir) + strlen(name) + 2);
    if (!path) return NULL;
    sprintf(path, "%s/%s", pixmapDir, name);

    struct sdl2_sprite *s = calloc(1, sizeof(*s));
    if (!s) { free(path); return NULL; }

    if (load_sprite(path, s, (struct sdl2_window *)wv)) {
        free(path);
        free(s);
        return NULL;
    }
    free(path);
    return (void *)s;
}

void W_DrawImage(int x, int y, void *sprite_v) {
    W_DrawSpriteAbsolute(sprite_v, x, y);
}

void W_DropImage(void *sprite_v) {
    struct sdl2_sprite *s = (struct sdl2_sprite *)sprite_v;
    if (!s) return;
    if (s->texture) SDL_DestroyTexture(s->texture);
    free(s);
}

/* =========================================================================
 * Screen-shot slideshow (stub — no FTS on all platforms)
 * ========================================================================= */

void W_NextScreenShot(W_Window wv, int x, int y) {
    (void)wv; (void)x; (void)y;
}

void W_DrawScreenShot(W_Window wv, int x, int y) {
    (void)wv; (void)x; (void)y;
}

/* =========================================================================
 * Background image helpers (stub — SDL2 lacks X11 background pixmap API)
 * ========================================================================= */

void *W_SetBackgroundImage(W_Window wv, char *name) {
    return W_ReadImage(wv, name);
}

#endif /* HAVE_SDL2 */
