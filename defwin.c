

#include "config.h"

#ifdef XTREKRC_HELP
/* taken from helpwin.c (copyright 1991 ERic mehlhaff Free to use, hack, etc.
 * Just keep these credits here. Use of this code may be dangerous to your
 * health and/or system. Its use is at your own risk. I assume no
 * responsibility for damages, real, potential, or imagined, resulting  from
 * the use of it.)
 * 
 * $Log: defwin.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:09  siegl
 * COW 3.0 initial revision
 * */

#include <stdio.h>
#include "math.h"
#include <signal.h>
#include <sys/types.h>
#include <time.h>
#include INC_SYS_TIME
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "playerlist.h"
#include INC_STRINGS

/* this is the number of help messages there are */

#define INT_DEF		0
#define BOOL_DEF	1
#define STR_DEF		2
#define SINT_DEF	3

#define NAME_WIDTH	18
#define VAL_WIDTH	8
#define INDENT		3
#define MAX_VLINES	42

#ifdef RECORD
extern char *recordFileName;

#endif

#define DEFMESSAGES	(sizeof(def_messages)/ sizeof(struct def))

char   *name = NULL, *cloak_chars = NULL, *bmap = NULL, *keymap = NULL,
       *plist = NULL, *ckeymap = NULL;

/* sure its a mess, but it gets the job done */

static
struct def
  {
    char   *name;
    int     type;
    char   *desc;
    int    *variable;

    struct
      {
	int     i_value;			 /* if int or bool */
	char   *s_value;			 /* if str */
	char   *desc;
      }
    values[10];
  }

