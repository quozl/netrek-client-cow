/* PlayBack.c
 *
 * Kevin O'Connor 9/22/97
 *
 * Routines neccessary to playback a game recording.
 *
 * $Log: playback.c,v $
 * Revision 1.15  2006/09/19 10:20:39  quozl
 * ut06 full screen, det circle, quit on motd, add icon, add desktop file
 *
 * Revision 1.14  2006/05/22 13:12:48  quozl
 * add camera frame counter
 *
 * Revision 1.13  2006/01/27 09:57:27  quozl
 * *** empty log message ***
 *
 * Revision 1.12  2002/06/22 04:43:24  tanner
 * Clean up of SDL code. #ifdef'd out functions not needed in SDL.
 *
 * Revision 1.11  2002/06/20 04:18:38  tanner
 * Merged COW_SDL_MIXER_BRANCH to TRUNK.
 *
 * Revision 1.8.2.1  2002/06/13 04:10:16  tanner
 * Wed Jun 12 22:52:13 2002  Bob Tanner  <tanner@real-time.com>
 *
 * 	* playback.c (pbmain):  Converted enter_ship.wav
 *
 * 	* input.c (Key113): Converted self_destruct.wav
 *
 * 	* input.c (Key109): Converted message.wav
 *
 * 	* local.c (DrawMisc): Converted warning.wav
 *
 * 	* local.c (DrawPlasmaTorps): Converted plasma_hit.wav
 *
 * 	* local.c (DrawTorps): Converted torp_hit.wav
 *
 * 	* sound.h: added EXPLOSION_OTHER_WAV, PHASER_OTHER_WAV,
 * 	FIRE_TORP_OTHER. and the code to load these new sounds.
 *
 * 	* local.c (DrawShips): Converted cloak.wav, uncloak.wav,
 * 	shield_down.wav, shield_up.wav, explosion.wav,
 * 	explosion_other.wav, phaser.wav, phaser_other.wav
 *
 * 	* cowmain.c (cowmain): Converted enter_ship.wav and engine.wav
 *
 * 	* sound.c: added isDirectory to check that the sounddir is
 * 	actually a directory.
 *
 * Tue Jun 11 01:10:51 2002  Bob Tanner  <tanner@real-time.com>
 *
 * 	* system.mk.in: Added SDL_CFLAGS, SDL_CONFIG, SDL_LIBS,
 * 	SDL_MIXER_LIBS
 *
 * 	* sound.c: Added HAVE_SDL wrapper, initialization of SDL system,
 * 	opening of audio device, and loading of 17 cow sounds.
 *
 * 	* cowmain.c (cowmain): HAVE_SDL wrapper to Init_Sound using SDL. I
 * 	moved the Init_Sound method to right after readdefaults() so the
 * 	intro can start playing ASAP.
 *
 * 	* configure.in: Added AC_CANONICAL_SYSTEM, added check for SDL,
 * 	add check for SDL_mixer.
 *
 * 	* config.h.in: add HAVE_SDL
 *
 * 	* spike: See spike/README for details
 *
 * Revision 1.9  2002/06/13 03:58:41  tanner
 * The changes for sound are mostly isolated in local.c, just a few other changes
 * in the commit.
 *
 * 	* playback.c (pbmain):  Converted enter_ship.wav
 *
 * 	* input.c (Key113): Converted self_destruct.wav
 *
 * 	* input.c (Key109): Converted message.wav
 *
 * Revision 1.8  2001/07/24 00:29:13  quozl
 * minor playback fix
 *
 * Revision 1.7  2001/06/12 02:48:49  quozl
 * add single-step playback keys
 *
 * Revision 1.6  2001/04/26 05:58:20  quozl
 * 	* Makefile (dist): change dist and distdoc targets to generate a
 * 	.tar.gz file that unpacks to a directory below the current
 * 	directory.  Note: also writes the output kit file to the current
 * 	directory rather than the directory above.
 *
 * 	* INSTALL: new file, standard generic installation instructions.
 *
 * 	* README: new file, explains the other important package
 * 	documentation files.
 *
 * 	* ChangeLog: new file, a programmers change log as per GNU
 * 	packaging standards and automated EMACS change log entry creation.
 *
 * 	* .cvsignore: add list of files to be ignored by CVS.
 *
 * Revision 1.5  2000/11/07 20:24:05  ahn
 * Add patch from Crist Clark <cjclark@alum.mit.edu>
 *
 * There was a server bust during the Mixed Tw^H^HDrinks-Smack Pack game
 * yesterday. All that was recovered was the cambot.pkt dump. I figured
 * it would be pretty easy to dump the messages from the playback to a
 * file and then run the stats scripts on that to get some pwstat-style
 * numbers.
 *
 * Well, it took a little client hacking (and then some toying with the
 * ancient pwstat.pl I had). I was modifying COW.3.00pl2. The two files
 * that need to be patched to get it to work are included at the
 * end. main.c needed changing since apparently using the '-f' option on
 * the command line just changes the name of the logfile, but does not
 * turn on logging (bug or feature?). I changed that. Second, playback.c
 * did not support logging at all, so I added the few lines of code it
 * needed.
 *
 * I was looking for the present COW development code, but could not
 * track it down; most Netrek pages I could find are long collections of
 * 404-links. (And my mail bounced when I tried to rejoin the Vanilla
 * server mail lists.) Where is the latest COW? If anyone finds the
 * patches interesting, feel free to use them. Finally, any "trusted" COW
 * builders up for making a blessed FreeBSD COW? I have an unblessed
 * FreeBSD client running, but need to run a Linux binary if I want
 * blessed.
 * --
 * Crist J. Clark                           cjclark@alum.mit.edu
 *
 * Revision 1.4  2000/05/19 14:24:52  jeffno
 * Improvements to playback.
 * - Can jump to any point in recording.
 * - Can lock on to cloaked players.
 * - Tactical/galactic repaint when paused.
 * - Can lock on to different players when recording paused.
 *
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
#include <stdlib.h>
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

int *pb_index;
int pb_num_index = 0;
int pb_goto = 0;
int pb_create_index;
int pb_index_exists;
int pb_num_context = 0;
int pb_num_fast_forward = 0;
int pb_sequence_count = 0;
int pb_stepping = 0;			/* non-zero if doing a step	*/
int pb_snapping = 0;			/* non-zero if taking camera snapshots	*/

