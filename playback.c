/* PlayBack.c
 *
 * Kevin O'Connor 9/22/97
 *
 * Routines neccessary to playback a game recording.
 *
 * $Log: playback.c,v $
 * Revision 1.3  1999/08/05 16:46:32  siegl
 * remove several defines (BRMH, RABBITEARS, NEWDASHBOARD2)
 *
 * Revision 1.2  1999/06/11 16:14:17  siegl
 * cambot replay patches
 *
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 * */
#include <setjmp.h>
#include "config.h"

#include INC_MACHINE_ENDIAN

#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include INC_SYS_TIME
#include INC_SYS_WAIT
#include INC_SYS_RESOURCE
#include INC_SYS_SELECT
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <pwd.h>
#include <string.h>
#include <ctype.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"
#include "version.h"
#include "patchlevel.h"
#include "cowapi.h"
#include "map.h"
#include "spopt.h"
#include "defs.h"
#include "playerlist.h"

#ifdef RECORDGAME
extern int opened_info;				 /* counter for infowin *

						  * 
						  * * popup, 6/1/93 LAB */
#define RETURNBASE 10

extern jmp_buf env;

int     pbdelay = 200000;

struct player dummyme;

struct player *packetsme;
struct player *displayme;

/* We want reverse-playback!!! */
#define REVERSE_PLAYBACK

/* Forward declarations for reverse playback */
void rpb_init(void);
void rpb_analyze(int diskpos, void *packet);
void rpb_dorev(char *buf);

struct player *packetsme;
struct player *displayme;

/* We want reverse-playback!!! */
#define REVERSE_PLAYBACK

/* Forward declarations for reverse playback */
void rpb_init(void);
void rpb_analyze(int diskpos, void *packet);
void rpb_dorev(char *buf);

