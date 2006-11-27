
/* getname.c
 * 
 * Kevin P. Smith 09/28/88
 *
 * $Log: getname.c,v $
 * Revision 1.4  2006/09/19 10:20:39  quozl
 * ut06 full screen, det circle, quit on motd, add icon, add desktop file
 *
 * Revision 1.3  2001/04/28 04:06:15  quozl
 * If server rejects guest login, allow the user to retry with a real name.
 * Current INL servers are coded to reject guest login.  Having to restart
 * the client is unnecessary.
 *
 * Revision 1.2  1999/07/29 23:25:51  cameron
 * fix typo
 *
 * Revision 1.1.1.1  1998/11/01 17:24:09  siegl
 * COW 3.0 initial revision
 * */
#include "config.h"
#include "copyright2.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/time.h>
#include <errno.h>
#include <pwd.h>
#include <string.h>

#include INC_SYS_SELECT

#include <ctype.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"

static char tempname[16];
static char password1[16];
static char password2[16];
static int state, autolog;
static char *alf = NULL;

#define SIZEOF(a)       (sizeof (a) / sizeof (*(a)))
#define ST_GETNAME 0
#define ST_GETPASS 1
#define ST_MAKEPASS1 2
#define ST_MAKEPASS2 3
#define ST_DONE 4

void    loginproced(char, char *);
void    adjustString(char, char *, char *);
extern void terminate(int error);

noautologin(void)
{
  autolog = 0;
  *defpasswd = *password1 = *password2 = '\0';
  alf = "Automatic login failed";
  W_WriteText(w, 100, 130, textColor, alf, strlen(alf),
	      W_BoldFont);

}

static int
        handleWEvents(char *defname)
{
  int     do_redraw = 0;
  char    ch;
  W_Event event;

  while (W_EventsPending())
    {
      W_NextEvent(&event);
      switch ((int) event.type)
	{
	case W_EV_EXPOSE:
	  if (event.Window == w)
	    do_redraw = 1;
	  break;
	case W_EV_KEY:
	  ch = event.key;
	  if (!autolog)
	    loginproced(ch, defname);
	}
    }

  if (do_redraw)
    {
      displayStartup(defname);
      showreadme();
    }
}


getname(char *defname, char *defpasswd)

/* Let person identify themselves from w */
{
  register char ch;
  int     secondsLeft = 199, laststate;
  char    tempstr[40];
  LONG    lasttime;
  char   *namptr, *passptr;
  register int j;
  struct timeval timeout;
  fd_set  readfds;

  autolog = (*defpasswd && *defname) ? 1 : 0;

  MZERO(mystats, sizeof(struct stats));

  mystats->st_tticks = 1;
  for (j = 0; j < 95; j++)
    {
      mystats->st_keymap[j] = j + 32;
      mystats->st_keymap[j + 96] = j + 32 + 96;

#ifdef MOUSE_AS_SHIFT
      mystats->st_keymap[j + 192] = j + 32;
      mystats->st_keymap[j + 288] = j + 32;
      mystats->st_keymap[j + 384] = j + 32;
#endif
    }
  mystats->st_keymap[95] = 0;
  mystats->st_flags = ST_MAPMODE + ST_NAMEMODE + ST_SHOWSHIELDS +
      ST_KEEPPEACE + ST_SHOWLOCAL * 2 + ST_SHOWGLOBAL * 2;

  lasttime = time(NULL);

  if (ghoststart)
    return;

  tempname[0] = '\0';
  password1[0] = '\0';
  password2[0] = '\0';

  laststate = state = ST_GETNAME;
  displayStartup(defname);
  while (1)
    {
      handleWEvents(defname);

      if (!autolog)
	{

#ifndef HAVE_WIN32
	  W_FullScreen(baseWin);
	  timeout.tv_sec = 1;
	  timeout.tv_usec = 0;
#else
	  /* Since we don't have a socket to check on Win32 for windowing *
	   * system events, we set the timeout to zero and effectively poll.
	   * * Yes, I could do the correct thing and call *
	   * WaitForMultipleObjects() etc. but I don't feel like it */
	  timeout.tv_sec = 0;
	  timeout.tv_usec = 100000;
#endif

	  FD_ZERO(&readfds);
	  FD_SET(sock, &readfds);
	  if (udpSock >= 0)
	    FD_SET(udpSock, &readfds);

#ifndef HAVE_WIN32
	  FD_SET(W_Socket(), &readfds);
#endif

	  if (SELECT(32, &readfds, 0, 0, &timeout) < 0)
	    {
	      perror("select");
	      continue;
	    }

	  if (FD_ISSET(sock, &readfds)
	      || (udpSock >= 0 && FD_ISSET(udpSock, &readfds)))
	    readFromServer(&readfds);

#ifndef HAVE_WIN32
	  if (FD_ISSET(W_Socket(), &readfds))
#else
	  if (W_EventsPending())
#endif

	    handleWEvents(defname);
	}
      else
	{
	  readFromServer(&readfds);
	}

      if (isServerDead())
	{
	  printf("Shit, we were ghostbusted\n");

#ifdef HAVE_XPM
	  W_GalacticBgd(GHOST_PIX);
#endif

#ifdef AUTOKEY
	  if (autoKey)
	    W_AutoRepeatOn();
#endif

	  terminate(0);
	}

      if (time(0) != lasttime)
	{
	  lasttime++;
	  secondsLeft--;
	  showreadme();
	  if (!autolog)
	    {
	      sprintf(tempstr, "Seconds to go: %d ", secondsLeft);
	      W_WriteText(w, 100, 400, textColor, tempstr, strlen(tempstr),
			  W_RegularFont);
	    }
	  if (secondsLeft == 0)
	    {
	      me->p_status = PFREE;
	      printf("Timed Out.\n");

#ifdef AUTOKEY
	      if (autoKey)
		W_AutoRepeatOn();
#endif

	      terminate(0);
	    }
	}
      if (state == ST_DONE)
	{
	  W_ClearWindow(w);
	  W_ClearWindow(mapw);
	  return;
	}
      if (autolog)
	{
	  switch (state)
	    {
	    case ST_GETNAME:
	      tempname[0] = '\0';
	      ch = 13;
	      j = 0;
	      break;

	    case ST_GETPASS:
	    case ST_MAKEPASS1:
	    case ST_MAKEPASS2:
	      ch = defpasswd[j++];
	      if (ch == '\0')
		{
		  j = 0;
		  ch = 13;
		}
	      break;

	    default:
	      break;
	    }

	  loginproced(ch, defname);

	}

      laststate = state;
    }
}

