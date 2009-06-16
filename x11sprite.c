#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/param.h>

#include <sys/stat.h>
#include <fts.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <Imlib2.h>

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"

#include "defaults.h"
#include "x11window.h"

#include "x11sprite.h"

#define W_Void2Window(win) (((struct window *) (win)))
#define NoPixmapError 0

struct S_Object
  {
    Drawable drawable;
    GC      gc;
    int     view, nviews, width, height, cloak;
    Pixmap  image;
    Pixmap  shape;
  };

#define NUM_BG_IMGS 6
#define NUM_PL_IMGS 11

#define PL_PIX_UKN 0
#define PL_PIX_ROCK 1
#define PL_PIX_AGRI 2
#define PL_PIX_ARMY 3
#define PL_PIX_REPAIR 4
#define PL_PIX_FUEL 5
#define PL_PIX_IND 6
#define PL_PIX_FED 7
#define PL_PIX_ROM 8
#define PL_PIX_KLI 9
#define PL_PIX_ORI 10

const int reremap[5] =
{NO_IND_PIX, NO_FED_PIX, NO_ROM_PIX,
 NO_KLI_PIX, NO_ORI_PIX};

static struct S_Object mplanetImg[NUM_PL_IMGS];
static struct S_Object shipImg[NUMTEAM + 1][NUM_TYPES];
static struct S_Object torpImg[NUMTEAM + 1][2];
static struct S_Object plasmaImg[NUMTEAM + 1][2];
static struct S_Object cloakImg;
static struct S_Object explosionImg[2];

/*
 * static struct S_Object planetImg[NUM_PL_IMGS];
 * static struct S_Object shieldImg[NUM_SH_IMGS];
 * static struct S_Object hullImg[NUM_HL_IMGS];
 */

static Pixmap backPix[NUM_BG_IMGS];

extern Window W_Root;
extern Colormap W_Colormap;
extern Visual *W_Visual;
extern int takeNearest;

extern Display *W_Display;

const char teamnames[NUMTEAM + 1][4] =
{"Ind", "Fed", "Rom", "Kli", "Ori"};

const char mplanetfiles[NUM_PL_IMGS][12] =
{"UNKN.png",
 "ROCK.png",
 "AGRI.png",
 "army.png",
 "repair.png",
 "fuel.png",
 "Ind.png",
 "Fed.png",
 "Rom.png",
 "Kli.png",
 "Ori.png"
};
const char shipfiles[NUM_TYPES][8] =
{"SC.png",
 "DD.png",
 "CA.png",
 "BB.png",
 "AS.png",
 "SB.png",
 "GA.png",
 "AT.png"
};
const char torpfiles[2][14] =
{"torp.png", "torp_det.png"};
const char plasmafiles[2][16] =
{"plasma.png", "plasma_det.png"};
const char cloakfile[10] = "cloak.png";
const char explosionfiles[2][18] =
{"explosion.png", "sbexplosion.png"};
const char bgfiles[NUM_BG_IMGS][16] =
{"map_back.png",
 "local_back.png",
 "ghostbust.png",
 "genocide.png",
 "greet.png",
 "hockey.png"
};


static int ReadFileToSprite(char *filename, struct S_Object *sprite, Drawable drawable)
{
  Imlib_Image im;
  int width, height, nviews;

  if (access(filename, R_OK) != 0) {
    fprintf(stderr, "image %s is not readable\n", filename);
    goto fail;
  }

  im = imlib_load_image(filename);
  if (!im) {
    fprintf(stderr, "image %s failed to load\n", filename);
    goto fail;
  }

  imlib_context_set_display(W_Display);
  imlib_context_set_visual(W_Visual);
  imlib_context_set_colormap(W_Colormap);
  imlib_context_set_image(im);
  imlib_context_set_drawable(drawable);
  imlib_render_pixmaps_for_whole_image(&sprite->image, &sprite->shape);

  width = imlib_image_get_width();
  height = imlib_image_get_height();
  nviews = height / width;
  if (nviews * width != height) {
    nviews = 1;
  }

  sprite->drawable = drawable;
  sprite->gc = XCreateGC(W_Display, sprite->image, 0, NULL);
  sprite->view = 0;
  sprite->nviews = nviews;
  sprite->width = width;
  sprite->height = height / nviews;
  sprite->cloak = 0;
#ifdef DEBUG
  fprintf(stderr, "image %s loaded, nv=%d w=%d h=%d (%d)\n", filename,
          nviews, width, height, sprite->height);
#endif
  imlib_free_image_and_decache();
  return 0;

 fail:
  sprite->image = NoPixmapError;
  sprite->nviews = 1;
  return 1;
}

