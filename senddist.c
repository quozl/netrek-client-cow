
/* distress.c
 *
 * $Log: senddist.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 * */
#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"



/* this loads all sorts of useful data into a distress struct. */
struct distress *
        loaddistress(enum dist_type i, W_Event * data)
{
  struct distress d;
  struct distress *dist;
  struct obtype *gettarget(W_Window ww, int x, int y, int targtype), *gettarget2(int x, int y, int targtype),
         *target;
  struct player *j;
  struct planet *l;

  dist = (struct distress *) malloc(sizeof(struct distress));

  dist->sender = me->p_no;
  dist->dam = (100 * me->p_damage) / me->p_ship.s_maxdamage;
  dist->shld = (100 * me->p_shield) / me->p_ship.s_maxshield;
  dist->arms = me->p_armies;
  dist->fuelp = (100 * me->p_fuel) / me->p_ship.s_maxfuel;
  dist->wtmp = (100 * me->p_wtemp) / me->p_ship.s_maxwpntemp;
  /* wtmp can be more than 100% - dont let it wrap */
  if (dist->wtmp > 0x7f)
    {
      dist->wtmp = 0x7f;
    }
  dist->etmp = (100 * me->p_etemp) / me->p_ship.s_maxegntemp;
  /* so.. call me paranoid -jmn */
  dist->sts = (me->p_flags & 0xff) | 0x80;
  dist->wtmpflag = ((me->p_flags & PFWEP) > 0) ? 1 : 0;
  dist->etempflag = ((me->p_flags & PFENG) > 0) ? 1 : 0;
  dist->cloakflag = ((me->p_flags & PFCLOAK) > 0) ? 1 : 0;

  dist->distype = i;
  if (dist->distype > generic || dist->distype < take)
    dist->distype = generic;

  target = gettarget2(me->p_x, me->p_y, TARG_PLANET);
  dist->close_pl = target->o_num;

  target = gettarget(data->Window, data->x, data->y, TARG_PLANET);
  dist->tclose_pl = target->o_num;

  target = gettarget2(me->p_x, me->p_y, TARG_ENEMY);
  dist->close_en = target->o_num;

  target = gettarget(data->Window, data->x, data->y, TARG_ENEMY);
  dist->tclose_en = target->o_num;

  target = gettarget2(me->p_x, me->p_y, TARG_FRIEND);
  dist->close_fr = target->o_num;

  target = gettarget(data->Window, data->x, data->y, TARG_FRIEND);
  dist->tclose_fr = target->o_num;

  target = gettarget2(me->p_x, me->p_y, TARG_PLAYER);
  dist->close_j = target->o_num;

  target = gettarget(data->Window, data->x, data->y, TARG_PLAYER);
  dist->tclose_j = target->o_num;

  /* lets make sure these aren't something stupid */
  dist->cclist[0] = 0x80;
  dist->preappend[0] = '\0';
  dist->macroflag = 0;

  return (dist);
}

/* Coordinating function for _SENDING_ a RCD */
/* Send an emergency signal out to everyone. */

emergency(enum dist_type i, W_Event * data)
{
  char    addrbuf[ADDRLEN];
  char    ebuf[200];
  struct distress *dist;
  char    cry[MSG_LEN];
  char   *info;
  int     len;
  int     recip;
  int     group;
  char    newbuf[100];


  group = MTEAM;
  recip = me->p_team;

  dist = loaddistress(i, data);

  if (gen_distress)
    {
      /* send a generic distress message */
      Dist2Mesg(dist, ebuf);
      pmessage(ebuf, recip, group | MDISTR);
    }
  else
    {
      len = makedistress(dist, cry, distmacro[dist->distype].macro);

      if (len > 0)
	{

	  /* klude alert */
	  info = cry;

	  if (strncmp((char *) getaddr2(MTEAM, recip), cry, 8) == 0)
	    {
	      /* this means we should _strip_ the leading bit because it's *
	       * * redundant */
	      info = cry + 9;
	    }

	  pmessage(info, recip, group);
	}
    }

  free(dist);
}

/* the primary subroutine for newmacro, converts the strange and wonderful *
 * newmacro syntax into an actual message. * This is about as inefficient as
 * they come, but how often is the player * going to send a macro?? *  6/3/93
 * - jn */
pmacro(int mnum, char who, W_Event * data)
{
  char    addr;
  int     group, len, recip;
  char    cry[MSG_LEN];
  char   *pm;
  struct distress *dist;


  if (!UseNewMacro)
    return 0;

  /* get recipient and group */

#ifdef TOOLS
  if (keys[0] != '\0')
    {
      if (pm = INDEX((char *) keys, who))
	who = macroKeys[((int) pm) - ((int) keys)].dest;
    }
#endif

  if ((who == 't') || (who == 'T'))
    addr = teamlet[me->p_team];
  else
    addr = who;

  group = getgroup(addr, &recip);

  if (!group)
    {
      printf("Bad group! %c %d %d\n", addr, recip, group);
      return (0);
    }


  pm = macro[mnum].string;

  dist = loaddistress(0, data);

  len = makedistress(dist, cry, pm);
  if (len > 0)
    {

#ifdef MULTILINE_MACROS
      if (multiline_enabled &&
	  (macro[mnum].type == NEWMULTIM))
	pmessage(cry, recip, group | MMACRO);

      else
#endif /* MULTILINE_MACROS */

	pmessage(cry, recip, group);
    }

  free(dist);
  return 1;
}