void
        loginproced(char ch, char *defname)
{

  if (ch == 10)
    ch = 13;

#ifdef CONTROL_KEY
  if ((ch == 4 || ch == ((char) ('d' + 96)) || ch == ((char) ('D' + 96))) && state == ST_GETNAME && *tempname == '\0')
#else
  if (ch == 4 && state == ST_GETNAME && *tempname == '\0')
#endif

    {

#ifdef AUTOKEY
      if (autoKey)
	W_AutoRepeatOn();
#endif

      terminate(0);
    }
  if (ch < 32 && ch != 21 && ch != 13 && ch != 8)
    return;
  switch (state)
    {
    case ST_GETNAME:
      if (ch == 13)
	{
	  if (*tempname == '\0')
	    {
	      STRNCPY(tempname, defname, sizeof(tempname));
	    }
	  loaddude();
	  displayStartup(defname);
	}
      else
	{
	  adjustString(ch, tempname, defname);
	}
      break;
    case ST_GETPASS:
      if (ch == 13)
	{
	  checkpassword();
	  displayStartup(defname);
	}
      else
	{
	  adjustString(ch, password1, defname);
	}
      break;
    case ST_MAKEPASS1:
      if (ch == 13)
	{
	  state = ST_MAKEPASS2;
	  displayStartup(defname);
	}
      else
	{
	  adjustString(ch, password1, defname);
	}
      break;
    case ST_MAKEPASS2:
      if (ch == 13)
	{
	  makeNewGuy();
	  displayStartup(defname);
	}
      else
	{
	  adjustString(ch, password2, defname);
	}
      break;
    }

  return;

}

loaddude(void)
{
  char    ppwd[16];

  STRNCPY(ppwd, "\0\0\0", 4);
  if (strncmp(tempname, "Guest", 5) == 0 || strncmp(tempname, "guest", 5) == 0)
    {
      loginAccept = -1;
      sendLoginReq(tempname, ppwd, login, 0);
      state = ST_DONE;
      me->p_pos = -1;
      me->p_stats.st_tticks = 1;		 /* prevent overflow */
      STRNCPY(me->p_name, tempname, sizeof(tempname));
      while (loginAccept == -1)
	{
	  socketPause();
	  readFromServer(NULL);
	  if (isServerDead())
	    {
	      printf("Server is hosed.\n");

#ifdef AUTOKEY
	      if (autoKey)
		W_AutoRepeatOn();
#endif

	      terminate(0);
	    }
	}
      if (loginAccept == 0)
	{
	  char *s = "Server refuses guest login, use another name.";
	  W_WriteText(w, 100, 70, textColor, s, strlen(s), W_BoldFont);
	  (void) W_EventsPending();
	  sleep(3);
	  W_ClearWindow(w);
	  state = ST_GETNAME;
	  *tempname = 0;
#ifdef AUTOKEY
	  if (autoKey)
	    W_AutoRepeatOn();
#endif
	}
      return;
    }
  /* Ask about the user */
  loginAccept = -1;
  sendLoginReq(tempname, ppwd, login, 1);
  while (loginAccept == -1)
    {
      socketPause();
      readFromServer(NULL);
      if (isServerDead())
	{
	  printf("Server is hosed.\n");

#ifdef AUTOKEY
	  if (autoKey)
	    W_AutoRepeatOn();
#endif

	  terminate(0);
	}
    }
  *password1 = *password2 = 0;
  if (loginAccept == 0)
    {
      state = ST_MAKEPASS1;
    }
  else
    {
      state = ST_GETPASS;
    }
}