int     ReadFileToTile(char *filename, Pixmap *pix, Drawable drawable)
{
  Imlib_Image im;

  if (access(filename, R_OK) != 0) {
    fprintf(stderr, "image %s is not readable\n", filename);
    goto fail;
  }

  im = imlib_load_image(filename);
  if (!im) {
    fprintf(stderr, "image %s failed to load\n", filename);
    goto fail;
  }

  imlib_context_set_display(W_Display);
  imlib_context_set_visual(W_Visual);
  imlib_context_set_colormap(W_Colormap);
  imlib_context_set_image(im);
  imlib_context_set_drawable(drawable);
  imlib_render_pixmaps_for_whole_image(pix, NULL);
#ifdef DEBUG
  fprintf(stderr, "tile %s loaded, w=%d h=%d\n", filename,
          imlib_image_get_width(), imlib_image_get_height());
#endif
  imlib_free_image_and_decache();
  return 0;

 fail:
  *pix = NoPixmapError;
  return 1;
}

static char pixmapDir[1024] = { '\0' };

static void GetPixmapDir()
{
  char   *pd;

  if (pixmapDir[0] != '\0') return;

  pd = getdefault("pixmapDir");
  if (pd != (char *) NULL)
    strncpy(pixmapDir, pd, 1024);
  else
    strcpy(pixmapDir, "/usr/share/pixmaps/netrek-client-cow");

  if ((strcmpi(pixmapDir, "None") == 0) || (pixMissing & NO_PIXMAPS)) {
    pixMissing = NO_IND_PIX | NO_FED_PIX | NO_ROM_PIX |
      NO_KLI_PIX | NO_ORI_PIX | NO_WEP_PIX | NO_EXP_PIX |
      NO_CLK_PIX | NO_MAP_PIX | NO_BG_PIX | NO_PIXMAPS;
    fprintf(stderr, "pixmaps turned off\n");
  } else {
    struct stat buf;

    if ((stat(pixmapDir, &buf)) || (!(S_ISDIR(buf.st_mode)))) {
      /* if .xtrekrc is wrong, and package default wrong, try default dir */
      if ((stat("pixmaps", &buf)) || (!(S_ISDIR(buf.st_mode)))) {
	pixMissing = NO_IND_PIX | NO_FED_PIX | NO_ROM_PIX |
	  NO_KLI_PIX | NO_ORI_PIX | NO_WEP_PIX | NO_EXP_PIX |
	  NO_CLK_PIX | NO_MAP_PIX | NO_BG_PIX | NO_PIXMAPS;
	fprintf(stderr, "pixmaps not here\n");
      } else {
	strcpy(pixmapDir, "pixmaps");
      }
    }
  }
}

