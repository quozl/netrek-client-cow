

/* pingstats.c        (mostly taken from stats.c)
 *
 * $Log: pingstats.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 * */
#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include <math.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"


#define	MIN(a,b)	(((a) < (b)) ? (a) : (b))

#define	BX_OFF()	((textWidth + 1) * W_Textwidth + S_IBORDER)
#define	BY_OFF(line)	((line) * (W_Textheight + S_IBORDER) + S_IBORDER)
#define	TX_OFF(len)	((textWidth - len) * W_Textwidth + S_IBORDER)
#define	TY_OFF(line)	BY_OFF(line)

/* right side labels */
#define TEXT_WIDTH		(5*W_Textwidth + 2*STAT_BORDER)
#define STAT_WIDTH		(260 + TEXT_WIDTH)
#define STAT_HEIGHT		BY_OFF(NUM_SLIDERS)
#define STAT_BORDER		2
#define S_IBORDER		5
#define STAT_X			422
#define STAT_Y			13

#define SL_WID			\
	(STAT_WIDTH -TEXT_WIDTH - 2 * S_IBORDER - (textWidth + 1) * W_Textwidth)
#define SL_HEI			(W_Textheight)

#define NUM_ELS(a)		(sizeof (a) / sizeof (*(a)))
#define NUM_SLIDERS		NUM_ELS(sliders)

typedef struct slider
  {
    char   *label;
    int     min, max;
    int     green, yellow;
    int     label_length;
    int     diff;
    int    *var;
    int     lastVal;
  }

SLIDER;

typedef struct record
  {
    int    *data;
    int     last_value;
  }

RECORD;

static SLIDER sliders[] =
{
  {"round trip time", 0, 500, 100, 200},
  {"average r.t. time", 0, 500, 100, 200},
  {"lag (st. dev.)", 0, 100, 20, 50},
  {"%pack in  loss", 0, 50, 10, 20},
  {"%pack out loss", 0, 50, 10, 20},
  {"tot %pack loss in", 0, 50, 5, 10},
  {"tot %pack loss out", 0, 50, 5, 10},
};

static int textWidth = 0;
static int initialized = 0;
static int box(int filled, int x, int y, int wid, int hei, W_Color color),
        text(int value, int y);

/* externals from ping.c (didn't feel like cluttering up data.c with them) */

extern int ping_iloss_sc;			 /* inc % loss 0--100, server

						  * 
						  * * * to client */
extern int ping_iloss_cs;			 /* inc % loss 0--100, client

						  * 
						  * * * to server */
extern int ping_tloss_sc;			 /* total % loss 0--100, *

						  * 
						  * * server to client */
extern int ping_tloss_cs;			 /* total % loss 0--100, *

						  * 
						  * * client to server */
extern int ping_lag;				 /* delay in ms of last ping */
extern int ping_av;				 /* average rt */
extern int ping_sd;				 /* standard deviation */

pStatsHeight(void)
{
  return STAT_HEIGHT;
}

pStatsWidth(void)
{
  return STAT_WIDTH;
}

initPStats(void)
{
  int     i;
  char   *str;

  if (initialized)
    return;
  initialized = 1;
  sliders[0].var = (int *) &ping_lag;
  sliders[1].var = (int *) &ping_av;
  sliders[2].var = (int *) &ping_sd;
  sliders[3].var = (int *) &ping_iloss_sc;
  sliders[4].var = (int *) &ping_iloss_cs;
  sliders[5].var = (int *) &ping_tloss_sc;
  sliders[6].var = (int *) &ping_tloss_cs;

  /* adjust */
  if (ping_av > 0)
    {
      sliders[0].max = MAX(ping_av * 2, 200);
      sliders[1].max = MAX(ping_av * 2, 200);
    }

  for (i = 0; i < NUM_SLIDERS; i++)
    {
      sliders[i].label_length = strlen(sliders[i].label);
      textWidth = MAX(textWidth, sliders[i].label_length);
      sliders[i].diff = sliders[i].max - sliders[i].min;
      sliders[i].lastVal = 0;
    }
}

void    redrawPStats(void)
{
  int     i;

  W_ClearWindow(pStats);
  initPStats();
  for (i = 0; i < NUM_SLIDERS; i++)
    {
      sliders[i].lastVal = 0;
    }
  for (i = 0; i < NUM_SLIDERS; i++)
    {
      W_WriteText(pStats, TX_OFF(sliders[i].label_length), TY_OFF(i),
		  textColor, sliders[i].label, sliders[i].label_length,
		  W_RegularFont);
      box(0, BX_OFF() - 1, BY_OFF(i) - 1, SL_WID + 2, SL_HEI + 2, borderColor);
      sliders[i].lastVal = 0;
    }
}

updatePStats(void)
{
  int     i, value, diff, old_x, new_x;
  W_Color color;
  SLIDER *s;

  /* do the average and standard deviation calculations */
  initPStats();

  for (i = 0; i < NUM_SLIDERS; i++)
    {
      s = &sliders[i];
      value = *(s->var);
      /* update decimal values at the right */
      text(*(s->var), BY_OFF(i));

      if (value < s->min)
	value = s->min;
      else if (value > s->max)
	value = s->max;
      if (value == s->lastVal)
	continue;
      diff = value - s->lastVal;
      if (diff < 0)
	{					 /* bar decreasing */
	  old_x = s->lastVal * SL_WID / s->diff;
	  new_x = value * SL_WID / s->diff;
	  box(1, BX_OFF() + new_x, BY_OFF(i), old_x - new_x, SL_HEI, backColor);

	  if (s->lastVal > s->green && value <= s->green)
	    box(1, BX_OFF(), BY_OFF(i), new_x, SL_HEI, gColor);
	  else if (s->lastVal > s->yellow && value <= s->yellow)
	    box(1, BX_OFF(), BY_OFF(i), new_x, SL_HEI, yColor);
	}
      else
	{					 /* bar increasing */
	  if (s->lastVal <= s->yellow && value > s->yellow)
	    {
	      color = rColor;
	      s->lastVal = 0;
	    }
	  else if (s->lastVal <= s->green && value > s->green)
	    {
	      color = yColor;
	      s->lastVal = 0;
	    }
	  else if (value > s->yellow)
	    color = rColor;
	  else if (value > s->green)
	    color = yColor;
	  else
	    color = gColor;

	  old_x = s->lastVal * SL_WID / s->diff;
	  new_x = value * SL_WID / s->diff;
	  box(1, BX_OFF() + old_x, BY_OFF(i), new_x - old_x, SL_HEI, color);
	}
      s->lastVal = value;
    }
}

static
        box(int filled, int x, int y, int wid, int hei, W_Color color)
{
  if (wid == 0)
    return;

  if (filled)
    {
      /* XFIX */
      W_FillArea(pStats, x, y, wid + 1, hei + 1, color);
      return;
    }

  W_MakeLine(pStats, x, y, x + wid, y, color);
  W_MakeLine(pStats, x + wid, y, x + wid, y + hei, color);
  W_MakeLine(pStats, x + wid, y + hei, x, y + hei, color);
  W_MakeLine(pStats, x, y + hei, x, y, color);
}

static
        text(int value, int y)
{
  char    buf[6];

  sprintf(buf, "(%3d)", value);			 /* fix */

  W_WriteText(pStats, STAT_WIDTH - TEXT_WIDTH, y, textColor,
	      buf, 5, W_RegularFont);
}
