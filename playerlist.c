


/* playerlist.c
 * 
 * Fairly substantial re-write to do variable player lists: Sept 93 DRG Major
 * rewrite for demand driven updates -> huge speedup: Oct. 94 [007] Major
 * rewrite to fix some bugs and speed things up     : Jan. 95 [Zork]
 * 
 * 
 * TODO:
 * 
 * Sort observers separatly: opserver if (pl->p_flags & PFOBSERV) is true.
 *
 * $Log: playerlist.c,v $
 * Revision 1.2  2006/05/22 13:13:39  quozl
 * change defaults
 *
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 * */

#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "string_util.h"
#include "playerlist.h"


#define MaxPlistField 18			 /* The width of the longest
						  * * * possible plist field */

#define IsEmpty(x) (!x)
#define IsNotEmpty(x) (x)
#define IsZero(x) (!x)
#define IsNotZero(x) (x)
#define TRUE 1
#define FALSE 0

/* Current list of flags       */
/* ' ' - White Space           */
/* 'b' - Armies Bombed         */
/* 'd' - Damage Inflicted (DI) */
/* 'k' - Max Kills             */
/* 'l' - Login Name            */
/* 'n' - Ship Number           */
/* 'p' - Planets Taken         */
/* 'r' - Ratio                 */
/* 's' - Speed                 */
/* 'v' - Deaths per hour       */
/* 'w' - War staus             */
/* 'B' - Bombing               */
/* 'C' - Curt (short) rank     */
/* 'D' - Defense               */
/* 'H' - Hours Played          */
/* 'K' - Kills                 */
/* 'L' - Losses                */
/* 'M' - Display, Host Machine */
/* 'N' - Name                  */
/* 'O' - Offense               */
/* 'P' - Planets               */
/* 'R' - Rank                  */
/* 'S' - Total Rating (stats)  */
/* 'T' - Ship Type             */
/* 'V' - Kills per hour        */
/* 'W' - Wins                  */

/*
 * Global Variables
 * 
 * partitionPlist    : Separate the goodies from baddies in the sorted list?
 * plistCustomLayout : The value of `playerlist' in the defaults file.
 * plistReorder      : True only if the order of the playerlist is out of date.
 * plistStyle        : The current style number for the player list.
 * plistUpdated      : True only if the player list is out of date.
 * sortMyTeamFirst   : Should my team go first or second in the sorted list?
 * sortPlayers       : Should the player list be sorted?
 * updatePlayer[plr] : The playerlist entry for "plr" is old.
 * 
 * plistHasHostile   : True if "Hostile" is a field in the current list.
 * plistHasSpeed     : True if "Speed" is true in the current playerlist.
 */

int     partitionPlist = FALSE;
char   *plistCustomLayout;
int     plistReorder = FALSE;
int     plistStyle = 0;
int     plistUpdated = FALSE;
int     sortMyTeamFirst = FALSE;
int     sortPlayers = TRUE;
char    updatePlayer[MAXPLAYER + 1];

#ifdef PLIST2
int     plistHasHostile = FALSE;
int     plistHasSpeed = FALSE;

#endif /* PLIST2 */


/*
 * Local Variables
 * 
 * plistLayout       : The fields in the current playerlist.
 * plistPos[plr]     : The player list row assigned to each player.
 * plistWidth        : The width of the playerlist.
 * my_classes        : The letters to go with each ship type.
 */

static char *plistLayout = "";
static int plistPos[MAXPLAYER];
static int plistWidth = 0;
static char *my_classes[NUM_TYPES] =
{"SC", "DD", "CA", "BB", "AS", "SB", "GA", "??"};


/* Local Functions */

static int PlistHeader(char *layout, int doWrite);
static void PlistLine(struct player *j, int pos);
static void WriteSortedPlist(void);
static void WriteUnsortedPlist(void);



