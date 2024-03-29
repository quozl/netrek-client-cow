/* interface.c
 * 
 * This file will include all the interfaces between the input routines and the
 * daemon.  They should be useful for writing robots and the like
 *
 */
#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include <time.h>
#include INC_SYS_TIME
#include INC_SYS_TIMEB

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"

#include "socket.h"

#include "interface.h"

void set_speed(int speed)
{
  sendSpeedReq(speed);
}

void set_course(unsigned char dir)
{
  sendDirReq(dir);
}

void shield_up(void)
{
  if (!(me->p_flags & PFSHIELD))
    {
      sendShieldReq(1);
    }
}

void shield_down(void)
{
  if (me->p_flags & PFSHIELD)
    {
      sendShieldReq(0);
    }
}

void shield_tog(void)
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

void bomb_planet(void)
{
  if (!(me->p_flags & PFBOMB))
    {
      sendBombReq(1);
    }
}

void beam_up(void)
{
  if (!(me->p_flags & PFBEAMUP))
    {
      sendBeamReq(1);				 /* 1 means up... */
    }
}

void beam_down(void)
{
  if (!(me->p_flags & PFBEAMDOWN))
    {
      sendBeamReq(2);				 /* 2 means down... */
    }
}

void repair(void)
{
  if (!(me->p_flags & PFREPAIR))
    {
      sendRepairReq(1);
    }
}

void repair_off(void)
{
  if (me->p_flags & PFREPAIR)
    {
      sendRepairReq(0);
    }
}

void cloak(void)
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

void cloak_on(void)
{
  if (!(me->p_flags & PFCLOAK))
    {
      sendCloakReq(1);
    }
}

void cloak_off(void)
{
  if (me->p_flags & PFCLOAK)
    {
      sendCloakReq(0);
    }
}

unsigned long ustime(void)
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
  return (tv.tv_sec - tv_base.tv_sec) * 1000000L +
      (tv.tv_usec - tv_base.tv_usec);
#else
  static unsigned long base;

  if (!base)
    {
      base = GetTickCount();
      return (0);
    }
  return (GetTickCount() - base) * 1000;
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

void run_clock(time_t curtime)
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
