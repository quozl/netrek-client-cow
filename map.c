
/* map.c
 *
 * Functions to maintain the galactic map.  This file is a merger
 * of code from redraw.c, which was too big before, and planets.c,
 * which was too small.
 *
 * $Log: map.c,v $
 * Revision 1.3  1999/08/05 16:46:32  siegl
 * remove several defines (BRMH, RABBITEARS, NEWDASHBOARD2)
 *
 * Revision 1.2  1999/01/31 16:38:17  siegl
 * Hockey rink background XPM on galactic map in hockey mode.
 *
 * Revision 1.1.1.1  1998/11/01 17:24:10  siegl
 * COW 3.0 initial revision
 * */

#include "config.h"
#include "copyright2.h"
#include <ctype.h>
#include <stdio.h>

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "map.h"

#ifdef HAVE_XPM
extern void *S_mPlanet(int);
extern void *S_mArmy(int);
extern void *S_mRepair(int);
extern void *S_mFuel(int);
extern void *S_mOwner(int);
extern int W_DrawSprite(void *, int, int, int);

#endif

/*
 *  Local Constants:
 *  
 *  DETAIL              -- Size of the redraw array.
 *  SIZE                -- Scale of the rough planet map "roughMap".
 *  
 *  Note: Detail *MUST* be a factor of GWIDTH.
 */

#define DETAIL 40
#define SIZE (GWIDTH/DETAIL)


/*
 *  Local Variables:
 *  
 *  roughMap[x][y]      -- Rough map of planets to help find overlaps.
 *  roughMap2[x][y]     -- Secondary rought map, to help with overlap.
 *  initialized         -- Has initPlanets() been called?
 */

static signed char roughMap[DETAIL][DETAIL];
static signed char roughMap2[DETAIL][DETAIL];
static int initialized = 0;


/*
 *  Global Variables:
 *
 *  redrawall           -- Erase and redraw the galactic?  Must be true
 *                              on first map() call.
 *  redrawPlayer[]      -- Flag for each player on whether their position
 *                              on the galactic is not out of date.
 */

int     redrawall = 1;
unsigned char redrawPlayer[MAXPLAYER];



/* Function Definitions */

void
        initPlanets(void)
/*
 *  Make a rough map of the location of all the planets to help decide
 *  whether a ship is possibly overlapping a planet.
 */
{
  int     x, y, k;
  int     startX, startY;
  int     endX, endY;
  struct planet *pl;
  const int pRadius = mplanet_width * GWIDTH / GWINSIDE / 2;
  const int tHeight = W_Textheight * GWIDTH / GWINSIDE;
  const int tWidth = W_Textwidth * GWIDTH / GWINSIDE;

  for (x = 0; x < DETAIL; x++)
    {
      for (y = 0; y < DETAIL; y++)
	{
	  roughMap[x][y] = -1;
	  roughMap2[x][y] = -1;
	}
    }

  for (k = 0, pl = planets; k < MAXPLANETS; k++, pl++)
    {
      /* 
       * Size of planet is pRadius but a ship will touch the planet if it is
       * one character away horizontally or half a character vertically.
       * Also remember the planet name at the bottom. This name can stick out 
       * about half a character to the right. */

      startX = (pl->pl_x - pRadius - tWidth) / SIZE;
      endX = (pl->pl_x + pRadius + tWidth + (tWidth / 2)) / SIZE;

      startY = (pl->pl_y - pRadius - (tHeight / 2)) / SIZE;
      endY = (pl->pl_y + pRadius + tHeight + (tHeight / 2)) / SIZE;

      if (startX < 0)
	startX = 0;

      if (endX >= DETAIL)
	endX = DETAIL - 1;

      if (startY < 0)
	startY = 0;

      if (endY >= DETAIL)
	endY = DETAIL - 1;

      for (x = startX; x <= endX; x++)
	{
	  for (y = startY; y <= endY; y++)
	    {
	      if (roughMap[x][y] == -1)
		roughMap[x][y] = k;
	      else
		roughMap2[x][y] = k;
	    }
	}
    }

  initialized = 1;
}