void    InitPlayerList()
/* Set the global variables associtated with the player list.  This
 * should be called when the defaults file is loaded or reloaded.
 */
{
  /* Find the default style number for the player list. */

  plistCustomLayout = getdefault("playerlist");



  /* Select a style number base on the information in the defaults * *
   * (.xtrekrc) file. */

  plistStyle = intDefault("playerListStyle", -1);

  if (IsZero(plistStyle) && IsEmpty(plistCustomLayout))
    {
      fputs(
       "Warning: `playerListStyle' set to 0 but `playerlist' is not set.\n",
	     stderr);
      fputs("\tPlease correct your defaults (.xtrekrc) file.\n", stderr);
      plistStyle = -1;
    }

#ifdef ALWAYS_USE_PLIST_CUSTOM_LAYOUT
  if (IsNotEmpty(plistCustomLayout))
    plistStyle = 0;
  else
#endif

  if (plistStyle > PLISTLASTSTYLE)
    {
      plistStyle = PLISTLASTSTYLE;
    }
  else if (plistStyle == -1)
    {
      /* Backward compatibility */

      if (IsNotEmpty(plistCustomLayout))
	{
	  plistStyle = 0;
	}
      else if (booleanDefault("newPlist", FALSE))
	{
	  plistStyle = 2;
	}
      else
	/* use the old playerlist */
	{
	  plistStyle = 1;
	}
    }


  /* Read the partitionPlist flag to tell us whether to break up the player * 
   * 
   * * list with blank lines. */

  partitionPlist = booleanDefault("partitionPlist", partitionPlist);


  /* Sort the playerlist? */

  sortPlayers = booleanDefault("sortPlayers", sortPlayers);


  /* My team goes first (or second)? */

  sortMyTeamFirst = booleanDefault("sortMyTeamFirst", sortMyTeamFirst);


  /* plistUpdate[MAXPLAYER] must always be TRUE because thats how we no when
   * * * to stop looking for a changed player. */

  updatePlayer[MAXPLAYER] = TRUE;
  RedrawPlayerList();
}


int     PlistMaxWidth()
/* Calculate the maximum width of all the defined player lists so that the
 * width of the player list window can be defined. */
{
  plistCustomLayout = getdefault("playerlist");

  if (IsNotEmpty(plistCustomLayout))
    plistWidth = PlistHeader(plistCustomLayout, FALSE);
  else
    plistWidth = 0;

  if (plistWidth < 83)
    plistWidth = 83;

  return plistWidth;
}


void    RedrawPlayerList()
/* Completly redraw the player list, rather than incrementally updating the
 * list as with UpdatePlayerList().
 * 
 * This function should be called if the plistStyle changes or if the
 * window has just been revealed.
 * 
 * This function is called automatically from InitPlayerList. */
{
  int     i;

  if (IsEmpty(me) || !W_IsMapped(playerw))
    return;


  /* Translate this style number into a player list layout string. */

  switch (plistStyle)
    {
    case 0:					 /* Custom */
      if (IsEmpty(plistCustomLayout))		 /* this is chacked for in */
	{					 /* InitPlayerList */
	  plistLayout = "";
	  printf("??? empty plistLayout? This can't happen!\n");
	}
      else
	plistLayout = plistCustomLayout;
      break;

    case 1:					 /* Old Style */
      plistLayout = "nTRNKWLr O D d ";
      break;

    case 2:					 /* COW Style */
      plistLayout = "nTR N  K lrSd";
      break;

    case 3:					 /* Kill Watch Style */
      plistLayout = "nTK  RNlr Sd";
      break;

    case 4:					 /* BRMH Style */
      plistLayout = "nTR N  K l M";
      break;

    default:
      fprintf(stderr, "Unknown plistStyle in ChangedPlistStyle().\n");
      break;
    }


  /* Redraw the player list. */

  W_ClearWindow(playerw);
  (void) PlistHeader(plistLayout, TRUE);

  plistReorder = TRUE;
  plistUpdated = TRUE;

  for (i = 0; i < MAXPLAYER; i++)
    updatePlayer[i] = TRUE;

  UpdatePlistFn();
}


void    UpdatePlistFn()
/* Update the player list.
 * 
 * This function should usually be called through the UpdatePlayerList() macro
 * (see playerlist.h).
 * 
 * This function works incrimentally.  If a dramatic change has taken place
 * (i.e. if plistStyle changes) then RedrawPlayerList() should be called
 * instead. */
{
  int     count;
  char   *update;

  plistUpdated = FALSE;

  if (!W_IsMapped(playerw))
    return;

  if (!plistReorder)
    {
      /* Redraw the lines that have changed. */

      update = updatePlayer - 1;

      for (;;)
	{
	  /* Find the changed player as quickly as possible. */

	  do
	    ++update;
	  while (!(*update));


	  /* Is this a valid player?  Remember updatePlayer[MAXPLAYER] is * * 
	   * always TRUE to make the above loop more efficient.  */

	  count = update - updatePlayer;
	  if (count == MAXPLAYER)
	    break;

	  *update = FALSE;


	  /* We should not get updates for free players any more, but just *
	   * * incase a packet arrives late... */

	  if (players[count].p_status != PFREE)
	    PlistLine(players + count, plistPos[count]);
	}
    }
  else
    {
      /* Reorder the player list.  Note that this may not require a full * *
       * rewrite. */

      plistReorder = FALSE;

      if (sortPlayers)
	WriteSortedPlist();
      else
	WriteUnsortedPlist();
    }
}


