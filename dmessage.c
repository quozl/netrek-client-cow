

/* dmessage.c
 * 
 * for the client of a socket based protocol.
 *
 * $Log: dmessage.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:09  siegl
 * COW 3.0 initial revision
 * */
#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include <math.h>
#include <time.h>
#include INC_SYS_TIME

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "version.h"
#include "patchlevel.h"

char   *censor(char *text);
extern char cowid[];
static int version_sent = 0;

dmessage(char *message, unsigned char flags, unsigned char from, unsigned char to)
{
  register int len;
  W_Color color;
  char    timebuf[10];
  LONG    curtime;
  struct tm *tm;
  char    buf[80];
  int     take, destroy, team, kill, killp, killa, bomb, conq;
  struct distress dist;

  take = MTEAM + MTAKE + MVALID;
  destroy = MTEAM + MDEST + MVALID;
  kill = MALL + MKILL + MVALID;
  killp = MALL + MKILLP + MVALID;
  killa = MALL + MKILLA + MVALID;
  bomb = MTEAM + MBOMB + MVALID;
  team = MTEAM + MVALID;
  conq = MALL + MCONQ + MVALID;

  time(&curtime);
  tm = localtime(&curtime);
  sprintf(timebuf, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
  len = strlen(message);
  if (from == 255)
    {
      /* From God */
      color = textColor;
    }
  else
    {
      color = playerColor(&(players[from]));
    }
  if (censorMessages)
    if ((flags != kill) && (flags != killp) &&
	(flags != killa) && (flags != bomb) &&
	(flags != take) && (flags != destroy))
      censor(message);

  /* aha! A new type distress/macro call came in. parse it appropriately */
  if (flags == (MTEAM | MDISTR | MVALID))
    {
      HandleGenDistr(message, from, to, &dist);
      len = makedistress(&dist, message, distmacro[dist.distype].macro);

#ifdef BEEPLITE
      if (UseLite)
	rcdlite(&dist);
#endif

      if (len <= 0)
	return;
      flags ^= MDISTR;
    }


  if (niftyNewMessages)
    {
      if (logmess)
	{
	  if (logFile != NULL)
	    {
	      fprintf(logFile, "%s: %s\n", timebuf, message);
	      fflush(logFile);
	    }
	  else
	    {
	      printf("%s: %s\n", timebuf, message);
	    }
	}
      if (!(logmess && logFile == NULL) && flags == conq)
	{
	  /* output conquer stuff to stdout in addition to message window */
	  fprintf(stdout, "%s\n", message);
	  if (instr(message, "kill"))
	    {
	      fprintf(stdout, "NOTE: The server here does not properly set message flags\n");
	      fprintf(stdout, "You should probably pester the server god to update....\n");
	    }
	}
      if (flags == (MCONFIG + MINDIV + MVALID))
	{
	  if (from == 255)
	    CheckFeatures(message);
	  return;
	}
      if ((flags == team) || (flags == take) || (flags == destroy))
	{
	  W_WriteText(messwt, 0, 0, color, message, len, shipFont(me));
	  if ((flags == team) &&
	      !strncmp(message + 10, "     ", 5) && (message[15] == 0))
	    {
	      printf("dmessage:flags==team PIG call from=%d\n", from);
	      pmessage(cowid, from, MINDIV);
	    }
	}

      else if ((flags == kill) || (flags == killp) ||
	       (flags == killa) || (flags == bomb))
	{
	  W_WriteText(messwk, 0, 0, color, message, len, 0);
	  if (!reportKills)
	    return;				 /* HW */
	}

      else if (flags & MINDIV)
	{
	  W_WriteText(messwi, 0, 0, color, message, len, 0);
	  if (!strncmp(message + 10, "     ", 5) && (message[15] == 0))
	    {
	      printf("dmessage:MINDIV PIG call from=%d\n", from);
	      pmessage(cowid, from, MINDIV);
	    }
	}
      else
	{					 /* if we don't know where *
						  * * the message beLONGs by
						  * * * this time, stick it
						  * in * * the all board... */
	  W_WriteText(messwa, 0, 0, color, message, len, 0);
	  if (!strncmp(message + 10, "     ", 5) && (message[15] == 0))
	    {
	      pmessage(cowid, from, MINDIV);
	    }
	}
      W_WriteText(reviewWin, 0, 0, color, message, len, 0);

    }
  else
    {

      /* ok, here we have the old kludge nastiness that we can turn on if we
       * * * HAVE to.  yuk, blech, ptooie... */

      if ((strncmp(message, "GOD->ALL", 8) == 0 &&
	   (instr(message, "was kill") ||
	    instr(message, "killed by"))) ||
	  (*message != ' ' && instr(message, "We are being attacked")))
	{
	  W_WriteText(messwk, 0, 0, color, message, len, 0);
	  if (!reportKills)
	    return;
	  W_WriteText(reviewWin, 0, 0, color, message, len, 0);
	  if (logmess)
	    {
	      if (logFile != NULL)
		{
		  fprintf(logFile, "%s ", timebuf);
		  fprintf(logFile, "%s\n", message);
		  fflush(logFile);
		}
	      else
		{
		  printf("%s ", message);
		  printf("%s\n", timebuf);
		}
	    }
	  return;
	}
      switch (flags & (MTEAM | MINDIV | MALL))
	{
	case MTEAM:
	  W_WriteText(messwt, 0, 0, color, message, len, 0);
	  if (!strncmp(message + 10, "     ", 5) && (message[15] == 0))
	    {
	      pmessage(cowid, from, MINDIV);
	    }
	  if (logmess)
	    {
	      if (logFile != NULL)
		{
		  fprintf(logFile, "%s ", timebuf);
		  fprintf(logFile, "%s\n", message);
		  fflush(logFile);
		}
	      else
		{
		  printf("%s ", message);
		  printf("%s\n", timebuf);
		}
	    }
	  break;
	case MINDIV:
	  if (!(flags & MCONFIG))
	    W_WriteText(messwi, 0, 0, color, message, len, 0);
	  if (!strncmp(message + 10, "     ", 5) && (message[15] == 0))
	    {
	      pmessage(cowid, from, MINDIV);
	    }
	  if (logmess)
	    {
	      if (logFile != NULL)
		{
		  fprintf(logFile, "%s ", timebuf);
		  fprintf(logFile, "%s\n", message);
		  fflush(logFile);
		}
	      else
		{
		  printf("%s ", message);
		  printf("%s\n", timebuf);
		}
	    }
	  break;
	default:
	  W_WriteText(messwa, 0, 0, color, message, len, 0);
	  if (!strncmp(message + 10, "     ", 5) && (message[15] == 0))
	    {
	      pmessage(cowid, from, MINDIV);
	    }
	  if (logmess)
	    {
	      if (logFile != NULL)
		{
		  fprintf(logFile, "%s ", timebuf);
		  fprintf(logFile, "%s\n", message);
		  fflush(logFile);
		}
	      else
		{
		  printf("%s", message);
		  printf("%s\n", timebuf);
		}
	    }
	  break;
	}
      W_WriteText(reviewWin, 0, 0, color, message, len, 0);
    }
}

instr(char *string1, char *string2)
{
  char   *s;
  int     length;

  length = strlen(string2);
  for (s = string1; *s != 0; s++)
    {
      if (*s == *string2 && strncmp(s, string2, length) == 0)
	return (1);
    }
  return (0);
}

nothing(void)
{
  int     foo;
  char    buf[80];

  STRNCPY(buf, "this does nothing, really", 30);
}

CheckFeatures(char *m)
{
  char    buf[BUFSIZ];
  char   *pek = &m[10];

  if (strlen(m) < 11)
    return;

  while ((*pek == ' ') && (*pek != '\0'))
    pek++;

  STRNCPY(buf, "COW: ", 6);

  if (!strcmp(pek, "NO_NEWMACRO"))
    {
      UseNewMacro = 0;
      strcat(buf, pek);
    }

  if (!strcmp(pek, "NO_SMARTMACRO"))
    {
      UseSmartMacro = 0;
      strcat(buf, pek);
    }

  if (!strcmp(pek, "WHY_DEAD"))
    {
      why_dead = 1;
      strcat(buf, pek);
    }

  if (!strcmp(pek, "RC_DISTRESS"))
    {
      gen_distress = 1;
      distmacro = dist_prefered;
      strcat(buf, pek);
    }

#ifdef MOTION_MOUSE
  if (!strcmp(pek, "NO_CONTINUOUS_MOUSE"))
    {
      motion_mouse_enablable = 0;
      strcat(buf, pek);
    }
#endif

#ifdef MULTILINE_MACROS
  if (!strcmp(pek, "MULTIMACROS"))
    {
      multiline_enabled = 1;
      strcat(buf, pek);
    }
#endif

  if (!strcmp(pek, "SBHOURS"))
    {
      SBhours = 1;
      strcat(buf, pek);
    }

  /* Client spezific notes sent by the server */
  if (!strncmp(pek, "INFO", 4))
    {
      strcat(buf, pek);
    }

  if (strlen(buf) == 5)
    {
      strcat(buf, "UNKNOWN FEATURE: ");
      strcat(buf, pek);
    }

  buf[79] = '\0';

  /* printf("%s\n", buf); */

  W_WriteText(reviewWin, 0, 0, W_White, buf, strlen(buf), 0);

#ifdef TOOLS
  W_WriteText(toolsWin, 0, 0, textColor, buf, strlen(buf), W_RegularFont);
#else
  W_WriteText(messwi, 0, 0, W_White, buf, strlen(buf), 0);
#endif
}


sendVersion(void)
{
  char    client_ver[15];

  if (!version_sent)
    {
      version_sent = 1;
      sprintf(client_ver, "@%s.%d", mvers, PATCHLEVEL);

      pmessage(client_ver, me->p_no, MINDIV | MCONFIG);
    }
}
