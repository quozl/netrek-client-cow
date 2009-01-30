#include "config.h"
#include "copyright.h"

#include <stdio.h>

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"

static char *teamname[9] =
{
  "IND",
  "FED",
  "ROM",
  "",
  "KLI",
  "",
  "",
  "",
  "ORI"
};

/* * Open a window which contains all the planets and their current *
 * statistics.  Players will not know about planets that their team * has not
 * orbited. */

void    planetlist(void)
{
  register int i;
  register int k = 0;
  char    buf[BUFSIZ];
  register struct planet *j;

  /* W_ClearWindow(planetw); */
  (void) sprintf(buf, "Planet Name      own armies REPAIR FUEL AGRI CORE info");
  W_WriteText(planetw, 2, 1, textColor, buf, strlen(buf), W_RegularFont);
  k = 2;
  for (i = 0, j = &planets[i]; i < MAXPLANETS; i++, j++)
    {
      if (j->pl_info & me->p_team)
	{
	  (void) sprintf(buf, "%-16s %3s %3d    %6s %4s %4s %4s %c%c%c%c",
			 j->pl_name,
			 teamname[j->pl_owner],
			 j->pl_armies,
			 (j->pl_flags & PLREPAIR ? "REPAIR" : "      "),
			 (j->pl_flags & PLFUEL ? "FUEL" : "    "),
			 (j->pl_flags & PLAGRI ? "AGRI" : "    "),
			 (j->pl_flags & PLCORE ? "CORE" : "    "),
			 (j->pl_info & FED ? 'F' : ' '),
			 (j->pl_info & ROM ? 'R' : ' '),
			 (j->pl_info & KLI ? 'K' : ' '),
			 (j->pl_info & ORI ? 'O' : ' '));
	  W_WriteText(planetw, 2, k++, planetColor(j), buf, strlen(buf),
		      planetFont(j));
	}
      else
	{
	  (void) sprintf(buf, "%-16s",
			 j->pl_name);
	  W_WriteText(planetw, 2, k++, unColor, buf, strlen(buf),
		      W_RegularFont);
	}
    }
}
