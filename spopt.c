/*
 * spopt.c
 * 
 * Functions to look after the Short Packet window.
 * *
 * * $Log: spopt.c,v $
 * * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * * COW 3.0 initial revision
 * * */

#include "config.h"

#ifdef SHORT_PACKETS
/* */
#include "copyright.h"

#include <stdio.h>
#include <ctype.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"
#include "spopt.h"


void    sprefresh(int i)
/*
 * Refresh button i in the Short Packed Window.
 * 
 * Buttons are:
 * SPK_VFIELD           - Variable short packets.
 * SPK_MFIELD           - Messages.
 * SPK_KFIELD           - Kill Messages.
 * SPK_WFIELD           - Warn Messages.
 * SPK_TFIELD           - Recieve Threshold.
 * SPK_DONE             - Done.
 */
{
  char    buf[BUFSIZ];

  switch (i)
    {

    case SPK_VFIELD:
      sprintf(buf, "%seceive variable and short packets",
	      recv_short ? "R" : "Don't r");
      break;
    case SPK_MFIELD:
      sprintf(buf, "%seceive messages", recv_mesg ? "R" : "Don't r");
      break;
    case SPK_KFIELD:
      sprintf(buf, "%seceive kill messages", recv_kmesg ? "R" : "Don't r");
      break;
    case SPK_WFIELD:
      sprintf(buf, "%seceive warning messages", recv_warn ? "R" : "Don't r");
      break;
    case SPK_TFIELD:
      sprintf(buf, "Receive threshold: %s_", recv_threshold_s);
      break;
    case SPK_DONE:
      sprintf(buf, "Done");
      break;
    }

  W_WriteText(spWin, 0, i, textColor, buf, strlen(buf), 0);
}


void    spaction(W_Event * data)
/*
 * Handle a button press.
 */
{
  int     v;
  register int i;
  register char *cp;

  switch (data->y)
    {

    case SPK_VFIELD:
      if (data->type == W_EV_BUTTON)
	{
	  if (recv_short)
	    sendShortReq(SPK_VOFF);
	  else
	    sendShortReq(SPK_VON);
	}
      break;

    case SPK_MFIELD:
      if (data->type == W_EV_BUTTON)
	{
	  if (recv_mesg)
	    sendShortReq(SPK_MOFF);
	  else
	    sendShortReq(SPK_MON);
	}
      break;

    case SPK_KFIELD:
      if (data->type == W_EV_BUTTON)
	{
	  if (recv_kmesg)
	    sendShortReq(SPK_M_NOKILLS);
	  else
	    sendShortReq(SPK_M_KILLS);
	}
      break;

    case SPK_WFIELD:
      if (data->type == W_EV_BUTTON)
	{
	  if (recv_warn)
	    sendShortReq(SPK_M_NOWARN);
	  else
	    sendShortReq(SPK_M_WARN);
	}
      break;

    case SPK_TFIELD:
      if (data->type == W_EV_KEY)
	{
	  switch (data->key)
	    {
	    case '\b':
	    case '\177':
	      cp = recv_threshold_s;
	      i = strlen(cp);
	      if (i > 0)
		{
		  cp += i - 1;
		  *cp = '\0';
		}
	      break;
	    case '\025':
	    case '\030':
	      recv_threshold_s[0] = '\0';
	      break;

	    default:
	      if (data->key >= '0' && data->key <= '9')
		{
		  cp = recv_threshold_s;
		  i = strlen(cp);
		  if (i < 4)
		    {
		      cp += i;
		      cp[1] = '\0';
		      cp[0] = data->key;
		    }
		}
	      break;
	    }
	  sprefresh(SPK_TFIELD);
	}
      break;

    case SPK_DONE:

      if (data->type == W_EV_BUTTON)
	{
	  if (sscanf(recv_threshold_s, "%d", &v) != 1)
	    strcpy(recv_threshold_s, "0");
	  else if (recv_threshold != v)
	    {
	      recv_threshold = v;
	      sendThreshold(recv_threshold);
	    }

	  spdone();
	}
      break;

    }
}


void    spwindow(void)
{
  register int i;

  for (i = 0; i < SPK_NUMFIELDS; i++)
    sprefresh(i);

  /* Map window */
  W_MapWindow(spWin);
}


void    spdone(void)
{
  /* Unmap window */
  W_UnmapWindow(spWin);
}
#endif