static void WriteSortedPlist()
/* Update the order of the players in the list and write out any changes. */
{
  int     row, i, last;
  struct player *current;
  int     teamPos[NUMTEAM + 1];
  int    *pos;

  static int plistLastRow = -1;
  static int blankLine = -1;
  static int blankLine2 = -1;
  static int myTeam = -1;


  /* If I have changed team, redraw everything (to get the colors right). */

  if (remap[me->p_team] != myTeam)
    {
      myTeam = remap[me->p_team];

      for (pos = plistPos + MAXPLAYER - 1; pos >= plistPos; --pos)
	*pos = -1;
    }


  /* If partitionPlist is false, reset the blank line markers */

  if (!partitionPlist)
    {
      blankLine = -1;
      blankLine2 = -1;
    }


  /* Count the number of players in each team. */

  for (i = NUMTEAM; i >= 0; --i)
    teamPos[i] = 0;

  for (current = players + MAXPLAYER - 1; current >= players; --current)
    {
      if (current->p_status != PFREE)
	++teamPos[remap[current->p_team]];
    }


  /* Find the row after the last player in each team. */

  last = 1;					 /* The title is line zero */

  if (sortMyTeamFirst)				 /* My team comes at the top */
    {
      last += teamPos[myTeam];
      teamPos[myTeam] = last;

      if (partitionPlist)
	{
	  if (blankLine != last)
	    {
	      blankLine = last;
	      W_ClearArea(playerw, 0, last, plistWidth, 1);
	    }

	  ++last;
	}
    }

  for (i = 1; i <= NUMTEAM; ++i)
    {
      if (i != myTeam)
	{
	  last += teamPos[i];
	  teamPos[i] = last;
	}
    }

  if (!sortMyTeamFirst)				 /* My team comes below the * 
						  * others */
    {
      if (partitionPlist)
	{
	  if (blankLine != last)
	    {
	      blankLine = last;
	      W_ClearArea(playerw, 0, last, plistWidth, 1);
	    }

	  ++last;
	}

      last += teamPos[myTeam];
      teamPos[myTeam] = last;
    }

  if (myTeam != NOBODY)
    {
      /* Separate the goodies from the arriving players. */

      if (partitionPlist)
	{
	  if (blankLine2 != last)
	    {
	      blankLine2 = last;
	      W_ClearArea(playerw, 0, last, plistWidth, 1);
	    }

	  ++last;
	}

      last += teamPos[NOBODY];
      teamPos[NOBODY] = last;
    }


  /* Clear some lines if people have left. */

  for (row = last; row < plistLastRow; ++row)
    W_ClearArea(playerw, 0, row, plistWidth, 1);

  plistLastRow = last;



  /* Write out each player that has either changed position or has * new *
   * stats. */

  for (i = MAXPLAYER - 1, current = players + MAXPLAYER - 1;
       i >= 0;
       --i, --current)
    {
      if (current->p_status == PFREE)
	{
	  updatePlayer[i] = FALSE;
	  continue;
	}

      row = --(teamPos[remap[current->p_team]]);

      if ((!updatePlayer[i]) && plistPos[i] == row)
	continue;

      plistPos[i] = row;
      updatePlayer[i] = FALSE;

      PlistLine(current, row);
    }

}


static void WriteUnsortedPlist(void)
/*
 *  Update the order of the players in the list and write out any
 *  changes.
 */
{
  int     count;
  int     pos;
  char   *update;
  static int myTeam = -1;


  /* 
   *  If I have changed team, redraw everything (to get the colors
   *  right).
   */

  if (remap[me->p_team] != myTeam)
    {
      myTeam = remap[me->p_team];

      for (update = updatePlayer + MAXPLAYER
	   ; update >= updatePlayer
	   ; --update)
	{
	  *update = TRUE;
	}
    }


  update = updatePlayer - 1;

  for (;;)
    {
      /* Find the changed player as quickly as possible. */

      do
	++update;
      while (!(*update));


      /* Is this a valid player?  Remember updatePlayer[MAXPLAYER] * is *
       * always TRUE to make the above loop more efficient.       */

      count = update - updatePlayer;
      if (count == MAXPLAYER)
	break;


      /* Update the player. */

      *update = FALSE;
      pos = count + 1;
      plistPos[count] = pos;

      if (players[count].p_status != PFREE)
	PlistLine(players + count, pos);
      else
	W_ClearArea(playerw, 0, pos, plistWidth, 1);
    }
}