const char *INDEX_FORMAT = "%d,%d,%d";
const int INDEX_GRANULARITY = 100;

static int packet_size = 0;

struct player dummyme;

struct player *packetsme;
struct player *displayme;

/* We want reverse-playback!!! */
#define REVERSE_PLAYBACK

void pb_read_index();
int pb_get_index(int sequence_num, int *jump_actual, int *offset, int *num_context);
int pb_index_compare(const void *a, const void *b);

/* Forward declarations for reverse playback */
void rpb_init(void);
void rpb_analyze(int diskpos, void *packet);
void rpb_dorev(char *buf);

struct player *packetsme;
struct player *displayme;

int
        pbmain(char *name)
{
  int     i;
  int     s_type;

  char index_filename[FILENAME_MAX+1];
  char context_filename[FILENAME_MAX+1];

  strncpy(index_filename, recordFileName, FILENAME_MAX-4);
  strncpy(context_filename, recordFileName, FILENAME_MAX-4);
  index_filename[FILENAME_MAX-4] = '\0';
  context_filename[FILENAME_MAX-4] = '\0';
  strcat(index_filename, ".idx");
  strcat(context_filename, ".cxt");

#ifdef REVERSE_PLAYBACK
  rpb_init();
#endif

  playback = PL_FORWARD;
  pseudo[0] = defpasswd[0] = '\0';

  i = setjmp(env);				 /* Error while initializing */
  if (i >= RETURNBASE)
    return (i - RETURNBASE);			 /* Terminate with retcode */

  if (logFileName != NULL)
    {
      logFile = fopen(logFileName, "a");
      if (logFile == NULL)
        {
          perror(logFileName);
          return (1);
        }
    }

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

  /* Open record file. */
  recordFile = fopen(recordFileName, "rb");
  if (recordFile == NULL)
    {
      perror(recordFileName);
      return (1);

    }

  /* Open index files of recording. */
  pb_index_exists = 1;
  if (!pb_create_index) {
      recordIndexFile = fopen(index_filename, "r");
      if (recordIndexFile == NULL)
          {
              perror(index_filename);
              pb_index_exists = 0;
          }

      recordContextFile = fopen(context_filename, "rb");
      if (recordContextFile == NULL)
      {
          perror(context_filename);
          pb_index_exists = 0;
      }
  }
  else
      pb_index_exists = 0;


  /* Create index files if we were told to index the recording. */
  if (pb_create_index  && (recordIndexFile =
       fopen(index_filename, "wb"))==NULL)
  {
      perror("Could not create index file.");
      exit(1);
  } 
  if (pb_create_index && (recordContextFile =
       fopen(context_filename, "wb"))==NULL)
  {
      perror("Could not create context file.");
      exit(1);
  }
  if (pb_create_index) {
      pbdelay = 0;
      playback = PL_FORWARD;
      printf("Creating index.\n");
  }

  me = &dummyme;
  myship = &(me->p_ship);
  mystats = &(me->p_stats);

  me->p_x = me->p_y = 50000;
  getship(myship, CRUISER);
  shipchange(CRUISER);
  displayme = me;
  packetsme = me;

  /* Read the index file.  We do this only once. */
  pb_read_index();

  /* Get first packet from file */
  readFromFile();

  displayme = packetsme;

  lastm = mctl->mc_current;

  mapAll();

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

#if defined(SOUND) && !defined(HAVE_SDL)
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
#if defined(HAVE_SDL)
  Play_Sound(ENTER_SHIP_WAV);
#else
  Play_Sound(ENTER_SHIP_SOUND);
  Play_Sound(ENGINE_SOUND);
#endif
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

      while (W_EventsQueuedCk())
	{
	  process_event();
	  /* W_Flush(); */
	}

      intrupt();
      W_Flush();
      if (!pb_stepping) usleep(pbdelay);
      if (pb_snapping) camera_snap(w);
    }
}