int
        pbmain(char *name)
{
  int     i;
  int     s_type;

#ifdef REVERSE_PLAYBACK
  rpb_init();
#endif

#ifdef REVERSE_PLAYBACK
  rpb_init();
#endif

  playback = PL_FORWARD;
  pseudo[0] = defpasswd[0] = '\0';

  i = setjmp(env);				 /* Error while initializing */
  if (i >= RETURNBASE)
    return (i - RETURNBASE);			 /* Terminate with retcode */

  for (i = 0; i < 80; i++)
    {
      outmessage[i] = '\0';
    }

  SRANDOM(time(0));

  initDefaults(deffile);

  SRANDOM(getpid() * time((LONG *) 0));

  newwin(display_host, name);

  resetdefaults();
  if (censorMessages)
    initCensoring();

  /* open memory...? */
  openmem();

  recordFile = fopen(recordFileName, "rb");
  if (recordFile == NULL)
    {
      perror(recordFileName);
      return (1);

    }

  me = &dummyme;
  myship = &(me->p_ship);
  mystats = &(me->p_stats);

  me->p_x = me->p_y = 50000;
  getship(myship, CRUISER);
  shipchange(CRUISER);
  displayme = me;
  packetsme = me;

  /* Get first packet from file */
  readFromFile();

  displayme = packetsme;

  lastm = mctl->mc_current;

  mapAll();

  (void) SIGNAL(SIGINT, SIG_IGN);

  /* Instructions from getname() */
  MZERO(mystats, sizeof(struct stats));

  mystats->st_tticks = 1;
  for (i = 0; i < 95; i++)
    {
      mystats->st_keymap[i] = i + 32;
      mystats->st_keymap[i + 96] = i + 32 + 96;

#ifdef MOUSE_AS_SHIFT
      mystats->st_keymap[i + 192] = i + 32;
      mystats->st_keymap[i + 288] = i + 32;
      mystats->st_keymap[i + 384] = i + 32;
#endif
    }
  mystats->st_keymap[95] = 0;
  mystats->st_flags = ST_MAPMODE + ST_NAMEMODE + ST_SHOWSHIELDS +
      ST_KEEPPEACE + ST_SHOWLOCAL * 2 + ST_SHOWGLOBAL * 2;

  /* End getname() */

  phaserWindow = booleanDefault("phaserWindow", phaserWindow);


#ifdef AUTOKEY
  /* autokey.c */
  autoKeyDefaults();
#endif /* AUTOKEY */

  initkeymap();

  /* Set p_hostile to hostile, so if keeppeace is on, the guy starts off * *
   * hating everyone (like a good fighter should) */
  me->p_hostile = (FED | ROM | KLI | ORI);

  if (!newDashboard)
    {
      char    buf[128];

      sprintf(buf,
	   "Maximum:      %2d  %3d %3d               %3d   %6d   %3d   %3d",
	      0, 0, 0, 0, 0, 0, 0);
      W_WriteText(tstatw, 50, 27, textColor, buf, strlen(buf), W_RegularFont);
    }

#ifdef AUTOKEY
  if (autoKey)
    {
      /* XX: changes entire state of display */
      W_AutoRepeatOff();
    }
#endif

#ifdef SOUND
  Init_Sound();
#endif

  i = setjmp(env);				 /* Reentry point of game */
  if (i >= RETURNBASE)
    return (i - RETURNBASE);			 /* Terminate with retcode */

#ifdef SOUND
  Abort_Sound(ENGINE_SOUND);
#endif

#ifdef nodef
  /* Code from entrywindow() */
  {
    run_clock(time(0));

    if (remap[me->p_team] == NOBODY)
      RedrawPlayerList();			 /* When you first login */
    else
      UpdatePlayerList();			 /* Otherwise */

  }
  /* End entrywindow() */
#endif

  redrawall = 2;

  getship(myship, myship->s_type);
  shipchange(myship->s_type);
  enter();
  calibrate_stats();
  W_ClearWindow(w);

  me->p_status = PALIVE;			 /* Put player in game */
  me->p_ghostbuster = 0;
  PlistNoteUpdate(me->p_no);

  if (showStats)				 /* Default showstats are on. 
						  * 
						  */
    W_MapWindow(statwin);

  if (W_IsMapped(lMeter))
    redrawLMeter();

  if (W_IsMapped(pStats))			 /* support ping stuff */
    redrawPStats();

#ifdef SOUND
  Play_Sound(ENTER_SHIP_SOUND);
  Play_Sound(ENGINE_SOUND);
#endif

#ifdef HOCKEY_LINES
  init_hockey_lines();
#endif

  while (1)
    {

#ifdef nodef
      fd_set  readfds;
      struct timeval timeout;
      int     xsock = W_Socket();

      timeout.tv_sec = 0;
      timeout.tv_usec = 0;
#endif

      if (keepInfo > 0 && opened_info != -2 &&	 /* 6/1/93 LAB */
	  opened_info < 0 && infomapped)
	destroyInfo();

      if (keepInfo > 0 && opened_info != -2)
	opened_info--;

      if (W_EventsQueuedCk())
	{
	  process_event();
	  /* W_Flush(); */
	}

      intrupt();
      W_Flush();
      usleep(pbdelay);
    }
}

void
        pbsetspeed(char key)
{
  if (playback == PL_PAUSE)
    playback = PL_FORWARD;
  switch (key)
    {
    case '0':
      playback = PL_PAUSE;
      break;
    case '1':
      pbdelay = 800000;
      break;
    case '2':
      pbdelay = 400000;
      break;
    case '3':
      pbdelay = 200000;
      break;
    case '4':
      pbdelay = 100000;
      break;
    case '5':
      pbdelay = 50000;
      break;
    case '6':
      pbdelay = 25000;
      break;
    case '7':
      pbdelay = 12500;
      break;
    case '8':
      pbdelay = 6250;
      break;
    case '9':
    case '#':
    case '!':
    case '@':
    case '%':
      pbdelay = 0;
      break;
    case '<':
      pbdelay /= 2;
      break;
    case '>':
      pbdelay *= 2;
      break;
    case 'R':
      {
#ifdef REVERSE_PLAYBACK
	rpb_init();
#endif
  	fseek (recordFile, 0, SEEK_SET);
	playback = PL_FORWARD;
	break;
      }
    case '(':
      playback = PL_REVERSE;
      break;
    case ')':
      playback = PL_FORWARD;
      break;
    }
}

