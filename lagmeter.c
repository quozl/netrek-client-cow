
/*
 *
 * $Log: lagmeter.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:10  siegl
 * COW 3.0 initial revision
 * */
#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include <ctype.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"

#define L_NB			3
#define L_LENGTHTEXT		3
#define L_TSP			3
#define L_NMARKS		11
#define L_WIDTH			((W_Textwidth*L_NB + L_TSP + L_NB*L_LENGTHTEXT \
					*W_Textwidth + 3*L_TSP)+2*L_IBORDER)
#define L_HEIGHT		((((W_Textheight+L_TSP) * L_NMARKS)+\
					W_Textheight+L_TSP)+2*L_IBORDER)
#define L_IBORDER		5
#define L_BWIDTH		(W_Textwidth*3)
#define L_BHEIGHT		(L_HEIGHT-(W_Textheight+L_TSP)-2*L_IBORDER+3)


lMeterHeight(void)
{
  return L_HEIGHT;
}

lMeterWidth(void)
{
  return L_WIDTH;
}

void    redrawLMeter(void)
{
  register i;
  char    buf[8];

  W_ClearWindow(lMeter);

  /* vertical number marks */
  for (i = 0; i < L_NMARKS; i++)
    {
      sprintf(buf, "%3d", (L_NMARKS - (i + 1)) * 10);
      W_WriteText(lMeter, L_IBORDER, i + (i * (W_Textheight + L_TSP)) + L_TSP + 2	/* X 
											 * 
											 */ ,
		  textColor, buf, strlen(buf), W_RegularFont);
    }

  /* horizontal text */
  W_WriteText(lMeter, L_IBORDER + L_LENGTHTEXT * W_Textwidth + L_TSP,
	      L_HEIGHT - L_IBORDER - W_Textheight / 2 - 3, textColor, "TOT", 3, W_RegularFont);

  W_WriteText(lMeter, L_IBORDER + 2 * (L_LENGTHTEXT * W_Textwidth + L_TSP),
	      L_HEIGHT - L_IBORDER - W_Textheight / 2 - 3, textColor, "SLS", 3, W_RegularFont);

  W_WriteText(lMeter, L_IBORDER + 3 * (L_LENGTHTEXT * W_Textwidth + L_TSP),
	      L_HEIGHT - L_IBORDER - W_Textheight / 2 - 3, textColor, "NF", 3, W_RegularFont);

  /* bars */
  lMeterBox(0, L_LENGTHTEXT * W_Textwidth + L_IBORDER + L_TSP,
	    L_IBORDER, L_BWIDTH, L_BHEIGHT, borderColor);
  lMeterBox(0, L_LENGTHTEXT * W_Textwidth + L_IBORDER + L_BWIDTH + L_TSP + L_TSP,
	    L_IBORDER, L_BWIDTH, L_BHEIGHT, borderColor);
  lMeterBox(0, L_LENGTHTEXT * W_Textwidth + L_IBORDER + 2 * (L_BWIDTH + L_TSP) + L_TSP,
	    L_IBORDER, L_BWIDTH, L_BHEIGHT, borderColor);

  updateLMeter();
}

updateLMeter(void)
{
  double  sd, sdl, ns_get_tstat(void), ns_get_lstat(void);
  int     nf, h;
  W_Color color;

  sd = ns_get_tstat();
  sdl = ns_get_lstat();
  nf = ns_get_nfailures();
  /* filled */

  lMeterBox(1, L_LENGTHTEXT * W_Textwidth + L_IBORDER + L_TSP + 1,
	    L_IBORDER + 1, L_BWIDTH - 2, L_BHEIGHT - 2, backColor);
  if (sd > 0.)
    {
      if (sd > 99.)
	sd = 99.;
      color = gColor;
      if (sd > 25.)
	color = yColor;
      if (sd > 45.)
	color = rColor;

      h = (int) (sd * (double) L_BHEIGHT / 100.);
      lMeterBox(1, L_LENGTHTEXT * W_Textwidth + L_IBORDER + L_TSP + 1,
		L_IBORDER + L_BHEIGHT - h, L_BWIDTH - 2, h - 1, color);
    }

  lMeterBox(1, L_LENGTHTEXT * W_Textwidth + L_IBORDER + L_BWIDTH + L_TSP + L_TSP + 1,
	    L_IBORDER + 1, L_BWIDTH - 2, L_BHEIGHT - 2, backColor);
  if (sdl > 0.)
    {
      if (sdl > 99.)
	sdl = 99.;
      color = gColor;
      if (sdl > 25.)
	color = yColor;
      if (sdl > 45.)
	color = rColor;
      h = (int) (sdl * (double) L_BHEIGHT / 100.);
      lMeterBox(1, L_LENGTHTEXT * W_Textwidth + L_IBORDER + L_BWIDTH + L_TSP + L_TSP + 1,
		L_IBORDER + L_BHEIGHT - h, L_BWIDTH - 2, h - 1, color);
    }

  lMeterBox(1, L_LENGTHTEXT * W_Textwidth + L_IBORDER + 2 * (L_BWIDTH + L_TSP) + L_TSP + 1,
	    L_IBORDER + 1, L_BWIDTH - 2, L_BHEIGHT - 2, backColor);
  if (nf > 0.)
    {
      if (nf > 99.)
	nf = 99.;
      h = (int) (nf * (double) L_BHEIGHT / 100.);
      lMeterBox(1, L_LENGTHTEXT * W_Textwidth + L_IBORDER + 2 * (L_BWIDTH + L_TSP) + L_TSP + 1,
		L_IBORDER + L_BHEIGHT - h, L_BWIDTH - 1, h - 2, rColor);
    }
}

lMeterBox(int filled, int x, int y, int w, int h, W_Color color)
{
  if (filled)
    {
      W_FillArea(lMeter, x, y, w + 1, h + 1, color);
      return;
    }
  W_MakeLine(lMeter, x, y, x + w, y, color);
  W_MakeLine(lMeter, x + w, y, x + w, y + h, color);
  W_MakeLine(lMeter, x + w, y + h, x, y + h, color);
  W_MakeLine(lMeter, x, y + h, x, y, color);
}