void    GetPixmaps(Display * d, struct window *win, W_Window t, W_Window g)
{
  register int i, j;
  char    buf[1024];
  int     missing;

  Drawable tactical = W_Void2Window(t)->window;
  Drawable galactic = W_Void2Window(g)->window;
  W_Display = d;

  GetPixmapDir();

  for (i = 0; i < NUMTEAM + 1; i++)
    {
      missing = 0;
      for (j = 0; j < NUM_TYPES; j++)
	{
	  sprintf(buf, "%s/%s/%s", pixmapDir, teamnames[i], shipfiles[j]);
	  missing += ReadFileToSprite(buf, &shipImg[i][j], tactical);
	}
      if (missing == NUM_TYPES)
	{
	  pixMissing |= reremap[i];
	  if (!(pixMissing & NO_PIXMAPS))
	    fprintf(stderr, "type %s ship pixmaps not available\n", teamnames[i]);
	}
    }

  missing = 0;
  for (i = 0; i < NUMTEAM + 1; i++)
    {
      for (j = 0; j < 2; j++)
	{
	  sprintf(buf, "%s/%s/%s", pixmapDir, teamnames[i], torpfiles[j]);
	  missing += ReadFileToSprite(buf, &torpImg[i][j], tactical);

	  sprintf(buf, "%s/%s/%s", pixmapDir, teamnames[i], plasmafiles[j]);
	  missing += ReadFileToSprite(buf, &plasmaImg[i][j], tactical);
	}
    }
  if (missing == (NUMTEAM + 1) * 4)
    {
      pixMissing |= NO_WEP_PIX;
      if (!(pixMissing & NO_PIXMAPS))
	fprintf(stderr, "type weapon pixmaps not available\n");
    }

  missing = 0;
  for (i = 0; i < 2; i++)
    {
      sprintf(buf, "%s/Misc/%s", pixmapDir, explosionfiles[i]);
      missing += ReadFileToSprite(buf, &explosionImg[i], tactical);
    }
  if (missing == 2)
    {
      pixMissing |= NO_EXP_PIX;
      if (!(pixMissing & NO_PIXMAPS))
	fprintf(stderr, "type explosion pixmaps not available\n");
    }

  missing = 0;
  for (i = 0; i < NUM_PL_IMGS && !missing; i++)
    {
      sprintf(buf, "%s/Planets/Map/%s", pixmapDir, mplanetfiles[i]);
      if (i == PL_PIX_AGRI && missing == 0)
	{
	  /* If the AGRI pixmap is missing, use the ROCK pixmap */
	  if (ReadFileToSprite(buf, &mplanetImg[i], galactic) != 0)
	    {
	      sprintf(buf, "%s/Planets/%s", pixmapDir, mplanetfiles[i - 1]);
	      ReadFileToSprite(buf, &mplanetImg[i], galactic);
	    }
	}
      else
	missing += ReadFileToSprite(buf, &mplanetImg[i], galactic);
    }
  if (missing)
    {
      pixMissing |= NO_MAP_PIX;
      if (!(pixMissing & NO_PIXMAPS))
	fprintf(stderr, "type map pixmaps not available\n");
    }

  sprintf(buf, "%s/Misc/%s", pixmapDir, cloakfile);
  if (ReadFileToSprite(buf, &cloakImg, tactical))
    {
      pixMissing |= NO_CLK_PIX;
      if (!(pixMissing & NO_PIXMAPS))
	fprintf(stderr, "type cloak pixmaps not available\n");
    }

  missing = 0;
  for (i = 0; i < NUM_BG_IMGS; i++)
    {
      sprintf(buf, "%s/Misc/%s", pixmapDir, bgfiles[i]);
      missing += ReadFileToTile(buf, &backPix[i], galactic);
    }
  if (missing == NUM_BG_IMGS)
    {
      pixMissing |= NO_BG_PIX;
      if (!(pixMissing & NO_PIXMAPS))
	fprintf(stderr, "type background pixmaps not available\n");
    }

  pixMissing &= ~NO_HALOS;
}

/*************************************************************************
 *                Now for the drawing routines                           *
 *************************************************************************/

int     W_DrawSprite(void *in, int x, int y, int winside)
{
  const int view = SCALE * winside / 2;
  struct S_Object *sprite = (struct S_Object *) in;
  int     dx, dy;

  if ((sprite == NULL) || (sprite->view < 0) || (sprite->view >= sprite->nviews))
    return 0;

  if (x > view || x < -view || y > view || y < -view)
    return 0;

  dx = x - (sprite->width) / 2;
  dy = y - (sprite->height) / 2;

  if ((sprite->cloak > 0) && !(pixFlags & NO_CLK_PIX))
    {
      XSetClipMask(W_Display, sprite->gc, cloakImg.shape);
      XSetClipOrigin(W_Display, sprite->gc,
		     dx, dy - cloakImg.view * (cloakImg.height));
    }
  else
    {
      XSetClipMask(W_Display, sprite->gc, sprite->shape);
      XSetClipOrigin(W_Display, sprite->gc,
		     dx, dy - sprite->view * (sprite->height));
    }

  XCopyArea(W_Display, sprite->image, sprite->drawable, sprite->gc,
	    0, (sprite->view) * (sprite->height),
	    sprite->width, sprite->height,
	    dx, dy);

  return (sprite->width);
}

