/* getname.c
 * 
 * Kevin P. Smith 09/28/88
 * Rewrite by James Cameron, 2008-07-28.
 */
#include "config.h"
#include "copyright2.h"

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include INC_SYS_SELECT

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"

#include "myf.h"
#include "socket.h"

#include "getname.h"

static char *n_def;
static char n_buf[16];
static char p_buf_a[16];
static char p_buf_b[16];
static int state;
static int automatic;
static char *err = NULL;
static int time_left = 199;
static int time_error_ends;

#define SIZEOF(a)       (sizeof (a) / sizeof (*(a)))
#define ST_GETNAME      0 /* asking user for name */
#define ST_TX_GUEST     1 /* asking server for access by guest */
#define ST_RX_GUEST     2
#define ST_TX_NAME      3 /* asking server for access by name */
#define ST_RX_NAME      4 
#define ST_GETPASS      5 /* asking user for password after ST_PROBE_NAME */
#define ST_MAKEPASS1    6 /* asking user for a new password */
#define ST_MAKEPASS2    7 /* asking user to confirm password */
#define ST_TX_LOGIN     8 /* asking server for access by name and password */
#define ST_RX_LOGIN     9
#define ST_DONE         10 /* completed */
#define ST_DISCONNECTED 11 /* disconnected */
#define ST_ERROR_PAUSE  12 /* pause for error display */

#define ERROR_PAUSE_SECONDS 3

#define X_L 90 /* X left edge, how far in to begin text writing */

extern void terminate(int error);


static char *asterisks(char *password)
{
  static char buf[17];
  int len;

  strcpy(buf, "****************");
  len = strlen(password);
  if (len > strlen(buf))
    len = strlen(buf);
  buf[len] = '\0';
  return buf;
}


static void redraw_readme(void)
{
  static char *README[] =
  {
    "--                                             --",
    "",
    "netrek-client-cow maintainer: quozl@us.netrek.org",
    "bug reports welcome",
    "",
    "--                                             --",
  };
  int i;

  for (i=0; i<SIZEOF(README); i++)
    myf(X_L, i * W_Textheight + 180, W_Cyan, W_RegularFont, "%s", README[i]);
}


static void redraw_time_left()
{
  if (automatic) return;
  myf(X_L, 400, W_Grey, W_RegularFont, "Seconds to go: %d ", time_left);
}


static void redraw()
{
  char *pad = "                ";

  if (state == ST_DONE) return;

  if (automatic) return;

  if (state == ST_DISCONNECTED) {
    W_ClearWindow(w);
    myf(X_L, TWINSIDE/2, W_Red, W_BoldFont,
        "Connection to server lost, press enter to quit.");
    return;
  }

  myf(X_L, 10, W_White, W_RegularFont, "Welcome to Netrek.");
  myf(X_L, 20, W_Grey, W_RegularFont, "Connected to server %s.", serverName);

  if (state == ST_GETNAME) {
    myf(X_L, 50, W_Green, W_RegularFont,
        "What is your name? : %s%s", strlen(n_buf) == 0 ? n_def : n_buf, pad);
    if (xtrekPort == DEFAULT_PORT) {
      myf(X_L, 70, W_Grey, W_RegularFont,
          "Use guest if you do not want to have your statistics saved.");
      myf(X_L, 90, W_Grey, W_RegularFont,
          "Servers keep statistics by name.");
      myf(X_L, 100, W_Grey, W_RegularFont,
          "This is the name the server will remember you by.");
      myf(X_L, 110, W_Grey, W_RegularFont,
          "You may want to use an alias, most players do.");
      myf(X_L, 120, W_Grey, W_RegularFont,
          "If the name is known, the server will ask for your password.");
      myf(X_L, 130, W_Grey, W_RegularFont,
          "If not an account is created and you get to set a password.");
    }
  } else if (state == ST_GETPASS) {
    myf(X_L, 50, W_Grey, W_RegularFont,
        "What is your name? : %s%s", n_buf, pad);
    myf(X_L, 60, W_Green, W_RegularFont,
        "What is your password? : %s%s", asterisks(p_buf_a), pad);
    myc(X_L, 70);
    myc(X_L, 90);
    myc(X_L, 100);
    myc(X_L, 110);
    myc(X_L, 120);
    myc(X_L, 130);
  } else if (state == ST_MAKEPASS1 || state == ST_MAKEPASS2) {
    myf(X_L, 50, W_Grey, W_RegularFont,
        "What is your name? : %s%s", n_buf, pad);
    myc(X_L, 70);
    myf(X_L, 70, W_Grey, W_BoldFont,
        "Name not known to server, so it will make one.");
    myf(X_L, 80, W_Grey, W_RegularFont,
        "Think of a password you can remember, and enter it.");
    myf(X_L, 90, W_Green, W_RegularFont,
        "What is your password? : %s%s", asterisks(p_buf_a), pad);
    myc(X_L, 100);
    myc(X_L, 110);
    myc(X_L, 120);
    myc(X_L, 130);
  }

  if (state == ST_MAKEPASS2) {
    myf(X_L, 110, W_Grey, W_BoldFont,
        "Enter it again to make sure you typed it right.");
    myf(X_L, 120, W_Green, W_RegularFont,
        "Your password? : %s%s", asterisks(p_buf_b), pad);
  }

// FIXME: focus green highlight shows up still in second password
// prompt, and during errors.

  if (err != NULL) {
    myf(X_L, 130, W_Red, W_BoldFont, err);
  } else {
    myc(X_L, 130);
  }

  switch (state) {
  case ST_RX_GUEST:
  case ST_RX_NAME:
  case ST_RX_LOGIN:
    myf(X_L, 140, W_Yellow, W_BoldFont, "(server wait)");
    break;
  default:
    myc(X_L, 140);
  }
#ifdef DEBUG
  myf(X_L, 150, W_Yellow, W_BoldFont, "client state %d", state);
#endif

  redraw_readme();
}


