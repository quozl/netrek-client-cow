
/* warning.c
 *
 * $Log: warning.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
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
#include <time.h>
#include INC_SYS_TIME

/* * The warning in text will be printed in the warning window. * The message
 * will last WARNTIME/10 seconds unless another message * comes through and
 * overwrites it. */
warning(char *text)
{
  int     doPhaser;
  LONG    curtime;
  struct tm *tm;
  char    newtext[128];

  if (warncount > 0)
    {
      /* XFIX */
      W_ClearArea(warnw, 5, 5, W_Textwidth * warncount, W_Textheight);
    }

  doPhaser = showPhaser && (strncmp(text, "Phaser burst", 12) == 0);

  warncount = strlen(text);

#ifdef PHASER_STATS
  if (doPhaser && phaserShowStats)
    {
      sprintf(newtext, "%s [%2d%%]", text,
	      phaserStatTry ? (phaserStatHit * 100) / phaserStatTry : 0);
      warncount += 6;
      W_WriteText(warnw, 5, 5, textColor, newtext, warncount, W_RegularFont);
    }
  else
#endif

    W_WriteText(warnw, 5, 5, textColor, text, warncount, W_RegularFont);
  warntimer = udcounter + WARNTIME;		 /* set the line to be * *
						  * cleared */


  if (logmess || doPhaser)
    {
      time(&curtime);
      tm = localtime(&curtime);

#ifdef PHASER_STATS
      sprintf(newtext, "%.100s %02d:%02d:%02d",
	      (doPhaser && phaserShowStats) ? newtext : text, tm->tm_hour,
#else
      sprintf(newtext, "%.100s %02d:%02d:%02d", text, tm->tm_hour,
#endif

	      tm->tm_min, tm->tm_sec);
      warncount = (warncount > 100) ? 109 : warncount + 9;

      if (logmess)
	{
	  if (logFile != NULL)
	    {
	      fprintf(logFile, "%s\n", newtext);
	      fflush(logFile);
	    }
	  else
	    puts(newtext);
	}

      if (doPhaser)
	{
	  W_WriteText(phaserwin, 0, 0, textColor, newtext, warncount, 0);
	  switch (showPhaser)
	    {
	      /* 0 -- none 1 -- review all 2 -- review team 3 -- review indiv
	       * * 4 -- review kill 5 -- total */
	    case 0:
	      break;
	    case 1:
	      W_WriteText(messwa, 0, 0, textColor, newtext, warncount, 0);
	      break;
	    case 2:
	      W_WriteText(messwt, 0, 0, textColor, newtext, warncount, 0);
	      break;
	    case 3:
	      W_WriteText(messwi, 0, 0, textColor, newtext, warncount, 0);
	      break;
	    case 4:
	      W_WriteText(messwk, 0, 0, textColor, newtext, warncount, 0);
	      break;
	    case 5:
	      W_WriteText(reviewWin, 0, 0, textColor, newtext, warncount, 0);
	      break;
	    default:
	      break;
	    }
	}
    }
}
