
/*
 * dashboard.c - graphic tstatw - 6/2/93
 * lab2-dashboard.c - 2nd graphic tstatw - 7/29/96
 * 
 * copyright 1993,1996 Lars Bernhardsson (lab@mtek.chalmers.se)
 * Free to use as long as this notice is left here.
 *
 * $Log: dashboard3.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:09  siegl
 * COW 3.0 initial revision
 * */

#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"


static int
        db_itoa(char *s, int v)
{
  char   *buf = s;


  *buf = '0' + (v / 100000);

  if (*buf != '0')
    {
      buf++;
    }

  *buf = '0' + ((v % 100000) / 10000);

  if ((*buf != '0') || (v >= 100000))
    {
      buf++;
    }

  *buf = '0' + ((v % 10000) / 1000);

  if ((*buf != '0') || (v >= 10000))
    {
      buf++;
    }

  *buf = '0' + ((v % 1000) / 100);

  if ((*buf != '0') || (v >= 1000))
    {
      buf++;
    }

  *buf = '0' + ((v % 100) / 10);

  if ((*buf != '0') || (v >= 100))
    {
      buf++;
    }

  *buf++ = '0' + (v % 10);

  return buf - s;
}


static int
        db_ftoa(char *s, float v)
{
  char   *buf = s;


  if (v >= 100.0)
    {
      *buf++ = '0' + ((int) (v / 100));
      *buf++ = '0' + ((int) (((int) v % 100) / 10));
    }
  else if (v >= 10.0)
    {
      *buf++ = '0' + ((int) (v / 10));
    }

  *buf++ = '0' + (((int) v) % 10);
  *buf++ = '.';
  *buf++ = '0' + (((int) (v * 10)) % 10);
  *buf++ = '0' + (((int) (v * 100)) % 10);

  return buf - s;
}


static void
        db_bar(char *l, int x, int y, int w, int h, int m, int t, int v, int b)
{
  int     i, hgt, val_pix, tmax_pix, yellow_pix, red_pix;
  int     label_len;
  char    label[32];


  W_ClearArea(tstatw, x, y, w, h);

  if (b)
    {
      val_pix = (w * v) / m;
    }
  else
    {
      val_pix = (w * (m - v)) / m;
    }

  tmax_pix = (w * t) / m;
  yellow_pix = w / 3;
  red_pix = (2 * w) / 3;

  if ((t < 0) ||
      (t > m) ||
      (v < 0) ||
      (v > m))
    {
      W_FillArea(tstatw, x, y, w, h, W_Red);

      W_MaskText(tstatw,
		 x + w - (3 * W_Textwidth) - 2,
		 y + h - W_Textheight - 2,
		 W_White, l, 3, W_BoldFont);

      label_len = db_itoa(label, v);

      W_MaskText(tstatw,
		 x, y,
		 W_White, label, label_len, W_BoldFont);

      label_len = db_itoa(label, t);

      if (t == m)
	{
	  W_MaskText(tstatw,
		     x, y + W_Textheight + 2,
		     W_Grey, label, label_len, W_RegularFont);
	}
      else
	{
	  W_MaskText(tstatw,
		     x, y + W_Textheight + 2,
		     W_White, label, label_len, W_RegularFont);
	}
    }
  else
    {
      for (i = 0; i < w; i++)
	{
	  hgt = h - ((h * i) / w);

	  if ((i >= tmax_pix) && (i % 2))
	    {
	      W_MakeLine(tstatw, x + i, y + hgt, x + i, y + h, W_Grey);
	    }
	  else if (i <= val_pix)
	    {
	      if (i < yellow_pix)
		{
		  W_MakeLine(tstatw, x + i, y + hgt, x + i, y + h, W_Green);
		}
	      else if (i < red_pix)
		{
		  W_MakeLine(tstatw, x + i, y + hgt, x + i, y + h, W_Yellow);
		}
	      else
		{
		  W_MakeLine(tstatw, x + i, y + hgt, x + i, y + h, W_Red);
		}
	    }
	}

      if (b ? (v > (m / 2)) : (v < (m / 2)))
	{
	  W_MaskText(tstatw,
		     x + w - (3 * W_Textwidth) - 2,
		     y + h - W_Textheight - 2,
		     W_White, l, 3, W_RegularFont);
	}
      else
	{
	  W_MaskText(tstatw,
		     x + w - (3 * W_Textwidth) - 2,
		     y + h - W_Textheight - 2,
		     W_Grey, l, 3, W_RegularFont);
	}

      label_len = db_itoa(label, v);

      if (b ? (v > (m / 2)) : (v < (m / 2)))
	{
	  W_MaskText(tstatw,
		     x, y,
		     W_White, label, label_len, W_BoldFont);
	}
      else
	{
	  W_MaskText(tstatw,
		     x, y,
		     W_Grey, label, label_len, W_BoldFont);
	}

      label_len = db_itoa(label, t);

      if (t == m)
	{
	  W_MaskText(tstatw,
		     x, y + W_Textheight + 2,
		     W_Grey, label, label_len, W_RegularFont);
	}
      else
	{
	  W_MaskText(tstatw,
		     x, y + W_Textheight + 2,
		     W_White, label, label_len, W_RegularFont);
	}
    }

  W_MakeLine(tstatw, x, y + h, x + w, y + h, W_Grey);
  W_MakeLine(tstatw, x + w, y, x + w, y + h, W_Grey);
}


