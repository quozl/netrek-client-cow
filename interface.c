
/* interface.c
 * 
 * This file will include all the interfaces between the input routines and the
 * daemon.  They should be useful for writing robots and the like
 *
 * $Log: interface.c,v $
 * Revision 1.3  1999/08/05 16:46:32  siegl
 * remove several defines (BRMH, RABBITEARS, NEWDASHBOARD2)
 *
 * Revision 1.2  1999/07/24 19:23:43  siegl
 * New default portSwap for UDP_PORTSWAP feature
 *
 * Revision 1.1.1.1  1998/11/01 17:24:10  siegl
 * COW 3.0 initial revision
 * */
#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>
#include INC_SYS_TIME
#include INC_SYS_TIMEB
#include <signal.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"

set_speed(int speed)
{
  sendSpeedReq(speed);
}

set_course(unsigned char dir)
{
  sendDirReq(dir);
}

shield_up(void)
{
  if (!(me->p_flags & PFSHIELD))
    {
      sendShieldReq(1);
    }
}

shield_down(void)
{
  if (me->p_flags & PFSHIELD)
    {
      sendShieldReq(0);
    }
}

shield_tog(void)
{
  if (me->p_flags & PFSHIELD)
    {
      sendShieldReq(0);
    }
  else
    {
      sendShieldReq(1);
    }
}

bomb_planet(void)
{
  if (!(me->p_flags & PFBOMB))
    {
      sendBombReq(1);
    }
}

beam_up(void)
{
  if (!(me->p_flags & PFBEAMUP))
    {
      sendBeamReq(1);				 /* 1 means up... */
    }
}

beam_down(void)
{
  if (!(me->p_flags & PFBEAMDOWN))
    {
      sendBeamReq(2);				 /* 2 means down... */
    }
}

repair(void)
{
  if (!(me->p_flags & PFREPAIR))
    {
      sendRepairReq(1);
    }
}

repair_off(void)
{
  if (me->p_flags & PFREPAIR)
    {
      sendRepairReq(0);
    }
}

repeat_message(void)
{
  if (++lastm == MAXMESSAGE);
  lastm = 0;
}

cloak(void)
{
  if (me->p_flags & PFCLOAK)
    {
      sendCloakReq(0);
    }
  else
    {
      sendCloakReq(1);
    }
}

cloak_on(void)
{
  if (!(me->p_flags & PFCLOAK))
    {
      sendCloakReq(1);
    }
}

cloak_off(void)
{
  if (me->p_flags & PFCLOAK)
    {
      sendCloakReq(0);
    }
}

unsigned long mstime(void)
{

#ifndef WIN32
  static
  struct timeval tv_base;
  struct timeval tv;

  if (!tv_base.tv_sec)
    {
      gettimeofday(&tv_base, NULL);
      return 0;
    }
  gettimeofday(&tv, NULL);
  return (tv.tv_sec - tv_base.tv_sec) * 1000L +
      (tv.tv_usec - tv_base.tv_usec) / 1000L;
#else
  static unsigned long base;

  if (!base)
    {
      base = GetTickCount();
      return (0);
    }
  return (GetTickCount() - base);
#endif /* WIN32 */
}

unsigned long msetime(void)
{

#ifndef WIN32
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return (tv.tv_sec - 732737182) * 1000 + tv.tv_usec / 1000;
#else
  return GetTickCount();
#endif
}

run_clock(time_t curtime)
{
  char    timebuf[9];
  struct tm *tm;

  static time_t tt;

  if ((curtime - tt) < 1)
    return;

  tm = localtime(&curtime);
  timebuf[0] = tm->tm_hour / 10 + '0';
  timebuf[1] = (tm->tm_hour % 10) + '0';
  timebuf[2] = ':';
  timebuf[3] = tm->tm_min / 10 + '0';
  timebuf[4] = (tm->tm_min % 10) + '0';
  timebuf[5] = ':';
  timebuf[6] = tm->tm_sec / 10 + '0';
  timebuf[7] = (tm->tm_sec % 10) + '0';

  switch (newDashboard)
    {
    case 1:
    case 2:
      W_WriteText(tstatw, 2, 30, textColor, timebuf, 8, W_RegularFont);
      break;
    default:
      W_WriteText(tstatw, 446, 27, textColor, timebuf, 8, W_RegularFont);
    }
  tt = curtime;
}
