/* main.c
 *
 * $Log: main.c,v $
 * Revision 1.13  2002/06/20 04:18:38  tanner
 * Merged COW_SDL_MIXER_BRANCH to TRUNK.
 *
 * Revision 1.10.2.1  2002/06/13 04:10:16  tanner
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
 * Revision 1.11  2002/06/11 05:55:13  tanner
 * Following XP made a simple change.
 *
 * I want cow to play the STTNG intro when started. That's it. Nothing else.
 *
 * Revision 1.10  2001/04/28 04:03:56  quozl
 * change -U to also adopt a local port number for TCP mode.
 * 		-- Benjamin `Quisar' Lerman  <quisar@quisar.ambre.net>
 *
 * Revision 1.9  2000/11/07 20:24:05  ahn
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
 * Revision 1.8  2000/05/19 14:24:52  jeffno
 * Improvements to playback.
 * - Can jump to any point in recording.
 * - Can lock on to cloaked players.
 * - Tactical/galactic repaint when paused.
 * - Can lock on to different players when recording paused.
 *
 * Revision 1.7  2000/01/07 17:36:02  siegl
 * final release infos
 *
 * Revision 1.6  1999/08/20 18:32:45  siegl
 * WindowMaker Docking support
 *
 * Revision 1.5  1999/08/05 16:43:03  siegl
 *  New -B option for automatic bug submition, wwwlink is queried from .xtrekrc
 *
 * Revision 1.4  1999/07/29 19:10:24  carlos
 *
 * Fixing random things that prevented trekhopd support from compiling
 * and working properly.
 *
 * --Carlos V.
 *
 * Revision 1.3  1999/04/02 19:09:34  siegl
 * Add usage for -F option for playback recorded file
 *
 * Revision 1.2  1999/03/05 23:06:38  carlos
 * In the Makefile, updated the KEYGOD address
 *
 * In all else, (cowmain.c, main.c newwin.c parsemeta.{c,h} x11window.c
 * added UDP metaserver query functionality as proposed by James Cameron
 * and implemented by James Cameron and Carlos Villalpando.
 *
 * Revision 1.1.1.1  1998/11/01 17:24:10  siegl
 * COW 3.0 initial revision
 * */
#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include INC_STRINGS
#include <time.h>
#include INC_SYS_TIME

#ifdef TOOLS
#include <stdlib.h>
#include "version.h"
#include "patchlevel.h"
#endif

#include "cowapi.h"
#include "defs.h"

#ifdef GATEWAY
extern int gw_serv_port, gw_port, gw_local_port; /* UDP */
extern char *gw_mach;                            /* UDP */
extern char *gateway;
extern char *serverName;
void read_servers();
#endif

extern int logmess;

char   *servertmp = NULL;

#ifdef EXPIRE
char    exptime[27];

#endif

#ifdef HAVE_XPM
#define NO_PIXMAPS 0x8000
extern int pixMissing;

#endif