static void
        db_flags(int fr)
{
  static unsigned int old_flags = -1;
  static unsigned char old_tourn = -1;
  char    buf[13];


  if (fr ||
      (old_flags != me->p_flags) ||
      (old_tourn != status->tourn))
    {
      buf[0] = (me->p_flags & PFSHIELD ? 'S' : ' ');

      if (me->p_flags & PFGREEN)
	{
	  buf[1] = 'G';
	}
      else if (me->p_flags & PFYELLOW)
	{
	  buf[1] = 'Y';
	}
      else
	{
	  buf[1] = 'R';
	}

      buf[2] = (me->p_flags & (PFPLLOCK | PFPLOCK) ? 'L' : ' ');
      buf[3] = (me->p_flags & PFREPAIR ? 'R' : ' ');
      buf[4] = (me->p_flags & PFBOMB ? 'B' : ' ');
      buf[5] = (me->p_flags & PFORBIT ? 'O' : ' ');

      if (me->p_ship.s_type == STARBASE)
	{
	  buf[6] = (me->p_flags & PFDOCKOK ? 'D' : ' ');
	}
      else
	{
	  buf[6] = (me->p_flags & PFDOCK ? 'D' : ' ');
	}

      buf[7] = (me->p_flags & PFCLOAK ? 'C' : ' ');
      buf[8] = (me->p_flags & PFWEP ? 'W' : ' ');
      buf[9] = (me->p_flags & PFENG ? 'E' : ' ');

      if (me->p_flags & PFPRESS)
	{
	  buf[10] = 'P';
	}
      else if (me->p_flags & PFTRACT)
	{
	  buf[10] = 'T';
	}
      else
	{
	  buf[10] = ' ';
	}

      if (me->p_flags & PFBEAMUP)
	{
	  buf[11] = 'u';
	}
      else if (me->p_flags & PFBEAMDOWN)
	{
	  buf[11] = 'd';
	}
      else
	{
	  buf[11] = ' ';
	}

      if (status->tourn)
	{
	  buf[12] = 't';
	}
      else
	{
	  buf[12] = ' ';
	}

      W_WriteText(tstatw,
		  2, 32,
		  W_White, buf, 13, W_RegularFont);

      old_flags = me->p_flags;
      old_tourn = status->tourn;
    }
}


