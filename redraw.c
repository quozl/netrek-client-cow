
/* redraw.c
 *
 * $Log: redraw.c,v $
 * Revision 1.2  1999/08/05 16:46:32  siegl
 * remove several defines (BRMH, RABBITEARS, NEWDASHBOARD2)
 *
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 * */
#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include INC_SYS_TIME
#include <sys/types.h>
#include <sys/socket.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"
#include "playerlist.h"
#include "local.h"
#include "map.h"

void    redrawTstats(void);


static unsigned short milli_time = 0;

static time_t lastread;
static int needredraw = 0;
static unsigned long lastredraw = 0;

intrupt(fd_set * readfds)
{
  time_t  time(time_t *);
  unsigned long t;

  udcounter++;

#ifdef RECORDGAME
  if (playback)
    needredraw |= readFromFile();
  else
#endif

    needredraw |=

	readFromServer(readfds);

  t = msetime();
  if (needredraw && (t >= lastredraw + redrawDelay * 100))
    {
      lastredraw = t;
      needredraw = 0;
      lastread = time(NULL);
      redraw();

      updateMaxStats(0);			 /* Update the max stats * *
						  * <isae> */

#ifdef WIN32
      W_FlushScrollingWindow(messwa);
      W_FlushScrollingWindow(messwt);
      W_FlushScrollingWindow(messwi);
      W_FlushScrollingWindow(messwk);
      W_FlushScrollingWindow(reviewWin);
      W_FlushScrollingWindow(phaserwin);
#endif

      UpdatePlayerList();
    }

  if (reinitPlanets)
    {
      initPlanets();
      reinitPlanets = 0;
    }

  if (me->p_status == POUTFIT)
    {
      death();
    }
}

redraw(void)
{
  /* erase warning line if necessary */
  if ((warntimer <= udcounter) && (warncount > 0))
    {
      /* XFIX */
      W_ClearArea(warnw, 5, 5, W_Textwidth * warncount, W_Textheight);
      warncount = 0;
    }

  run_clock(lastread);				 /* for hosers who don't know
						  * * * what a Xclock is */
  clearLocal();

#ifdef BEEPLITE
  if (tts_timer)
    {
      static int last_width;

      tts_timer--;
      if (!tts_timer)
	{
	  /* timed out */
	  W_EraseTTSText(w, TWINSIDE, tts_pos, last_width);
	  last_width = 0;
	}
      else if (tts_timer == tts_time - 1 && last_width)
	{
	  /* first draw -- erase previous */
	  W_EraseTTSText(w, TWINSIDE, tts_pos, last_width);
	  /* draw new */
	  W_WriteTTSText(w, TWINSIDE, tts_pos, tts_width, lastIn,
			 tts_len);
	  last_width = tts_width;
	}
      else
	{
	  /* regular draw */
	  W_WriteTTSText(w, TWINSIDE, tts_pos, tts_width, lastIn, tts_len);
	  last_width = tts_width;
	}
    }
#endif

  local();					 /* redraw local window */

  /* XFIX */
  W_FlushLineCaches(w);

  if (newDashboard)				 /* 6/2/93 LAB */
    if (newDashboard == old_db)
      db_redraw(0);
    else
      redrawTstats();
  else if (newDashboard == old_db)
    stline(0);
  else
    redrawTstats();

  old_db = newDashboard;

  if (W_IsMapped(statwin))
    updateStats();

  /* XFIX: last since its least accurate information */
  map();
}


