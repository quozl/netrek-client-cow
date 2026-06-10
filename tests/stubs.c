/* stubs.c — minimal stubs for game globals used by sdl2window.c/sdl2sprite.c.
 * config.h must come first (defines LONG/SIZEOF_LONG for struct.h).
 * Wlib.h must come before struct.h and data.h (defines W_Color, W_Window, etc.).
 */

#include "../config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* Wlib.h first — defines W_Color, W_Window, W_Font, W_Icon */
#include "../Wlib.h"
#include "../defs.h"
#include "../struct.h"
#include "../data.h"

/* ---- game entity arrays ---- */
static struct player  _players[MAXPLAYER];
static struct planet  _planets[MAXPLANETS];
static struct torp    _torps[MAXTORP];
static struct plasmatorp _plasmatorps[MAXPLASMA];

struct player *players     = _players;
struct player *me          = _players;
struct planet *planets     = _planets;
struct torp   *torps       = _torps;
struct plasmatorp *plasmatorps = _plasmatorps;

/* ---- simple scalars ---- */
int  pixMissing   = 0;
int  playback     = 0;
int  pixFlags     = 0;
int  remap[NUMTEAM + 1] = {0, 1, 2, 3, 4};
int  server_ups   = 5;
int  showlocal    = 0;
int  showgalactic = 1;

/* ---- window handles ---- */
W_Window w       = NULL;
W_Window mapw    = NULL;
W_Window baseWin = NULL;
W_Window statwin  = NULL;
W_Window infow    = NULL;
W_Window iconWin  = NULL;
W_Window tstatw   = NULL;
W_Window messagew = NULL;

/* ---- color globals declared in data.h ---- */
W_Color borderColor = 1;
W_Color backColor   = 1;
W_Color textColor   = 0;
W_Color myColor     = 3;
W_Color warningColor= 2;

/* ---- bitmap icon arrays declared in data.h ---- */
W_Icon bplanets[7]  = {NULL};
W_Icon mbplanets[7] = {NULL};
W_Icon bplanets2[8] = {NULL};
W_Icon mbplanets2[8]= {NULL};
W_Icon bplanets3[NUM_PLANET_BITMAPS2] = {NULL};
W_Icon mbplanets3[NUM_PLANET_BITMAPS2]= {NULL};
W_Icon bplanets4[8] = {NULL};
W_Icon mbplanets4[8]= {NULL};

/* ---- defaults stub ---- */
char *getdefault(char *key) { (void)key; return NULL; }
int   strcmpi(const char *a, const char *b) { return strcasecmp(a, b); }
int   booleanDefault(char *def, int preferred) { (void)def; return preferred; }
