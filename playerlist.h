/*
   playerlist.h
   
   All functions which update the playerlist should include this
   header file.
 *
 * $Log: playerlist.h,v $
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 * */

#ifndef h_playerlist
#define h_playerlist


/*
   Constants.
*/

#define PLISTLASTSTYLE 4  /* The number of the default last plist style */


/*
   Global Variables
   
   partitionPlist    : Separate the goodies from baddies in the sorted list?
   plistCustomLayout : The value of `playerlist' in the defaults file.
   plistReorder      : True only if the order of the playerlist is out of date.
   plistStyle        : The current style number for the player list.
   plistUpdated      : True only if the player list is out of date.
   sortMyTeamFirst   : Should my team go first or second in the sorted list?
   sortPlayers       : Should the player list be sorted?
   updatePlayer[plr] : The playerlist entry for "plr" is old.
   
   plistHasHostile   : True if "Hostile" is a field in the current list.
   plistHasSpeed     : True if "Speed" is true in the current playerlist.
*/

extern int partitionPlist;
extern char *plistCustomLayout;
extern int plistReorder;
extern int plistStyle;
extern int plistUpdated;
extern int sortMyTeamFirst;
extern int sortPlayers;
extern char updatePlayer[MAXPLAYER+1];
					
#ifdef PLIST2					
extern int plistHasHostile = FALSE;
extern int plistHasSpeed = FALSE;
#endif


/*
   Macro Declarations
*/


/*
   void PlistNoteUpdate(i)
	int i;
	
   Note the update of a player so that the entry in the player list
   can be update.
*/
#define PlistNoteUpdate(i) { updatePlayer[i] = 1; plistUpdated = 1; }


/*
   void PlistNoteArrive(i)
	int i;

   Note the arrive or leaving of a player from a team.  This
   call should also be made if a player changes teams.
*/
#define PlistNoteArrive(i) { plistReorder = 1; }


/*
   void PlistNoteHostile(i)
	int i;
	
   Note the change in war status of player `i'.  Only update if
   the war status is shown on the current player list.
*/
#ifdef PLIST2
#define PlistNoteHostile(i) { if (plistHasHostile) PlistNoteUpdate(i); }
#else
#define PlistNoteHostile(i) {};
#endif


/*
   void PlistNoteSpeed(i)
	int i;
	
   Note the change in speed of player `i'.  Only update if
   speed is shown on the current player list.
*/
#ifdef PLIST2
#define PlistNoteSpeed(i) { if (plistHasSpeed) PlistNoteUpdate(i); }
#else
#define PlistNoteSpeed(i) {};
#endif


/*
   Function Declarations
*/
					
void InitPlayerList(void);
/*
   Set the global variables associtated with the player list.
   
   This should be called when the defaults file is loaded or reloaded.
*/


int PlistMaxWidth(void);
/*
   Calculate the maximum width of all the defined player lists so
   that the width of the player list window can be defined.
*/


void RedrawPlayerList(void);
/*
   Completly redraw the player list, rather than incrimentally updating
   the list as with UpdatePlayerList().
   
   This function should be called if the plistStyle changes or if the
   window has just been revealed.
   
   This function is called automatically from InitPlayerList.
*/


/*
   void UpdatePlayerList()
   
   
   Update the player list.
   
   This function works incrimentally.  If a dramatic change has taken
   place (i.e. if plistStyle changes) then RedrawPlayerList() should
   be called instead.
   
   This function is called through a macro.  It is expected that 9
   times out of 10 the call will not do any work.  We can predict
   when work will be done by looking at "plistUpdated".  To avoid
   redundant procedure calls, the macro only calls the function if
   plistUpdated suggests that work will be done.
*/
void UpdatePlistFn(void);
#define UpdatePlayerList()	if (plistUpdated) UpdatePlistFn()


#endif /* defined h_playerlist */
