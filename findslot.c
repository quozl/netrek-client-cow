
/* findslot.c
 * 
 * Kevin Smith 03/23/88
 *
 * $Log: findslot.c,v $
 * Revision 1.2  2002/06/22 04:43:24  tanner
 * Clean up of SDL code. #ifdef'd out functions not needed in SDL.
 *
 * Revision 1.1.1.1  1998/11/01 17:24:09  siegl
 * COW 3.0 initial revision
 * */
#include "config.h"
#include "copyright2.h"

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

#define WAITMOTD

#define WAITWIDTH 180
#define WAITHEIGHT 60
#define WAITTITLE 15				 /* height of title for wait
						  * * * window */

extern void terminate(int error);

findslot(void)
{
  int     oldcount = -1;
  W_Window waitWin, qwin, countWin, motdButtonWin;

  W_Window motdWin;
  extern int MaxMotdLine;
  int     WaitMotdLine = 0;
  int     mapMotd = booleanDefault("showMotd", 1);
  W_Event event;


  /* Wait for some kind of indication about in/not in */
  while (queuePos == -1)
    {
      socketPause();
      if (isServerDead())
	{

#if defined(SOUND) && !defined(HAVE_SDL)
	  Exit_Sound();
#endif

	  printf("Shit!  Ghostbusted!\n");
	  terminate(0);
	}
      readFromServer(NULL);
      if (me != NULL)
	{
	  /* We are in! */
	  ANNOUNCESOCKET;
	  return (me->p_no);
	}
    }

  /* We have to wait.  Make appropriate windows, etc... */

  waitWin = W_MakeWindow("wait", 0, 0, WAITWIDTH, WAITHEIGHT, NULL, 2,
			 foreColor);
  countWin = W_MakeWindow("count", WAITWIDTH / 3, WAITTITLE, WAITWIDTH / 3,
			  WAITHEIGHT - WAITTITLE, waitWin, 1, foreColor);
  qwin = W_MakeWindow("waitquit", 0, WAITTITLE, WAITWIDTH / 3,
		      WAITHEIGHT - WAITTITLE, waitWin, 1, foreColor);
  motdButtonWin = W_MakeWindow("motdbutton", 2 * WAITWIDTH / 3, WAITTITLE,
			     WAITWIDTH / 3, WAITHEIGHT - WAITTITLE, waitWin,
			       1, foreColor);
  W_MapWindow(waitWin);
  W_MapWindow(countWin);
  W_MapWindow(motdButtonWin);
  W_MapWindow(qwin);
  if (mapMotd)
    {
      motdWin = W_MakeWindow("waitmotd", 1, WAITWIDTH + 1, TWINSIDE,
			     TWINSIDE, 0, 2, foreColor);
      W_MapWindow(motdWin);
      showMotd(motdWin, WaitMotdLine);
    }

  for (;;)
    {
      socketPause();
      readFromServer(NULL);
      if (isServerDead())
	{

#if defined(SOUND) && !defined(HAVE_SDL)
	  Exit_Sound();
#endif

	  printf("Damn, We've been ghostbusted!\n");
	  terminate(0);
	}
      while (W_EventsPending())
	{
	  W_NextEvent(&event);
	  switch ((int) event.type)
	    {
	    case W_EV_BUTTON:
	    case W_EV_KEY:
	      if (mapMotd && event.Window == motdWin)
		{
		  if (event.key == ' ' || event.key == 'q')
		    {
		      W_DestroyWindow(motdWin);
		      mapMotd = !mapMotd;
		    }
		  else
		    {
		      if (event.key == 'b')
			{
			  WaitMotdLine -= 28;
			  WaitMotdLine = MAX(WaitMotdLine, 0);
			}
		      else
			{
			  WaitMotdLine += 28;
			  /* scroll to start if it goes over */
			  if (WaitMotdLine > MaxMotdLine)
			    WaitMotdLine = 0;
			}
		      W_ClearWindow(motdWin);
		      showMotd(motdWin, WaitMotdLine);
		      break;
		    }
		}
	      else if (event.Window == motdButtonWin)
		{
		  if (mapMotd)
		    {
		      W_DestroyWindow(motdWin);
		    }
		  else
		    {
		      motdWin = W_MakeWindow("waitmotd", 1, WAITWIDTH + 1,
					     TWINSIDE, TWINSIDE, 0, 2,
					     foreColor);
		      W_MapWindow(motdWin);
		      showMotd(motdWin, WaitMotdLine);
		    }
		  mapMotd = !mapMotd;
		}
	      else if (event.Window == qwin)
		{

#if defined(SOUND) && !defined(HAVE_SDL)
		  Exit_Sound();
#endif

		  printf("OK, bye!\n");
		  terminate(0);
		}
	      break;
	    case W_EV_EXPOSE:
	      if (event.Window == waitWin)
		{
		  mapWaitWin(waitWin);
		}
	      else if (event.Window == motdWin)
		{
		  showMotd(motdWin, WaitMotdLine);
		}
	      else if (event.Window == qwin)
		{
		  mapWaitQuit(qwin);
		}
	      else if (event.Window == countWin)
		{
		  mapWaitCount(waitWin, countWin, queuePos);
		}
	      else if (event.Window == motdButtonWin)
		{
		  mapWaitMotdButton(motdButtonWin);
		}
	      break;
	    default:
	      break;
	    }
	}
      if (queuePos != oldcount)
	{
	  mapWaitCount(waitWin, countWin, queuePos);
	  oldcount = queuePos;
	}
      if (me != NULL)
	{
	  W_DestroyWindow(waitWin);
	  if (mapMotd)
	    {
	      W_DestroyWindow(motdWin);
	    }
	  ANNOUNCESOCKET;
	  W_Beep();
	  W_Beep();
	  return (me->p_no);
	}
    }
}

mapWaitWin(W_Window waitWin)
{
  char   *s;
  char    buf[60];

  sprintf(buf, "Game full at %s", serverName);
  s = buf;

  W_WriteText(waitWin, 15, 5, textColor, s, strlen(s), W_RegularFont);
}

mapWaitQuit(W_Window qwin)
{
  char   *s = "Quit";

  W_WriteText(qwin, 10, 15, textColor, s, strlen(s), W_RegularFont);
}

mapWaitCount(W_Window waitWin, W_Window countWin, int count)
{
  char   *s1 = "Wait";
  char   *s2 = "Queue";
  char    buf[80];
  int     length;

  W_WriteText(countWin, 17, 5, textColor, s1, strlen(s1), W_RegularFont);
  W_WriteText(countWin, 15, 15, textColor, s2, strlen(s2), W_RegularFont);
  sprintf(buf, "%d    ", count);
  if (count == -1)
    STRNCPY(buf, "?", 2);
  W_WriteText(countWin, WAITWIDTH / 6 - strlen(buf) * W_Textwidth / 2, 25,
	      textColor, buf, strlen(buf), W_RegularFont);
  sprintf(buf, "Q%d @ %s", count, serverName);
  W_SetWindowName(waitWin, buf);
}

mapWaitMotdButton(W_Window motdButtonWin)
{
  char   *s = "Motd";

  W_WriteText(motdButtonWin, 10, 15, textColor, s, strlen(s), W_RegularFont);
}
