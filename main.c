/* main.c
*/
#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include <stdlib.h>
#include INC_STRINGS
#include <time.h>
#include INC_SYS_TIME
#include <locale.h>

#include "cowapi.h"
#include "defs.h"
#include "defaults.h"
#include "version.h"

extern int logmess;

char   *servertmp = NULL;

#define NO_PIXMAPS 0x8000
extern int pixMissing;

extern int gather_stats;

static void printUsage(char *prog);

#ifndef WIN32
int main(int argc, char **argv)
#else
int main2(int argc, char **argv)
#endif
{
  int     usage = 0;
  int     err = 0;
  int     inplayback = 0;

  char   *name, *ptr;

  int     xtrekPort = -1;

  program = argv[0];

  setlocale(LC_ALL, "");

#ifdef WINDOWMAKER
  wm_argv=argv;
  wm_argc=argc;
#endif

  name = *argv++;
  argc--;
  if ((ptr = RINDEX(name, '/')) != NULL)
    name = ptr + 1;

  pseudo[0] = defpasswd[0] = '\0';

  while (*argv)
    {
      if (!strcmp(*argv, "--help")) {
	printUsage(name);
	exit(0);
      }

      if (!strcmp(*argv, "--fast-guest")) {
	fastGuest++;
	argv++; argc--;
	continue;
      }

      if (!strcmp(*argv, "--small-screen")) {
	extern int small_screen;
	small_screen++;
	argv++; argc--;
	continue;
      }

      if (!strcmp(*argv, "--server")) {
	argv++; argc--;
	if (*argv) {
	  servertmp = *argv;
	  argv++; argc--;
	  continue;
	}
      }

      if (!strcmp(*argv, "--port")) {
	argv++; argc--;
	if (*argv) {
	  xtrekPort = atoi(*argv);
	  argv++; argc--;
	  continue;
	}
      }

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
	      argc--;
	      argv++;
	      break;

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

	    case 'b':
	      pixMissing |= NO_PIXMAPS;
	      break;

	    case 'S': /* analyse a cambot recording for visualisation */
	      gather_stats++;
	      break;

	    default:
	      fprintf(stderr, "%s: unknown option '%c'\n", name, *ptr);
	      err++;
	      break;
	    }
	  ptr++;
	}
    }

  if (usage || err)
    {
      printUsage(name);
      exit(err);
    }

#ifdef RECORDGAME
  if (inplayback)
    err = pbmain(name);
  else
#endif

    err = cowmain(servertmp, xtrekPort, name);

  exit(err);
}

static void printUsage(char *prog)
{
  printf("%s\n", version);
  printf("Usage: %s [options] [display-name]\n", prog);
  printf("Options:\n");
  printf(" [--server n]        Connect to a server immediately\n");
  printf(" [--port n]          Port to connect to\n");
  printf(" [--fast-guest]      Bypass login and play as guest\n");
  printf(" [--small-screen]    Shrink windows to fit on an 800x600 screen\n");
  printf(" [-r defaultsfile]   Specify defaults file\n");
  printf(" [-s socketnum]      Specify listen socket port for manual start\n");
  printf(" [-G playernum]      Reconnect after ghostbust.  Use with -s\n");
  printf(" [-u]   show usage\n");
  printf(" [-A]   character password\n");
  printf(" [-C]   character name\n");

#ifdef IGNORE_SIGNALS_SEGV_BUS
  printf(" [-i]   ignore SIGSEGV and SIGBUS\n");
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

  printf(" [-b]   do not attempt to load color pixmaps\n");
}
