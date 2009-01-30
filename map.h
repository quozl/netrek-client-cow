/*
 * map.h
 *
 * Functions to maintain the galactic map.  This file is a merger
 * of code from redraw.c, which was too big before, and planets.c,
 * which was too small.
 *
 */
 
#ifndef h_map
#define h_map
	

/*
 *  Global Variables:
 *
 *  redrawall		-- Erase and redraw the galactic?
 *  redrawPlayer[]	-- Flag for each player on whether their position
 *				on the galactic is not out of date.
 */
 
extern int redrawall;
extern unsigned char redrawPlayer[];


/* Global Functions */

void
initPlanets(void);
/*
 *  Make a rough map of the location of all the planets to help decide
 *  whether a ship is possibly overlapping a planet.
 */


void map(void);
/*
 *  Update the 'galactic' map.
 */
 
 
#endif /*defined h_map */