stline(int flag)
{
  static char buf1[80];
  static char buf2[80];
  static char whichbuf = 0;
  register char *buf, *oldbuf;
  register char *s;
  register int i, j;
  int     k;
  struct player *plr;

  if ((me->p_flags & (PFPLOCK | PFOBSERV)) == (PFPLOCK | PFOBSERV))
    plr = players + me->p_playerl;
  else
    plr = me;


  /* Instead of one sprintf, we do all this by hand for optimization */

  if (flag)
    whichbuf = 0;				 /* We must completely * *
						  * refresh */

  if (whichbuf != 2)
    {
      buf = buf1;
      oldbuf = buf2;
    }
  else
    {
      buf = buf2;
      oldbuf = buf1;
    }
  buf[0] = (me->p_flags & PFSHIELD ? 'S' : ' ');
  if (me->p_flags & PFGREEN)
    buf[1] = 'G';
  else if (me->p_flags & PFYELLOW)
    buf[1] = 'Y';
  else if (me->p_flags & PFRED)
    buf[1] = 'R';
  buf[2] = (me->p_flags & (PFPLLOCK | PFPLOCK) ? 'L' : ' ');
  buf[3] = (me->p_flags & PFREPAIR ? 'R' : ' ');
  buf[4] = (me->p_flags & PFBOMB ? 'B' : ' ');
  buf[5] = (me->p_flags & PFORBIT ? 'O' : ' ');
  if (me->p_ship.s_type == STARBASE)
    buf[6] = (me->p_flags & PFDOCKOK ? 'D' : ' ');
  else
    buf[6] = (me->p_flags & PFDOCK ? 'D' : ' ');
  buf[7] = (me->p_flags & PFCLOAK ? 'C' : ' ');
  buf[8] = (me->p_flags & PFWEP ? 'W' : ' ');
  buf[9] = (me->p_flags & PFENG ? 'E' : ' ');
  if (me->p_flags & PFPRESS)
    buf[10] = 'P';
  else if (me->p_flags & PFTRACT)
    buf[10] = 'T';
  else
    buf[10] = ' ';
  if (me->p_flags & PFBEAMUP)
    buf[11] = 'u';
  else if (me->p_flags & PFBEAMDOWN)
    buf[11] = 'd';
  else
    buf[11] = ' ';
  buf[12] = (status->tourn) ? 't' : ' ';
  buf[13] = ' ';
  buf[14] = '0' + ((me->p_speed % 100) / 10);
  if (buf[14] == '0')
    buf[14] = ' ';
  buf[15] = '0' + (me->p_speed % 10);		 /* speed */
  buf[16] = ' ';
  buf[17] = ' ';
  buf[18] = '0' + (me->p_damage / 100);
  if (buf[18] == '0')
    buf[18] = ' ';
  buf[19] = '0' + ((me->p_damage % 100) / 10);
  if ((buf[19] == '0') && (me->p_damage < 100))
    buf[19] = ' ';
  buf[20] = '0' + (me->p_damage % 10);
  buf[21] = ' ';
  buf[22] = '0' + (me->p_shield / 100);
  if (buf[22] == '0')
    buf[22] = ' ';
  buf[23] = '0' + ((me->p_shield % 100) / 10);
  if ((buf[23] == '0') && (me->p_shield < 100))
    buf[23] = ' ';
  buf[24] = '0' + (me->p_shield % 10);
  buf[25] = ' ';
  buf[26] = ' ';
  buf[27] = '0' + ((plr->p_ntorp % 100) / 10);
  if (buf[27] == '0')
    buf[27] = ' ';
  buf[28] = '0' + (plr->p_ntorp % 10);
  buf[29] = ' ';
  buf[30] = ' ';
  buf[31] = ' ';
  buf[32] = '0' + ((int) (plr->p_kills / 100));
  if (buf[32] == '0')
    buf[32] = ' ';
  buf[33] = '0' + ((int) (plr->p_kills / 10)) % 10;
  if (buf[32] == ' ' && buf[33] == '0')
    buf[33] = ' ';
  buf[34] = '0' + (((int) plr->p_kills) % 10);
  buf[35] = '.';
  buf[36] = '0' + (((int) (plr->p_kills * 10)) % 10);
  buf[37] = '0' + (((int) (plr->p_kills * 100)) % 10);
  buf[38] = ' ';
  buf[39] = ' ';
  buf[40] = ' ';
  buf[41] = ' ';
  buf[42] = '0' + ((me->p_armies % 100) / 10);
  if (buf[42] == '0')
    buf[42] = ' ';
  buf[43] = '0' + (me->p_armies % 10);
  buf[44] = ' ';
  buf[45] = ' ';

  buf[46] = '0' + (me->p_fuel / 100000);
  if (buf[46] == '0')
    buf[46] = ' ';
  buf[47] = '0' + ((me->p_fuel % 100000) / 10000);
  if ((buf[47] == '0') && (me->p_fuel < 100000))
    buf[47] = ' ';
  buf[48] = '0' + ((me->p_fuel % 10000) / 1000);
  if ((buf[48] == '0') && (me->p_fuel < 10000))
    buf[48] = ' ';
  buf[49] = '0' + ((me->p_fuel % 1000) / 100);
  if ((buf[49] == '0') && (me->p_fuel < 1000))
    buf[49] = ' ';
  buf[50] = '0' + ((me->p_fuel % 100) / 10);
  if ((buf[50] == '0') && (me->p_fuel < 100))
    buf[50] = ' ';
  buf[51] = '0' + (me->p_fuel % 10);
  buf[52] = ' ';
  buf[53] = ' ';
  buf[54] = ' ';

  buf[55] = '0' + ((me->p_wtemp / 10) / 100);
  if (buf[55] == '0')
    buf[55] = ' ';
  buf[56] = '0' + (((me->p_wtemp / 10) % 100) / 10);
  if ((buf[56] == '0') && (me->p_wtemp < 1000))
    buf[56] = ' ';
  buf[57] = '0' + ((me->p_wtemp / 10) % 10);

  buf[58] = ' ';
  buf[59] = ' ';
  buf[60] = ' ';

  buf[61] = '0' + ((me->p_etemp / 10) / 100);
  if (buf[61] == '0')
    buf[61] = ' ';
  buf[62] = '0' + (((me->p_etemp / 10) % 100) / 10);
  if ((buf[62] == '0') && (me->p_etemp < 1000))
    buf[62] = ' ';
  buf[63] = '0' + ((me->p_etemp / 10) % 10);

  if (whichbuf == 0)
    {
      /* Draw status line */
      W_WriteText(tstatw, 50, 16, textColor, buf, 64, W_RegularFont);
      whichbuf = 1;
    }
  else
    {						 /* Hacks to make it print *
						  * * only what is necessary */
      whichbuf = 3 - whichbuf;
      j = -1;
      for (i = 0; i < 64; i++)
	{
	  if (*(buf++) != *(oldbuf++))
	    {
	      /* Different string */
	      if (j == -1)
		{
		  k = i;
		  s = buf - 1;
		}
	      j = 0;
	    }
	  else
	    {
	      /* Same string */
	      if (j == -1)
		continue;
	      j++;
	      if (j == 20)
		{				 /* Random number */
		  W_WriteText(tstatw, 50 + W_Textwidth * k, 16, textColor,
			      s, i - k - 19, W_RegularFont);
		  j = -1;
		}
	    }
	}
      if (j != -1)
	{
	  W_WriteText(tstatw, 50 + W_Textwidth * k, 16, textColor, s, i - k - j,
		      W_RegularFont);
	}
    }
}