static int PlistHeader(char *layout, int doWrite)
/* Analyse the heading (field names) for a player list, and set the
 * plistHasSpeed and plistHasHostile flags.
 * 
 * If doWrite is TRUE, write the heading to the list.
 * 
 * RETURN the width of the player list. */
{
  char    header[BUFSIZ];
  int     num = 0;

#ifdef PLIST2
  plistHasSpeed = FALSE;
  plistHasHostile = FALSE;
#endif /* PLIST2 */

  for (; IsNotZero(*layout); ++layout)
    {
      if (num + MaxPlistField >= BUFSIZ)
	{
	  /* Assume that we have tested the standard layouts so that only * * 
	   * custom layouts can be too long. *  * If a standard layout is *
	   * found to be too long then some compiler's * code will dump core
	   * * here because of an attempt to write over a * constant string. */

	  fprintf(stderr, "Playerlist truncated to fit buffer.\n");
	  layout = '\0';
	  break;
	}

      switch (*layout)
	{
	case 'n':				 /* Ship Number */
	  STRNCPY(&header[num], " No", 3);
	  num += 3;
	  break;
	case 'T':				 /* Ship Type */
	  STRNCPY(&header[num], " Ty", 3);
	  num += 3;
	  break;
	case 'C':				 /* Curt (short) Rank */
	  STRNCPY(&header[num], " Rank", 5);
	  num += 5;
	  break;
	case 'R':				 /* Rank */
	  STRNCPY(&header[num], " Rank      ", 11);
	  num += 11;
	  break;
	case 'N':				 /* Name */
	  STRNCPY(&header[num], " Name            ", 17);
	  num += 17;
	  break;
	case 'K':				 /* Kills */
	  STRNCPY(&header[num], " Kills", 6);
	  num += 6;
	  break;
	case 'l':				 /* Login Name */
	  STRNCPY(&header[num], " Login           ", 17);
	  num += 17;
	  break;
	case 'O':				 /* Offense */
	  STRNCPY(&header[num], " Offse", 6);
	  num += 6;
	  break;
	case 'W':				 /* Wins */
	  STRNCPY(&header[num], "  Wins", 6);
	  num += 6;
	  break;
	case 'D':				 /* Defense */
	  STRNCPY(&header[num], " Defse", 6);
	  num += 6;
	  break;
	case 'L':				 /* Losses */
	  STRNCPY(&header[num], "  Loss", 6);
	  num += 6;
	  break;
	case 'S':				 /* Total Rating (stats) */
	  STRNCPY(&header[num], " Stats", 6);
	  num += 6;
	  break;
	case 'r':				 /* Ratio */
	  STRNCPY(&header[num], " Ratio", 6);
	  num += 6;
	  break;
	case 'd':				 /* Damage Inflicted (DI) */
	  STRNCPY(&header[num], "      DI", 8);
	  num += 8;
	  break;
	case ' ':				 /* White Space */
	  header[num] = ' ';
	  num += 1;
	  break;

#ifdef PLIST1
	case 'B':				 /* Bombing */
	  STRNCPY(&header[num], " Bmbng", 6);
	  num += 6;
	  break;
	case 'b':				 /* Armies Bombed */
	  STRNCPY(&header[num], " Bmbed", 6);
	  num += 6;
	  break;
	case 'P':				 /* Planets */
	  STRNCPY(&header[num], " Plnts", 6);
	  num += 6;
	  break;
	case 'p':				 /* Planets Taken */
	  STRNCPY(&header[num], " Plnts", 6);
	  num += 6;
	  break;
	case 'M':				 /* Display, Host Machine */
	  STRNCPY(&header[num], " Host Machine    ", 17);
	  num += 17;
	  break;
	case 'H':				 /* Hours Played */
	  STRNCPY(&header[num], " Hours ", 7);
	  num += 7;
	  break;
	case 'k':				 /* Max Kills */
	  STRNCPY(&header[num], " Max K", 6);
	  num += 6;
	  break;
	case 'V':				 /* Kills per hour */
	  STRNCPY(&header[num], "   KPH", 6);
	  num += 6;
	  break;
	case 'v':				 /* Deaths per hour */
	  STRNCPY(&header[num], "   DPH", 6);
	  num += 6;
	  break;
#endif

#ifdef PLIST2
	case 'w':				 /* War staus */
	  STRNCPY(&header[num], " War Stat", 9);
	  num += 9;
	  plistHasHostile = TRUE;
	  break;
	case 's':				 /* Speed */
	  STRNCPY(&header[num], " Sp", 3);
	  num += 3;
	  plistHasSpeed = TRUE;
	  break;
#endif

	default:
	  fprintf(stderr,
		  "%c is not an option for the playerlist\n", *layout);
	  break;
	}
    }

  header[num] = '\0';

  if (doWrite)
    W_WriteText(playerw, 0, 0, textColor, header, num, W_RegularFont);

  return num;
}


