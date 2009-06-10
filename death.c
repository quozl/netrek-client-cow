#include <setjmp.h>
#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>

#include <time.h>
#include INC_SYS_TIME

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"

#include "inform.h"
#include "option.h"
#include "short.h"

extern jmp_buf env;
W_Font  deathFont;

#ifdef nodef
static struct itimerval udt;

#endif
static char *teamstring[9] =
{"", "and the Feds",
 "and the Roms", "",
 "and the Klis", "", "", "",
 "and the Oris"};

void death(void)
{
  W_Event event;
  W_Window oldw;

  oldw = w;

#ifdef AUTOKEY
  if (autoKey)
    autoKeyAllOff();
#endif /* AUTOKEY */

#ifdef nodef
  /* Reset the signal */
  SIGNAL(SIGALRM, SIG_IGN);
  udt.it_interval.tv_sec = 0;
  udt.it_interval.tv_usec = 0;
  udt.it_value.tv_sec = 0;
  udt.it_value.tv_usec = 0;
  setitimer(ITIMER_REAL, &udt, 0);
  SIGNAL(SIGALRM, SIG_DFL);
#endif

  W_ClearWindow(w);
  W_ClearWindow(iconWin);

  sound_flags = PFSHIELD;

  if (oldalert != PFGREEN)
    {
      if (extraBorder)
	W_ChangeBorder(oldw, gColor);
      W_ChangeBorder(baseWin, gColor);
      oldalert = PFGREEN;
    }
  if (W_IsMapped(statwin))
    {
      W_UnmapWindow(statwin);
      showStats = 1;
    }
  else
    {
      showStats = 0;
    }
  if (infomapped)
    destroyInfo();
  W_UnmapWindow(planetw);
  W_UnmapWindow(rankw);
  W_UnmapWindow(war);
  if (optionWin)
    optiondone();

#ifdef SOUND
  if (soundWin)
    sounddone();
#endif

  if (promoted)
    {
      /* Use deathmessage as a buffer because it will be updated in a moment
       * * anyway */

      if (mystats->st_rank < nranks) {
        sprintf(deathmessage, "Congratulations, you were promoted to %s",
                ranks[mystats->st_rank].name);
      } else {
        sprintf(deathmessage, "Congratulations, you were promoted");
      }
      if (warncount > 0)
	W_ClearArea(warnw, 5, 5, W_Textwidth * warncount, W_Textheight);
      W_WriteText(warnw, 5, 5, W_Green, deathmessage, strlen(deathmessage),
		  W_BoldFont);
      warncount = strlen(deathmessage);
      promoted = 0;
    } else {
      static int first = 10;
      if (first) {
        first--;
        sprintf(deathmessage, "Okay, you died, but that is NORMAL!  Press Enter for new ship!");
        if (warncount > 0)
          W_ClearArea(warnw, 5, 5, W_Textwidth * warncount, W_Textheight);
        W_WriteText(warnw, 5, 5, W_Green, deathmessage, strlen(deathmessage),
                    W_BoldFont);
        warncount = strlen(deathmessage);
      }
    }


  deathFont = W_RegularFont;

  switch (me->p_whydead)
    {
    case KQUIT:
      strcpy(deathmessage, "           You QUIT!!");
      break;
    case KTORP:
      sprintf(deathmessage,
	      "You were thumped by a photon torpedo from %s (%c%c).",
	      players[me->p_whodead].p_name,
	      teamlet[players[me->p_whodead].p_team],
	      shipnos[me->p_whodead]);
      break;
    case KPLASMA:
      sprintf(deathmessage,
	      "You were SMACKed by a plasma torpedo from %s (%c%c) ",
	      players[me->p_whodead].p_name,
	      teamlet[players[me->p_whodead].p_team],
	      shipnos[me->p_whodead]);
      break;
    case KPHASER:
      sprintf(deathmessage,
	      "You were phasered to death by %s (%c%c)",
	      players[me->p_whodead].p_name,
	      teamlet[players[me->p_whodead].p_team],
	      shipnos[me->p_whodead]);
      break;
    case KPLANET:
      sprintf(deathmessage, "You were killed by planetary fire from %s (%c)",
	      planets[me->p_whodead].pl_name,
	      teamlet[planets[me->p_whodead].pl_owner]);
      break;
    case KSHIP:
      sprintf(deathmessage, "You were killed by the explosion of %s (%c%c)",
	      players[me->p_whodead].p_name,
	      teamlet[players[me->p_whodead].p_team],
	      shipnos[me->p_whodead]);
      break;
    case KDAEMON:
      strcpy(deathmessage, "You were killed by a dying daemon.");
      break;
    case KWINNER:
      sprintf(deathmessage, "Galaxy has been conquered by %s (%c%c) %s",
	      players[me->p_whodead].p_name,
	      teamlet[players[me->p_whodead].p_team],
	      shipnos[players[me->p_whodead].p_no],
	      teamstring[players[me->p_whodead].p_team]);
      deathFont = W_BoldFont;
      W_GalacticBgd(GENO_PIX);

      break;
    case KGHOST:
      strcpy(deathmessage, "You were killed by a confused daemon.");
      break;
    case KGENOCIDE:
      sprintf(deathmessage, "You were GENOCIDED by %s (%c%c) %s. You suck!",
	      players[me->p_whodead].p_name,
	      teamlet[players[me->p_whodead].p_team],
	      shipnos[me->p_whodead],
	      teamstring[players[me->p_whodead].p_team]);
      deathFont = W_BoldFont;
      break;
    case KPROVIDENCE:
      strcpy(deathmessage, "You were nuked by GOD.");
      break;
    case KOVER:
      strcpy(deathmessage, "The game is  over!");
      break;
    case TOURNSTART:
      strcpy(deathmessage, "The tournament game has begun!");

#if defined(SOUND) && defined(sgi)
	Play_Sound(BUZZER_SOUND);
#endif
      break;
    case TOURNEND:
      strcpy(deathmessage, "The tournament game has ended.");
      break;
    case KBADBIN:
      strcpy(deathmessage, "Your netrek executable didn't verify correctly.");
      break;
    default:
      strcpy(deathmessage,
	     "You were killed by something unknown to this game?");
      break;
    }

  W_ClearWindow(messagew);
  W_WriteText(messagew, 5, 5, W_Yellow, deathmessage, strlen(deathmessage),
	      deathFont);

  w = oldw;

  /* This is a good time to expire all the torps and phasors that we have *
   * missed the TFREE and PFREE packes for. */
  resetWeaponInfo();


#ifndef THREADED
  while (W_EventsPending())
    W_NextEvent(&event);

  longjmp(env, 0);
#else
  /* Threaded: when using threads, this thread has been spawned to handle *
   * network I/O and so we cannot longjmp here, into another thread! Instead
   * * we call W_TerminateWait which makes the main thead's W_WaitForEvent()
   * * return 0  and exitthread */
  W_TerminateWait();
  ENDTHREAD
#endif /* Threaded */
}

void updatedeath(void)
{
  if (deathFont != W_BoldFont)			 /* Initialise deathFont */
    deathFont = W_RegularFont;

  W_WriteText(messagew, 5, 5, W_Yellow, deathmessage, strlen(deathmessage),
	      deathFont);
}
