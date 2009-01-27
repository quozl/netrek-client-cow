#include "config.h"
#include "copyright2.h"

#include INC_MACHINE_ENDIAN

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include INC_NETINET_IN
#include INC_NETINET_TCP
#include <netdb.h>
#include <math.h>
#include <errno.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"

#include "lagmeter.h"
#include "netstat.h"
#include "netstatopt.h"
#include "interface.h"

/* #define NETDEBUG */

static int lastread;
static int dead;
static int start;
static int counter;

static int sum, n, s2;				 /* total since last reset */
static int M, var;
static double sd;

static int suml, nl, s2l;			 /* total since last death */
static int Ml, varl;
static double sdl;

static int nf;					 /* network failures */

static char nfthresh_s[8] = NETSTAT_DF_NFT_S;
static int nfthresh = NETSTAT_DF_NFT;

void ns_init(int v)
{
  start = v;
  sum = n = s2 = 0;

  suml = nl = s2l = 0;
  nf = 0;
}

void ns_record_update(int count)
{
  int     now;
  int     et;
  static int lastupdateSpeed;
  static int lasttime = -1;

  et = 1000 / server_ups;			 /* expected time */

  if (!me)
    return;

  now = mstime();

  if (lasttime < (et + et / 4) && now - lastread < et / 2)
    {

#ifdef NETDEBUG
      printf("skipping %d\n", now - lastread);
#endif

      return;
    }

  /* wait a few updates to stabilize */
  if (start)
    {
      start--;
      lastread = now;
      return;
    }

  /* wait a few updates to stabilize */
  if (me->p_status != PALIVE)
    {

#ifdef NETDEBUG
      printf("waiting after death...\n");
#endif

      dead = 5;
      lastread = now;
      suml = nl = s2l = 0;
      return;
    }
  else if (dead)
    {
      dead--;
      lastread = now;
      return;
    }

  /* reset if we change updates */
  if (server_ups != lastupdateSpeed)
    {
      ns_init(3);
      lastupdateSpeed = server_ups;
      lastread = now;
      return;
    }
  lastupdateSpeed = server_ups;

  lasttime = now - lastread;

  if (lasttime >= nfthresh)
    {
      nf++;					 /* network failure */
      nsrefresh(NETSTAT_FAILURES);
      updateLMeter();

#ifdef NETDEBUG
      printf("network failure: %d\n", lasttime);;
#endif
    }
  else
    {
      counter++;
      ns_do_stat(lasttime, counter);

#ifdef NETDEBUG
      printf("%d\n", lasttime);
#endif
    }

  lastread = now;
}

void ns_do_stat(int v, int c)
{
  int     uf;

  n++;
  nl++;
  sum += v;
  suml += v;
  s2 += (v * v);
  s2l += (v * v);

  if (n <= 1 || nl <= 1)
    return;

  uf = (server_ups * 10) / netstatfreq;
  if (uf == 0)
    uf = 1;

  if ((c % uf) == 0)
    {

      M = sum / n;
      var = (s2 - M * sum) / (n - 1);
      if (debug)
	printf("ns1: var=%d\n", var);
      sd = (int) sqrt((double) var);
      nsrefresh(NETSTAT_TOTAL);

      Ml = suml / nl;
      varl = (s2l - Ml * suml) / (nl - 1);
      if (debug)
	printf("ns2: varl=%d\n", varl);
      sdl = (int) sqrt((double) varl);

      nsrefresh(NETSTAT_LOCAL);
      updateLMeter();
    }
}

double
        ns_get_tstat(void)
{
  return sd;
}

double
        ns_get_lstat(void)
{
  return sdl;
}

int ns_get_nfailures(void)
{
  return nf;
}

char   *
        ns_get_nfthresh_s(void)
{
  return nfthresh_s;
}

void ns_set_nfthresh_s(char *s)
{
  strcpy(nfthresh_s, s);
}

int ns_get_nfthresh(void)
{
  return nfthresh;
}

void ns_set_nfthresh(int v)
{
  nfthresh = v;
}
