/* findslot.c
 *
 * Kevin Smith 03/23/88
 */
#include "config.h"
#include "copyright2.h"

#include <stdio.h>
#include <sys/types.h>

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"

#include "cowmain.h"
#include "defaults.h"
#include "findslot.h"
#include "newwin.h"
#include "socket.h"

/*

   window hierarchy diagram

   w			wait queue window
    \_ quit		button
    \_ count		button (not reactive)
    \_ motd		button

   w_motd		message of the day window

*/

/* wait queue window constants, in pixels */
#define WQ_W 405        /* window width */
#define WQ_H 100        /* window height */
#define WQ_HEAD 20      /* heading height */
#define BM 10           /* button margin */

/* calculate an X coordinate for centering given text */
static int xc(char *s)
{
  return WQ_W / 6 - BM - strlen(s) * W_Textwidth / 2;
}

/* draw or redraw the heading in the wait queue window */
static void draw_heading(W_Window w)
{
  char    buf[60];

  sprintf(buf, "You are on the queue at %s, please wait.", serverName);
  W_WriteText(w, 10, 10, textColor, buf, strlen(buf), W_RegularFont);
}

/* draw or redraw the quit button */
static void draw_b_quit(W_Window w)
{
  char   *s = "Quit";

  W_WriteText(w, xc(s), 25, textColor, s, strlen(s), W_RegularFont);
}

/* draw or redraw the count button */
static void draw_b_count(W_Window ww, W_Window cw, int count)
{
  char   *s1 = "queue";
  char   *s2 = "position";
  char    buf[80];

  W_WriteText(cw, xc(s1),  5, textColor, s1, strlen(s1), W_RegularFont);
  W_WriteText(cw, xc(s2), 15, textColor, s2, strlen(s2), W_RegularFont);
  sprintf(buf, "   %d   ", count);
  if (count == -1) strcpy(buf, "   ?   ");
  W_WriteText(cw, xc(buf), 35, textColor, buf, strlen(buf), W_RegularFont);

  /* update the window title bar for the window manager to display */
  sprintf(buf, "Q%d @ %s", count, serverName);
  W_SetWindowName(ww, buf);
}

/* draw or redraw the motd button */
static void draw_b_motd(W_Window w)
{
  char   *s1 = "show server";
  char   *s2 = "welcome message";

  W_WriteText(w, xc(s1), 20, textColor, s1, strlen(s1), W_RegularFont);
  W_WriteText(w, xc(s2), 30, textColor, s2, strlen(s2), W_RegularFont);
}

