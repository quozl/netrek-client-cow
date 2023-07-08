#include <setjmp.h>
#include "config.h"
#include "copyright.h"

#include INC_MACHINE_ENDIAN

#include <stdio.h>
#include INC_STRINGS
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <pwd.h>
#include <time.h>
#include INC_SYS_TIME
#include INC_SYS_WAIT
#include INC_SYS_RESOURCE
#include INC_SYS_SELECT

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"
#include "version.h"
#include "patchlevel.h"

#include "censor.h"
#include "check.h"
#include "defaults.h"
#include "dmessage.h"
#include "enter.h"
#include "feature.h"
#include "findslot.h"
#include "getname.h"
#include "getship.h"
#include "input.h"
#include "lagmeter.h"
#include "map.h"
#include "newwin.h"
#include "ping.h"
#include "pingstats.h"
#include "playerlist.h"
#include "parsemeta.h"
#include "short.h"
#include "smessage.h"
#include "socket.h"
#include "stats.h"
#include "warning.h"

#include "cowmain.h"

int     takeNearest = 0;

jmp_buf env;
int     isFirstEntry;

#define RETURNBASE 10
void    terminate(int error);


#ifdef IGNORE_SIGNALS_SEGV_BUS
int     died_from_signal = 0;
void reset_game(int);

#endif

/* ------------------------------------------------------------------------- */

char    defaulttmp[100];
void handle_exception(int);

#ifdef PACKET_LOG
extern int log_packets;

#endif

/* Variables passing Optional Arguments to cowmain */

char   *deffile = NULL;
char   *recordFileName = NULL;
char   *logFileName = NULL;
char   *display_host = NULL;
int     passive = 0;				 /* whether -s specified on

						  * 
						  * * commandline */
int     checking = 0;				 /* whether -c specified on

						  * 
						  * * commandline */

#ifdef META
int     usemeta = 0;
#define DEFAULT_METATYPE 1

#endif

