
/*
 * $Log: netstatopt.c,v $
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


nswindow(void)
{
  register int i;

  for (i = 0; i < NETSTAT_NUMFIELDS; i++)
    nsrefresh(i);

  /* Map window */
  W_MapWindow(netstatWin);
}

/* Refresh item i */
nsrefresh(int i)
{
  double  ns_get_tstat(void), ns_get_lstat(void);
  char    buf[BUFSIZ], *ns_get_nfthresh_s(void);
  W_Color color;

  switch (i)
    {

    case NETSTAT_SWITCH:
      sprintf(buf, "%sollect network stats",
	      netstat ? "C" : "Don't c");
      color = textColor;
      break;
    case NETSTAT_RESET:
      sprintf(buf, "Reset network stats");
      color = textColor;
      break;
    case NETSTAT_TOTAL:
      sprintf(buf, "Total              : %4.2f", ns_get_tstat());
      color = yColor;
      break;
    case NETSTAT_LOCAL:
      sprintf(buf, "This ship          : %4.2f", ns_get_lstat());
      color = yColor;
      break;
    case NETSTAT_FAILURES:
      sprintf(buf, "Network failures   : %d", ns_get_nfailures());
      color = yColor;
      break;
    case NETSTAT_NFTHRESH:
      sprintf(buf, "Network failure threshold: %s_", ns_get_nfthresh_s());
      color = textColor;
      break;
    case NETSTAT_DONE:
      sprintf(buf, "Done");
      color = textColor;
      break;
    }
  W_WriteText(netstatWin, 0, i, color, buf, strlen(buf), 0);
}

void    nsaction(W_Event * data)
{
  char   *ns_get_nfthresh_s(void);
  int     v;
  register int i;
  register char *cp;

  switch (data->y)
    {

    case NETSTAT_SWITCH:
      if (data->type == W_EV_BUTTON)
	{
	  if (netstat)
	    {
	      netstat = 0;
	      W_UnmapWindow(lMeter);
	    }
	  else
	    {
	      netstat = 1;
	      ns_init(5);
	    }
	}
      nsrefresh(NETSTAT_SWITCH);
      break;

    case NETSTAT_RESET:
      if (data->type == W_EV_BUTTON)
	{
	  ns_init(0);
	  nsrefresh(NETSTAT_TOTAL);
	  nsrefresh(NETSTAT_LOCAL);
	  nsrefresh(NETSTAT_FAILURES);
	}
      break;

    case NETSTAT_NFTHRESH:
      if (data->type == W_EV_KEY)
	{
	  switch (data->key)
	    {
	    case '\b':
	    case '\177':
	      cp = ns_get_nfthresh_s();
	      i = strlen(cp);
	      if (i > 0)
		{
		  cp += i - 1;
		  *cp = '\0';
		}
	      break;
	    case '\025':
	    case '\030':
	      ns_set_nfthresh_s("");
	      break;

	    default:
	      if (data->key >= '0' && data->key <= '9')
		{
		  cp = ns_get_nfthresh_s();
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
	  nsrefresh(NETSTAT_NFTHRESH);
	}
      break;



    case NETSTAT_DONE:
      if (data->type == W_EV_BUTTON)
	{
	  if (sscanf(ns_get_nfthresh_s(), "%d", &v) != 1)
	    {
	      ns_set_nfthresh_s(NETSTAT_DF_NFT_S);
	      ns_set_nfthresh(NETSTAT_DF_NFT);
	    }
	  else
	    ns_set_nfthresh(v);

	  nsdone();
	}
      break;
    }
}

nsdone(void)
{
  /* Unmap window */
  W_UnmapWindow(netstatWin);
}