void
        pblockplayer(int who)
{
   displayme = &players[who];
}

void
        pblockplanet(int pl)
{
   displayme = &dummyme;
   displayme->p_x = planets[pl].pl_x;
   displayme->p_y = planets[pl].pl_y;
}

inline int
        ckRecordPacket(char packet)
{
  switch (packet)
    {
    case SP_MESSAGE:
    case SP_S_MESSAGE:
    case SP_S_WARNING:
    case SP_WARNING:
      /* SP_MOTD */

    case SP_PLAYER_INFO:
    case SP_PLAYER:
    case SP_S_PLAYER:
    case SP_KILLS:
    case SP_S_KILLS:

    case SP_TORP_INFO:
    case SP_TORP:
    case SP_S_TORP:
    case SP_S_TORP_INFO:
    case SP_S_8_TORP:
    case SP_PLASMA_INFO:
    case SP_PLASMA:

    case SP_PHASER:
    case SP_S_PHASER:

    case SP_YOU:
    case SP_S_YOU:
    case SP_S_YOU_SS:

    case SP_STATUS:

    case SP_PLANET:
    case SP_S_PLANET:
    case SP_PLANET_LOC:

    case SP_FLAGS:
      /*    case SP_MASK: */
    case SP_PSTATUS:
    case SP_HOSTILE:
    case SP_STATS:
    case SP_S_STATS:
    case SP_PL_LOGIN:
    case SP_SHIP_CAP:


    case SP_SEQUENCE:
    case SP_SC_SEQUENCE:
    case SP_S_SEQUENCE:

      return 1;
    }
  return 0;
}

extern struct packet_handler handlers[];

int
pb_dopacket(char *buf)
{
  int size, count;

  count = fread(buf, 1, 4, recordFile);
  if (count < 4)
    {
      return 1;
    }

  size = handlers[buf[0]].size;
  if (size == -1)
    {
      if (buf[0] == SP_S_MESSAGE)
	{
	  /* UGH.  SP_S_MESSAGE needs next word to calculate size */
	  count += fread(buf+count, 1, 4, recordFile);
	  if (count < 8)
	    {
	      return 1;
	    }
	}
      size = getvpsize(buf);
    }
  if (size > count)
    count += fread(buf+count, 1, size-count, recordFile);
  if (debug)
    printf("Reading packet %d\n", buf[0]);
  if (count < size)
    {
      return 1;
    }

  (*(handlers[buf[0]].handler)) (buf
#ifdef CORRUPTED_PACKETS
				 ,recordFile
#endif
				 );
  return 0;
}

int
readFromFile()
{
#define MAXPACKETSIZE 128
  static uint aligned_buf[MAXPACKETSIZE/sizeof(uint)];
  char *buf = (char *) &aligned_buf;
  int diskpos;

  if (playback == PL_PAUSE)
    return 0;
  else if (playback == PL_FORWARD)
    while (1)
      {
	diskpos = ftell(recordFile);
	me = packetsme;
 
	if (pb_dopacket(buf))
	  {
 	    playback = PL_PAUSE;
 
 	    packetsme = me;
 	    me = displayme;
 
 	    return 0;
 	  }
#ifdef REVERSE_PLAYBACK
 	rpb_analyze(diskpos, buf);
#endif
 
	packetsme = me;
 	me = displayme;
 
 	if (buf[0] == SP_SEQUENCE || buf[0] == SP_SC_SEQUENCE
 	    || buf[0] == SP_S_SEQUENCE)
 	  {
 	    return 1;
 	  }
       }
#ifdef REVERSE_PLAYBACK
   else if (playback == PL_REVERSE)
    {
      diskpos = ftell(recordFile);
      me = packetsme;

      rpb_dorev(buf);

      packetsme = me;
      me = displayme;
  
      return 1;
    }
#endif
  playback = PL_PAUSE;
  return 0;
}
  

