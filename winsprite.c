/* winsprite.c
 * Xpm Windowing code for GNU Win32 API
 * Kurt Siegl May 1998
 * 
 * $Log: winsprite.c,v $
 * Revision 1.3  1999/06/20 15:57:49  siegl
 * Add a magic 3 to draw the sprites on the correct position
 *
 * Revision 1.2  1999/01/31 16:38:17  siegl
 * Hockey rink background XPM on galactic map in hockey mode.
 *
 * Revision 1.1.1.1  1998/11/01 17:24:12  siegl
 * COW 3.0 initial revision
 * */
#include "copyright2.h"

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "config.h"
#ifdef HAVE_XPM
#include <stdlib.h>
#include <sys/param.h>

#include <sys/stat.h>
#include <unistd.h>

#define FOR_MSW 
#include INC_XPM

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"

typedef struct tagWindow
   {
   HWND hwnd;
   short type;
   short border;
   W_Color BorderColor;
   short UsingCreatedCursor;
   HCURSOR cursor;
   short tiled;
   struct Icon *TileIcon;
   short NumItems;
   struct menuItem *items;
   struct stringList *strings;
   short AddedStrings;
   short TextHeight;
   short TextWidth;
   RECT ClipRect;
   W_Callback HandleKeydown;
   W_Callback HandleKeyup;
   W_Callback HandleButton;
   W_Callback HandleExpose;
   }
Window;


#define W_Void2Window(win) ((win) ? ((Window *) (win)) : (mylocal))
#define NoPixmapError 0