/* that's not used anymore      13/04/94 [007] newcourse(int x, int y) {
 * return ((unsigned char) nint(atan2((double) (x - me->p_x), (double)
 * (me->p_y - y)) / 3.14159 * 128.)); } */

void    redrawTstats(void)
{
  if (newDashboard)				 /* 6/2/93 LAB */
    db_redraw(1);
  else
    {
      W_ClearWindow(tstatw);
      stline(1);				 /* This is for refresh. We * 
						  * 
						  * * redraw player stats too 
						  */
      updateMaxStats(1);			 /* <isae> Seperated it */
    }
}

/* update stat window record for max speed, army capacity */
updateMaxStats(int redraw)
{
  char    buf[BUFSIZ];
  static int lastdamage = -1;
  static int lastkills = -1;
  static int lastship = -1;
  int     maxspeed;
  int     troop_capacity;
  float   kills;
  int     mykills;

  if (newDashboard)
    return;

  if ((me->p_flags & (PFPLOCK | PFOBSERV)) == (PFPLOCK | PFOBSERV))
    kills = players[me->p_playerl].p_kills;
  else
    kills = me->p_kills;

  mykills = (int) (10. * kills);

  /* don't really need a update if nothing's changed! */
  if (!redraw && lastkills == mykills && lastship == me->p_ship.s_type &&
      lastdamage == me->p_damage)
    return;

  lastkills = mykills;
  lastdamage = me->p_damage;
  lastship = me->p_ship.s_type;

  if (me->p_ship.s_type == ASSAULT)
    troop_capacity = (((kills * 3) > me->p_ship.s_maxarmies) ?
		      me->p_ship.s_maxarmies : (int) (kills * 3));
  else if (me->p_ship.s_type != STARBASE)
    troop_capacity = (((kills * 2) > me->p_ship.s_maxarmies) ?
		      me->p_ship.s_maxarmies : (int) (kills * 2));
  else
    troop_capacity = me->p_ship.s_maxarmies;

  maxspeed = (me->p_ship.s_maxspeed + 2) -
      (me->p_ship.s_maxspeed + 1) *
      ((float) me->p_damage / (float) (me->p_ship.s_maxdamage));
  if (maxspeed > me->p_ship.s_maxspeed)
    maxspeed = me->p_ship.s_maxspeed;
  if (maxspeed < 0)
    maxspeed = 0;


  sprintf(buf,
  "Flags        Warp Dam Shd Torps  Kills Armies   Fuel  Wtemp Etemp  Time");
  W_WriteText(tstatw, 50, 5, textColor, buf, strlen(buf), W_RegularFont);
  sprintf(buf,
	"Maximum:   %2d/%2d  %3d %3d              %2d/%2d  %6d   %3d   %3d",
	  maxspeed, me->p_ship.s_maxspeed,
	  me->p_ship.s_maxdamage, me->p_ship.s_maxshield,
	  troop_capacity, me->p_ship.s_maxarmies,
	  me->p_ship.s_maxfuel, me->p_ship.s_maxwpntemp / 10,
	  me->p_ship.s_maxegntemp / 10);
  W_WriteText(tstatw, 50, 27, textColor, buf, strlen(buf), W_RegularFont);
}