void
        pbsetspeed(char key)
{
#define JUMP_MAX 7
  static char jump_str[JUMP_MAX] = "";
  static int jump_idx = 0;
  static int jump_on = 0;
  static int tmp_playback;
  static int tmp_pbdelay = -1;

  int old_playback;

  /* Used at end of function. */
  old_playback = playback;

  /* Read in sequence number to jump to. */
  if (jump_on) {
      switch (key) {
      case '0': case '1': case '2': case '3': case '4': case '5':
      case '6': case '7': case '8': case '9':

          /* Ignore further input if we run out of room. */
          if (jump_idx > JUMP_MAX - 2)
              break;

          /* Get the next number. */
          jump_str[jump_idx] = key;
          jump_idx++;
          jump_str[jump_idx] = '\0';
          printf("Jump input: %s\n", jump_str);

          break;
      case 'J': /* abort jump */
          jump_on = 0;
          playback = tmp_playback;
          printf("Aborting jump.\n", jump_str);
          jump_str[0] = '\0';
          break;
      default: /* Done entering jump data when non-digit key hit. */
          jump_str[jump_idx] = '\0';

          /* Convert string to number. */
          pb_goto = strtol(jump_str, NULL, 10);
          if (pb_goto < 1)
              pb_goto = 1;

          jump_on = 0;
          jump_str[0] = '\0';
          break;
      }
      goto end;
  }

  if (playback == PL_PAUSE)
    playback = PL_FORWARD;
  switch (key)
    {
    case 0x8:	/* step backward one frame	*/
      pb_stepping++;
      playback = PL_REVERSE;
      break;
    case 0xd:	/* step forward one frame	*/
      pb_stepping++;
      break;
    case ' ':	/* turn on or off single step	*/
      if (old_playback == PL_PAUSE)
	playback = PL_FORWARD;
      else
	pb_stepping++;
      break;
    case '0':
      playback = PL_PAUSE;
      break;
    case '1':
      tmp_pbdelay = pbdelay = 800000;
      break;
    case '2':
      tmp_pbdelay = pbdelay = 400000;
      break;
    case '3':
      tmp_pbdelay = pbdelay = 200000;
      break;
    case '4':
      tmp_pbdelay = pbdelay = 100000;
      break;
    case '5':
      tmp_pbdelay = pbdelay = 50000;
      break;
    case '6':
      tmp_pbdelay = pbdelay = 25000;
      break;
    case '7':
      tmp_pbdelay = pbdelay = 12500;
      break;
    case '8':
      tmp_pbdelay = pbdelay = 6250;
      break;
    case '9':
    case '#':
    case '!':
    case '@':
    case '%':
      tmp_pbdelay = pbdelay = 0;
      break;
    case '<':
      if (tmp_pbdelay == -1)
          tmp_pbdelay = pbdelay /= 2;
      else
          pbdelay = tmp_pbdelay /= 2;
      break;
    case '>':
      if (tmp_pbdelay == -1)
          tmp_pbdelay = pbdelay *= 2;
      else
          pbdelay = tmp_pbdelay *= 2;
      break;
    case 'R':
      {
#ifdef REVERSE_PLAYBACK
	rpb_init();
#endif
  	fseek (recordFile, 0, SEEK_SET);
	playback = PL_FORWARD;
        pb_sequence_count = 0;
	break;
      }
    case '(':
      playback = PL_REVERSE;
      break;
    case ')':
      playback = PL_FORWARD;
      break;
    case 'j':
      printf("Jumping..\n");
      tmp_playback = playback;
      playback = PL_PAUSE;
      jump_on = 1;
      jump_idx = 0;
      jump_str[0] = '\0';
      break;
    case 's':
      fprintf(stderr, "toggle-snap\n");
      pb_snapping = ~pb_snapping;
      break;
    }

 end:
    /* If we are paused, set the delay to something reasonable. */
    if (playback == PL_PAUSE && old_playback != PL_PAUSE) {
        tmp_pbdelay = pbdelay;
        pbdelay = 100000;
    }

    /* If we were paused, but now are not, reset the original delay. */
    if (old_playback == PL_PAUSE && playback != PL_PAUSE && tmp_pbdelay != -1) {
        pbdelay = tmp_pbdelay;
        tmp_pbdelay = -1;
    }
}

