
/* tools.c - shell escape, graphic toolsw - 10/10/93
 * 
 * copyright 1993 Kurt Siegl <007@netrek.org> Free to use, hack, etc.
 * Just keep these credits here. Use of this code may be dangerous to your
 * health and/or system. Its use is at your own risk. I assume no
 * responsibility for damages, real, potential, or imagined, resulting  from
 * the use of it.
 * 
 *
 * $Log: tools.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 * */

#include "config.h"

#ifdef TOOLS
#include <stdio.h>
#include "math.h"
#include <signal.h>
#include <sys/types.h>
#include INC_STRINGS
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"

void
        sendTools(char *str)
{
  char    pipebuf[100];
  char    c;
  FILE   *pipefp;
  int     len, i;

  if (sscanf(str, "set %s %c", pipebuf, &c) == 2)
    {
      for (i = 0; keys[i]; i++)
	{
	  if (strcmpi(macroKeys[i].name, pipebuf) == 0)
	    {
	      macroKeys[i].dest = c;
	      W_WriteText(toolsWin, 0, 0, textColor, str, strlen(str), W_RegularFont);
	      return;
	    }
	}
    }

  if (!W_IsMapped(toolsWin))
    showToolsWin();

#ifndef WIN32

#ifndef VMS
  SIGNAL(SIGCHLD, SIG_DFL);
  if (shelltools && (pipefp = popen(str, "r")) != NULL)
    {
      while (fgets(pipebuf, 80, pipefp) != NULL)
	{
	  len = strlen(pipebuf);
	  if (pipebuf[len - 1] == '\n')
	    pipebuf[len - 1] = '\0';
	  W_WriteText(toolsWin, 0, 0, textColor, pipebuf,
		      strlen(pipebuf), W_RegularFont);
	}
      pclose(pipefp);
    }
  else
#endif

#endif

    W_WriteText(toolsWin, 0, 0, textColor, str, strlen(str), W_RegularFont);
}

showToolsWin(void)
{
  if (W_IsMapped(toolsWin))
    W_UnmapWindow(toolsWin);
  else
    W_MapWindow(toolsWin);
}
#endif /* TOOLS */