#ifndef WIN32
main(int argc, char **argv)
#else
main2(int argc, char **argv)
#endif
{
  int     usage = 0;
  int     err = 0;
  int     inplayback = 0;

  char   *name, *ptr, *cp;
  struct passwd *pwent;

#ifdef TOOLS
  char    url[1024];

#endif

#ifdef EXPIRE
  time_t  expday, daycomp, today;

#endif

#ifdef GATEWAY
  int     hset = 0;

#endif
  int     first = 1;
  int     i;
  char   *log;
  int     xtrekPort = -1;

#ifdef WINDOWMAKER
  wm_argv=argv;
  wm_argc=argc;
#endif

  name = *argv++;
  argc--;
  if ((ptr = RINDEX(name, '/')) != NULL)
    name = ptr + 1;

#ifdef GATEWAY
  netaddr = 0;
#endif

#ifdef EXPIRE
  daycomp = tv_ctime.tv_sec;
  expday = daycomp + EXPIRE * 24 * 3600;
  today = time(NULL);
  STRNCPY(exptime, ctime(&expday), sizeof(exptime));
#endif

#ifdef TOOLS
  url[0] = '\0';
#endif

  pseudo[0] = defpasswd[0] = '\0';

  while (*argv)
    {
      if (**argv == '-')
	++ * argv;
      else
	break;

      argc--;
      ptr = *argv++;
      while (*ptr)
	{
	  switch (*ptr)
	    {

	    case 'C':				 /* character name */
	      (void) STRNCPY(pseudo, *argv, sizeof(pseudo));
	      argv++;
	      argc--;
	      break;

	    case 'A':				 /* authorization password */
	      (void) STRNCPY(defpasswd, *argv, sizeof(defpasswd));
	      argv++;
	      argc--;
	      break;

	    case 'u':
	      usage++;
	      break;
	    case 'c':
	      checking = 1;
	      break;
	    case 's':
	      if (*argv)
		{
		  xtrekPort = atoi(*argv);
		  passive = 1;
		  argv++;
		  argc--;
		}
	      break;

#ifdef RECORDGAME
	    case 'F':
	      inplayback = 1;
              if (*(ptr+1) == 'i')
                  pb_create_index = 1;
	      /* No break */
	    case 'f':
	      recordFileName = *argv;
	      argv++;
	      argc--;
	      break;
#endif

	    case 'l':
	      logFileName = *argv;
              logmess = 1;
	      argv++;
	      argc--;
	      break;
	    case 'p':
	      if (*argv)
		{
		  xtrekPort = atoi(*argv);
		  argv++;
		  argc--;
		}
	      break;
	    case 'd':
	      display_host = *argv;
	      argc--;
	      argv++;
	      break;

#ifdef META
	    case 'm': /* use multiple metaservers by UDP */
	      if (usemeta && usemeta != 1)
		{
		  fputs("The options -k and -m and -M are mutually exclusive\n",
			stderr);
		  err++;
		}
	      usemeta = 1;
	      break;

	    case 'k': /* use metaserver cache from prior -M usage */
	      if (usemeta && usemeta != 2)
		{
		  fputs("The options -k and -m and -M are mutually exclusive\n",
			stderr);
		  err++;
		}
	      usemeta = 2;
	      break;
 
           case 'M': /* use single metaserver by TCP */
             if (usemeta && usemeta != 3)
               {
                 fputs("The options -k, -m and -M are mutually exclusive\n",
                       stderr);
                 err++;
               }
             usemeta = 3;
             break;
#endif

#ifdef RSA
	    case 'o':
	      RSA_Client = -1;			 /* will be reset leter, set
						  * * * negative here * to
						  * flag * * that it should
						  * override * * xtrekrc */
	      printf("Using standard binary verification\n");
	      break;
	    case 'R':
	      RSA_Client = -2;			 /* will be reset leter, set
						  * * * negative here * to
						  * flag * * that it should
						  * override * * xtrekrc */
	      printf("Using RSA verification\n");
	      break;
#else
	    case 'R':
	      printf("This client does not support RSA verification\n");
	    case 'o':
	      printf("Using standard binary verification\n");
	      break;
#endif

	    case 'h':
	      servertmp = *argv;

#ifdef GATEWAY
	      gw_mach = *argv;
#endif

	      argc--;
	      argv++;
	      break;

#ifdef GATEWAY
	    case 'H':
	      hset++;
              read_servers();
              serverName = gateway;
	      netaddr = strToNetaddr(*argv);
	      /* netaddrstr = *argv; */
	      argc--;
	      argv++;
	      break;
#endif

	    case 'U':
	      if ((baseLocalPort = atoi(*argv)) == 0)
		{
		  fprintf(stderr, "Error: -U requires a port number\n");
		  exit(1);
		}
	      argc--;
	      argv++;
	      break;

#ifdef PACKET_LOG
	    case 'P':
	      log_packets++;
	      break;
#endif

	    case 'G':
	      if (*argv)
		{
		  ghoststart++;
		  ghost_pno = atoi(*argv);
		  printf("Emergency restart being attempted...\n");
		  argv++;
		  argc--;
		}
	      break;

	    case 't':
	      title = *argv;
	      argc--;
	      argv++;
	      break;
	    case 'r':
	      deffile = *argv;
	      argv++;
	      argc--;
	      break;

	    case 'D':
	      debug++;
	      break;
	    case 'v':
	      printf("%s\n", cowid);
	      printf("Compile options used: %s\n", cflags);
	      printf("Compiled on %s by %s\n", cdate, cwho);
	      printf("%s\n", cbugs);

#ifdef RSA
	      printf("RSA key installed: %s --- Created by: %s\n", key_name, client_creator);
	      printf("     Client type: %s\n", client_type);
	      printf("     Client arch: %s\n", client_arch);
	      printf("     Key permutation date: %s\n", client_key_date);
	      printf("     Comments: %s\n", client_comments);
#endif

#ifdef EXPIRE
	      printf("THIS CLIENT WILL EXPIRE ON %s\n", exptime);
#endif

	      exit(0);
	      break;

#ifdef IGNORE_SIGNALS_SEGV_BUS
	    case 'i':
	      printf("Ignoring signals SIGSEGV and SIGBUS\n");
	      ignore_signals = -1;
	      break;
#endif

#ifndef WIN32
	    case 'n':
	      takeNearest = 1;
	      break;
#endif

#ifdef HAVE_XPM
	    case 'b':
	      pixMissing |= NO_PIXMAPS;
	      break;
#endif

#ifdef TOOLS
	    case 'L':
	      sprintf(url, upgradeURL, arch);
	      break;

	    case 'V':
	      sprintf(url, releaseURL, mvers, PATCHLEVEL);
	      break;

	    case 'B':
	      sprintf(url, bugURL, mvers, PATCHLEVEL, arch);
	      break;
#endif

	    default:
	      fprintf(stderr, "%s: unknown option '%c'\n", name, *ptr);
	      err++;
	      break;
	    }
	  ptr++;
	}
    }

#ifdef TOOLS
  if (*url)
    {
      char    webcall[1024];

      initDefaults(deffile);
      if (getdefault("wwwlink") != NULL)
	wwwlink = getdefault("wwwlink");

      sprintf(webcall, wwwlink, url);
      system(webcall);
      url[0] = '\0';
      exit(0);
    }
#endif

#ifdef EXPIRE
  daycomp = tv_ctime.tv_sec;
  expday = daycomp + EXPIRE * 24 * 3600;
  today = time(NULL);
  STRNCPY(exptime, ctime(&expday), sizeof(exptime));

  if ((expday - today) < 0.2 * (expday - daycomp) || (expday - today) / (24 * 3600) < 5)
    {
      printf("!!!!!!!!!!!!!!!!!!!!!!WARNING!!!!!!!!!!!!!!!!!!!!!!!\n");
      printf("This client will expire on %s\n", exptime);
      printf("Please obtain a newer version from your favourite ftp site.\n");
      printf("At the moment of writing http://cow.netrek.org/ is the COW home.\n");

#ifdef TOOLS
      printf("Or try the -L option to get a new version.\n");
#endif
    }
  if (today > expday)
    {
      printf("Sorry. This client has expired. It can no longer be used.\n");

#ifdef TOOLS
      printf("Try the -L option to get a new version.\n");
#endif

      exit(0);
    }
#endif

  if (usage || err)
    {
      printUsage(name);
      exit(err);
    }

#ifdef GATEWAY
  if (!hset)
    use_trekhopd = 0;				 /* allow use via normal * *
						  * connections */
  if (netaddr == 0)
    {
      fprintf(stderr,
	      "netrek: no remote address set (-H).  Restricted server will not work.\n");
    }
#endif

#ifdef RECORDGAME
  if (inplayback)
    err = pbmain(name);
  else
#endif

    err = cowmain(servertmp, xtrekPort, name);

  exit(err);
}