#ifdef REVERSE_PLAYBACK

/**************************** Reverse playback ***************************/
/**************************** Reverse playback ***************************/
/**************************** Reverse playback ***************************/

/* Reverse-Playback -
 *  Reverse playback does just what it implies.  It allows the client to
 * play a game recording in reverse.  It is a nifty feature useful
 * for analyzing a game.  With forward only playback, if a desired point
 * is played over, the only way to see that point again would be to reset
 * the playback to the very beginning and forward-play the recording until
 * the point is re-located.  With a long recording this quickly becomes
 * infeasible.  With reverse recording one can simply forward and reverse
 * over interesting points.
 *
 * That said, no sane person would have implemented this without
 * getting paid for it...
 *
 * Attempting to reset the netrek state to a position in the past is
 * a considerable challenge when the input is a dynamic stream.
 * I only implemented it because I thought it was an interesting challenge..
 * Ok, so I dont document my code very well - I'm not getting paid - so
 * here is a brief run-down of how I accomplished this task.
 *
 * First step is to analyze all incoming packets.  Each packet is then
 * flagged with a unique handler for each piece of unique data the server
 * sent.  (IE. the SP_HOSTILE packet contains RPB_PLAYER_HOSTILE data, while
 * SP_YOU contains RPB_PLAYER_HOSTILE, RPB_PLAYER_FUEL, RPB_PLAYER_ARMIES,
 * and RPB_PLAYER_FLAGS.  SP_TORP contains 1 RPB_TORP_POSITION data, while
 * SP_S_TORP contains 8 RPB_TORP_POSITIONs)
 *
 * The next step is to build a table from all these RPB_ server information
 * handles.  The key to this table is the ability to find the previous
 * packet that contained any given RPB_ server information, and be able
 * to backtrack through all the packets that the server ever sent that also
 * updated that RPB_ data.  In addition to being able to access the table
 * via RPB_ keys, it must also be accessed via relative disk
 * positions.  The table is in essence a series of multiply linked
 * linked-lists.
 * (I implemented the table using a series of singly linked lists that
 * are contained within a dynamicly resizing array of linked-list nodes.)
 *
 * The bad news is, the table uses alot of memory.  There is no way
 * around this.
 * The good news is, any virtual OS should be able to handle this with
 * no problems.  (Only recently used items are referenced within the
 * array, thus the OS can swap out earlier pages of the array.)
 *
 * Once it comes time to reverse playback, a frame is chosen to be replayed.
 * The program then finds all the packets that contributed to that
 * frame's information, and one by one re-reads them.  As long as
 * there are RPB_ handles for each piece of data the server can send,
 * reconstructing the frame is trivial.
 *
 * Notes:  This doesn't work for messages..  There is lots of room
 *  for improvement..  Reverse playback is pretty slow.. (but there
 *  is no need to reverse only 1 update at a time..)  I'm pretty
 *  sure this will start to exhibit 'weird' effects in high packet loss..
 *		-Kevin O'Connor 03/17/98
 */


/* List of integer handles.
 * These handles represent each piece of information the server
 * can send within a packet
 */
#define RPB_NORMAL_SEQUENCE 0
#define RPB_NORMAL_STATUS 1
#define RPB_NORMAL_MAX 2

#define RPB_PLAYER_OFFSET (RPB_NORMAL_MAX)
#define RPB_PLAYER_INFO 0
#define RPB_PLAYER_LOGIN 1
#define RPB_PLAYER_HOSTILE 2
#define RPB_PLAYER_STATS 3
#define RPB_PLAYER_FLAGS 4
#define RPB_PLAYER_KILLS 5
#define RPB_PLAYER_POSITION 6
#define RPB_PLAYER_FUEL 7
#define RPB_PLAYER_ARMIES 8
#define RPB_PLAYER_STATUS 9
#define RPB_PLAYER_PHASER 10
#define RPB_PLAYER_MAX 11