void
        db_redraw_lab2(int fr)
{
  static int old_spd = -1, old_cur_max = -1;
  static int old_shl = -1, old_dam = -1;
  static int old_arm = -1, old_cur_arm = -1;
  static int old_wpn = -1, old_egn = -1;
  static int old_ful = -1;
  static float old_kills = -1;
  int     cur_max, cur_arm, label_len;
  char    label[32];


  if (me->p_ship.s_type == ASSAULT)
    {
      cur_arm = (3 * me->p_kills);
    }
  else
    {
      cur_arm = (2 * me->p_kills);
    }

  if (cur_arm < 0)
    {
      cur_arm = 0;
    }
  else if ((cur_arm > me->p_ship.s_maxarmies) ||
	   (me->p_ship.s_type == STARBASE))
    {
      cur_arm = me->p_ship.s_maxarmies;
    }

  cur_max = ((me->p_ship.s_maxspeed + 2) -
	     ((me->p_ship.s_maxspeed + 1) *
	      ((float) me->p_damage /
	       (float) me->p_ship.s_maxdamage)));

  if (cur_max < 0)
    {
      cur_max = 0;
    }
  else if (cur_max > me->p_ship.s_maxspeed)
    {
      cur_max = me->p_ship.s_maxspeed;
    }

  if (fr)
    {
      W_ClearWindow(tstatw);
    }

  db_flags(fr);

  if (fr ||
      (me->p_speed != old_spd) ||
      (old_cur_max != cur_max))
    {
      db_bar("Spd", 2, 2, 75, 25,
	     me->p_ship.s_maxspeed,
	     cur_max,
	     me->p_speed,
	     1);
    }

  if (fr ||
      (old_ful != me->p_fuel))
    {
      db_bar("Ful", 82, 2, 75, 25,
	     me->p_ship.s_maxfuel,
	     me->p_ship.s_maxfuel,
	     me->p_fuel,
	     0);
    }

  if (fr ||
      (old_shl != me->p_shield))
    {
      db_bar("Shl", 162, 2, 75, 25,
	     me->p_ship.s_maxshield,
	     me->p_ship.s_maxshield,
	     me->p_shield,
	     0);
    }

  if (fr ||
      (old_dam != me->p_damage))
    {
      db_bar("Dam", 242, 2, 75, 25,
	     me->p_ship.s_maxdamage,
	     me->p_ship.s_maxdamage,
	     me->p_damage,
	     1);
    }

  if (me->p_ship.s_type == STARBASE)
    {
      if (fr ||
	  (old_wpn != me->p_wtemp))
	{
	  db_bar("Wpn", 322, 2, 75, 25,
		 me->p_ship.s_maxwpntemp / 10,
		 me->p_ship.s_maxwpntemp / 10,
		 me->p_wtemp / 10,
		 1);
	}

      if (fr ||
	  (old_egn != me->p_etemp))
	{
	  W_ClearArea(tstatw, 324, 32, 78, W_Textheight);

	  label[0] = 'E';
	  label[1] = 'g';
	  label[2] = 'n';
	  label[3] = ':';
	  label[4] = ' ';
	  label_len = 5 + db_itoa(&label[5], me->p_etemp / 10);
	  label[label_len++] = '/';
	  label_len += db_itoa(&label[label_len],
			       me->p_ship.s_maxegntemp / 10);

	  if (me->p_etemp > (me->p_ship.s_maxegntemp / 2))
	    {
	      W_WriteText(tstatw,
			  324, 32,
			  W_White, label, label_len, W_BoldFont);
	    }
	  else
	    {
	      W_WriteText(tstatw,
			  324, 32,
			  W_Grey, label, label_len, W_RegularFont);
	    }
	}
    }
  else
    {
      if (fr ||
	  (old_egn != me->p_etemp))
	{
	  db_bar("Egn", 322, 2, 75, 25,
		 me->p_ship.s_maxegntemp / 10,
		 me->p_ship.s_maxegntemp / 10,
		 me->p_etemp / 10,
		 1);
	}

      if (fr ||
	  (old_wpn != me->p_wtemp))
	{
	  W_ClearArea(tstatw, 324, 32, 78, W_Textheight);

	  label[0] = 'W';
	  label[1] = 'p';
	  label[2] = 'n';
	  label[3] = ':';
	  label[4] = ' ';
	  label_len = 5 + db_itoa(&label[5], me->p_wtemp / 10);
	  label[label_len++] = '/';
	  label_len += db_itoa(&label[label_len],
			       me->p_ship.s_maxwpntemp / 10);

	  if (me->p_wtemp > (me->p_ship.s_maxwpntemp / 2))
	    {
	      W_WriteText(tstatw,
			  324, 32,
			  W_White, label, label_len, W_BoldFont);
	    }
	  else
	    {
	      W_WriteText(tstatw,
			  324, 32,
			  W_Grey, label, label_len, W_RegularFont);
	    }
	}
    }

  if (fr ||
      (old_arm != me->p_armies) ||
      (old_cur_arm != cur_arm))
    {
      W_ClearArea(tstatw, 402, 2, 98, W_Textheight);

      if (cur_arm > 0)
	{
	  label[0] = 'A';
	  label[1] = 'r';
	  label[2] = 'm';
	  label[3] = 'i';
	  label[4] = 'e';
	  label[5] = 's';
	  label[6] = ':';
	  label[7] = ' ';
	  label_len = 8 + db_itoa(&label[8], me->p_armies);
	  label[label_len++] = '/';
	  label_len += db_itoa(&label[label_len], cur_arm);

	  if (me->p_armies >= cur_arm)
	    {
	      W_WriteText(tstatw,
			  402, 2,
			  W_Red, label, label_len, W_BoldFont);
	    }
	  else if (me->p_armies > 0)
	    {
	      W_WriteText(tstatw,
			  402, 2,
			  W_Yellow, label, label_len, W_BoldFont);
	    }
	  else
	    {
	      W_WriteText(tstatw,
			  402, 2,
			  W_Green, label, label_len, W_BoldFont);
	    }
	}
    }

  if (fr ||
      (old_kills != me->p_kills))
    {
      W_ClearArea(tstatw, 402, 4 + W_Textheight, 98, W_Textheight);

      if (me->p_kills > 0.0)
	{
	  label[0] = ' ';
	  label[1] = 'K';
	  label[2] = 'i';
	  label[3] = 'l';
	  label[4] = 'l';
	  label[5] = 's';
	  label[6] = ':';
	  label[7] = ' ';
	  label_len = 8 + db_ftoa(&label[8], me->p_kills);

	  if (cur_arm > 4)
	    {
	      W_WriteText(tstatw,
			  402, 4 + W_Textheight,
			  W_White, label, label_len, W_BoldFont);
	    }
	  else if (cur_arm > 1)
	    {
	      W_WriteText(tstatw,
			  402, 4 + W_Textheight,
			  W_White, label, label_len, W_RegularFont);
	    }
	  else
	    {
	      W_WriteText(tstatw,
			  402, 4 + W_Textheight,
			  W_Grey, label, label_len, W_RegularFont);
	    }
	}

      old_kills = me->p_kills;
    }

  old_spd = me->p_speed;
  old_cur_max = cur_max;
  old_shl = me->p_shield;
  old_dam = me->p_damage;
  old_arm = me->p_armies;
  old_cur_arm = cur_arm;
  old_wpn = me->p_wtemp;
  old_egn = me->p_etemp;
  old_ful = me->p_fuel;
}
