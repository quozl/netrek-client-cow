
/* death.c
 *
 * $Log: death.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:09  siegl
 * COW 3.0 initial revision
 * */
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

death(void)
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
  W_ClearWindow(mapw);
  W_ClearWindow(iconWin);
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

      sprintf(deathmessage, "Congratulations, You have scummed up to %s",
	      ranks[mystats->st_rank].name);
      W_WriteText(w, 50, 100, W_Yellow, deathmessage, strlen(deathmessage),
		  W_BoldFont);
      promoted = 0;
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

#ifdef HAVE_XPM
      W_GalacticBgd(GENO_PIX);
#endif

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

  W_WriteText(w, 50, 80, textColor, deathmessage, strlen(deathmessage),
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

updatedeath(void)
{
  if (deathFont != W_BoldFont)			 /* Initialise deathFont */
    deathFont = W_RegularFont;

  W_WriteText(w, 50, 80, textColor, deathmessage, strlen(deathmessage),
	      deathFont);
}