printUsage(char *prog)
{
  printf("%s\n", cowid);
  printf("Usage: %s [options] [display-name]\n", prog);
  printf("Options:\n");
  printf(" [-h servername]     Specify a server\n");
  printf(" [-p port number]     Specify a port to connect to\n");
  printf(" [-r defaultsfile]   Specify defaults file\n");
  printf(" [-s socketnum]      Specify listen socket port for manual start\n");
  printf(" [-G playernum]      Reconnect after ghostbust.  Use with -s\n");
  printf(" [-u]   show usage\n");
  printf(" [-A]   character password\n");
  printf(" [-C]   character name\n");

#ifdef IGNORE_SIGNALS_SEGV_BUS
  printf(" [-i]   ignore SIGSEGV and SIGBUS\n");
#endif

#ifdef GATEWAY
  printf(" [-H]   specify host (via gateway)\n");
#endif

  printf(" [-U port]       Specify client UDP or TCP port (useful for some firewalls)\n");

#ifdef RSA
  printf(" [-o]   use old-style binary verification)\n");
  printf(" [-R]   use RSA binary verification\n");
#endif

#ifdef PACKET_LOG
  printf(" [-P]   Log server packets, repeat for increased information\n");
#endif

  printf(" [-c]   to just do ck_players on the server\n");
  printf(" [-f filename]   Record game into 'filename'\n");
  printf(" [-F filename]   Plays the recorded game from 'filename'\n");
  printf(" [-l filename]   Record messages into 'filename'\n");

#ifdef META
  printf(" [-m]   list servers, using UDP/IP to multiple metaservers\n");
  printf(" [-M]   list servers, using TCP/IP to single metaserver\n");
  printf(" [-k]   list servers from cache generated by -M\n");
#endif

#ifndef WIN32
  printf(" [-n]   use nearest colors in shared colormap\n");
#endif

#ifdef HAVE_XPM
  printf(" [-b]   do not attempt to load color pixmaps\n");
#endif

#ifdef TOOLS
  printf(" [-L]   upgrade to Latest version (requires running netscape)\n");
  printf(" [-V]   Version info and release notes (requires running netscape)\n");
  printf(" [-B]   submit a Bug report (requires running netscape)\n");
#endif

  printf(" [-v]   display client version info\n");

#ifdef EXPIRE
  printf("THIS CLIENT WILL EXPIRE ON %s\n", exptime);
#endif
}
