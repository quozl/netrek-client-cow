/*
 * Local.h
 *
 * Functions to maintain the local map.
 *
 *
 * $Log: local.h,v $
 * Revision 1.1.1.1  1998/11/01 17:24:10  siegl
 * COW 3.0 initial revision
 * */

#ifndef h_local
#define h_local

/* Global Defines */

#define TORP_UPDATE_FUSE 6	 /* Ignore torp if no update in this time */
#define PHASER_UPDATE_FUSE 11	 /* Ignore phaser if no update in this time */
#define PLASMA_UPDATE_FUSE 6	 /* Ignore plasma if no update in this time */


/* Global Functions */
 
inline void clearLocal(void);
/*
   Clear the local map (intelligently rather than just simply wiping
   the map).
*/

inline void local(void);
/*
   Draw out the 'tactical' map.
*/



#endif  /* Not defined h_local */