def_messages[] =
{
  {
    "extraAlertBorder", BOOL_DEF, "Show alert on local border",
	&extraBorder,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "keepPeace", BOOL_DEF, "Stay peaceful when reborn",
	&keeppeace,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "keepInfo", SINT_DEF, "No. of updates to keep info windows",
	&keepInfo,
    {
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "netStatFreq", SINT_DEF, "Frequency of updates to calc lag",
	&netstatfreq,
    {
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "enemyPhasers", SINT_DEF, "Width of enemy phasers",
	&enemyPhasers,
    {
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "netStats", BOOL_DEF, "Lag stats and lag meter display",
	&netstat,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,

#ifdef VARY_HULL
  {
    "warnHull", BOOL_DEF, "Warn hull state based on damage",
	&vary_hull,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
#endif

#ifdef VSHIELD_BITMAPS
  {
    "varyShields", BOOL_DEF, "Vary shields base on damage",
	&VShieldBitmaps,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
#endif

  {
    "warnShields", BOOL_DEF, "Shiild color based on alert status",
	&warnShields,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,

  {
    "newPlist", BOOL_DEF, "Show new player list",
	&newPlist,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "reportKills", BOOL_DEF, "Report kill messages",
	&reportKills,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "showGalactic", INT_DEF, "Galactic planet bitmaps",
	&showgalactic,
    {
      {
	0, NULL, "show nothing on galactic map"
      }
      ,
      {
	1, NULL, "show owner on galactic map"
      }
      ,
      {
	2, NULL, "show standard resources on galactic map"
      }
      ,
      {
	3, NULL, "show MOO resources on galactic map"
      }
      ,
      {
	4, NULL, "show rabbit ears on galactic map"
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "showLocal", INT_DEF, "Local planet bitmaps",
	&showlocal,
    {
      {
	0, NULL, "show nothing on local map"
      }
      ,
      {
	1, NULL, "show owner on local map"
      }
      ,
      {
	2, NULL, "show standard resources on local map"
      }
      ,
      {
	3, NULL, "show MOO resources on local map"
      }
      ,
      {
	4, NULL, "show rabbit ears on local map"
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "showLock", INT_DEF, "Lock display for planets/players",
	&showLock,
    {
      {
	0, NULL, "don't show lock"
      }
      ,
      {
	1, NULL, "show lock on galactic only"
      }
      ,
      {
	2, NULL, "show lock on tactical only"
      }
      ,
      {
	3, NULL, "show lock on both"
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
  }
  ,
  {
    "name", STR_DEF, "Default player name",
	(int *) &(name),
    {
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "keymap", STR_DEF, "Keyboard map",
	(int *) &(keymap),
    {
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "ckeymap", STR_DEF, "Control keyboard map",
	(int *) &(ckeymap),
    {
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "buttonmap", STR_DEF, "Mouse button map",
	(int *) &(bmap),
    {
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "cloakChars", STR_DEF, "Cloak chars for map",
	(int *) &(cloak_chars),
    {
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "playerListStyle", INT_DEF, "The style for the playerlist",
	&plistStyle,
    {
      {
	0, NULL, "Custom player list"
      }
      ,
      {
	1, NULL, "Old Player List"
      }
      ,
      {
	2, NULL, "COW Player List"
      }
      ,
      {
	3, NULL, "Kill Watch Player List"
      }
      ,
      {
	4, NULL, "BRMH Player List"
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
  }
  ,
  {
    "playerlist", STR_DEF, "What to show on custom player list",
	(int *) &(plistCustomLayout),
    {
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "partitionPlist", BOOL_DEF, "Use blank space in player list",
	&partitionPlist,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "showPlanetNames", BOOL_DEF, "Show names on map/local",
	&namemode,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "showTractorPressor", BOOL_DEF, "Show my tract/press",
	&showTractorPressor,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "continuetractor", BOOL_DEF, "Keep showing tract/press",
	&continuetractor,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,

#ifdef SHORT_PACKETS
  {
    "tryShort", BOOL_DEF, "Try SHORT-PACKETS at startup",
	&tryShort1,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
#endif

  {
    "tryUdp", BOOL_DEF, "Try UDP automatically",
	&tryUdp1,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "udpClientReceive", INT_DEF, "UDP receive mode",
	&udpClientRecv,
    {
      {
	0, NULL, "TCP only"
      }
      ,
      {
	1, NULL, "simple UDP"
      }
      ,
      {
	2, NULL, "fat UDP"
      }
      ,
      {
	3, NULL, "double UDP (obsolete)"
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "udpClientSend", INT_DEF, "UDP send mode",
	&udpClientSend,
    {
      {
	0, NULL, "TCP only"
      }
      ,
      {
	1, NULL, "simple UDP"
      }
      ,
      {
	2, NULL, "enforced UDP (state only)"
      }
      ,
      {
	3, NULL, "enforced UDP (state & weapon)"
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "udpSequenceCheck", BOOL_DEF, "UDP sequence checking",
	&udpSequenceChk,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,

#ifdef RSA
  {
    "useRSA", BOOL_DEF, "Use RSA checking",
	&RSA_Client,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
#endif

  {
    "newdashboard", INT_DEF, "Use new dashboard",
	&newDashboard,
    {
      {
	0, NULL, "Text dashboard"
      }
      ,
      {
	1, NULL, "COW style dashboard"
      }
      ,
      {
	2, NULL, "KRP style dashboard"
      }
      ,
      {
	3, NULL, "LABs new dashboard"
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,

#ifdef BEEPLITE
  {
    "lite", BOOL_DEF, "Use message highliting",
	&UseLite,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "DefLite", BOOL_DEF, "Use default lites",
	&DefLite,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
#endif

  {
    "newDistress", BOOL_DEF, "Use new distress call",
	&UseNewDistress,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "rejectMacro", BOOL_DEF, "Reject macros",
	&rejectMacro,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "showIND", BOOL_DEF, "Show independant planets w/X",
	&showIND,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,

#ifdef IGNORE_SIGNALS_SEGV_BUS
  {
    "ignoreSignals", BOOL_DEF, "Ignore SIGSEGV and SIGBUS",
	&ignore_signals,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
#endif

#ifdef MOTION_MOUSE
  {
    "continuousMouse", BOOL_DEF, "Continuous mouse input",
	&motion_mouse,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
  {
    "motionThresh", SINT_DEF, "Threshold for above",
	&user_motion_thresh,
    {
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
#endif

  {
    "ignoreCaps", BOOL_DEF, "Ignore the Capslock key",
	&ignoreCaps,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,

#ifdef SHIFTED_MOUSE
  {
    "shiftedMouse", BOOL_DEF, "More mouse buttons with shift",
	&extended_mouse,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
#endif

#ifdef TNG_FED_BITMAPS
  {
    "useTNGBitmaps", BOOL_DEF, "Use next generation bitmaps",
	&use_tng_fed_bitmaps,
    {
      {
	0, NULL, ""
      }
      ,
      {
	0, NULL, NULL
      }
      ,
    }
    ,
  }
  ,
#endif
};

char   *
        itos(int v)
{
  static char value[10];

  sprintf(value, "%d", v);
  return value;
}

char   *
        btoa(int v)
{
  if (v)
    return "on";
  else
    return "off";
}

void
        showdef(void)
{
  register i, j, x = 0, y = 0, xo = 0, yo = 0, max_desc = 0, height = 1,
          width = 1;
  register struct def *d;
  char   *val;

  name = getdefault("name");
  keymap = getdefault("keymap");
  ckeymap = getdefault("ckeymap");
  plist = getdefault("playerlist");
  cloak_chars = cloakChars;
  bmap = getdefault("buttonmap");

  if (!defWin)
    defWin = W_MakeTextWindow("xtrekrc_help", 1, 100, 174, 41, NULL, BORDER);

  for (i = 0, d = def_messages; i < DEFMESSAGES; i++, d++)
    {
      x = xo;
      y = yo;

      W_WriteText(defWin, x, y, W_Yellow, d->name, strlen(d->name),
		  W_BoldFont);
      x += NAME_WIDTH;

      W_WriteText(defWin, x, y, textColor, d->desc, strlen(d->desc),
		  W_RegularFont);
      if (strlen(d->desc) > max_desc)
	{
	  max_desc = strlen(d->desc);
	  width = MAX(width, x + max_desc);
	}
      y++;
      x = xo + INDENT;

      if (d->type != STR_DEF)
	{
	  if (!d->values[0].desc && d->variable)
	    {
	      if (d->type == SINT_DEF)
		val = itos(*d->variable);
	      else
		val = itos(d->values[0].i_value);

	      W_WriteText(defWin, x, y, W_Green, val, strlen(val),
			  W_RegularFont);
	      y++;
	    }
	  for (j = 0; d->values[j].desc; j++)
	    {
	      switch (d->type)
		{
		case INT_DEF:
		  val = itos(d->values[j].i_value);
		  if (d->values[j].i_value == *d->variable)
		    {
		      W_WriteText(defWin, x, y, W_Green, val, strlen(val),
				  W_BoldFont);
		      if (W_Mono())
			{
			  W_WriteText(defWin, x + 1, y, W_Green, "*", 1,
				      W_RegularFont);
			}
		    }
		  else
		    W_WriteText(defWin, x, y, textColor, val, strlen(val),
				W_RegularFont);
		  x = xo + NAME_WIDTH;
		  W_WriteText(defWin, x, y, textColor, d->values[j].desc,
			      strlen(d->values[j].desc), W_RegularFont);
		  y++;
		  x = xo + INDENT;
		  break;

		case BOOL_DEF:
		  val = btoa(*d->variable);
		  W_WriteText(defWin, x, y, W_Green, val, strlen(val),
			      W_RegularFont);
		  y++;
		  x = xo + INDENT;
		  break;
		default:
		  fprintf(stderr, "Unknown type.\n");
		  break;
		}
	    }
	}
      else if (d->variable && *d->variable)
	{
	  W_WriteText(defWin, x, y, W_Green, *((char **) d->variable),
		      strlen(*((char **) d->variable)),
		      W_RegularFont);
	  y++;
	}

      height = MAX(height, y);
      if (y > MAX_VLINES)
	{
	  yo = 0;
	  xo += NAME_WIDTH + max_desc + 2;
	  max_desc = 0;
	}
      else
	{
	  yo = y + 1;
	}
    }

  W_ResizeTextWindow(defWin, width, height);
  W_MapWindow(defWin);
}
#endif