#ifdef none					 /* Debugging code */
void
        showRegions(void)
/*
 *  Make a rough map of the location of all the planets to help decide
 *  whether a ship is possibly overlapping a planet.
 */
{
  int     x, y, k;
  int     startX, startY, centre;
  int     endX, endY;
  struct planet *pl;
  const int pRadius = mplanet_width * GWIDTH / GWINSIDE / 2;
  const int tHeight = W_Textheight * GWIDTH / GWINSIDE;
  const int tWidth = W_Textwidth * GWIDTH / GWINSIDE;

  for (k = 0, pl = planets; k < MAXPLANETS; k++, pl++)
    {
      startX = (pl->pl_x - pRadius - tWidth) / SIZE;
      endX = (pl->pl_x + pRadius + tWidth + (tWidth / 2)) / SIZE;

      startY = (pl->pl_y - pRadius - (tHeight / 2)) / SIZE;
      endY = (pl->pl_y + pRadius + tHeight + (tHeight / 2)) / SIZE;

      if (startX < 0)
	startX = 0;

      if (endX > DETAIL)
	endX = DETAIL;

      if (startY < 0)
	startY = 0;

      if (endY > DETAIL)
	endY = DETAIL;

      startX = startX * SIZE * GWINSIDE / GWIDTH;
      startY = startY * SIZE * GWINSIDE / GWIDTH;
      endX = (endX * SIZE + SIZE - 1) * GWINSIDE / GWIDTH;
      endY = (endY * SIZE + SIZE - 1) * GWINSIDE / GWIDTH;

      W_MakeLine(mapw, startX, startY, startX, endY, W_White);
      W_MakeLine(mapw, startX, startY, endX, startY, W_White);
      W_MakeLine(mapw, endX, endY, startX, endY, W_White);
      W_MakeLine(mapw, endX, endY, endX, startY, W_White);
    }
}
#endif	/* none */ /* Debugging code */


inline static void
        checkRedraw(int x, int y)
/*
 *  Compare the given location of a ship with the rough planet map created
 *  by initPlanets() to decide if part of the planet may have been erased
 *  by the ship.
 */
{
  int     i;

  x /= SIZE;
  y /= SIZE;

  i = roughMap[x][y];

  if (i != -1)
    {
      planets[i].pl_flags |= PLREDRAW;

      i = roughMap2[x][y];

      if (i != -1)
	{
	  planets[i].pl_flags |= PLREDRAW;
	}
    }
}


inline static W_Icon
        planetmBitmap(register struct planet *p)
/*
 *  Choose the bitmap for a planet.
 */
{
  int     i;

  if (showgalactic == 0)			 /* nothing */
    {
      return (mbplanets[0]);
    }
  else if ((p->pl_info & me->p_team)

#ifdef RECORDGAME
	   || playback
#endif

      )
    {
      if (showgalactic == 1)			 /* owner */
	{
	  return (mbplanets[remap[p->pl_owner]]);
	}
      else
	/* resources */
	{
	  i = 0;
	  if (p->pl_armies > 4)
	    i += 4;
	  if (p->pl_flags & PLREPAIR)
	    i += 2;
	  if (p->pl_flags & PLFUEL)
	    i += 1;
	  switch (showgalactic)
	    {
	    case 2:                                      /* standard */
	      return (mbplanets2[i]);
	    case 3:                                      /* MOO bitmap */
	      return (mbplanets3[i]);
	    case 4:                                      /* rabbit ears */
	      return (mbplanets4[i]);
	    default:
	      return (mbplanets[0]);
	    }
	}
    }
  else
    {
      return (mbplanets[5]);
    }
}