static void error_pause(char *msg)
{
  time_error_ends = time_left - ERROR_PAUSE_SECONDS;
  err = msg;
  state = ST_ERROR_PAUSE;
}


static void automatic_failed()
{
  automatic = 0;
  *defpasswd = *p_buf_a = *p_buf_b = '\0';
  err = "Automatic login failed";
  W_WriteText(w, X_L, 130, W_Red, err, strlen(err), W_BoldFont);
}


static void key_to_buffer(char ch, char *str)
{
  int     strLen;

  strLen = strlen(str);

  if (ch == 21) {
    *str = '\0';
    redraw();
  } else if (ch == 8 || ch == '\177') {
    if (strLen > 0) {
      str[strLen - 1] = '\0';
      redraw();
    }
  } else {
    if (strLen == 15)
      return;
    str[strLen + 1] = '\0';
    str[strLen] = ch;
    redraw();
  }
}


static void key(char ch)
{
  if (ch == 10) ch = 13;

#ifdef CONTROL_KEY
  if ((ch == 4 || ch == ((char) ('d' + 96)) || ch == ((char) ('D' + 96))) && state == ST_GETNAME && *n_buf == '\0')
#else
  if (ch == 4 && state == ST_GETNAME && *n_buf == '\0')
#endif

    {
      terminate(0);
    }
  if (ch < 32 && ch != 21 && ch != 13 && ch != 8) return;
  switch (state) {
  case ST_GETNAME:
    if (ch == 13) {
      err = NULL;
      if (*n_buf == '\0') 
        STRNCPY(n_buf, n_def, sizeof(n_buf));
      if (strncmp(n_buf, "Guest", 5) == 0 ||
          strncmp(n_buf, "guest", 5) == 0) {
        state = ST_TX_GUEST;
      } else {
        state = ST_TX_NAME;
      }
      redraw();
    } else {
      key_to_buffer(ch, n_buf);
    }
    break;
  case ST_GETPASS:
    if (ch == 13) {
      err = NULL;
      state = ST_TX_LOGIN;
      redraw();
    } else {
      key_to_buffer(ch, p_buf_a);
    }
    break;
  case ST_MAKEPASS1:
    if (ch == 13) {
      err = NULL;
      state = ST_MAKEPASS2;
      redraw();
    } else {
      key_to_buffer(ch, p_buf_a);
    }
    break;
  case ST_MAKEPASS2:
    if (ch == 13) {
      err = NULL;
      if (strcmp(p_buf_a, p_buf_b) != 0) {
        if (!automatic) {
          error_pause("Passwords do not match, starting again!");
        } else {
          automatic_failed();
          state = ST_GETNAME;
        }
        *n_buf = 0;
      } else {
        state = ST_TX_LOGIN;
      }
      redraw();
    } else {
      key_to_buffer(ch, p_buf_b);
    }
    break;
  case ST_DISCONNECTED:
    if (ch == 13) {
      terminate(EXIT_LOGIN_FAILURE);
    }
    /* fall through */
  default:
    W_Beep();
    break;
  }
  
  return;
}

static void events()
{
  int     do_redraw = 0;
  char    ch;
  W_Event event;

  while (W_EventsPending()) {
    W_NextEvent(&event);
    switch (event.type) {
    case W_EV_EXPOSE:
      if (event.Window == w)
        do_redraw = 1;
      break;
    case W_EV_KEY:
      ch = event.key;
      if (!automatic) key(ch);
      break;
    case W_EV_CLOSED:
      if (event.Window == baseWin) {
        fprintf(stderr, "you quit, by closing the login window\n");
        terminate(0);
      }
      break;
    }
  }
  
  if (do_redraw) {
    redraw();
    redraw_time_left();
  }
}