void W_DrawSpriteAbsolute(void *in, int x, int y)
{
  struct S_Object *sprite = (struct S_Object *) in;

  if (sprite == NULL)
    return;

  XSetClipMask(W_Display, sprite->gc, sprite->shape);
  XSetClipOrigin(W_Display, sprite->gc, x, y);
  XCopyArea(W_Display, sprite->image, sprite->drawable, sprite->gc,
            0, 0, sprite->width, sprite->height, x, y);
}

void W_ClearSpriteAbsolute(void *in, int x, int y)
{
  struct S_Object *sprite = (struct S_Object *) in;

  if (sprite == NULL)
    return;

  XSetClipMask(W_Display, sprite->gc, sprite->shape);
  XSetClipOrigin(W_Display, sprite->gc, x, y);
  XClearArea(W_Display, sprite->drawable,
             x, y, sprite->width, sprite->height, False);
}

void   *S_Ship(int playerno)
{
  struct S_Object *sprite;
  struct player *this;

  if ((playerno > MAXPLAYER) || (playerno < 0))
    return ((void *) NULL);

  this = &players[playerno];
  sprite = &(shipImg[remap[this->p_team]][this->p_ship.s_type]);
  sprite->cloak = 0;

  /* The following generalizes "rosette(x)" for an arbitrary number of
     views in the image */

  sprite->view = ((((this->p_dir) + 128 / (sprite->nviews))
		   / (256 / (sprite->nviews))) % (sprite->nviews));

  if ((this->p_status != PALIVE) && (this->p_status != PEXPLODE))
    {
      return ((void *) NULL);
    }
  else if ((this->p_flags & PFOBSERV) && !(this->p_flags & PFPLOCK))
    {
      return ((void *) NULL);
    }
  else if ((this->p_flags & PFOBSERV) && !(this->p_flags & PFCLOAK))
    {
      return ((void *) NULL);
    }
  else if (this->p_status == PEXPLODE)
    {
      int     i = this->p_explode * 5 / server_ups;

      if (pixFlags & NO_EXP_PIX)
	return ((void *) NULL);

      if (this->p_ship.s_type == STARBASE)
	sprite = &explosionImg[1];
      else
	sprite = &explosionImg[0];

      if (i >= sprite->nviews)
	return ((void *) NULL);

      sprite->view = i;
      this->p_explode++;
    }
  else if ((this->p_flags & PFCLOAK) || (this->p_cloakphase > 0))
    {
      if (this->p_cloakphase == (CLOAK_PHASES - 1))
	{
	  if ((this == me) && !(pixFlags & NO_CLK_PIX))
	    {
	      sprite = &cloakImg;
	      sprite->view = cloakImg.nviews - 1;
	    }
	  else
	    {
	      return ((void *) NULL);
	    }
	}
      else
	{
	  sprite->cloak = this->p_cloakphase;
	  cloakImg.view = cloakImg.nviews - this->p_cloakphase - 1;
	}
    }

  if ((sprite->image == NoPixmapError) ||
      (pixFlags & reremap[remap[this->p_team]]))
    return ((void *) NULL);
  else
    return ((void *) sprite);
}

void   *S_mPlanet(int planetno)
{
  struct S_Object *sprite;
  struct planet *this = &planets[planetno];

  if (pixFlags & NO_MAP_PIX)
    return ((void *) NULL);

  if ((this->pl_info & me->p_team)

#ifdef RECORDGAME
      || playback
#endif

      )
    {
      if ((this->pl_flags & PLAGRI) && (F_agri_pix))
	sprite = &mplanetImg[PL_PIX_AGRI];
      else
	sprite = &mplanetImg[PL_PIX_ROCK];

    }
  else
    {
      sprite = &mplanetImg[PL_PIX_UKN];
    }

  sprite->view = 0;

  return ((void *) sprite);
}

void   *S_mArmy(int planetno)
{
  struct S_Object *sprite = NULL;
  struct planet *this = &planets[planetno];

  if ((pixFlags & NO_MAP_PIX) || (showgalactic != 1))
    return ((void *) NULL);

  if (((this->pl_info & me->p_team)

#ifdef RECORDGAME
       || playback
#endif

      ) && (this->pl_armies > 4))
    {
      sprite = &mplanetImg[PL_PIX_ARMY];
      sprite->view = 0;
    }

  return ((void *) sprite);
}