void
        pblockplayer(int who)
{
   me = displayme = &players[who];
}

void
        pblockplanet(int pl)
{
   me = &dummyme;
   displayme->p_x = planets[pl].pl_x;
   displayme->p_y = planets[pl].pl_y;
   displayme = me;
}

inline int
        ckRecordPacket(char packet)
{
  return 1;
#if 0
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
#endif
}

extern struct packet_handler handlers[];

/**
 * Read the next packet from the record file and call the appropiate handler.
 *
 * @param buf Stores the next packet.
 *
 * @return 1 if EOF or error, or 0 for success.
 */
int
pb_dopacket(char *buf)
{
  int size, count;

  count = fread(buf, 1, 4, recordFile);
  if (count < 4)
    {
      return 1;
    }


  /* Determine how many more bytes we need to read. */
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

  packet_size = size;

  /* Read the rest of the packet. */
  if (size > count)
    count += fread(buf+count, 1, size-count, recordFile);
  if (debug)
    printf("Reading packet %d\n", buf[0]);

  /* If we couldn't read enough */
  if (count < size)
    {
      return 1;
    }

  /* Call the packet handler and return success (zero). */
  (*(handlers[buf[0]].handler)) (buf
#ifdef CORRUPTED_PACKETS
				 ,recordFile
#endif
				 );
  return 0;
}