static void PlistLine(struct player *j, int pos)
/* Write the player list entry for player `j' on line `pos'. */
{
  char    buf[BUFSIZ];
  char   *ptr;
  char   *buffPoint;
  int     kills, losses, my_ticks;
  float   pRating, oRating, dRating, bRating, Ratings;
  float   KillsPerHour, LossesPerHour;		 /* Added 12/27/93 ATH */
  double  ratio, max_kills;

  if (j->p_ship.s_type == STARBASE)
    {
      kills = j->p_stats.st_sbkills;
      losses = j->p_stats.st_sblosses;
      max_kills = j->p_stats.st_sbmaxkills;

      if (SBhours)
	my_ticks = j->p_stats.st_sbticks;
      else
	my_ticks = j->p_stats.st_tticks;

      KillsPerHour = (float) (my_ticks == 0) ? 0.0 :
	  (float) kills *36000.0 / (float) my_ticks;

      LossesPerHour = (float) (my_ticks == 0) ? 0.0 :
	  (float) losses *36000.0 / (float) my_ticks;
    }
  else
    {
      kills = j->p_stats.st_kills + j->p_stats.st_tkills;
      losses = j->p_stats.st_losses + j->p_stats.st_tlosses;
      max_kills = j->p_stats.st_maxkills;
      my_ticks = j->p_stats.st_tticks;
      KillsPerHour = (float) (my_ticks == 0) ? 0.0 :
	  (float) j->p_stats.st_tkills * 36000.0 / (float) my_ticks;
      LossesPerHour = (float) (my_ticks == 0) ? 0.0 :
	  (float) j->p_stats.st_tlosses * 36000.0 / (float) my_ticks;
    }

  if (losses == 0)
    ratio = (double) kills;
  else
    ratio = (double) kills / (double) losses;

  if (!j->p_stats.st_tticks)
    {
      oRating = pRating = bRating = Ratings = 0.0;
      dRating = defenseRating(j);
    }
  else
    {
      oRating = offenseRating(j);
      pRating = planetRating(j);
      bRating = bombingRating(j);
      Ratings = oRating + pRating + bRating;
      dRating = defenseRating(j);
      if ((j->p_ship.s_type == STARBASE) && (SBhours))
	{					 /* If SB, show KPH for * *
						  * offense etc. */
	  oRating = KillsPerHour;
	  dRating = LossesPerHour;
	}
    }


  buffPoint = buf;

  for (ptr = plistLayout; IsNotZero(*ptr); ++ptr)
    {
      *(buffPoint++) = ' ';

      switch (*ptr)
	{
	case 'n':				 /* Ship Number */
	  if (j->p_status != PALIVE)
	    {
	      *(buffPoint++) = ' ';
	      *(buffPoint++) = shipnos[j->p_no];
	    }
	  else
	    {
	      *(buffPoint++) = teamlet[j->p_team];
	      *(buffPoint++) = shipnos[j->p_no];
	    }

	  break;

	case 'T':				 /* Ship Type */
	  if (j->p_status != PALIVE)
	    {
	      *(buffPoint++) = ' ';
	      *(buffPoint++) = ' ';
	    }
	  else
	    {
	      *(buffPoint++) = my_classes[j->p_ship.s_type][0];
	      *(buffPoint++) = my_classes[j->p_ship.s_type][1];
	    }

	  break;

	case 'C':				 /* Curt (short) Rank */
	  format(buffPoint, ranks[j->p_stats.st_rank].cname, 4, 0);
	  buffPoint += 4;
	  break;

	case 'R':				 /* Rank */
	  format(buffPoint, ranks[j->p_stats.st_rank].name, 10, 0);
	  buffPoint += 10;
	  break;

	case 'N':				 /* Name */
	  format(buffPoint, j->p_name, 16, 0);
	  buffPoint += 16;
	  break;

	case 'K':				 /* Kills */
	  if (j->p_kills > 100.0)
	    /* Cheat a bit */
	    ftoa(j->p_kills, buffPoint - 1, 0, 3, 2);
	  else
	    ftoa(j->p_kills, buffPoint, 0, 2, 2);
	  buffPoint += 5;
	  break;

	case 'l':				 /* Login Name */
	  format(buffPoint, j->p_login, 16, 0);
	  buffPoint += 16;
	  break;

	case 'O':				 /* Offense */
	  ftoa(oRating, buffPoint, 0, 2, 2);
	  buffPoint += 5;
	  break;

	case 'W':				 /* Wins */
	  itoapad(kills, buffPoint, 0, 5);
	  buffPoint += 5;
	  break;

	case 'D':				 /* Defense */
	  ftoa(dRating, buffPoint, 0, 2, 2);
	  buffPoint += 5;
	  break;

	case 'L':				 /* Losses */
	  itoapad(losses, buffPoint, 0, 5);
	  buffPoint += 5;
	  break;

	case 'S':				 /* Total Rating (stats) */
	  ftoa(Ratings, buffPoint, 0, 2, 2);
	  buffPoint += 5;
	  break;

	case 'r':				 /* Ratio */
	  ftoa(ratio, buffPoint, 0, 2, 2);
	  buffPoint += 5;
	  break;

	case 'd':				 /* Damage Inflicted (DI) */
	  ftoa(Ratings * (j->p_stats.st_tticks / 36000.0),
	       buffPoint, 0, 4, 2);
	  buffPoint += 7;
	  break;

	case ' ':				 /* White Space */
	  break;

#ifdef PLIST1
	case 'B':				 /* Bombing */
	  ftoa(bRating, buffPoint, 0, 2, 2);
	  buffPoint += 5;
	  break;

	case 'b':				 /* Armies Bombed */
	  itoapad(j->p_stats.st_tarmsbomb + j->p_stats.st_armsbomb,
		  buffPoint, 0, 5);
	  buffPoint += 5;
	  break;

	case 'P':				 /* Planets */
	  ftoa(pRating, buffPoint, 0, 2, 2);
	  buffPoint += 5;
	  break;

	case 'p':				 /* Planets Taken */
	  itoapad(j->p_stats.st_tplanets + j->p_stats.st_planets,
		  buffPoint, 0, 5);
	  buffPoint += 5;
	  break;

	case 'M':				 /* Display, Host Machine */
	  format(buffPoint, j->p_monitor, 16, 0);
	  buffPoint += 16;
	  break;

	case 'H':				 /* Hours Played */
	  ftoa(my_ticks / 36000.0, buffPoint, 0, 3, 2);
	  buffPoint += 6;
	  break;

	case 'k':				 /* Max Kills  */
	  ftoa(max_kills, buffPoint, 0, 2, 2);
	  buffPoint += 5;
	  break;

	case 'V':				 /* Kills Per Hour  */
	  ftoa(KillsPerHour, buffPoint, 0, 3, 1);
	  buffPoint += 5;
	  break;

	case 'v':				 /* Deaths Per Hour  */
	  ftoa(LossesPerHour, buffPoint, 0, 3, 1);
	  buffPoint += 5;
	  break;
#endif

#ifdef PLIST2
	case 'w':				 /* War staus */
	  if (j->p_swar & me->p_team)
	    STRNCPY(buffPoint, "WAR     ", 8);
	  else if (j->p_hostile & me->p_team)
	    STRNCPY(buffPoint, "HOSTILE ", 8);
	  else
	    STRNCPY(buffPoint, "PEACEFUL", 8);

	  buffPoint += 8;
	  break;

	case 's':				 /* Speed */
	  itoapad(j->p_speed, buffPoint, 0, 2);
	  buffPoint += 2;
	  break;
#endif

	default:
	  break;
	}
    }

  *buffPoint = '\0';
  W_WriteText(playerw, 0, pos, playerColor(j),
	      buf, buffPoint - buf, shipFont(j));
}
