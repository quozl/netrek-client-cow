/* main.c
*/
#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include INC_STRINGS
#include <time.h>
#include INC_SYS_TIME
#include <locale.h>
#include "gettext.h"

#ifdef TOOLS
#include <stdlib.h>
#include "version.h"
#include "patchlevel.h"
#endif

#include "cowapi.h"
#include "defs.h"
#include "defaults.h"

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

#ifdef TOOLS
  char    url[1024];

#endif

#ifdef EXPIRE
  time_t  expday, daycomp, today;

#endif

#ifdef GATEWAY
  int     hset = 0;

#endif
  int     xtrekPort = -1;

  program = argv[0];

  setlocale(LC_ALL, "");
  bindtextdomain("netrek-client-cow", "po/");
  textdomain("netrek-client-cow");

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
      if (!strcmp(*argv, "--fast-guest")) {
	fastGuest++;
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
		  fputs(_("The options -k and -m and -M are mutually exclusive\n"),
			stderr);
		  err++;
		}
	      usemeta = 1;
	      break;

	    case 'k': /* use metaserver cache from prior -M usage */
	      if (usemeta && usemeta != 2)
		{
		  fputs(_("The options -k and -m and -M are mutually exclusive\n"),
			stderr);
		  err++;
		}
	      usemeta = 2;
	      break;
 
           case 'M': /* use single metaserver by TCP */
             if (usemeta && usemeta != 3)
               {
                 fputs(_("The options -k, -m and -M are mutually exclusive\n"),
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
	      printf(_("Using standard binary verification\n"));
	      break;
	    case 'R':
	      RSA_Client = -2;			 /* will be reset leter, set
						  * * * negative here * to
						  * flag * * that it should
						  * override * * xtrekrc */
	      printf(_("Using RSA verification\n"));
	      break;
#else
	    case 'R':
	      printf(_("This client does not support RSA verification\n"));
	    case 'o':
	      printf(_("Using standard binary verification\n"));
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
		  fprintf(stderr, _("Error: -U requires a port number\n"));
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
		  printf(_("Emergency restart being attempted...\n"));
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
	      printf(_("Compile options used: %s\n"), cflags);
	      printf(_("Compiled on %s by %s\n"), cdate, cwho);
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
	      printf(_("Ignoring signals SIGSEGV and SIGBUS\n"));
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
	    case 'S': /* analyse a cambot recording for visualisation */
	      gather_stats++;
	      break;

	    default:
	      fprintf(stderr, _("%s: unknown option '%c'\n"), name, *ptr);
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

static void printUsage(char *prog)
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

  printf(" [-b]   do not attempt to load color pixmaps\n");

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
