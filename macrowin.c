
/* macrowin.c from helpwin.c copyright 1993 Nick Trown copyright 1991 ERic
 * mehlhaff Free to use, hack, etc. Just keep these credits here. Use of this
 * code may be dangerous to your health and/or system. Its use is at your own
 * risk. I assume no responsibility for damages, real, potential, or
 * imagined, resulting  from the use of it. Yeah.....what Eric said...
 *
 * $Log: macrowin.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:10  siegl
 * COW 3.0 initial revision
 * */

#include "config.h"
#include <stdio.h>
#include "math.h"
#include <signal.h>
#include <sys/types.h>

#include <time.h>
#include INC_SYS_TIME
#include INC_STRINGS

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"


/* Fills in macro window with the macros defined in the .xtrekrc. */

#define NUMLINES 80

#define MAXMACRO 65
/* maximum length in characters of key explanation */

#define MACROLEN 255
/* length of construction string since we don't know how long a macro can be */


int     lineno = 0;
char    maclines[10][MAXMACRO];
int     maclevel = 0;

int
        formatline(char *line)
{
  register int end;
  char   *temp;
  int     num = 0;

  if (!line)
    return 0;
  if (strlen(line) <= MAXMACRO)
    {
      STRNCPY(maclines[num], line, sizeof(maclines[0]));
      lineno++;
      return 1;
    }

  temp = line;
  while (1)
    {
      end = MAXMACRO - 1;
      if (end > strlen(temp))
	{
	  lineno++;
	  STRNCPY(maclines[num++], temp, sizeof(maclines[0]));
	  return (num);
	}
      else
	for (; temp[end] != '%'; end--);

      lineno++;
      STRNCPY(maclines[num++], temp, end);

      temp = temp + end;
    }
}


void
        filldist(int fill)
{
  register int i;
  register int row;
  register int c;
  int     num;
  char    key[3];

  lineno = 0;
  for (i = 1, row = 5; distmacro[i].macro != '\0'; i++)
    {
      if (fill)
	{
	  if (distmacro[i].c < 128)
	    sprintf(key, "%c\0", distmacro[i].c);
	  else
	    sprintf(key, "^%c\0", distmacro[i].c - 96);
	  sprintf(maclines[0], "%-8s %s",
		  key,
		  distmacro[i].name);
	  W_WriteText(macroWin, 2, row++, W_Yellow, maclines[0],
		      strlen(maclines[0]), W_RegularFont);
	}
      lineno++;
      num = formatline(distmacro[i].macro);
      if (fill)
	{
	  for (c = 0; c < num; c++)
	    {
	      W_WriteText(macroWin, 8, row++, textColor, maclines[c],
			  strlen(maclines[c]), W_RegularFont);
	    }
	}
      if (lineno > NUMLINES)
	continue;
    }
}



void
        fillmacro(void)
{
  register int row, i;
  char    macromessage[MACROLEN];

  W_ClearWindow(macroWin);
  sprintf(macromessage, "Packages active:  NBT%s%s\0",
	  (UseNewMacro ? ", NEWMACRO" : ""),
	  (UseSmartMacro ? ", SMARTMACRO" : ""));

  W_WriteText(macroWin, 2, 1, textColor,
	      macromessage, strlen(macromessage), W_RegularFont);

  sprintf(macromessage, "Currently showing: %s\0",
	  (maclevel ? "Macros" : "RCDS"));

  W_WriteText(macroWin, 2, 2, textColor,
	      macromessage, strlen(macromessage), W_RegularFont);


  if (maclevel == 0)
    {
      W_WriteText(macroWin, 2, 4, W_Yellow,
		  "Key     Distress Name", 21, W_RegularFont);
      filldist(1);
      return;
    }

  /* 4 column macro window. This may be changed depending on font size */
  for (row = 4, i = 0; i < macrocnt; row++, i++)
    {
      if (macro[i].key <= 128)
	sprintf(macromessage, "%c \0", macro[i].key);
      else
	sprintf(macromessage, "^%c\0", macro[i].key - 96);
      if (macro[i].type == NEWMMOUSE)
	{
	  switch (macro[i].who)
	    {
	    case MACRO_PLAYER:
	      strcat(macromessage, " PL MS ");
	      break;
	    case MACRO_TEAM:
	      strcat(macromessage, " TM MS ");
	      break;
	    default:
	      strcat(macromessage, " SELF  ");
	      break;
	    }
	}
      else
	{
	  switch (macro[i].who)
	    {
	    case 'T':
	      strcat(macromessage, " TEAM  ");
	      break;
	    case 'A':
	      strcat(macromessage, " ALL   ");
	      break;
	    case 'F':
	      strcat(macromessage, " FED   ");
	      break;
	    case 'R':
	      strcat(macromessage, " ROM   ");
	      break;
	    case 'K':
	      strcat(macromessage, " KLI   ");
	      break;
	    case 'O':
	      strcat(macromessage, " ORI   ");
	      break;
	    case 'M':
	      strcat(macromessage, " MOO   ");
	      break;

#ifdef TOOLS
	    case '!':
	      strcat(macromessage, " SHELL ");
	      break;
#endif

#ifdef NEWMACRO
	    case '\0':
	      strcat(macromessage, " SPEC  ");
	      break;
#endif

	    default:
	      strcat(macromessage, " ----  ");
	      break;
	    }
	}
      strcat(macromessage, macro[i].string);
      macromessage[MAXMACRO] = '\0';
      W_WriteText(macroWin, 2, row, textColor,
		  macromessage, strlen(macromessage), W_RegularFont);
    }
}

void    switchmacros(void)
{
  int     num = macrocnt + 5;


  if (!macroWin)
    return;					 /* paranoia? */

  maclevel = abs(maclevel - 1);

  if (maclevel == 0)
    {
      lineno = 0;
      filldist(0);
      num = lineno + 5;
    }

  W_ResizeTextWindow(macroWin, 80, num);
  W_SetWindowExposeHandler(macroWin, fillmacro);
  W_SetWindowButtonHandler(macroWin, switchmacros);
  W_MapWindow(macroWin);
}



showMacroWin(void)
{
  int     num = macrocnt + 5;

  if (!macroWin)
    {
      if (maclevel == 0)
	{
	  lineno = 0;
	  filldist(0);
	  num = lineno + 5;
	}

      /* we'll use GWINSIDE since it is (probably) less than or equal to *
       * TWINSIDE.  The smaller the better. */
      macroWin = W_MakeTextWindow("macrow", GWINSIDE + BORDER, BORDER,
				  80, num, NULL, BORDER);

      W_ResizeTextWindow(macroWin, 80, num);
      W_DefineTrekCursor(macroWin);
      W_SetWindowExposeHandler(macroWin, fillmacro);
      W_SetWindowButtonHandler(macroWin, switchmacros);
      W_MapWindow(macroWin);
    }
  else if (W_IsMapped(macroWin))
    W_UnmapWindow(macroWin);
  else
    W_MapWindow(macroWin);
}