static void mystats_init()
{
  int j;
  MZERO(mystats, sizeof(struct stats));

  mystats->st_tticks = 1;
  for (j = 0; j < 95; j++) {
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
}

/* Let person identify themselves from w */
void getname(char *defname, char *defpasswd)
{
  char ch;
  time_t lasttime;
  int j;
  struct timeval timeout;
  fd_set  readfds;

  char    ppwd[16];

  STRNCPY(ppwd, "\0\0\0", 4);

  automatic = (*defpasswd && *defname) ? 1 : 0;
  n_def = defname;
  mystats_init();

  lasttime = time(NULL);

  if (ghoststart) return;

  n_buf[0] = '\0';
  p_buf_a[0] = '\0';
  p_buf_b[0] = '\0';

  state = ST_GETNAME;
  redraw();
  while (1) {

    switch (state) {
    case ST_TX_GUEST: /* asking server for access by guest */
      loginAccept = -1;
      sendLoginReq(n_buf, ppwd, login, 0);
      state = ST_RX_GUEST;
      redraw();
      break;
    case ST_TX_NAME: /* asking server for access by name */
      loginAccept = -1;
      sendLoginReq(n_buf, ppwd, login, 1);
      state = ST_RX_NAME;
      redraw();
      break;
    case ST_TX_LOGIN: /* asking server for access by name and password */
      loginAccept = -1;
      sendLoginReq(n_buf, p_buf_a, login, 0);
      state = ST_RX_LOGIN;
      redraw();
      break;
    }

    events(0);
    
    if (automatic) {
      timeout.tv_sec = 0;
      timeout.tv_usec = 1000;
    } else {
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;
    }

    FD_ZERO(&readfds);
    if (!isServerDead()) {
      FD_SET(sock, &readfds);
      if (udpSock >= 0)
        FD_SET(udpSock, &readfds);
    }
    FD_SET(W_Socket(), &readfds);

    if (SELECT(32, &readfds, 0, 0, &timeout) < 0) {
      perror("select");
      sleep(1);
      continue;
    }
    
    if (!isServerDead()) {
      if (FD_ISSET(sock, &readfds)
          || (udpSock >= 0 && FD_ISSET(udpSock, &readfds)))
        readFromServer(&readfds);
    }

    if (isServerDead()) {
      state = ST_DISCONNECTED;
      redraw();
    }

    switch (state) {
    case ST_RX_GUEST: /* asking server for access by guest */
      if (loginAccept != -1) {
        if (loginAccept == 0) {
          error_pause("Server refuses guest login, use another name.");
          fastGuest = 0;
          *n_buf = 0;
        } else {
          me->p_pos = -1;
          me->p_stats.st_tticks = 1;             /* prevent overflow */
          STRNCPY(me->p_name, n_buf, sizeof(n_buf));
          state = ST_DONE;
        }
        redraw();
      }
      break;
    case ST_RX_NAME: /* asking server for access by name */
      if (loginAccept != -1) {
        *p_buf_a = *p_buf_b = 0;
        if (loginAccept == 0) {
          state = ST_MAKEPASS1;
        } else {
          state = ST_GETPASS;
        }
        redraw();
      }
      break;
    case ST_RX_LOGIN: /* asking server for access by name and password */
      if (loginAccept != -1) {
        if (loginAccept == 0) {
          if (!automatic) {
            error_pause("Bad password!");
          } else {
            automatic_failed();
            state = ST_GETNAME;
          }
          *n_buf = 0;
        } else {
          STRNCPY(me->p_name, n_buf, sizeof(n_buf));
          keeppeace = (me->p_stats.st_flags / ST_KEEPPEACE) & 1;
          state = ST_DONE;
        }
        redraw();
      }
      break;
    }

    if (FD_ISSET(W_Socket(), &readfds))
      while (W_EventsQueuedCk()) events(0);

    if (time(0) != lasttime) {
      lasttime++;
      time_left--;
      redraw_time_left();
      if (time_left == 0) {
        me->p_status = PFREE;
        printf("Timed Out.\n");
        terminate(0);
      }
      switch (state) {
      case ST_ERROR_PAUSE:
        if (time_left < time_error_ends) {
          state = ST_GETNAME;
          W_ClearWindow(w);
          err = NULL;
          redraw();
        }
      }
    }

    if (state == ST_DONE) {
      W_ClearWindow(w);
      W_ClearWindow(mapw);
      return;
    }

    if (automatic) {
      switch (state) {
      case ST_GETNAME:
        n_buf[0] = '\0';
        ch = 13;
        j = 0;
        key(ch);
        break;
      case ST_GETPASS:
      case ST_MAKEPASS1:
      case ST_MAKEPASS2:
        ch = defpasswd[j++];
        if (ch == '\0') {
          j = 0;
          ch = 13;
        }
        key(ch);
        break;
      }
    }
  }
}