/* find a server slot, returns the slot number once it is known, but
if the server reports we are placed in a queue then display the wait
queue window and maintain the count while optionally showing the
server message of the day */
int findslot(void)
{
  int     oldcount = -1;
  W_Window w, b_quit, b_count, b_motd;

  W_Window w_motd;
  extern int motd_last;
  int     WaitMotdLine = 0;
  int     mapMotd = booleanDefault("showMotd", 0);
  W_Event event;

  /* Wait for some kind of indication about in/not in */
  while (queuePos == -1) {
    socketPauseNoUser();
    if (isServerDead()) {
#if defined(SOUND)
      Exit_Sound();
#endif
      fprintf(stderr, "server connection lost, during pre-queue\n");
      terminate(0);
    }
    readFromServer(NULL);
    if (me != NULL) {
      /* We are in! */
      return me->p_no;
    }
  }

  /* We have to wait.  Make appropriate windows, etc... */

  w       = W_MakeWindow("wait", 0, 0, WQ_W, WQ_H, NULL, 2,
                         foreColor);
  b_quit  = W_MakeWindow("waitquit", BM, WQ_HEAD + BM,
                         WQ_W / 3 - BM * 2, WQ_H - WQ_HEAD - 2 - BM * 2,
                         w, 2, W_Red);
  b_count = W_MakeWindow("count", WQ_W / 3 + BM, WQ_HEAD + BM,
                         WQ_W / 3 - BM * 2, WQ_H - WQ_HEAD - 2 - BM * 2,
                         w, 2, W_Yellow);
  b_motd  = W_MakeWindow("motdbutton", 2 * WQ_W / 3 + BM, WQ_HEAD + BM,
                         WQ_W / 3 - BM * 2, WQ_H - WQ_HEAD - 2 - BM * 2,
                         w, 2, W_Green);
  W_MapWindow(w);
  W_MapWindow(b_count);
  W_MapWindow(b_motd);
  W_MapWindow(b_quit);
  if (mapMotd) {
    w_motd = W_MakeWindow("waitmotd", 1, WQ_W + 1, TWINSIDE,
                           TWINSIDE, 0, 2, foreColor);
    W_MapWindow(w_motd);
    showMotd(w_motd, WaitMotdLine);
  }

  /* a local event loop, exits when the player is given a slot, or if
     the server disconnects, or if the user choses to quit */
  for (;;) {
    socketPause();
    readFromServer(NULL);
    if (isServerDead()) {
#if defined(SOUND)
      Exit_Sound();
#endif
      fprintf(stderr, "server connection lost, during queue wait\n");
      terminate(0);
    }
    while (W_EventsPending()) {
      W_NextEvent(&event);
      switch ((int) event.type) {
      case W_EV_BUTTON:
      case W_EV_KEY:
        if (mapMotd && event.Window == w_motd) {
          if (event.key == ' ' || event.key == 'q') {
            W_DestroyWindow(w_motd);
            mapMotd = !mapMotd;
          } else {
            if (event.key == 'b') {
              WaitMotdLine -= 28;
              WaitMotdLine = MAX(WaitMotdLine, 0);
            } else {
              WaitMotdLine += 28;
              if (WaitMotdLine > motd_last)
                WaitMotdLine = 0;
            }
            W_ClearWindow(w_motd);
            showMotd(w_motd, WaitMotdLine);
            break;
          }
        } else if (event.Window == b_motd) {
          if (mapMotd) {
            W_DestroyWindow(w_motd);
          } else {
            w_motd = W_MakeWindow("waitmotd", 1, WQ_W + 1,
                                   TWINSIDE, TWINSIDE, 0, 2,
                                   foreColor);
            W_MapWindow(w_motd);
            WaitMotdLine = 0;
            showMotd(w_motd, WaitMotdLine);
          }
          mapMotd = !mapMotd;
        } else if (event.Window == b_quit || event.key == 'q') {
#if defined(SOUND)
          Exit_Sound();
#endif
          fprintf(stderr, "you selected quit\n");
          terminate(0);
        }
        break;
      case W_EV_EXPOSE:
        if (event.Window == w) {
          draw_heading(w);
        } else if (event.Window == w_motd) {
          showMotd(w_motd, WaitMotdLine);
        } else if (event.Window == b_quit) {
          draw_b_quit(b_quit);
        } else if (event.Window == b_count) {
          draw_b_count(w, b_count, queuePos);
        } else if (event.Window == b_motd) {
          draw_b_motd(b_motd);
        }
        break;
      case W_EV_CLOSED:
        if (event.Window == w) {
          fprintf(stderr, "you quit, by closing the wait window\n");
          terminate(0);
        } else if (event.Window == w_motd) {
          W_DestroyWindow(w_motd);
          mapMotd = 0;
        }
        break;
      default:
        break;
      }
    }

    /* keep the count button up to date based on the server responses */
    if (queuePos != oldcount) {
      draw_b_count(w, b_count, queuePos);
      oldcount = queuePos;
    }

    /* if we have a slot now, destroy the windows and begin playing */
    if (me != NULL) {
      W_DestroyWindow(w);
      if (mapMotd) {
        W_DestroyWindow(w_motd);
      }
      W_Beep();
      W_Beep();
      return me->p_no;
    }
  }
}