void   *S_mRepair(int planetno)
{
  struct S_Object *sprite = NULL;
  struct planet *this = &planets[planetno];

  if ((pixFlags & NO_MAP_PIX) || (showgalactic != 1))
    return ((void *) NULL);

  if (((this->pl_info & me->p_team)

#ifdef RECORDGAME
       || playback
#endif

      ) && (this->pl_flags & PLREPAIR))
    {
      sprite = &mplanetImg[PL_PIX_REPAIR];
      sprite->view = 0;
    }

  return ((void *) sprite);
}

void   *S_mFuel(int planetno)
{
  struct S_Object *sprite = NULL;
  struct planet *this = &planets[planetno];

  if ((pixFlags & NO_MAP_PIX) || (showgalactic != 1))
    return ((void *) NULL);

  if (((this->pl_info & me->p_team)

#ifdef RECORDGAME
       || playback
#endif

      ) && (this->pl_flags & PLFUEL))
    {
      sprite = &mplanetImg[PL_PIX_FUEL];
      sprite->view = 0;
    }

  return ((void *) sprite);
}

void   *S_mOwner(int planetno)
{
  struct S_Object *sprite = NULL;
  struct planet *this = &planets[planetno];

  if ((pixFlags & NO_MAP_PIX) || (showgalactic != 0))
    return ((void *) NULL);

  if ((this->pl_info & me->p_team)

#ifdef RECORDGAME
      || playback
#endif

      )
    {
      sprite = &mplanetImg[PL_PIX_IND + remap[this->pl_owner]];
      sprite->view = 0;
    }

  return ((void *) sprite);
}

void   *S_Torp(int torpno)
{
  struct S_Object *sprite;
  struct torp *this = &torps[torpno];
  int numdetframes, frame;

  if (this->t_status == TEXPLODE)
    {
      sprite = &torpImg[remap[players[this->t_owner].p_team]][1];
      this->t_fuse--;
      numdetframes = NUMDETFRAMES * server_ups / 5;
      frame = this->t_fuse * 5 / server_ups;
      if (this->t_fuse <= 0)
	{
	  this->t_status = PTFREE;
	  players[this->t_owner].p_ntorp--;
	}
      else if (this->t_fuse >= numdetframes)
	{
	  this->t_fuse = numdetframes - 1;
	}
      else
	{
	  sprite->view = frame;
	}
    }
  else
    {
      sprite = &torpImg[remap[players[this->t_owner].p_team]][0];
      sprite->view = ++sprite->view % sprite->nviews;
      // FIXME: torps rotate faster with higher client update rates
    }

  if ((sprite->image == NoPixmapError) || (pixFlags & NO_WEP_PIX))
    return (NULL);
  else
    return ((void *) sprite);
}

void   *S_Plasma(int plasmatorpno)
{
  struct S_Object *sprite;
  struct plasmatorp *this = &plasmatorps[plasmatorpno];
  int numdetframes, frame;

  if (this->pt_status == PTEXPLODE)
    {
      sprite = &plasmaImg[remap[players[this->pt_owner].p_team]][1];
      this->pt_fuse--;
      numdetframes = NUMDETFRAMES * server_ups / 10;
      frame = this->pt_fuse * 10 / server_ups;
      if (this->pt_fuse <= 0)
	{
	  this->pt_status = PTFREE;
	  players[this->pt_owner].p_nplasmatorp--;
	}
      else if (this->pt_fuse >= numdetframes)
	{
	  this->pt_fuse = numdetframes - 1;
	}
      else
	{
	  sprite->view = frame;
	}
    }
  else
    {
      sprite = &plasmaImg[remap[players[this->pt_owner].p_team]][0];
      sprite->view = ++sprite->view % sprite->nviews;
    }

  if ((sprite->image == NoPixmapError) || (pixFlags & NO_WEP_PIX))
    return (NULL);
  else
    return ((void *) sprite);
}

void    W_GalacticBgd(int which)
{
  struct window *win = W_Void2Window(mapw);

  if ((backPix[which] == NoPixmapError) || (pixFlags & NO_BG_PIX))
    W_UnTileWindow(mapw);
  else
    XSetWindowBackgroundPixmap(W_Display, win->window, backPix[which]);

  W_ClearWindow(mapw);
}