int readFromFile() {
    int offset = ftell(recordFile);
    int jump_actual; /* The sequence number we actually jump to. */
    FILE *tmp_file;
    int result;
    int num_fast_forward;

    /* If jumping to a spot in the recording, read the required number of
       context packets first. */
    if (pb_goto) {
        playback = PL_FORWARD;
        num_fast_forward = pb_get_index(pb_goto, &jump_actual,
                                        &offset, &pb_num_context);

        if (pb_num_context > 0) {
            printf("Reading in %d context packets..\n", pb_num_context);
            rewind(recordContextFile);
            tmp_file = recordFile;
            recordFile = recordContextFile;
            pb_sequence_count = 0;

            readFromFile0();

            recordFile = tmp_file;
        }

        pb_sequence_count = jump_actual - 1;
        pb_num_fast_forward = num_fast_forward;

#ifdef REVERSE_PLAYBACK
        rpb_init();
#endif
    }

    /* Get up to the next sequence packet. */
    fseek(recordFile, offset, SEEK_SET);
    result = readFromFile0();

    /* If we jumped pause playback and reset state back to normal. */
    if (pb_goto) {
        playback = PL_PAUSE;
        pb_goto = 0;

        printf("Done jumping.\n");
    }

    return result;
}


int
readFromFile0()
{
#define MAXPACKETSIZE 128
  static uint aligned_buf[MAXPACKETSIZE/sizeof(uint)];
  static int num_context_written = 0;

  char *buf = (char *) &aligned_buf;
  int diskpos;
  int sequence_start_pos = -1;
  int sequence_num_context = 0;
  int read_context = 0;


  if (playback == PL_PAUSE)
    return 1;
#ifdef REVERSE_PLAYBACK
    else if (playback == PL_REVERSE) {
        diskpos = ftell(recordFile);
        me = packetsme;

        rpb_dorev(buf);

        packetsme = me;
        me = displayme;

        if (pb_stepping) {
	  playback = PL_PAUSE;
	  pb_stepping = 0;
        }

        return 1;
    }
#endif

  if (pb_stepping) {
    playback = PL_PAUSE;
    pb_stepping = 0;
  }

  /* Read packets. */
  while (1) {
      diskpos = ftell(recordFile);
      me = packetsme;

      if (sequence_start_pos == -1)
          sequence_start_pos = diskpos;

      /* Read a packet and call it's handler. */
      if (pb_dopacket(buf)) {
          /* End of file reached! */
          playback = PL_PAUSE;
          pb_num_fast_forward = 0;
          packetsme = me;
          me = displayme;
          printf("End of file.\n");

          if (pb_create_index)
              exit(0);

          return 1;
      }

#ifdef REVERSE_PLAYBACK
      if (!pb_create_index && !pb_num_context)
          rpb_analyze(diskpos, buf);
#endif

      /* If we are reading in context packts.. */
      if (pb_num_context) {
          pb_num_context--;

          if (pb_num_context < 1) {
              printf("Done reading context packets.\n");
              return 1;
          }
      }

      /* If we are creating an index of the recording, write packet if necessary */
      if (pb_create_index && PB_CONTEXT(buf[0]) ) {
          if (!fwrite(buf, 1, packet_size, recordContextFile)) {
              perror("Bad write on context file.");
              exit(1);
          }
          num_context_written++;
          sequence_num_context++;
      }

      packetsme = me;
      me = displayme;

      if (buf[0] == SP_SEQUENCE || buf[0] == SP_SC_SEQUENCE
          || buf[0] == SP_S_SEQUENCE)
      {
          pb_sequence_count++;

          if (pb_num_fast_forward > 0)
              pb_num_fast_forward--;

          /* If we are creating an index of the recording, write out what
             sequence number we are on, where we are in the file, and how
             many context packets we have to read back in to get back
             here. */
          if ( pb_create_index && !(pb_sequence_count % INDEX_GRANULARITY) ) {
              fprintf(recordIndexFile, INDEX_FORMAT, pb_sequence_count,
                      sequence_start_pos, num_context_written - sequence_num_context);
              fprintf(recordIndexFile, "\n");
          }

          if ( pb_create_index && !(pb_sequence_count % 1000) )
              printf("Indexed %d sequences.\n", pb_sequence_count);

          /* For testing, cut this short.  A recording should be under
             100k sequences */
          if (pb_sequence_count > 1000000) {
              printf("I've seen enough, exiting!\n");
              exit(0);
          }


          sequence_start_pos = -1;
          sequence_num_context = 0;

          /* Return 1 if redraw needed. */
          if (pb_create_index || pb_num_fast_forward)
              ;
          else
              return 1;

      } /* if sequence packet */

  } /* while(1) */

}
  