static void DrawPlanets()
/*
 *  Draw the planets on the galactic map.
 */
{
  register struct planet *l;
  register int dx, dy;
  char    ch;

#ifdef HAVE_XPM
  void   *sprite;

#endif

  for (l = planets + MAXPLANETS - 1; l >= planets; --l)
    {
      if (!(l->pl_flags & PLREDRAW))
	continue;

      l->pl_flags &= ~PLREDRAW;			 /* Turn redraw flag off! */

      dx = l->pl_x * GWINSIDE / GWIDTH;
      dy = l->pl_y * GWINSIDE / GWIDTH;


      /* Erase the planet first */

      if (pl_update[l->pl_no].plu_update == 1)
	{
	  /* Allow Moving Planets */

	  int     odx, ody;

	  odx = pl_update[l->pl_no].plu_x * GWINSIDE / GWIDTH;
	  ody = pl_update[l->pl_no].plu_y * GWINSIDE / GWIDTH;

	  /* XFIX */
	  W_ClearArea(mapw, odx - (mplanet_width / 2),
		      ody - (mplanet_height / 2),
		      mplanet_width, mplanet_height);
	  W_WriteText(mapw, odx - (mplanet_width / 2),
		      ody + (mplanet_height / 2),
		      backColor, l->pl_name, 3, planetFont(l));

	  pl_update[l->pl_no].plu_update = 0;
	}
      else

#ifndef WIN32
      if (l->pl_flags & PLCLEAR)
#endif

	{
	  /* Clear the planet normally */

	  /* XFIX */
	  W_ClearArea(mapw, dx - (mplanet_width / 2 + 4),
		      dy - (mplanet_height / 2 + 4),
		      mplanet_width + 8, mplanet_height + 8);
	  l->pl_flags &= ~PLCLEAR;
	}


      /* Draw the new planet */

#ifdef BEEPLITE
      if (UseLite && emph_planet_seq_n[l->pl_no] > 0)
	{
	  int     seq_n = emph_planet_seq_n[l->pl_no] % emph_planet_seq_frames;

	  W_OverlayBitmap(dx - (emph_planet_seq_width / 2 + 1),
			  dy - (emph_planet_seq_height / 2),
			  emph_planet_seq[seq_n],
			  W_White);

	  W_WriteBitmap(dx - (mplanet_width / 2), dy - (mplanet_height / 2),
			planetmBitmap(l), planetColor(l));

	  emph_planet_seq_n[l->pl_no] -= 1;
	  l->pl_flags |= PLREDRAW;		 /* Leave redraw on until * * 
						  * done highlighting */
	  l->pl_flags |= PLCLEAR;		 /* Leave redraw on until * * 
						  * done highlighting */
	}
      else
	{
#endif

#ifdef HAVE_XPM
	  sprite = S_mPlanet(l->pl_no);

	  if (sprite == NULL)
	    pixFlags |= NO_MAP_PIX;

	  if (!(pixFlags & NO_MAP_PIX))
	    {
	      W_DrawSprite(sprite, dx, dy, GWINSIDE);
	      W_DrawSprite(S_mArmy(l->pl_no), dx, dy, GWINSIDE);
	      W_DrawSprite(S_mRepair(l->pl_no), dx, dy, GWINSIDE);
	      W_DrawSprite(S_mFuel(l->pl_no), dx, dy, GWINSIDE);
	      W_DrawSprite(S_mOwner(l->pl_no), dx, dy, GWINSIDE);
	      if (!(pixFlags & NO_HALOS))
		W_Halo(dx, dy, planetColor(l));
	    }

	  else
#endif /* HAVE_XPM */

	    W_OverlayBitmap(dx - (mplanet_width / 2), dy - (mplanet_height / 2),
			    planetmBitmap(l), planetColor(l));


#ifdef BEEPLITE
	}
#endif


      W_WriteText(mapw, dx - (mplanet_width / 2), dy + (mplanet_height / 2),
		  planetColor(l), l->pl_name, 3, planetFont(l));

      if (showIND && ((l->pl_info & me->p_team)

#ifdef RECORDGAME
		      || playback
#endif

	  ) && (l->pl_owner == NOBODY))
	{
	  W_MakeLine(mapw, dx + (mplanet_width / 2 - 1),
		     dy + (mplanet_height / 2 - 1),
		     dx - (mplanet_width / 2), dy - (mplanet_height / 2),
		     W_White);
	  W_MakeLine(mapw, dx - (mplanet_width / 2),
		     dy + (mplanet_height / 2 - 1),
		     dx + (mplanet_width / 2 - 1), dy - (mplanet_height / 2),
		     W_White);
	}

      if (showPlanetOwner)
	{
	  ch = ((l->pl_info & me->p_team)

#ifdef RECORDGAME
		|| playback
#endif

	      )? tolower(teamlet[l->pl_owner]) : '?';
	  W_WriteText(mapw, dx + (mplanet_width / 2) + 2, dy - 4,
		      planetColor(l), &ch, 1, planetFont(l));
	}
    }
}