void    W_LocalBgd(int which)
{
  struct window *win = W_Void2Window(w);

  if ((backPix[which] == NoPixmapError) || (pixFlags & NO_BG_PIX))
    W_UnTileWindow(w);
  else
    XSetWindowBackgroundPixmap(W_Display, win->window, backPix[which]);

  W_ClearWindow(w);
}

void    W_SetBackground(W_Window w, int which)
{
  struct window *win = W_Void2Window(w);

  if ((backPix[which] == NoPixmapError) || (pixFlags & NO_BG_PIX))
    W_UnTileWindow(w);
  else
    XSetWindowBackgroundPixmap(W_Display, win->window, backPix[which]);

  W_ClearWindow(w);
}

void *W_SetBackgroundImage(W_Window w, char *name)
{
  Drawable drawable = W_Void2Window(w)->window;
  struct S_Object *sprite = calloc(1, sizeof(struct S_Object));
  char *path;

  if (sprite == NULL) return NULL;

  GetPixmapDir();
  path = malloc(strlen(pixmapDir) + strlen(name) + 2);
  if (path == NULL) return NULL;

  sprintf(path, "%s/%s", pixmapDir, name);

  if (ReadFileToSprite(path, sprite, drawable)) {
    free(path);
    return NULL;
  }
  free(path);

  XSetWindowBackgroundPixmap(W_Display, drawable, sprite->image);
  W_ClearWindow(w);
  return (void *) sprite;
}

static struct S_Object *ss = NULL;
static int ss_size = 0;
static int ss_next = 0;
static int ss_show = 0;

static void ss_init(W_Window w)
{
  char *path, *argv[2];
  FTS *fts;
  FTSENT *ent;
  Drawable drawable = W_Void2Window(w)->window;

  if (ss != NULL) return;

  GetPixmapDir();

  path = malloc(strlen(pixmapDir) + 4);
  sprintf(path, "%s/ss", pixmapDir);
  argv[0] = path;
  argv[1] = NULL;

  ss_size = 50;
  ss = (struct S_Object *) malloc(ss_size * sizeof(struct S_Object));
  ss_next = 0;

  fts = fts_open(argv, FTS_LOGICAL, NULL);
  while ((ent = fts_read(fts))) {
    if (ent->fts_info != FTS_F) continue;
    memset(&ss[ss_next], 0, sizeof(struct S_Object));
    if (ReadFileToSprite(ent->fts_path, &ss[ss_next], drawable)) continue;
    ss_next++;
    if (ss_next >= ss_size) {
      ss_size += 10;
      ss = realloc(ss, ss_size * sizeof(struct S_Object));
    }
  }
  fts_close(fts);
  free(path);
}

void    W_NextScreenShot(W_Window w, int x, int y)
{
  ss_init(w);
  if (ss_next == 0) return;
  W_ClearSpriteAbsolute(&ss[ss_show], x, y);
  ss_show++;
  if (ss_show >= ss_next) ss_show=0;
  W_DrawSpriteAbsolute(&ss[ss_show], x, y);
}

void    W_DrawScreenShot(W_Window w, int x, int y)
{
  ss_init(w);
  if (ss_next == 0) return;
  W_DrawSpriteAbsolute(&ss[ss_show], x, y);
}

void    *W_ReadImage(W_Window w, char *name)
{
  Drawable drawable = W_Void2Window(w)->window;
  struct S_Object *sprite = calloc(1, sizeof(struct S_Object));
  char *path;

  if (sprite == NULL) return NULL;

  GetPixmapDir();
  path = malloc(strlen(pixmapDir) + strlen(name) + 2);
  if (path == NULL) return NULL;

  sprintf(path, "%s/%s", pixmapDir, name);

  if (ReadFileToSprite(path, sprite, drawable)) {
    free(path);
    return NULL;
  }
  free(path);
  return (void *) sprite;
}

void    W_DrawImage(int x, int y, void *sprite_v)
{
  struct S_Object *sprite = (struct S_Object *) sprite_v;

  if (sprite == NULL) return;

  W_DrawSpriteAbsolute(sprite, x, y);
}

void    W_DropImage(void *sprite_v)
{
  struct S_Object *sprite = (struct S_Object *) sprite_v;

  if (sprite == NULL) return;

  XFreeGC(W_Display, sprite->gc);
  XFreePixmap(W_Display, sprite->image);
  XFreePixmap(W_Display, sprite->shape);
  free(sprite);
}
