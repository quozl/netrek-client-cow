
/****************************************************************************/
/**  File:  war.c                                                          **/
/**                                                                        **/
/**  Function:                                                             **/
/**     This file contains the code for modifying war status.  This        **/
/**     includes the display of the war options menu, the interpreting of  **/
/**     the user's selection, and the validation of the user's selection.  **/
/**                                                                        **/
/**  Revisions:                                                            **/
/*
 * $Log: war.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 *
 *      9/20/93  VEG modified function waraction() to use switch          *
 *      structure instead of successive if statements.  Altered spacing   *
 *      to improve clarity.                                               */
/**************************************************************************/
#include "config.h"
#include "copyright.h"
#include <stdio.h>
#include <math.h>
#include <signal.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"

/******************************************************************************/
/***  newhostile - identifies user request from the war options window      ***/
/******************************************************************************/
static int newhostile;

/******************************************************************************/
/***              Text needed for drawing the war options window            ***/
/******************************************************************************/
static char *feds = "FED - ";
static char *roms = "ROM - ";
static char *klis = "KLI - ";
static char *oris = "ORI - ";
static char *gos = "  Save";
static char *exs = "  Exit - no change";

static char *peaces = "Peace";
static char *hostiles = "Hostile";
static char *wars = "War";

/******************************************************************************/
/***  fillwin() displays text into war window, deciding which color to use  ***/
/******************************************************************************/
void
        fillwin(int menunum, char *string, int hostile, int warbits, int team)
{
  char    buf[80];

  if (team & warbits)
    {
      (void) sprintf(buf, "  %s%s", string, wars);
      W_WriteText(war, 0, menunum, rColor, buf, strlen(buf), 0);
    }
  else if (team & hostile)
    {
      (void) sprintf(buf, "  %s%s", string, hostiles);
      W_WriteText(war, 0, menunum, yColor, buf, strlen(buf), 0);
    }
  else
    {
      (void) sprintf(buf, "  %s%s", string, peaces);
      W_WriteText(war, 0, menunum, gColor, buf, strlen(buf), 0);
    }
}

/******************************************************************************/
/***  warrefresh()  redraws the text into the war options window            ***/
/******************************************************************************/
void
        warrefresh(void)
{
  fillwin(0, feds, newhostile, me->p_swar, FED);
  fillwin(1, roms, newhostile, me->p_swar, ROM);
  fillwin(2, klis, newhostile, me->p_swar, KLI);
  fillwin(3, oris, newhostile, me->p_swar, ORI);
  W_WriteText(war, 0, 4, textColor, gos, strlen(gos), 0);
  W_WriteText(war, 0, 5, textColor, exs, strlen(exs), 0);
}

/******************************************************************************/
/***  warwindow()  draws the war options window                             ***/
/******************************************************************************/
void
        warwindow(void)
{
  W_MapWindow(war);
  newhostile = me->p_hostile;
  warrefresh();
}

/******************************************************************************/
/***  waraction()  accpets Xwindow event and processes the user's request.  ***/
/***  This procedure will toggle war status for races, and will print error ***/
/***  message when the request cannot be granted.                           ***/
/******************************************************************************/
void
        waraction(W_Event * data)
{
  int     enemyteam;

  switch (data->y)
    {
    case 0:
      enemyteam = FED;
      break;
    case 1:
      enemyteam = ROM;
      break;
    case 2:
      enemyteam = KLI;
      break;
    case 3:
      enemyteam = ORI;
      break;
    case 4:
      W_UnmapWindow(war);
      sendWarReq(newhostile);
      return;
      break;
    case 5:
      W_UnmapWindow(war);
      return;
      break;
    }

  if (me->p_swar & enemyteam)
    {
      warning("You are already at war. Status cannot be changed.");
      W_Beep();
    }
  else
    {
      if (me->p_team == enemyteam)
	{
	  warning("You can't declare war on your own team, fool.");
	}
      else
	{
	  newhostile ^= enemyteam;
	}
    }
  warrefresh();
}
