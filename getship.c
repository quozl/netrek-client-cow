

/* getship.c for client of socket protocol.
 * 
 * This file has been mangled so it only sets the ship characteristics needed.  *
 * $Log: getship.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:09  siegl
 * COW 3.0 initial revision
 * */
#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include <sys/types.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"

/* fill in ship characteristics */

void    getshipdefaults()
{
  shipvals[SCOUT].s_phaserdamage = 75;		 /* scout: was 75 */
  shipvals[SCOUT].s_torpspeed = 16;		 /* scout: was 16 */
  shipvals[SCOUT].s_maxspeed = 12;		 /* scout:  */
  shipvals[SCOUT].s_maxfuel = 5000;		 /* scout:  */
  shipvals[SCOUT].s_maxarmies = 2;		 /* scout:  */
  shipvals[SCOUT].s_maxshield = 75;		 /* scout: was 75 */
  shipvals[SCOUT].s_maxdamage = 75;		 /* scout:  */
  shipvals[SCOUT].s_maxwpntemp = 1000;		 /* scout:  */
  shipvals[SCOUT].s_maxegntemp = 1000;		 /* scout:  */
  shipvals[SCOUT].s_type = SCOUT;		 /* scout:  */
  shipvals[SCOUT].s_width = 20;			 /* scout:  */
  shipvals[SCOUT].s_height = 20;		 /* scout:  */

  shipvals[DESTROYER].s_phaserdamage = 85;	 /* destroyer: */
  shipvals[DESTROYER].s_torpspeed = 14;		 /* destroyer: */
  shipvals[DESTROYER].s_maxspeed = 10;		 /* destroyer: */
  shipvals[DESTROYER].s_maxfuel = 7000;		 /* destroyer: */
  shipvals[DESTROYER].s_maxarmies = 5;		 /* destroyer: */
  shipvals[DESTROYER].s_maxshield = 85;		 /* destroyer: */
  shipvals[DESTROYER].s_maxdamage = 85;		 /* destroyer: */
  shipvals[DESTROYER].s_maxwpntemp = 1000;	 /* destroyer: */
  shipvals[DESTROYER].s_maxegntemp = 1000;	 /* destroyer: */
  shipvals[DESTROYER].s_width = 20;		 /* destroyer: */
  shipvals[DESTROYER].s_height = 20;		 /* destroyer: */
  shipvals[DESTROYER].s_type = DESTROYER;	 /* destroyer: */

  shipvals[BATTLESHIP].s_phaserdamage = 105;	 /* battleship: */
  shipvals[BATTLESHIP].s_torpspeed = 12;	 /* battleship: */
  shipvals[BATTLESHIP].s_maxspeed = 8;		 /* battleship: */
  shipvals[BATTLESHIP].s_maxfuel = 14000;	 /* battleship: */
  shipvals[BATTLESHIP].s_maxarmies = 6;		 /* battleship: */
  shipvals[BATTLESHIP].s_maxshield = 130;	 /* battleship: */
  shipvals[BATTLESHIP].s_maxdamage = 130;	 /* battleship: */
  shipvals[BATTLESHIP].s_maxwpntemp = 1000;	 /* battleship: */
  shipvals[BATTLESHIP].s_maxegntemp = 1000;	 /* battleship: */
  shipvals[BATTLESHIP].s_width = 20;		 /* battleship: */
  shipvals[BATTLESHIP].s_height = 20;		 /* battleship: */
  shipvals[BATTLESHIP].s_type = BATTLESHIP;	 /* battleship: */

  shipvals[ASSAULT].s_phaserdamage = 80;	 /* assault */
  shipvals[ASSAULT].s_torpspeed = 16;		 /* assault */
  shipvals[ASSAULT].s_maxspeed = 8;		 /* assault */
  shipvals[ASSAULT].s_maxfuel = 6000;		 /* assault */
  shipvals[ASSAULT].s_maxarmies = 20;		 /* assault */
  shipvals[ASSAULT].s_maxshield = 80;		 /* assault */
  shipvals[ASSAULT].s_maxdamage = 200;		 /* assault */
  shipvals[ASSAULT].s_maxwpntemp = 1000;	 /* assault */
  shipvals[ASSAULT].s_maxegntemp = 1200;	 /* assault */
  shipvals[ASSAULT].s_width = 20;		 /* assault */
  shipvals[ASSAULT].s_height = 20;		 /* assault */
  shipvals[ASSAULT].s_type = ASSAULT;		 /* assault */

  shipvals[STARBASE].s_phaserdamage = 120;	 /* starbase */
  shipvals[STARBASE].s_torpspeed = 14;		 /* starbase */
  shipvals[STARBASE].s_maxfuel = 60000;		 /* starbase */
  shipvals[STARBASE].s_maxarmies = 25;		 /* starbase */
  shipvals[STARBASE].s_maxshield = 500;		 /* starbase */
  shipvals[STARBASE].s_maxdamage = 600;		 /* starbase */
  shipvals[STARBASE].s_maxspeed = 2;		 /* starbase */
  shipvals[STARBASE].s_maxwpntemp = 1300;	 /* starbase */
  shipvals[STARBASE].s_maxegntemp = 1000;	 /* starbase */
  shipvals[STARBASE].s_width = 20;		 /* starbase */
  shipvals[STARBASE].s_height = 20;		 /* starbase */
  shipvals[STARBASE].s_type = STARBASE;		 /* starbase */

  shipvals[ATT].s_phaserdamage = 10000;		 /* att: */
  shipvals[ATT].s_torpspeed = 30;		 /* att: */
  shipvals[ATT].s_maxspeed = 60;		 /* att: */
  shipvals[ATT].s_maxfuel = 12000;		 /* att: */
  shipvals[ATT].s_maxarmies = 1000;		 /* att: */
  shipvals[ATT].s_maxshield = 30000;		 /* att: */
  shipvals[ATT].s_maxdamage = 30000;		 /* att: */
  shipvals[ATT].s_maxwpntemp = 10000;		 /* att: */
  shipvals[ATT].s_maxegntemp = 10000;		 /* att: */
  shipvals[ATT].s_width = 28;			 /* att: */
  shipvals[ATT].s_height = 28;			 /* att: */
  shipvals[ATT].s_type = ATT;			 /* att: */

  shipvals[SGALAXY].s_phaserdamage = 10000;	 /* galaxy: */
  shipvals[SGALAXY].s_torpspeed = 30;		 /* galaxy: */
  shipvals[SGALAXY].s_maxspeed = 60;		 /* galaxy: */
  shipvals[SGALAXY].s_maxfuel = 12000;		 /* galaxy: */
  shipvals[SGALAXY].s_maxarmies = 1000;		 /* galaxy: */
  shipvals[SGALAXY].s_maxshield = 30000;	 /* galaxy: */
  shipvals[SGALAXY].s_maxdamage = 30000;	 /* galaxy: */
  shipvals[SGALAXY].s_maxwpntemp = 10000;	 /* galaxy: */
  shipvals[SGALAXY].s_maxegntemp = 10000;	 /* galaxy: */
  shipvals[SGALAXY].s_width = 20;		 /* galaxy: */
  shipvals[SGALAXY].s_height = 20;		 /* galaxy: */
  shipvals[SGALAXY].s_type = SGALAXY;		 /* galaxy: */

  shipvals[CRUISER].s_phaserdamage = 100;	 /* cruiser: */
  shipvals[CRUISER].s_torpspeed = 12;		 /* cruiser: */
  shipvals[CRUISER].s_maxspeed = 9;		 /* cruiser: */
  shipvals[CRUISER].s_maxfuel = 10000;		 /* cruiser: */
  shipvals[CRUISER].s_maxarmies = 10;		 /* cruiser: */
  shipvals[CRUISER].s_maxshield = 100;		 /* cruiser: */
  shipvals[CRUISER].s_maxdamage = 100;		 /* cruiser: */
  shipvals[CRUISER].s_maxwpntemp = 1000;	 /* cruiser: */
  shipvals[CRUISER].s_maxegntemp = 1000;	 /* cruiser: */
  shipvals[CRUISER].s_width = 20;		 /* cruiser: */
  shipvals[CRUISER].s_height = 20;		 /* cruiser: */
  shipvals[CRUISER].s_type = CRUISER;		 /* cruiser: */
}

getship(struct ship *shipp, int s_type)
{
  memcpy((char *) shipp, (char *) &(shipvals[s_type]), sizeof(struct ship));
}
