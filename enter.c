
/* enter.c
 * 
 * This version modified to work as the client in a socket based protocol.
 *
 * $Log: enter.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:09  siegl
 * COW 3.0 initial revision
 * */
#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <pwd.h>
#include <string.h>
#include <ctype.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"

/* Enter the game */

/* long random();        */

enter(void)
{
  drawTstats();
  delay = 0;
}

openmem(void)
{
  int     i;

  players = universe.players;
  torps = universe.torps;
  plasmatorps = universe.plasmatorps;
  status = universe.status;
  planets = universe.planets;
  phasers = universe.phasers;
  mctl = universe.mctl;
  messages = universe.messages;
  for (i = 0; i < MAXPLAYER; i++)
    {
      players[i].p_status = PFREE;
      players[i].p_cloakphase = 0;
      players[i].p_no = i;
      players[i].p_ntorp = 0;
      players[i].p_explode = 1;
      players[i].p_stats.st_tticks = 1;
    }
  mctl->mc_current = 0;
  status->time = 1;
  status->timeprod = 1;
  status->kills = 1;
  status->losses = 1;
  status->time = 1;
  status->planets = 1;
  status->armsbomb = 1;
  for (i = 0; i < MAXPLAYER * MAXTORP; i++)
    {
      torps[i].t_status = TFREE;
      torps[i].t_owner = (i / MAXTORP);
    }
  for (i = 0; i < MAXPLAYER; i++)
    {
      phasers[i].ph_status = PHFREE;
    }
  for (i = 0; i < MAXPLAYER * MAXPLASMA; i++)
    {
      plasmatorps[i].pt_status = PTFREE;
      plasmatorps[i].pt_owner = (i / MAXPLASMA);
    }
  for (i = 0; i < MAXPLANETS; i++)
    {
      planets[i].pl_no = i;
    }
  /* initialize planet redraw for moving planets */
  for (i = 0; i < MAXPLANETS; i++)
    {
      pl_update[i].plu_update = -1;
    }

  /* initialize pointers if ghost start */
  if (ghoststart)
    {
      me = &players[ghost_pno];
      myship = &(me->p_ship);
      mystats = &(me->p_stats);
    }
}

drawTstats(void)
{
  char    buf[BUFSIZ];

  if (newDashboard)
    return;
  sprintf(buf, "Flags        Warp Dam Shd Torps  Kills Armies   Fuel  Wtemp Etemp");
  W_WriteText(tstatw, 50, 5, textColor, buf, strlen(buf), W_RegularFont);
  sprintf(buf,
	  "Maximum:      %2d  %3d %3d               %3d   %6d   %3d   %3d",
	  me->p_ship.s_maxspeed, me->p_ship.s_maxdamage,
	  me->p_ship.s_maxshield, me->p_ship.s_maxarmies,
	  me->p_ship.s_maxfuel, me->p_ship.s_maxwpntemp / 10,
	  me->p_ship.s_maxegntemp / 10);
  W_WriteText(tstatw, 50, 27, textColor, buf, strlen(buf), W_RegularFont);
}