#define RPB_PLASMA_OFFSET (RPB_PLAYER_OFFSET + RPB_PLAYER_MAX*MAXPLAYER)
#define RPB_PLASMA_POSITION 0
#define RPB_PLASMA_INFO 1
#define RPB_PLASMA_MAX 2

#define RPB_TORP_OFFSET (RPB_PLASMA_OFFSET + RPB_PLASMA_MAX*MAXPLASMA*MAXPLAYER)
#define RPB_TORP_POSITION 0
#define RPB_TORP_INFO 1
#define RPB_TORP_MAX 2

#define RPB_PLANET_OFFSET (RPB_TORP_OFFSET + RPB_TORP_MAX*MAXTORP*MAXPLAYER)
#define RPB_PLANET_INFO 0
#define RPB_PLANET_POSITION 1
#define RPB_PLANET_MAX 2

#define RPB_TOTAL (RPB_PLANET_OFFSET + RPB_PLANET_MAX*MAXPLANETS)

struct LocPacketInfo {
  int prev;
  uint diskpos;
};

struct LocPacketInfo *header = NULL;
int lastPos[RPB_TOTAL];
int max_alloc = 0;
int current;

/* Number of LocPacketInfo to re-allocate each time an expansion
 * is required.  This should be on the edge of a page boundry to reduce
 * system overhead. */
#define EXPANDCOUNT 512

/* Initialize the data structures used in reverse playback */
void
rpb_init(void)
{
  int i;
  for (i=0; i<RPB_TOTAL; i++)
    lastPos[i] = -1;
  current = 0;
}

/* Add a server information handler to the current table */
void
rpb_insert(int diskpos, int hdl)
{
  if (current >= max_alloc)
    {
      /* Expand table size */
      max_alloc += EXPANDCOUNT;
      header = realloc(header, max_alloc * sizeof(struct LocPacketInfo));
    }
  header[current].prev = lastPos[hdl];
  header[current].diskpos = diskpos;
  lastPos[hdl] = current;
  current++;
}

/* Given a packet, generate a set of integer handles for the data
 * contained within the packet, and then add them to the current table */