void
        map(void)
/*
 *  Update the 'galactic' map.
 */
{
  register int i;
  register unsigned char *update;
  register struct player *j;
  register struct planet *l;
  register int dx, dy;

  static char clearlock = 0;
  static int mclearzone[6][MAXPLAYER];
  static int clearlmark[4];
  static unsigned char lastUpdate[MAXPLAYER];


  if (redrawall)
    {
      /* 
       * Set things up so that the galactic will be redraw completely. This
       * code should also put critical variables into a sane state in case
       * COW has just started. */

      if (!initialized)
	{
	  /* Don't do anything if initPlanets() has not been called */
	  return;
	}

#ifdef HAVE_XPM
#ifdef HOCKEY_LINES
  if (hockey_s_lines)
    W_GalacticBgd(HOCKEY_PIX);
  else
#endif
    W_GalacticBgd(MAP_PIX);
#endif

      W_ClearWindow(mapw);
      clearlock = 0;

      for (i = 0; i < MAXPLAYER; i++)
	{
	  lastUpdate[i] = 0;
	  mclearzone[2][i] = 0;
	  redrawPlayer[i] = 1;
	}

      for (l = planets + MAXPLANETS - 1; l >= planets; --l)
	l->pl_flags |= PLREDRAW;

      redrawall = 0;
    }
  else
    {
      if (clearlock)
	{
	  clearlock = 0;
	  W_WriteTriangle(mapw, clearlmark[0], clearlmark[1],
			  clearlmark[2], clearlmark[3], backColor);
	}


      /* Erase the ships */

      for (i = 0, update = lastUpdate; i < MAXPLAYER; i++, update++)
	{
	  if (redrawPlayer[i])
	    {
	      /* Erase the player if redrawPlayer[i] is set * or lastUpdate * 
	       * allows it. */

	      if (mclearzone[2][i])
		{
		  /* XFIX */
		  W_ClearArea(mapw, mclearzone[0][i], mclearzone[1][i],
			      mclearzone[2][i], mclearzone[3][i]);

		  /* Redraw the hole just left next update */
		  checkRedraw(mclearzone[4][i], mclearzone[5][i]);
		  mclearzone[2][i] = 0;
		}


	      /* Reset the last redrawn counter */

	      *update = 0;
	    }
	  else if (*update == 10)
	    {
	      /* 
	       *  Redraw stationary ships every update so that these
	       *  ships are not hidden by planet updates.
	       */

	      redrawPlayer[i] = 1;
	    }
	  else
	    {
	      ++(*update);
	    }
	}
    }


  /* Draw Planets */

  DrawPlanets();


  /* Draw ships */

  for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++)
    {
      /* lastUpdate[i] has been set to 0 if redrawall or the ship has * been
       * * erased or a redraw has not taken place for a while.  These * *
       * decisions are made at the top of the file.             */

      if (!redrawPlayer[i])
	continue;
      if (j->p_status != PALIVE)
	continue;
      if (j->p_flags & PFOBSERV)
	continue;				 /* jmn - observer support */
      if (j->p_x < 0 || j->p_x >= GWIDTH || j->p_y < 0 || j->p_y >= GWIDTH)
	continue;


      dx = j->p_x * GWINSIDE / GWIDTH;
      dy = j->p_y * GWINSIDE / GWIDTH;


      if (j->p_flags & PFCLOAK)
	{
	  W_WriteText(mapw, dx - W_Textwidth,
		      dy - W_Textheight / 2, unColor, cloakChars,
		      (cloakChars[1] == '\0' ? 1 : 2), W_RegularFont);
	}
      else
	{
	  W_WriteText(mapw, dx - W_Textwidth,
		    dy - W_Textheight / 2, playerColor(j), j->p_mapchars, 2,
		      shipFont(j));
	}

#ifdef BEEPLITE
      if ((UseLite && emph_player_seq_n[i] > 0)
	  && (liteflag & LITE_PLAYERS_MAP))
	{
	  int     seq_n = emph_player_seq_n[i] % emph_player_seq_frames;

	  W_WriteBitmap(dx - (emph_player_seq_width / 2 - 1),
			dy - (emph_player_seq_height / 2 + 1),
			emph_player_seq[seq_n],
			W_White);
	  emph_player_seq_n[i] -= 1;
	  mclearzone[0][i] = dx - (emph_player_seq_width / 2 - 1);
	  mclearzone[1][i] = dy - (emph_player_seq_height / 2 + 1);
	  mclearzone[2][i] = emph_player_seq_width;
	  mclearzone[3][i] = emph_player_seq_height;
	  mclearzone[4][i] = j->p_x;
	  mclearzone[5][i] = j->p_y;

	  /* Leave redraw on until done highlighting */
	  redrawPlayer[i] = 1;
	}
      else
	{
#endif

	  mclearzone[0][i] = dx - W_Textwidth;
	  mclearzone[1][i] = dy - W_Textheight / 2;
	  mclearzone[2][i] = W_Textwidth * 2;
	  mclearzone[3][i] = W_Textheight;

	  /* Set these so we can checkRedraw() next time */
	  mclearzone[4][i] = j->p_x;
	  mclearzone[5][i] = j->p_y;
	  redrawPlayer[i] = 0;

#ifdef BEEPLITE
	}
#endif
    }


  /* Draw the lock symbol (if needed */

  if ((me->p_flags & PFPLOCK) && (showLock & 1))
    {
      j = &players[me->p_playerl];

      if (j->p_status == PALIVE && !(j->p_flags & PFCLOAK))
	{
	  dx = j->p_x * GWINSIDE / GWIDTH;
	  dy = j->p_y * GWINSIDE / GWIDTH;
	  W_WriteTriangle(mapw, dx, dy + 6, 4, 1, foreColor);

	  clearlmark[0] = dx;
	  clearlmark[1] = dy + 6;
	  clearlmark[2] = 4;
	  clearlmark[3] = 1;
	  clearlock = 1;
	}
    }
  else if ((me->p_flags & PFPLLOCK) && (showLock & 1))
    {
      struct planet *l = &planets[me->p_planet];

      dx = l->pl_x * GWINSIDE / GWIDTH;
      dy = l->pl_y * GWINSIDE / GWIDTH;
      W_WriteTriangle(mapw, dx, dy - (mplanet_height) / 2 - 4,
		      4, 0, foreColor);

      clearlmark[0] = dx;
      clearlmark[1] = dy - (mplanet_height) / 2 - 4;
      clearlmark[2] = 4;
      clearlmark[3] = 0;
      clearlock = 1;
    }
}