struct S_Object
{
  W_Window       *parent;
  int            view, nviews, width, height, cloak;
  HBITMAP image;
  HBITMAP shape;
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

const int reremap[5] = { NO_IND_PIX, NO_FED_PIX, NO_ROM_PIX, 
                         NO_KLI_PIX, NO_ORI_PIX };

static struct S_Object mplanetImg[NUM_PL_IMGS];
static struct S_Object shipImg[NUMTEAM+1][NUM_TYPES];
static struct S_Object torpImg[NUMTEAM+1][2];
static struct S_Object plasmaImg[NUMTEAM+1][2];
static struct S_Object cloakImg;
static struct S_Object explosionImg[2];
/*
static struct S_Object planetImg[NUM_PL_IMGS];
static struct S_Object shieldImg[NUM_SH_IMGS];
static struct S_Object hullImg[NUM_HL_IMGS];
 */

HDC GlobalMemDC, GlobalMemDC2;
static HBITMAP backPix[NUM_BG_IMGS];
  
extern Window  W_Root;
extern int takeNearest;

Display W_Display;
Window *mylocal;

const char teamnames[NUMTEAM+1][4] = {"Ind","Fed","Rom","Kli","Ori"};

const char mplanetfiles[NUM_PL_IMGS][12] = { "UNKN.xpm",
                                          "ROCK.xpm",
                                          "AGRI.xpm",
                                          "army.xpm",
                                          "repair.xpm",
                                          "fuel.xpm",
                                          "Ind.xpm",
                                          "Fed.xpm",
                                          "Rom.xpm",
                                          "Kli.xpm",
                                          "Ori.xpm"
                                        };
const char shipfiles[NUM_TYPES][8] = { "SC.xpm",
                                      "DD.xpm",
                                      "CA.xpm",
                                      "BB.xpm",
                                      "AS.xpm",
                                      "SB.xpm",
                                      "GA.xpm",
                                      "AT.xpm"
                                     };
const char torpfiles[2][14] = { "torp.xpm", "torp_det.xpm"};
const char plasmafiles[2][16] = { "plasma.xpm", "plasma_det.xpm"};
const char cloakfile[10] = "cloak.xpm";
const char explosionfiles[2][18] = { "explosion.xpm", "sbexplosion.xpm"};
const char bgfiles[NUM_BG_IMGS][16] = { "map_back.xpm",
                                      "local_back.xpm",
                                      "ghostbust.xpm",
                                      "genocide.xpm",
                                      "greet.xpm",
                                      "hockey.xpm"
                                    }; 

int ReadFileToSprite(char *filename, struct S_Object *sprite, W_Window *w)
{
  register int i,j,k;
  XpmAttributes attr;
  int nviews=0;
  Window *win = W_Void2Window(mylocal);
  XImage  *image;
  XImage  *shape;

#ifdef DEBUG
   printf("ReadFileToSprite: %s\n",filename);
#endif

  attr.visual = 0;
  attr.colormap = 0;
  attr.exactColors = takeNearest;
  attr.closeness = 40000;
  attr.valuemask = XpmVisual|XpmColormap|XpmExactColors|
                     XpmReturnExtensions|XpmCloseness;

  if (XpmReadFileToImage(W_Display,filename,&image,&shape,&attr) != XpmSuccess)
  {
    if (!(pixMissing & NO_PIXMAPS))
      fprintf(stderr,"Unable to read file %s.\n", filename);
    sprite->image = NoPixmapError;
    sprite->nviews = 1;
    return(1);
  }
  sprite->image = image->bitmap;
  sprite->shape = NoPixmapError; // shape->bitmap;
  XImageFree(image);
//  XImageFree(shape);

  for (k=0; k < attr.nextensions; k++)
  {
    if (strcmpi(attr.extensions[0].name, "num_views") == 0)
      nviews = atoi(attr.extensions[0].lines[0]);
  }

  if (nviews == 0)
  {
    int guess;

    guess = (int)((attr.height)/(attr.width));
    if ( guess == (attr.height)/(attr.width))
    {
      nviews = guess;
    }
    else
    {
      if (!(pixMissing & NO_PIXMAPS))
        fprintf(stderr,"NUM_VIEWS not suppied in %s.  Unable to estimate ...\n",
              filename);
      sprite->image = NoPixmapError;
      sprite->nviews = 1;
      return(1);
    }
  }

  sprite->parent=w;
  sprite->nviews=nviews;
  sprite->width=attr.width;
  sprite->height=(attr.height)/nviews;

  return(0);
}

int ReadFileToTile(char *filename, HBITMAP *pix)
{
  XpmAttributes attr;
  Window *win = W_Void2Window(mylocal);
  XImage  *image;

  #ifdef DEBUG
   printf("ReadFileToTile: %s\n",filename);
#endif

  attr.visual = 0;
  attr.colormap = 0;
  attr.exactColors = takeNearest;
  attr.closeness = 40000;
  attr.valuemask = XpmVisual|XpmColormap|XpmExactColors|
                     XpmReturnExtensions|XpmCloseness;

  if (XpmReadFileToImage(W_Display,  filename, &image, NULL,&attr)
              != XpmSuccess)
  {
    if (!(pixMissing & NO_PIXMAPS))
      fprintf(stderr,"Unable to read file %s.\n", filename);
    *pix = NoPixmapError;
    return(1);
  }
  *pix = image->bitmap;
  XImageFree(image);

  return(0);
}

void GetPixmaps(Display *d, Window *win)
{
  register int i,j,k;
  char buf[1024], pixmapDir[1024];
  char *pd;
  XImage *image, *shape;
  int missing;

  mylocal=win;
  W_Display=d;

  pd = getdefault("pixmapDir");
  if (pd != (char *) NULL)
    strcpy(pixmapDir, pd);
  else
    strcpy(pixmapDir,"./pixmaps");

  if ((strcmpi(pixmapDir,"None") == 0) || (pixMissing & NO_PIXMAPS))
  {
    pixMissing = NO_IND_PIX | NO_FED_PIX | NO_ROM_PIX |
                 NO_KLI_PIX | NO_ORI_PIX | NO_WEP_PIX | NO_EXP_PIX |
                 NO_CLK_PIX | NO_MAP_PIX | NO_BG_PIX | NO_PIXMAPS ;
    fprintf(stderr,"Pixmaps turned OFF\n");
  }
  else
  {
    struct stat buf;
 	if ((stat(pixmapDir,&buf)) || (! (S_ISDIR(buf.st_mode))))
    {
        pixMissing = NO_IND_PIX | NO_FED_PIX | NO_ROM_PIX |
                     NO_KLI_PIX | NO_ORI_PIX | NO_WEP_PIX | NO_EXP_PIX |
                     NO_CLK_PIX | NO_MAP_PIX | NO_BG_PIX | NO_PIXMAPS ;
        fprintf(stderr,"Pixmaps dir not found - turned OFF\n");
    }
  }

  for (i=0; i<NUMTEAM+1; i++)
  {
    missing = 0;
    for (j=0; j<NUM_TYPES; j++)
    {
      sprintf(buf,"%s/%s/%s",pixmapDir,teamnames[i],shipfiles[j]);
      missing += ReadFileToSprite(buf, &shipImg[i][j], &w);
    }
    if (missing == NUM_TYPES)
    {
      pixMissing |= reremap[i];
      if (!(pixMissing & NO_PIXMAPS))
        fprintf(stderr,"TYPE %s ship PIXMAPS NOT AVAILABLE\n", teamnames[i]);
    }
  }

  missing = 0;
  for (i=0; i<NUMTEAM+1; i++)
  {
    for (j=0; j<2; j++)
    {
      sprintf(buf,"%s/%s/%s",pixmapDir,teamnames[i],torpfiles[j]);
      missing += ReadFileToSprite(buf, &torpImg[i][j], &w);

      sprintf(buf,"%s/%s/%s",pixmapDir,teamnames[i],plasmafiles[j]);
      missing += ReadFileToSprite(buf, &plasmaImg[i][j], &w);
    }
  }
  if (missing == (NUMTEAM+1)*4)
  {
    pixMissing |= NO_WEP_PIX;
    if (!(pixMissing & NO_PIXMAPS))
      fprintf(stderr,"TYPE weapon PIXMAPS NOT AVAILABLE\n");
  }

  missing=0;
  for (i=0; i<2; i++)
  {
    sprintf(buf,"%s/Misc/%s",pixmapDir,explosionfiles[i]);
    missing += ReadFileToSprite(buf, &explosionImg[i], &w);
  }
  if (missing == 2)
  {
    pixMissing |= NO_EXP_PIX;
    if (!(pixMissing & NO_PIXMAPS))
      fprintf(stderr,"TYPE explosion PIXMAPS NOT AVAILABLE\n");
  }

  missing=0;
  for (i=0; i<NUM_PL_IMGS && !missing; i++)
  {
    sprintf(buf,"%s/Planets/Map/%s",pixmapDir,mplanetfiles[i]);
    if ( i == PL_PIX_AGRI && missing == 0)
    {
      /*  If the AGRI pixmap is missing, use the ROCK pixmap */
      if (ReadFileToSprite(buf, &mplanetImg[i], &mapw) != 0)
      {
        sprintf(buf,"%s/Planets/%s",pixmapDir,mplanetfiles[i-1]);
        ReadFileToSprite(buf, &mplanetImg[i], &mapw);
      }
    }
    else
      missing+=ReadFileToSprite(buf, &mplanetImg[i], &mapw);
  }
  if (missing)
  {
    pixMissing |= NO_MAP_PIX;
    if (!(pixMissing & NO_PIXMAPS))
      fprintf(stderr,"TYPE map PIXMAPS NOT AVAILABLE\n");
  }

  sprintf(buf,"%s/Misc/%s",pixmapDir,cloakfile);
  if (ReadFileToSprite(buf, &cloakImg, &w))
  {
    pixMissing |= NO_CLK_PIX;
    if (!(pixMissing & NO_PIXMAPS))
      fprintf(stderr,"TYPE cloak PIXMAPS NOT AVAILABLE\n");
  }

  missing = 0;
  for (i=0; i<NUM_BG_IMGS; i++)
  {
      sprintf(buf,"%s/Misc/%s",pixmapDir,bgfiles[i]);
      missing += ReadFileToTile(buf, &backPix[i]);
  }
  if (missing == NUM_BG_IMGS)
  {
    pixMissing |= NO_BG_PIX;
    if (!(pixMissing & NO_PIXMAPS))
      fprintf(stderr,"TYPE background PIXMAPS NOT AVAILABLE\n");
  }

  pixMissing &= ~NO_HALOS;
}

/*************************************************************************
 *                Now for the drawing routines                           *
 *************************************************************************/

int W_DrawSprite(void *in, int x, int y)
{
  const int view = SCALE * TWINSIDE / 2;
  Window *win;
  HDC hdc;
  struct S_Object *sprite = (struct S_Object *)in;
  int dx,dy,frame;

  if ((sprite == NULL)||(sprite->view < 0)||(sprite->view >= sprite->nviews))
    return;

  if (x > view || x < -view || y > view || y < -view)
    return;

  win = W_Void2Window(*(sprite->parent));

  dx = x-(sprite->width)/2 +3; /* [007] Dont ask why but I need this magic 3 */
  dy = y-(sprite->height)/2 +3;

#ifdef LATER
  if ((sprite->cloak > 0) && !(pixFlags & NO_CLK_PIX))
  {
    XSetClipMask(W_Display,sprite->gc,cloakImg.shape);
    XSetClipOrigin(W_Display, sprite->gc,
                   dx, dy - cloakImg.view*(cloakImg.height));
  }
  else
  {
    XSetClipMask(W_Display, sprite->gc, sprite->shape);
    XSetClipOrigin(W_Display, sprite->gc,
                   dx, dy - sprite->view*(sprite->height));
  }
#endif   

   hdc = GetDC( win->hwnd );
/*
   if (NetrekPalette)
      {
      SelectPalette(hdc, NetrekPalette, FALSE);
      RealizePalette(hdc);
      }
*/
   SelectObject(GlobalMemDC, sprite->image);

   BitBlt(hdc, dx, dy,                                //Copy the bitmap
            sprite->width,
            sprite->height,
            GlobalMemDC,
            0,
            (sprite->view)*(sprite->height),
            SRCPAINT);                                   // <-- using OR mode

   ReleaseDC( win->hwnd, hdc);

  return(sprite->width);
}

void *S_Ship(int playerno)
{
  struct S_Object *sprite;
  struct player *this;

  if ((playerno > MAXPLAYER) || (playerno < 0))
    return((void *)NULL);

  this  = &players[playerno];
  sprite=&(shipImg[remap[this->p_team]][this->p_ship.s_type]);
  sprite->cloak = 0;

  /*  The following generalizes "rosette(x)" for
      an arbitrary number of views in the xpm    */

  sprite->view=((( (this->p_dir) + 128/(sprite->nviews))
                   / (256/(sprite->nviews))) % (sprite->nviews) );

  if ((this->p_status != PALIVE) && (this->p_status != PEXPLODE))
  {
    return((void *)NULL);
  }
  else if ((this->p_flags & PFOBSERV) && !(this->p_flags & PFPLOCK))
  {
    return((void *)NULL);
  }
  else if ((this->p_flags & PFOBSERV) && !(this->p_flags & PFCLOAK))
  {
    return((void *)NULL);
  }
  else if (this->p_status == PEXPLODE)
  {
    int i=this->p_explode;

    if (pixFlags & NO_EXP_PIX)
      return((void *)NULL);

    if (this->p_ship.s_type == STARBASE)
      sprite=&explosionImg[1];
    else
      sprite=&explosionImg[0];

    if (i >= sprite->nviews)
      return((void *)NULL);

    sprite->view=i;
    this->p_explode++;
  }
  else if ((this->p_flags & PFCLOAK) || (this->p_cloakphase > 0))
  {
    if (this->p_cloakphase == (CLOAK_PHASES - 1))
    {
      if ((this == me) && !(pixFlags & NO_CLK_PIX))
      {
        sprite=&cloakImg;
        sprite->view = cloakImg.nviews - 1;
      }
      else
      {
        return((void *)NULL);
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
    return((void *)NULL);
  else
    return((void *)sprite);
}

void *S_mPlanet(int planetno)
{
  struct S_Object *sprite;
  struct planet *this = &planets[planetno];

  if (pixFlags & NO_MAP_PIX)
    return((void *)NULL);

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

  return((void *)sprite);
}

void *S_mArmy(int planetno)
{
  struct S_Object *sprite = NULL;
  struct planet *this = &planets[planetno];

  if ((pixFlags & NO_MAP_PIX) || (showgalactic != 1))
    return((void *)NULL);

  if (((this->pl_info & me->p_team)
#ifdef RECORDGAME
       || playback
#endif
       ) && (this->pl_armies > 4))
  {
    sprite = &mplanetImg[PL_PIX_ARMY];
    sprite->view = 0;
  }

  return((void *)sprite);
}

void *S_mRepair(int planetno)
{
  struct S_Object *sprite = NULL;
  struct planet *this = &planets[planetno];

  if ((pixFlags & NO_MAP_PIX) || (showgalactic != 1))
    return((void *)NULL);

  if (((this->pl_info & me->p_team) 
#ifdef RECORDGAME
       || playback
#endif
       ) && (this->pl_flags & PLREPAIR))
  {
    sprite = &mplanetImg[PL_PIX_REPAIR];
    sprite->view = 0;
  }

  return((void *)sprite); 
}

void *S_mFuel(int planetno)
{
  struct S_Object *sprite = NULL;
  struct planet *this = &planets[planetno];

  if ((pixFlags & NO_MAP_PIX) || (showgalactic != 1))
    return((void *)NULL);

  if (((this->pl_info & me->p_team)
#ifdef RECORDGAME
       || playback
#endif
       ) && (this->pl_flags & PLFUEL))
  {
    sprite = &mplanetImg[PL_PIX_FUEL];
    sprite->view = 0;
  }

  return((void *)sprite); 
}

void *S_mOwner(int planetno)
{
  struct S_Object *sprite = NULL;
  struct planet *this = &planets[planetno];

  if ((pixFlags & NO_MAP_PIX) || (showgalactic != 0))
    return((void *)NULL);

  if ((this->pl_info & me->p_team)
#ifdef RECORDGAME
      || playback
#endif
      )
  {
    sprite = &mplanetImg[PL_PIX_IND+remap[this->pl_owner]];
    sprite->view = 0;
  }

  return((void *)sprite); 
}

void *S_Torp(int torpno)
{
  struct S_Object *sprite;
  struct torp *this = &torps[torpno];

  if (this->t_status == TEXPLODE)
  {
    sprite = &torpImg[remap[players[this->t_owner].p_team]][1];
    this->t_fuse--;
    if (this->t_fuse <= 0)
    {
      this->t_status = PTFREE;
      players[this->t_owner].p_ntorp--;
    }
    else if (this->t_fuse >= NUMDETFRAMES)
    {
      this->t_fuse = NUMDETFRAMES - 1;
    }
    else
    {
      sprite->view = this->t_fuse;
    }
  }
  else
  {
    sprite = &torpImg[remap[players[this->t_owner].p_team]][0];
    sprite->view = ++sprite->view % sprite->nviews;
  }

  if ((sprite->image == NoPixmapError) || (pixFlags & NO_WEP_PIX))
    return(NULL); 
  else
    return((void *)sprite); 
}

void *S_Plasma(int plasmatorpno)
{
  struct S_Object *sprite;
  struct plasmatorp *this = &plasmatorps[plasmatorpno];

  if (this->pt_status == PTEXPLODE)
  {
    sprite = &plasmaImg[remap[players[this->pt_owner].p_team]][1];
    this->pt_fuse--;
    if (this->pt_fuse <= 0)
    {
      this->pt_status = PTFREE;
      players[this->pt_owner].p_nplasmatorp--;
    }
    else if (this->pt_fuse >= NUMDETFRAMES)
    {
      this->pt_fuse = NUMDETFRAMES - 1;
    }
    else
    {
      sprite->view=this->pt_fuse;
    }
  }
  else
  {
    sprite = &plasmaImg[remap[players[this->pt_owner].p_team]][0];
    sprite->view = ++sprite->view % sprite->nviews;
  }

  if ((sprite->image == NoPixmapError) || (pixFlags & NO_WEP_PIX))
    return(NULL); 
  else
    return((void *)sprite); 
}

void W_GalacticBgd(int which)
{
  Window *win = W_Void2Window(mapw);

  if ((backPix[which] == NoPixmapError) || (pixFlags & NO_BG_PIX))
    W_UnTileWindow(mapw);
  else
    ;
#ifdef LATER
    XSetWindowBackgroundPixmap(W_Display,win->window, backPix[which]);
#endif

  W_ClearWindow(mapw);
}

void W_LocalBgd(int which)
{
  Window *win = W_Void2Window(w);

  if ((backPix[which] == NoPixmapError) || (pixFlags & NO_BG_PIX))
    W_UnTileWindow(w);
  else
;
 #ifdef LATER
   XSetWindowBackgroundPixmap(W_Display,win->window, backPix[which]);
#endif

  W_ClearWindow(w);
}

#endif  /* HAVE_XPM */