#ifdef HOCKEY_LINES
init_hockey_lines(void)
{
  int     i = 0;				 /* This is incremented for * 

						  * 
						  * 
						  * * each line added */

  /* For speed, the normal netrek walls are not done this way */

  /* Defines for Hockey lines and the Hockey lines themselves */
#define RINK_TOP 0
#define RINK_BOTTOM (GWIDTH)
#define TENTH (((RINK_BOTTOM - RINK_TOP)/10))
#define R_MID (((RINK_BOTTOM - RINK_TOP)/2))	 /* center (red) line */
#define RINK_LENGTH ((RINK_BOTTOM - RINK_TOP))
#define RINK_WIDTH ((GWIDTH*2/3))
#define G_MID ((GWIDTH/2))			 /* center of goal */
#define RINK_LEFT ((G_MID-(RINK_WIDTH/2)))
#define RINK_RIGHT ((G_MID+(RINK_WIDTH/2)))
#define G_LFT (R_MID-TENTH)			 /* left edge of goal */
#define G_RGT (R_MID+TENTH)			 /* right edge of goal */
#define RED_1 (RINK_LEFT + (1*RINK_WIDTH/5))
#define RED_2 (RINK_LEFT + (2*RINK_WIDTH/5))
#define RED_3 (RINK_LEFT + (3*RINK_WIDTH/5))
#define RED_4 (RINK_LEFT + (4*RINK_WIDTH/5))
#define ORI_G (RINK_BOTTOM - /*2* */TENTH)	 /* Ori goal line */
#define ORI_E (RINK_BOTTOM -   TENTH/2)		 /* end of Ori goal */
#define ORI_B (RINK_BOTTOM - (RINK_LENGTH/3))	 /* Ori blue line */
#define KLI_G (RINK_TOP    + /*2* */TENTH)	 /* Kli goal line */
#define KLI_E (RINK_TOP    +   TENTH/2)		 /* end of Kli goal */
#define KLI_B (RINK_TOP    + (RINK_LENGTH/3))	 /* Kli blue line */

  /* The Kli goal line */
  s_lines[i].begin_x = G_LFT;
  s_lines[i].end_x = G_RGT;
  s_lines[i].begin_y = s_lines[i].end_y = KLI_G;
  s_lines[i].color = W_Red;
  s_lines[i].flag = &hockey_s_lines;
  s_lines[i++].orientation = S_LINE_HORIZONTAL;
  /* fprintf(stderr,"Kli Goal: x: %i to %i, y: %i to * *
   * %i\n",s_lines[i-1].begin_x, * *
   * s_lines[i-1].end_x,s_lines[i-1].begin_y,s_lines[i-1].end_y); */

  /* The left side goal line */
  s_lines[i].begin_x = s_lines[i].end_x = G_LFT;
  s_lines[i].begin_y = KLI_G;
  s_lines[i].end_y = KLI_E;
  s_lines[i].color = W_Green;
  s_lines[i].flag = &hockey_s_lines;
  s_lines[i++].orientation = S_LINE_VERTICAL;
  /* fprintf(stderr,"L K Goal: x: %i to %i, y: %i to * *
   * %i\n",s_lines[i-1].begin_x, * *
   * s_lines[i-1].end_x,s_lines[i-1].begin_y,s_lines[i-1].end_y); */

  /* The right side goal line */
  s_lines[i].begin_x = s_lines[i].end_x = G_RGT;
  s_lines[i].begin_y = KLI_G;
  s_lines[i].end_y = KLI_E;
  s_lines[i].color = W_Green;
  s_lines[i].flag = &hockey_s_lines;
  s_lines[i++].orientation = S_LINE_VERTICAL;
  /* fprintf(stderr,"K R Goal: x: %i to %i, y: %i to * *
   * %i\n",s_lines[i-1].begin_x, * *
   * s_lines[i-1].end_x,s_lines[i-1].begin_y,s_lines[i-1].end_y); */

  /* The End of kli goal line */
  s_lines[i].begin_x = G_LFT;
  s_lines[i].end_x = G_RGT;
  s_lines[i].begin_y = s_lines[i].end_y = KLI_E;
  s_lines[i].color = W_Green;
  s_lines[i].flag = &hockey_s_lines;
  s_lines[i++].orientation = S_LINE_HORIZONTAL;
  /* fprintf(stderr,"K B Goal: x: %i to %i, y: %i to * *
   * %i\n",s_lines[i-1].begin_x, * *
   * s_lines[i-1].end_x,s_lines[i-1].begin_y,s_lines[i-1].end_y); */

  /* The Kli blue line */
  s_lines[i].begin_x = RINK_LEFT;
  s_lines[i].end_x = RINK_RIGHT;
  s_lines[i].begin_y = s_lines[i].end_y = KLI_B;
  s_lines[i].color = W_Cyan;
  s_lines[i].flag = &hockey_s_lines;
  s_lines[i++].orientation = S_LINE_HORIZONTAL;
  /* fprintf(stderr,"Kli Blue: x: %i to %i, y: %i to * *
   * %i\n",s_lines[i-1].begin_x, * *
   * s_lines[i-1].end_x,s_lines[i-1].begin_y,s_lines[i-1].end_y); */

  /* The Ori goal line */
  s_lines[i].begin_x = G_LFT;
  s_lines[i].end_x = G_RGT;
  s_lines[i].begin_y = s_lines[i].end_y = ORI_G;
  s_lines[i].color = W_Red;
  s_lines[i].flag = &hockey_s_lines;
  s_lines[i++].orientation = S_LINE_HORIZONTAL;
  /* fprintf(stderr,"Ori Goal: x: %i to %i, y: %i to * *
   * %i\n",s_lines[i-1].begin_x, * *
   * s_lines[i-1].end_x,s_lines[i-1].begin_y,s_lines[i-1].end_y); */

  /* The left side goal line */
  s_lines[i].begin_x = s_lines[i].end_x = G_LFT;
  s_lines[i].begin_y = ORI_G;
  s_lines[i].end_y = ORI_E;
  s_lines[i].color = W_Cyan;
  s_lines[i].flag = &hockey_s_lines;
  s_lines[i++].orientation = S_LINE_VERTICAL;
  /* fprintf(stderr,"O L Goal: x: %i to %i, y: %i to * *
   * %i\n",s_lines[i-1].begin_x, * *
   * s_lines[i-1].end_x,s_lines[i-1].begin_y,s_lines[i-1].end_y); */

  /* The right side goal line */
  s_lines[i].begin_x = s_lines[i].end_x = G_RGT;
  s_lines[i].begin_y = ORI_G;
  s_lines[i].end_y = ORI_E;
  s_lines[i].color = W_Cyan;
  s_lines[i].flag = &hockey_s_lines;
  s_lines[i++].orientation = S_LINE_VERTICAL;
  /* fprintf(stderr,"O R Goal: x: %i to %i, y: %i to * *
   * %i\n",s_lines[i-1].begin_x, * *
   * s_lines[i-1].end_x,s_lines[i-1].begin_y,s_lines[i-1].end_y); */

  /* The End of ori goal line */
  s_lines[i].begin_x = G_LFT;
  s_lines[i].end_x = G_RGT;
  s_lines[i].begin_y = s_lines[i].end_y = ORI_E;
  s_lines[i].color = W_Cyan;
  s_lines[i].flag = &hockey_s_lines;
  s_lines[i++].orientation = S_LINE_HORIZONTAL;
  /* fprintf(stderr,"O B Goal: x: %i to %i, y: %i to * *
   * %i\n",s_lines[i-1].begin_x, * *
   * s_lines[i-1].end_x,s_lines[i-1].begin_y,s_lines[i-1].end_y); */

  /* The Ori blue line */
  s_lines[i].begin_x = RINK_LEFT;
  s_lines[i].end_x = RINK_RIGHT;
  s_lines[i].begin_y = s_lines[i].end_y = ORI_B;
  s_lines[i].color = W_Cyan;
  s_lines[i].flag = &hockey_s_lines;
  s_lines[i++].orientation = S_LINE_HORIZONTAL;
  /* fprintf(stderr,"Ori Blue: x: %i to %i, y: %i to * *
   * %i\n",s_lines[i-1].begin_x, * *
   * s_lines[i-1].end_x,s_lines[i-1].begin_y,s_lines[i-1].end_y); */

  /* The red line */
  s_lines[i].begin_x = RINK_LEFT;
  s_lines[i].end_x = RINK_RIGHT;
  s_lines[i].begin_y = s_lines[i].end_y = R_MID;
  s_lines[i].color = W_Red;
  s_lines[i].flag = &hockey_s_lines;
  s_lines[i++].orientation = S_LINE_HORIZONTAL;
  /* fprintf(stderr,"Red Line: x: %i to %i, y: %i to * *
   * %i\n",s_lines[i-1].begin_x, * *
   * s_lines[i-1].end_x,s_lines[i-1].begin_y,s_lines[i-1].end_y); */

  /* Right rink boundary */
  s_lines[i].begin_x = s_lines[i].end_x = RINK_RIGHT;
  s_lines[i].begin_y = 0;
  s_lines[i].end_y = GWIDTH - 1;
  s_lines[i].color = W_Grey;
  s_lines[i].flag = &hockey_s_lines;
  s_lines[i++].orientation = S_LINE_VERTICAL;
  /* fprintf(stderr,"Rt. Line: x: %i to %i, y: %i to * *
   * %i\n",s_lines[i-1].begin_x, * *
   * s_lines[i-1].end_x,s_lines[i-1].begin_y,s_lines[i-1].end_y); */

  /* Left rink boundary */
  s_lines[i].begin_x = s_lines[i].end_x = RINK_LEFT;
  s_lines[i].begin_y = 0;
  s_lines[i].end_y = GWIDTH - 1;
  s_lines[i].color = W_Grey;
  s_lines[i].flag = &hockey_s_lines;
  s_lines[i++].orientation = S_LINE_VERTICAL;
  /* fprintf(stderr,"Lef Line: x: %i to %i, y: %i to * *
   * %i\n",s_lines[i-1].begin_x, * *
   * s_lines[i-1].end_x,s_lines[i-1].begin_y,s_lines[i-1].end_y); */

  /* NOTE:  The number of lines must EXACTLY match the NUM_HOCKEY_LINES */
  /* in defs.h for it to run properly.                           */
}
#endif