void
rpb_analyze(int diskpos, void *packet)
{
  int id;

  switch (*(char *) packet)
    {
    case SP_STATUS:
      rpb_insert(diskpos, RPB_NORMAL_STATUS);
      break;
    case SP_SEQUENCE:
    case SP_S_SEQUENCE:
    case SP_SC_SEQUENCE:
      rpb_insert(diskpos, RPB_NORMAL_SEQUENCE);
      break;

    case SP_PLAYER_INFO:
      id = ((struct plyr_info_spacket *)packet)->pnum;
      rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
	+ RPB_PLAYER_INFO);
      break;
    case SP_PL_LOGIN:
      id = ((struct plyr_login_spacket *)packet)->pnum;
      rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
	+ RPB_PLAYER_LOGIN);
      break;
    case SP_HOSTILE:
      id = ((struct hostile_spacket *)packet)->pnum;
      rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
	+ RPB_PLAYER_HOSTILE);
      break;
    case SP_STATS:
      id = ((struct stats_spacket *)packet)->pnum;
      rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
	+ RPB_PLAYER_STATS);
      break;
    case SP_FLAGS:
      id = ((struct flags_spacket *)packet)->pnum;
      rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
	+ RPB_PLAYER_FLAGS);
      break;
    case SP_KILLS:
      id = ((struct kills_spacket *)packet)->pnum;
      rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
	+ RPB_PLAYER_KILLS);
      break;
    case SP_PLAYER:
      id = ((struct player_spacket *)packet)->pnum;
      rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
	+ RPB_PLAYER_POSITION);
      break;
    case SP_YOU:
      id = ((struct you_spacket *)packet)->pnum;
      rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
	+ RPB_PLAYER_HOSTILE);
      rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
	+ RPB_PLAYER_FLAGS);
      rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
	+ RPB_PLAYER_FUEL);
      rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
	+ RPB_PLAYER_ARMIES);
      break;
    case SP_PSTATUS:
      id = ((struct pstatus_spacket *)packet)->pnum;
      rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
	+ RPB_PLAYER_STATUS);
      break;
    case SP_S_YOU:
       id = ((struct youshort_spacket *)packet)->pnum;
       rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
 	+ RPB_PLAYER_HOSTILE);
       rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
 	+ RPB_PLAYER_FLAGS);
       rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
	+ RPB_PLAYER_ARMIES);
      break;
    case SP_S_YOU_SS:
      if (F_many_self)
	id = ((struct youss_spacket *)packet)->pad1;
      else
	id = me->p_no;
      rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
	+ RPB_PLAYER_FUEL);
      break;
    case SP_S_KILLS:
      {
	int i, numofkills;

	numofkills = ((unsigned char *) packet)[1];

	for (i = 0; i < numofkills; i++)
	  {
	    id = ((unsigned char *) packet)[i*2+3] >> 2;
 	    rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
 		       + RPB_PLAYER_KILLS);
 	  }
      }
      break;
    case SP_S_PLAYER:
      {
	int i, numofplayers, offset;
 	unsigned char *sbuf = (unsigned char *) packet;
 
 	numofplayers = sbuf[1] & 0x3f;
 
 	if (sbuf[1] & 128)
 	  {
 	    offset = 32;
 	    sbuf += 4;
 	  }
 	else if (sbuf[1] & 64)
 	  {
 	    offset = 0;
 	    if (shortversion == SHORTVERSION)
 	      if (sbuf[2] == 2)
 		sbuf += 8;
 	      else if (sbuf[2] == 1)
 		sbuf += 4;
 	  }
 	else
 	  {
 	    offset = 0;
 	    sbuf += 12;
 	    id = me->p_no;
 	    rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
 		       + RPB_PLAYER_POSITION);
 	  }
 
 	for (i = 0; i < numofplayers; i++)
 	  {
 	    id = (sbuf[i*4] & 0x1f) + offset;
 	    rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
 		       + RPB_PLAYER_POSITION);
 	  }
       }
       break;
     case SP_S_STATS:
       id = ((struct stats_s_spacket *)packet)->pnum;
       rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
 	+ RPB_PLAYER_STATS);
       break;
     case SP_PHASER:
       id = ((struct phaser_spacket *)packet)->pnum;
       rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
 		 + RPB_PLAYER_PHASER);
       break;
     case SP_S_PHASER:
       id = ((struct phaser_s_spacket *)packet)->pnum & 0x3f;
       rpb_insert(diskpos, RPB_PLAYER_OFFSET + id * RPB_PLAYER_MAX
 		 + RPB_PLAYER_PHASER);
       break;
 
     case SP_PLASMA_INFO:
       id = ntohs(((struct plasma_info_spacket *)packet)->pnum);
       rpb_insert(diskpos, RPB_PLASMA_OFFSET + id * RPB_PLASMA_MAX
 	+ RPB_PLASMA_INFO);
       break;
     case SP_PLASMA:
       id = ntohs(((struct plasma_spacket *)packet)->pnum);
       rpb_insert(diskpos, RPB_PLASMA_OFFSET + id * RPB_PLASMA_MAX
 	+ RPB_PLASMA_POSITION);
       break;
 
     case SP_TORP_INFO:
       id = ntohs(((struct torp_info_spacket *)packet)->tnum);
       rpb_insert(diskpos, RPB_TORP_OFFSET + id * RPB_TORP_MAX
 	+ RPB_TORP_INFO);
       break;
     case SP_TORP:
       id = ntohs(((struct torp_spacket *)packet)->tnum);
       rpb_insert(diskpos, RPB_TORP_OFFSET + id * RPB_TORP_MAX
 	+ RPB_TORP_POSITION);
       break;
     case SP_S_8_TORP:
     case SP_S_TORP:
     case SP_S_TORP_INFO:
       {
 	int i;
 
 	if (*(unsigned char *)packet == SP_S_8_TORP)
 	  id = ((unsigned char *)packet)[1] * 8;
 	else
 	  id = ((unsigned char *)packet)[2] * 8;
 	for (i=0; i<8; i++)
 	  {
 	    rpb_insert(diskpos, RPB_TORP_OFFSET + (id+i) * RPB_TORP_MAX
 		       + RPB_TORP_INFO);
 	    rpb_insert(diskpos, RPB_TORP_OFFSET + (id+i) * RPB_TORP_MAX
 		       + RPB_TORP_POSITION);
 	  }
       }
       break;
     case SP_PLANET:
       id = ((struct planet_spacket *)packet)->pnum;
       rpb_insert(diskpos, RPB_PLANET_OFFSET + id * RPB_PLANET_MAX
 	+ RPB_PLANET_INFO);
       break;
     case SP_PLANET_LOC:
       id = ((struct planet_loc_spacket *)packet)->pnum;
       rpb_insert(diskpos, RPB_PLANET_OFFSET + id * RPB_PLANET_MAX
 	+ RPB_PLANET_POSITION);
       break;
     case SP_S_PLANET:
       {
 	int i, numofplanets;
 	struct planet_s_spacket *ppacket;
 
 	numofplanets = ((unsigned char *)packet)[1];
 	ppacket = (struct planet_s_spacket *) &(((char *)packet)[2]);
 
 	for (i=0; i<numofplanets; i++, ppacket++)
 	  {
 	    id = ppacket->pnum;
 	    rpb_insert(diskpos, RPB_PLANET_OFFSET + id * RPB_PLANET_MAX
 		       + RPB_PLANET_INFO);
 	  }
       }
       break;