void pb_read_index() {
    const int MAXLINE = 100;
    char line[MAXLINE+1];
    int num_lines = 0;

    if (!pb_index_exists) {
        pb_index = NULL;
        pb_num_index = 0;
        return;
    }

    rewind(recordIndexFile);

    /* Count how many lines we have. */
    while (fgets(line, MAXLINE, recordIndexFile))
        num_lines++;

    pb_index = calloc(3*num_lines, sizeof(int));
    pb_num_index = num_lines;

    rewind(recordIndexFile);

    {
        int i = 0;
        while (fscanf(recordIndexFile,
                      INDEX_FORMAT,
                      &pb_index[i*3], /* Sequence num */
                      &pb_index[i*3 + 1], /* offset into file */
                      &pb_index[i*3 + 2]) /* number of context packets */
               > 0)
        {
            i++;
        }
    }

}

int pb_get_index(int sequence_num, int *p_jump_actual, int *offset, int *num_context) {
    int *found;
    int jump_actual;
    int num_left;
    int first_index_num;
    int last_index_num;

    /* If the index doesn't exist go to the beginning and fast forward */
    if (!pb_index_exists){
        jump_actual = 1;
        *offset = 0;
        *num_context = 0;
        num_left = sequence_num - 1;
        goto end;
    }

    first_index_num = pb_index[0];
    last_index_num = pb_index[(pb_num_index-1) * 3];

    /* Convert the sequence number they want to goto to the closest sequence
       we have indexed that is earlier.  For example, if they want to go to
       #801 and we index every 100 sequences, look for #700.  We go back an
       extra 100 so we can read in recent packets that contain info like what
       ships are cloaked, armies carried, etc. */
    jump_actual = (sequence_num / INDEX_GRANULARITY) * INDEX_GRANULARITY - INDEX_GRANULARITY;

    /* If they go before the first index, take them to the
       beginning and fast forward. */
    if (jump_actual < first_index_num) {
        jump_actual = 1;
        *offset = 0;
        *num_context = 0;
        num_left = sequence_num - 1;
        goto end;
    }

    /* If they go too far ahead, take them to the last sequence indexed. */
    if (jump_actual > last_index_num) {
        printf("Jumping ahead to last sequence.\n");
        jump_actual = last_index_num;
    }

    /* Calculate how many sequences we have to fast forward after jumping ahead. */
    num_left = sequence_num - jump_actual;

    found = bsearch(&jump_actual, pb_index, pb_num_index, sizeof(int)*3, pb_index_compare);
    if (found == NULL) {
        /* Shouldn't happen. */
        jump_actual = 1;
        *offset = 0;
        *num_context = 0;
        num_left = sequence_num - 1;
    }
    else {
        int *f = (int *)found;
        *offset = *(f+1);
        *num_context = *(f+2);
    }


 end:
    *p_jump_actual = jump_actual;
    num_left++;
    return num_left;
}

int pb_index_compare(const void *a, const void *b) {
    int x = *(int *)a;
    int y = *(int *)b;

    if (x > y)
        return 1;
    else if (x < y)
        return -1;
    else
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
  pb_sequence_count--;
}
#endif /* REVERSE_PLAYBACK */

#endif /* RECORDGAME */