checkpassword(void)
/* Check dude's password. If he is ok, move to state ST_DONE. */
{
  char   *s;

  sendLoginReq(tempname, password1, login, 0);
  loginAccept = -1;
  while (loginAccept == -1)
    {
      socketPause();
      readFromServer(NULL);
      if (isServerDead())
	{
	  printf("Server is hosed.\n");

#ifdef AUTOKEY
	  if (autoKey)
	    W_AutoRepeatOn();
#endif

	  terminate(0);
	}
    }
  if (loginAccept == 0)
    {
      if (!autolog)
	{
	  s = "Bad password!";
	  W_WriteText(w, 100, 100, textColor, s, strlen(s), W_BoldFont);
	  (void) W_EventsPending();
	  sleep(3);
	  W_ClearWindow(w);
	}
      else
	noautologin();
      *tempname = 0;
      state = ST_GETNAME;
      return;
    }
  STRNCPY(me->p_name, tempname, sizeof(tempname));
  keeppeace = (me->p_stats.st_flags / ST_KEEPPEACE) & 1;
  state = ST_DONE;
}

makeNewGuy(void)
/* Make the dude with name tempname and password password1. Move to state
 * ST_DONE. */
{
  char   *s;

  if (strcmp(password1, password2) != 0)
    {
      if (!autolog)
	{
	  s = "Passwords do not match";
	  W_WriteText(w, 100, 130, textColor, s, strlen(s), W_BoldFont);
	  (void) W_EventsPending();
	  sleep(3);
	  W_ClearWindow(w);
	}
      else
	noautologin();
      *tempname = 0;
      state = ST_GETNAME;
      return;
    }

  /* same routine! */
  checkpassword();
}

void
        adjustString(char ch, char *str, char *defname)
{
  int     strLen;

  strLen = strlen(str);

  if (ch == 21)
    {
      *str = '\0';
      if (state == ST_GETNAME)
	displayStartup(defname);
    }
  else if (ch == 8 || ch == '\177')
    {
      if (strLen > 0)
	{
	  str[strLen - 1] = '\0';
	  if (state == ST_GETNAME)
	    displayStartup(defname);
	}
    }
  else
    {
      if (strLen == 15)
	return;
      str[strLen + 1] = '\0';
      str[strLen] = ch;
      if (state == ST_GETNAME)
	displayStartup(defname);
    }
}

displayStartup(char *defname)

/* Draws entry screen based upon state. */
{
  char    buf[100];
  char    s[100];
  register char *t;

  if (state == ST_DONE || autolog)
    return;

  if (alf != NULL)
    W_WriteText(w, 100, 130, textColor, alf, strlen(alf), W_BoldFont);
  t = "Welcome to Netrek.";
  W_WriteText(w, 100, 10, textColor, t, strlen(t), W_RegularFont);
  sprintf(buf, "Connected to server %s", serverName);
  t = buf;
  W_WriteText(w, 100, 20, textColor, t, strlen(t), W_RegularFont);
  t = "Keep your mouse in this window to type.";
  W_WriteText(w, 100, 30, textColor, t, strlen(t), W_RegularFont);
  t = "Press Control/D to quit at this point, but use Shift/Q later.";
  W_WriteText(w, 100, 40, textColor, t, strlen(t), W_RegularFont);
  sprintf(s, "What is your name? [default is %s]: %s               ", defname, tempname)
      ;
  W_WriteText(w, 100, 50, textColor, s, strlen(s), W_RegularFont);
  if (state == ST_GETPASS)
    {
      alf = NULL;
      t = "What is your password? : ";
      W_WriteText(w, 100, 60, textColor, t, strlen(t), W_RegularFont);
    }
  if (state > ST_GETPASS)
    {
      alf = NULL;
      t = "You need to make a password.";
      W_WriteText(w, 100, 70, textColor, t, strlen(t), W_BoldFont);
      t = "So think of a password you can remember, and enter it.";
      W_WriteText(w, 100, 80, textColor, t, strlen(t), W_RegularFont);
      t = "What is your password? :";
      W_WriteText(w, 100, 90, textColor, t, strlen(t), W_RegularFont);
    }
  if (state == ST_MAKEPASS2)
    {
      alf = NULL;
      t = "Enter it again to make sure you typed it right.";
      W_WriteText(w, 100, 110, textColor, t, strlen(t), W_BoldFont);
      t = "Your password? :";
      W_WriteText(w, 100, 120, textColor, t, strlen(t), W_RegularFont);
    }
}

noserver(void)
{
  printf("No server name was given. Please put a default server in\n");
  printf("your .xtrekrc file or specify the server in the command line.\n");
  terminate(1);
}

showreadme(void)
{
  static char *README[] =
  {
    "",
  };
  int     i, length;


  for (i = 0; i < SIZEOF(README); i++)
    {
      length = strlen(README[i]);

      W_WriteText(w, 20, i * W_Textheight + 180, textColor, README[i],
		  length, W_RegularFont);
    }
}