#ifdef nodef
     default:
       printf("packet type %d", *(unsigned char *)packet);
#endif
    }
}

/* silly compare function used by the build in qsort() function */
int
intcomp(const void *a, const void *b)
{
  if (*(int *)a < *(int *)b)
    return -1;
  else if (*(int *)a > *(int *)b)
    return 1;
  else
    return 0;
}

/* Called from the main playback loop.  This routine un-does
 * a server update. */
void
rpb_dorev(char *buf)
{
  int startpos;
  int i;
  int temp[RPB_TOTAL];
  int last = -1;
  /*  int min;
      int extra = RPB_TOTAL; */

  if (lastPos[RPB_NORMAL_SEQUENCE] == -1
      || header[lastPos[RPB_NORMAL_SEQUENCE]].prev == -1)
    {
      playback = PL_PAUSE;
      return;
    }

  startpos = header[lastPos[RPB_NORMAL_SEQUENCE]].prev;

  /*  min = startpos; */
  for (i=0; i<RPB_TOTAL; i++)
    while (lastPos[i] > startpos)
     {
	/*	if (lastPos[i] < min)
		min = lastPos[i]; */
 	lastPos[i] = header[lastPos[i]].prev;
       }
 
  /* Well, this hack doesn't seem neccessary afterall -
   * I'm leaving it as a comment just in case it evenutally is
   * required */
#ifdef nodef
  /* HACK++ - add in extra me position/sequence packets inline with other
   * packets.
   * The me position packets are inserted because sp2 torp/player
   * packets are based upon the current position of me.
   * The sequence packets are inserted just for reference.
   * Although these packets are gouped together here, the qsort
   * procedure will arrange them inline with the other packets. */
   i=lastPos[RPB_PLAYER_OFFSET + RPB_PLAYER_POSITION
 	   + RPB_PLAYER_MAX * me->p_no];
   if (i != -1)
     do
      {
	i = header[i].prev;
 	temp[extra++] = i;
      } while (i > min);
   i=startpos;
   do
     {
       i = header[i].prev;
       temp[extra++] = i;
     } while (i > min);
#endif
 
  memcpy(temp, lastPos, sizeof(lastPos));
  qsort(temp, RPB_TOTAL, sizeof(int), intcomp);

  for (i=0; i<RPB_TOTAL; i++)
    if (temp[i] != last)
      {
	last = temp[i];
	fseek(recordFile, header[last].diskpos, SEEK_SET);
	pb_dopacket(buf);
      }
  current = startpos + 1;
}
#endif REVERSE_PLAYBACK

#endif /* RECORDGAME */