int     cowmain(char *server, int port, char *name)
{
  int     team, s_type;

  char   *cp;
  char    buf[80];
  struct passwd *pwent;

  int     i;

  i = setjmp(env);				 /* Error while initializing */
  if (i >= RETURNBASE)
    return i - RETURNBASE;			 /* Terminate with retcode */

  pseudo[0] = defpasswd[0] = '\0';

#ifdef RECORDGAME
  if (recordFileName != NULL)
    {
      recordFile = fopen(recordFileName, "wb");
      if (recordFile == NULL)
	{
	  perror(recordFileName);
	  return 1;

	}
    }
#endif

  if (logFileName != NULL)
    {
      logFile = fopen(logFileName, "a");
      if (logFile == NULL)
	{
	  perror(logFileName);
	  return 1;
	}
    }
  for (i = 0; i < 80; i++)
    {
      outmessage[i] = '\0';
    }

#ifndef DEBUG
  (void) SIGNAL(SIGFPE, handle_exception);
#endif

  SRANDOM(time(0));

  initDefaults(deffile);
  identityBlind = booleanDefault("identityBlind", identityBlind);
  versionHide = booleanDefault("versionHide", versionHide);

  xtrekPort = port;
  if (server)
    {
      if (xtrekPort < 0)
	{
	  sprintf(defaulttmp, "port.%s", server);
	  xtrekPort = intDefault(defaulttmp, -1);
	  if (checking)
	    xtrekPort = xtrekPort - 1;
	}
      sprintf(defaulttmp, "server.%s", server);	 /* check for "abbreviation" */
      serverName = getdefault(defaulttmp);
      if (!serverName)				 /* no abbreviation found */
	serverName = server;
    }

  SRANDOM(getpid() * time(NULL));

  if (!passive)
    {
      if (!serverName)
	serverName = getdefault("server");

      if (!serverName)

#ifdef META
	if (!usemeta)
	  {
	    fprintf(stderr, "using metaserver\n");
	    usemeta = 1;
	  }
#else
      printf("No server name was given. Please put a default server in\n");
      printf("your .xtrekrc file or specify the server in the command line.\n");
      terminate(1);
#endif
    }

  if (xtrekPort < 0)
    {
      xtrekPort = intDefault("port", DEFAULT_PORT);
      if (checking)
	xtrekPort = xtrekPort - 1;
    }
  if (checking)
    check();

  if (!logFileName)
    logFileName = getdefault("logfile");
  if (logFileName != NULL)
    {
      logFile = fopen(logFileName, "a");
      if (logFile == NULL)
	{
	  perror(logFileName);
	  return 1;
	}
    }

#ifdef META
  if (usemeta)
    {
      newwinmeta(display_host, name);
      parsemeta();
      metawindow();
      metainput(); /* returns on quit, may fork(2) */
      W_Deinitialize();
      exit(0);
    }
#endif

  newwin(display_host, name);
  resetdefaults();

#if defined(SOUND) && (defined(HAVE_SDL) || defined(sgi))
    Init_Sound();
#endif

  if (censorMessages)
    initCensoring();

  /* open memory...? */
  openmem();

  cp = getdefault("login");
  if (cp == NULL) {
    if ((pwent = getpwuid(getuid())) != NULL)
      cp = pwent->pw_name;
    else
      cp = "Bozo";
  }
  (void) STRNCPY(login, cp, sizeof(login));
  login[sizeof(login) - 1] = '\0';

  if (fastGuest) strcpy(pseudo, "guest");
  else if (pseudo[0] == '\0')
    {
      if ((cp = getdefault("name")) != 0)
	(void) STRNCPY(pseudo, cp, sizeof(pseudo));
      else
	(void) STRNCPY(pseudo, login, sizeof(pseudo));
    }
  pseudo[sizeof(pseudo) - 1] = '\0';

  if (fastGuest) strcpy(defpasswd, "guest");
  else if (defpasswd[0] == '\0')
    if ((cp = getdefault("password")) != 0)
      (void) STRNCPY(defpasswd, cp, sizeof(defpasswd));
  defpasswd[sizeof(defpasswd) - 1] = '\0';

  if (!passive)
    {
      callServer(xtrekPort, serverName);
    }
  else
    {
      connectToServer(xtrekPort);
    }

#ifdef FEATURE_PACKETS
  sendFeature("FEATURE_PACKETS", 'S', 1, 0, 0);
#endif

  findslot();

  mapAll();

#ifndef RWATCH
  getname(pseudo, defpasswd);
#else
  MZERO(mystats, sizeof(mystats));
  mystats->st_tticks = 1;
  mystats->st_flags = ST_MAPMODE + ST_NAMEMODE + ST_SHOWSHIELDS +
      ST_KEEPPEACE + ST_SHOWLOCAL * 2 + ST_SHOWGLOBAL * 2;
#endif /* RWATCH */

  loggedIn = 1;
  phaserWindow = booleanDefault("phaserWindow", phaserWindow);

#ifdef AUTOKEY
  /* autokey.c */
  autoKeyDefaults();
#endif /* AUTOKEY */

  initkeymap();
  sendOptionsPacket();

  /* Set p_hostile to hostile, so if keeppeace is on, the guy starts off * *
   * hating everyone (like a good fighter should) */
  me->p_hostile = (FED | ROM | KLI | ORI);

  if (!newDashboard)
    {
      sprintf(buf,
	   "Maximum:      %2d  %3d %3d               %3d   %6d   %3d   %3d",
	      0, 0, 0, 0, 0, 0, 0);
      W_WriteText(tstatw, 50, 27, textColor, buf, strlen(buf), W_RegularFont);
    }
  me->p_planets = 0;
  me->p_genoplanets = 0;
  me->p_armsbomb = 0;
  me->p_genoarmsbomb = 0;
  /* Set up a reasonable default */

#ifndef RWATCH
  me->p_whydead = KQUIT;
  me->p_team = ALLTEAM;
  s_type = CRUISER;
#endif /* RWATCH */

  startPing();					 /* support ping stuff */

#ifdef AUTOKEY
  if (autoKey)
    {
      /* XX: changes entire state of display */
      W_AutoRepeatOff();
    }
#endif

  /* Moved SDL sound initialization to right after readdefaults() so
   * the intro can start playing ASAP 
   */
#if defined(SOUND) && !defined(HAVE_SDL) && !defined(sgi)
  Init_Sound();
#endif

  isFirstEntry = 1;				 /* First entry into game */

  i = setjmp(env);				 /* Reentry point of game */
  if (i >= RETURNBASE) {
    W_FullScreenOff(baseWin);
    W_Flush();
    W_DestroyWindow(baseWin);
    W_Deinitialize();
    return i - RETURNBASE;			 /* Terminate with retcode */
  }


#ifdef IGNORE_SIGNALS_SEGV_BUS
  if (ignore_signals)
    {
      (void) SIGNAL(SIGBUS, reset_game);
      (void) SIGNAL(SIGSEGV, reset_game);
    }

  if (died_from_signal)
    {
      died_from_signal = 0;
      input();
    }
  else
    {
#endif

#if defined(SOUND)
#if defined(sgi)
      Engine_Sound(ENG_OFF);			/* turn off engine sound */
#else
      /* text in sound.c:soundrefresh() says engine sound is not supported
      Abort_Sound(ENGINE_SOUND); */
#endif
#endif

      /* give the player the motd and find out which team he wants */

#ifndef RWATCH
      /* check if this dude is trying to resume a ghostbust slot */
      if (ghoststart)
	{
	  char   *getteam = "IFR K   O";
	  int     i;

	  ghoststart = 0;

	  for (i = 0; i < 9; i++)
	    if (getteam[i] == me->p_mapchars[0])
	      break;

	  me->p_team = i;

	  if (me->p_damage > me->p_ship.s_maxdamage)
	    {
	      me->p_status = POUTFIT;
	      entrywindow(&team, &s_type);
	    }
	  else
	    me->p_status = PALIVE;

	}
      else
	{
	  /* give the player the motd and find out which team he wants */
	  entrywindow(&team, &s_type);
	}


      if (team == -1)
	{
	  W_FullScreenOff(baseWin);
	  W_Flush();
	  W_DestroyWindow(baseWin);

#ifdef AUTOKEY
	  if (autoKey)
	    W_AutoRepeatOn();
#endif

	  sendByeReq();

	  fprintf(stderr, "you quit\n");

#if defined(SOUND)
	  Exit_Sound();
#endif
	  if (logFile != NULL)
	    fclose(logFile);

#ifdef PACKET_LOG
	  if (log_packets)
	    Dump_Packet_Log_Info();
#endif
	  W_Deinitialize();
	  return 0;


	}
#endif /* RWATCH */

      {
	char    buf[80];
	if (me->p_stats.st_rank < nranks) {
	  sprintf(buf, "Welcome aboard %s!", ranks[me->p_stats.st_rank].name);
	} else {
	  sprintf(buf, "Welcome aboard!");
	}
	warning(buf);
	W_ClearArea(messagew, 5, 5, W_Textwidth * 80, W_Textheight);
      }

      sendVersion();
      getship(myship, myship->s_type);

      redrawall = 2;

      shipchange(s_type);
      enter();
      calibrate_stats();
      W_ClearWindow(w);
      /* for (i = 0; i < NSIG; i++) { (void) SIGNAL(i, SIG_IGN); } */

      me->p_flags &= ~(PFYELLOW | PFRED | PFENG);	/* Reset flags to avoid sounds */
      me->p_flags |= PFGREEN | PFSHIELD;		/* ... from previous alerts */
      me->p_status = PALIVE;				/* Put player in game */

      PlistNoteUpdate(me->p_no);

      if (showStats)				 /* Default showstats are on. 
						  * 
						  */
	W_MapWindow(statwin);

      if (W_IsMapped(lMeter))
	redrawLMeter();

      if (W_IsMapped(pStats))			 /* support ping stuff */
	redrawPStats();

      if (isFirstEntry)
	{
	  if (tryUdp && commMode != COMM_UDP)
	    sendUdpReq(COMM_UDP);

#ifdef SHORT_PACKETS				 /* should we be checking for
						  * * * udp on here? */
	  if (tryShort)
	    sendShortReq(SPK_VON);
	  else
	    {
	      /* 
	       * If tryShort is true, we get an automatic `-' style update
	       * when the short packets kick in, so this is not necessary. */

	      sendUdpReq(COMM_UPDATE);
	    }
#else
	  /* `=' style update to get the kills in the playerlist right */
	  sendUdpReq(COMM_UPDATE);
#endif

	  isFirstEntry = 0;
	}

      DisplayMessage();

#ifdef SOUND
#if defined(sgi)
	Engine_Sound(ENG_ON);
#else
      /* text in sound.c:soundrefresh() says engine sound is not supported
      Play_Sound(ENGINE_SOUND); */
#endif

      Play_Sound(ENTER_SHIP_SOUND);
#endif

#ifdef HOCKEY_LINES
      init_hockey_lines();
#endif

      /* Get input until the player quits or dies */
      /* on death(), we longjmp to setjmp above */
      input();

#ifdef IGNORE_SIGNALS_SEGV_BUS
    }
#endif
  return 0;
}

void
handle_exception(int _dummy)
{
  printf("floating point exception error detected; attempting to continue\n");

#ifdef WIN32
  /* Under Watcom C++, after a signal handler is called the signal reverts to 
   * 
   * * SIG_DFL so has to be reset... is this standard? */
  (void) SIGNAL(SIGFPE, handle_exception);
#endif
}

#ifdef IGNORE_SIGNALS_SEGV_BUS
void
reset_game(int _dummy)
{
  died_from_signal = 1;
  longjmp(env, 0);
}
#endif

void    terminate(int error)
{
#ifdef RECORDGAME
  if (recordFile)
    fclose(recordFile);
#endif

  longjmp(env, RETURNBASE + error);
}
