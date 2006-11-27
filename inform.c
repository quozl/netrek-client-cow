
/* inform.c
 *
 * $Log: inform.c,v $
 * Revision 1.2  2006/05/16 06:16:35  quozl
 * add PLCORE
 *
 * Revision 1.1.1.1  1998/11/01 17:24:10  siegl
 * COW 3.0 initial revision
 * */
#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include <math.h>
#include <signal.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"

/* Display information about the nearest objext to mouse */

/* * When the player asks for info, this routine finds the object * nearest
 * the mouse, either player or planet, and pop up a window * with the desired
 * information in it. *
 * 
 * We intentionally provide less information than is actually * available. Keeps
 * the fog of war up. *
 * 
 * There is a different sized window for each type player/planet * and we take
 * care to keep it from extending beyond the main * window boundaries. */

static char *my_classes[NUM_TYPES] =
{
  "SC", "DD", "CA", "BB", "AS", "SB", "GA", "??"
};

inform(W_Window ww, int x, int y, char key)
{
  char    buf[BUFSIZ];
  int     line = 0;
  register struct player *j;
  register struct planet *k;
  int     mx, my;
  double  dist;
  struct obtype *gettarget(W_Window ww, int x, int y, int targtype), *target;
  int     windowWidth, windowHeight;
  float   KillsPerHour, LossesPerHour;		 /* SB info window changed to

						  * 
						  * * * use these instead of
						  * * * Offense and Defense.
						  * * * 12/27/93 ATH */

  mx = x;
  my = y;
  infomapped = 1;
  if (key == 'i')
    {
      target = gettarget(ww, x, y, TARG_PLAYER | TARG_PLANET);
    }
  else
    {
      target = gettarget(ww, x, y, TARG_PLAYER | TARG_SELF);
    }

  /* This is pretty lame.  We make a graphics window for the info window so * 
   * 
   * * we can accurately space the thing to barely fit into the galactic map
   * or * * whatever. */

  windowWidth = W_WindowWidth(ww);
  windowHeight = W_WindowHeight(ww);
  if (target->o_type == PLAYERTYPE)
    {
      if (key == 'i')
	{
	  /* Too close to the edge? */
	  if (mx + 23 * W_Textwidth + 2 > windowWidth)
	    mx = windowWidth - 23 * W_Textwidth - 2;
	  if (my + 5 * W_Textheight + 2 > windowHeight)
	    my = windowHeight - 5 * W_Textheight - 2;

	  infow = W_MakeWindow("info", mx, my, 23 * W_Textwidth, 6 * W_Textheight,
			       ww, 2, foreColor);
	  W_MapWindow(infow);
	  j = &players[target->o_num];
	  (void) sprintf(buf, "%s (%c%c)", j->p_name, teamlet[j->p_team], shipnos[j->p_no]);
	  W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf), shipFont(j));
	  (void) sprintf(buf, "Speed:   %-d", j->p_speed);
	  W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf),
		      W_RegularFont);

	  (void) sprintf(buf, "kills:   %-4.2f", j->p_kills);
	  W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf),
		      W_RegularFont);
	  (void) sprintf(buf, "Ship Type: %-s", my_classes[j->p_ship.s_type]);
	  W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf),
		      W_RegularFont);

	  if (j->p_swar & me->p_team)
	    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), "WAR", 3,
			W_RegularFont);
	  else if (j->p_hostile & me->p_team)
	    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), "HOSTILE", 7,
			W_RegularFont);
	  else
	    W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), "PEACEFUL", 8,
			W_RegularFont);
	  (void) sprintf(buf, "%s@%s", j->p_login, j->p_monitor);
	  W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j),
		      buf, strlen(buf), W_RegularFont);
	}
      else
	{					 /* New information window! */
	  if (mx + 24 * W_Textwidth + 2 > windowWidth)
	    mx = windowWidth - 24 * W_Textwidth - 2;
	  if (my + 10 * W_Textheight + 2 > windowHeight)
	    my = windowHeight - 10 * W_Textheight - 2;

	  infow = W_MakeWindow("info", mx, my, 24 * W_Textwidth, 10 * W_Textheight,
			       ww, 2, foreColor);
	  W_MapWindow(infow);
	  j = &players[target->o_num];
	  (void) sprintf(buf, "%s (%c%c):", j->p_name, teamlet[j->p_team], shipnos[j->p_no]);
	  W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf), shipFont(j));
	  (void) sprintf(buf, "Login   %-s", j->p_login);
	  W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf),
		      W_RegularFont);
	  (void) sprintf(buf, "Display %-s", j->p_monitor);
	  W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf),
		      W_RegularFont);
	  STRNCPY(buf, "        Rating    Total", 25);
	  W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf),
		      W_RegularFont);
	  sprintf(buf, "Bombing: %5.2f  %5d",
		  bombingRating(j),
		  j->p_stats.st_armsbomb + j->p_stats.st_tarmsbomb);
	  W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf),
		      W_RegularFont);
	  sprintf(buf, "Planets: %5.2f  %5d",
		  planetRating(j),
		  j->p_stats.st_planets + j->p_stats.st_tplanets);
	  W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf),
		      W_RegularFont);
	  if ((j->p_ship.s_type == STARBASE) && (SBhours))
	    {
	      KillsPerHour = (float) (j->p_stats.st_sbticks == 0) ?
		  0.0 :
		  (float) j->p_stats.st_sbkills * 36000.0 /
		  (float) j->p_stats.st_sbticks;
	      sprintf(buf, "KPH:     %5.2f  %5d",
		      KillsPerHour,
		      j->p_stats.st_sbkills);
	    }
	  else
	    {
	      sprintf(buf, "Offense: %5.2f  %5d",
		      offenseRating(j),
		      j->p_stats.st_kills + j->p_stats.st_tkills);
	    }
	  W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf),
		      W_RegularFont);
	  if ((j->p_ship.s_type == STARBASE) && (SBhours))
	    {
	      LossesPerHour = (float) (j->p_stats.st_sbticks == 0) ?
		  0.0 :
		  (float) j->p_stats.st_sblosses * 36000.0 /
		  (float) j->p_stats.st_sbticks;
	      sprintf(buf, "DPH:     %5.2f  %5d",
		      LossesPerHour,
		      j->p_stats.st_sblosses);
	    }
	  else
	    {
	      sprintf(buf, "Defense: %5.2f  %5d",
		      defenseRating(j),
		      j->p_stats.st_losses + j->p_stats.st_tlosses);
	    }
	  W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf),
		      W_RegularFont);
	  if (j->p_ship.s_type == STARBASE)
	    {
	      sprintf(buf, "  Maxkills: %6.2f", j->p_stats.st_sbmaxkills);
	    }
	  else
	    {
	      sprintf(buf, "  Maxkills: %6.2f", j->p_stats.st_maxkills);
	    }
	  W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf),
		      W_RegularFont);
	  if ((j->p_ship.s_type == STARBASE) && (SBhours))
	    {
	      sprintf(buf, "  Hours:    %6.2f",
		      (float) j->p_stats.st_sbticks / 36000.0);
	    }
	  else
	    {
	      sprintf(buf, "  Hours:    %6.2f",
		      (float) j->p_stats.st_tticks / 36000.0);
	    }
	  W_WriteText(infow, W_Textwidth, W_Textheight * line++, playerColor(j), buf, strlen(buf),
		      W_RegularFont);
	}
    }
  else
    {						 /* Planet */
      /* Too close to the edge? */
      if (mx + 23 * W_Textwidth + 2 > windowWidth)
	mx = windowWidth - 28 * W_Textwidth - 2;
      if (my + 3 * W_Textheight + 2 > windowHeight)
	my = windowHeight - 3 * W_Textheight - 2;

      infow = W_MakeWindow("info", mx, my, W_Textwidth * 28, W_Textheight * 3, ww,
			   2, foreColor);
      W_MapWindow(infow);
      k = &planets[target->o_num];
      dist = hypot((double) (me->p_x - k->pl_x),
		   (double) (me->p_y - k->pl_y));
      if ((k->pl_info & me->p_team)

#ifdef RECORDGAME
	  || playback
#endif

	  )
	{
	  (void) sprintf(buf, "%s (%c)", k->pl_name, teamlet[k->pl_owner]);
	  W_WriteText(infow, W_Textwidth, W_Textheight * line++, planetColor(k), buf, strlen(buf),
		      planetFont(k));
	  (void) sprintf(buf, "Armies %d", k->pl_armies);
	  W_WriteText(infow, W_Textwidth, W_Textheight * line++, planetColor(k), buf, strlen(buf),
		      W_RegularFont);
	  (void) sprintf(buf, "%s %s %s %s %c%c%c%c",
			 (k->pl_flags & PLREPAIR ? "REPAIR" : "      "),
			 (k->pl_flags & PLFUEL ? "FUEL" : "    "),
			 (k->pl_flags & PLAGRI ? "AGRI" : "    "),
			 (k->pl_flags & PLCORE ? "CORE" : "    "),
			 (k->pl_info & FED ? 'F' : ' '),
			 (k->pl_info & ROM ? 'R' : ' '),
			 (k->pl_info & KLI ? 'K' : ' '),
			 (k->pl_info & ORI ? 'O' : ' '));
	  W_WriteText(infow, W_Textwidth, W_Textheight * line++, planetColor(k), buf, strlen(buf),
		      W_RegularFont);
	}
      else
	{
	  (void) sprintf(buf, "%s", k->pl_name);
	  W_WriteText(infow, W_Textwidth, W_Textheight * line++, planetColor(k), buf, strlen(buf),
		      W_RegularFont);
	  (void) sprintf(buf, "No other info");
	  W_WriteText(infow, W_Textwidth, W_Textheight * line++, planetColor(k), buf, strlen(buf),
		      W_RegularFont);
	}
    }
}


destroyInfo(void)
{
  W_DestroyWindow(infow);
  infow = 0;
  infomapped = 0;
}
