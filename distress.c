/* distress.c
 *
 * $Log: distress.c,v $
 * Revision 1.2  2006/08/15 22:37:09  quozl
 * follow bill
 *
 * Revision 1.1.1.1  1998/11/01 17:24:09  siegl
 * COW 3.0 initial revision
 * */
#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include INC_STDLIB
#include <signal.h>
#include <ctype.h>

#ifndef SERVER
#include "Wlib.h"
#endif

#include "defs.h"
#include "struct.h"
#include "data.h"

#include "string_util.h"

/* #$!@$#% length of address field of messages */
#define ADDRLEN 10

void    testmacro(char *bufa, char *bufb, int *inda, int *indb);

/* The two in-line defs that follow enable us to avoid calling strcat over
 * and over again. */
char   *pappend;

#define APPEND(ptr,str)     \
   pappend = str;           \
   while(*pappend)          \
       *ptr++ = *pappend++;

#define APPEND_CAP(ptr,cap,str) \
   pappend = str;               \
   while(*pappend)              \
   {                            \
       *ptr++ = (cap ? toupper(*pappend) : *pappend); \
       pappend++;               \
   }

/* This is a hacked version from the K&R book.  Basically it puts <n> into
 * <s> in reverse and then reverses the string... MH.  10-18-93 */
itoa2(int n, char *s)
{
  int     i, c, j, len;

  if ((c = n) < 0)
    n = -n;

  i = 0;
  do
    {
      s[i++] = n % 10 + '0';
    }
  while ((n /= 10) > 0);

  if (c < 0)
    s[i++] = '-';

  s[i] = '\0';

  len = i--;

  for (j = 0; i > j; j++, i--)
    {
      c = s[i];
      s[i] = s[j];
      s[j] = c;
    }

  return len;
}

/* Like APPEND, and APPEND_CAP, APPEND_INT is an in-line function that stops
 * us from calling sprintf over and over again. */
#define APPEND_INT(ptr, i) \
    ptr += itoa2(i, ptr);


#ifdef SERVER
#define ADDRLEN 10
#define MAXMACLEN 85
extern char *shiptypes[];

#define warning(x)      fprintf(stderr,x)
#endif


char   *getaddr(char who), *getaddr2(int flags, int recip);

#ifdef RCM
extern char *whydeadmess[];

#endif


/* This takes an MDISTR flagged message and makes it into a dist struct */
HandleGenDistr(char *message, unsigned char from, unsigned char to, struct distress *dist)
{

  char   *mtext;
  unsigned char i;

  mtext = &message[ADDRLEN];

#ifndef SERVER
  MZERO((char *) dist, sizeof(dist));
#else
  bzero((char *) dist, sizeof(dist));
#endif

  dist->sender = from;
  dist->distype = mtext[0] & 0x1f;
  dist->macroflag = ((mtext[0] & 0x20) > 0);
  dist->fuelp = mtext[1] & 0x7f;
  dist->dam = mtext[2] & 0x7f;
  dist->shld = mtext[3] & 0x7f;
  dist->etmp = mtext[4] & 0x7f;
  dist->wtmp = mtext[5] & 0x7f;
  dist->arms = mtext[6] & 0x1f;
  dist->sts = mtext[7] & 0x7f;
  dist->wtmpflag = ((dist->sts & PFWEP) > 0) ? 1 : 0;
  dist->etempflag = ((dist->sts & PFENG) > 0) ? 1 : 0;
  dist->cloakflag = ((dist->sts & PFCLOAK) > 0) ? 1 : 0;
  dist->close_pl = mtext[8] & 0x7f;
  dist->close_en = mtext[9] & 0x7f;
  dist->tclose_pl = mtext[10] & 0x7f;
  dist->tclose_en = mtext[11] & 0x7f;
  dist->tclose_j = mtext[12] & 0x7f;
  dist->close_j = mtext[13] & 0x7f;
  dist->tclose_fr = mtext[14] & 0x7f;
  dist->close_fr = mtext[15] & 0x7f;
  i = 0;
  while ((mtext[16 + i] & 0xc0) == 0xc0 && (i < 6))
    {
      dist->cclist[i] = mtext[16 + i] & 0x1f;
      i++;
    }
  dist->cclist[i] = mtext[16 + i];
  if (dist->cclist[i] == 0x80)
    dist->pre_app = 1;
  else
    dist->pre_app = 0;
  dist->preappend[0] = '\0';

  if (mtext[16 + i + 1] != '\0')
    {
      STRNCPY(dist->preappend, mtext + 16 + i + 1, MSG_LEN - 1);
      dist->preappend[MSG_LEN - 1] = '\0';
    }
}

/* this converts a dist struct to the appropriate text (excludes F1->FED text
 * bit).. sorry if this is not what we said earlier jeff.. but I lost the
 * paper towel I wrote it all down on */

Dist2Mesg(struct distress *dist, char *buf)
{
  int     len, i;

  sprintf(buf, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
	  (dist->macroflag << 5) + (dist->distype),
	  dist->fuelp | 0x80,
	  dist->dam | 0x80,
	  dist->shld | 0x80,
	  dist->etmp | 0x80,
	  dist->wtmp | 0x80,
	  dist->arms | 0x80,
	  dist->sts | 0x80,
	  dist->close_pl | 0x80,
	  dist->close_en | 0x80,
	  dist->tclose_pl | 0x80,
	  dist->tclose_en | 0x80,
	  dist->tclose_j | 0x80,
	  dist->close_j | 0x80,
	  dist->tclose_fr | 0x80,
	  dist->close_fr | 0x80);

  /* cclist better be terminated properly otherwise we hose here */
  i = 0;
  while (((dist->cclist[i] & 0xc0) == 0xc0))
    {
      buf[16 + i] = dist->cclist[i];
      i++;
    }
  /* get the pre/append cclist terminator in there */
  buf[16 + i] = dist->cclist[i];
  buf[16 + i + 1] = '\0';

  len = 16 + i + 1;
  if (dist->preappend[0] != '\0')
    {
      strncat(buf, dist->preappend, MSG_LEN - len);	/* false sense of * * 
							 * security? */
      buf[MSG_LEN - 1] = '\0';
    }
}

/* small permutation on the newmacro code... this takes a pointer to a
 * distress structure and a pointer to a macro syntax string, and converts it
 * into a line of text.  9/1/93 - jn */
makedistress(struct distress *dist, char *cry, char *pm)
/* the info */
/* the call for help! (output) - should be array */
/* macro to parse, used for distress and macro */
{
  char    buf1[10 * MAXMACLEN];
  char   *pbuf1;
  char    buf2[10 * MAXMACLEN];
  char    buf3[10 * MAXMACLEN];
  char    tmp[10 * MAXMACLEN];
  int     index = 0;
  int     index2 = 0;
  int     index3 = 0;
  int     cap = 0;
  struct player *sender;
  struct player *j;
  struct planet *l;
  char   *strcap(char *s);

#ifndef SERVER
  extern int ping_tloss_sc;			 /* total % loss 0--100, *

						  * 
						  * * server to client */
  extern int ping_tloss_cs;			 /* total % loss 0--100, *

						  * 
						  * * client to server */
  extern int ping_av;				 /* average rt */
  extern int ping_sd;				 /* standard deviation */

#endif
  char    c;


  sender = &players[dist->sender];

  if (!(*pm))
    {
      cry[0] = '\0';
      return (0);
    }

  buf1[0] = '\0';
  pbuf1 = buf1;

  /* first step is to substitute variables */
  while (*pm)
    {
      if (*pm == '%')
	{
	  pm++;

	  if (!*pm)
	    continue;

	  switch (c = *(pm++))
	    {
	    case ' ':
	      *pbuf1++ = ' ';
	      break;
	    case 'O':				 /* push a 3 character team * 
						  * 
						  * * name into buf */
	      cap = 1;
	    case 'o':				 /* push a 3 character team * 
						  * 
						  * * name into buf */
	      APPEND_CAP(pbuf1, cap, teamshort[sender->p_team]);
	      cap = 0;
	      break;
	    case 'a':				 /* push army number into buf 
						  * 
						  */
	      APPEND_INT(pbuf1, dist->arms);
	      break;
	    case 'd':				 /* push damage into buf */
	      APPEND_INT(pbuf1, dist->dam);
	      break;
	    case 's':				 /* push shields into buf */
	      APPEND_INT(pbuf1, dist->shld);
	      break;
	    case 'f':				 /* push fuel into buf */
	      APPEND_INT(pbuf1, dist->fuelp);
	      break;
	    case 'w':				 /* push wtemp into buf */
	      APPEND_INT(pbuf1, dist->wtmp);
	      break;
	    case 'e':				 /* push etemp into buf */
	      APPEND_INT(pbuf1, dist->etmp);
	      break;

	    case 'P':				 /* push player id into buf */
	    case 'G':				 /* push friendly player id * 
						  * 
						  * * into buf */
	    case 'H':				 /* push enemy target player
						  * * * id into buf */

	    case 'p':				 /* push player id into buf */
	    case 'g':				 /* push friendly player id * 
						  * 
						  * * into buf */
	    case 'h':				 /* push enemy target player
						  * * * id into buf */

	      switch (c)
		{
		case 'p':
		  j = &players[dist->tclose_j];
		  break;
		case 'g':
		  j = &players[dist->tclose_fr];
		  break;
		case 'h':
		  j = &players[dist->tclose_en];
		  break;
		case 'P':
		  j = &players[dist->close_j];
		  break;
		case 'G':
		  j = &players[dist->close_fr];
		  break;
		default:
		  j = &players[dist->close_en];
		  break;
		}
	      *pbuf1++ = j->p_mapchars[1];
	      break;

	    case 'n':				 /* push planet armies into * 
						  * 
						  * * buf */
	      l = &planets[dist->tclose_pl];
	      APPEND_INT(pbuf1,
		       ((l->pl_info & sender->p_team) ? l->pl_armies : -1));
	      break;
	    case 'B':
	      cap = 1;
	    case 'b':				 /* push planet into buf */
	      l = &planets[dist->close_pl];
	      tmp[0] = l->pl_name[0] - 'A' + 'a';
	      tmp[1] = l->pl_name[1];
	      tmp[2] = l->pl_name[2];
	      tmp[3] = '\0';
	      APPEND_CAP(pbuf1, cap, tmp);
	      cap = 0;
	      break;
	    case 'L':
	      cap = 1;
	    case 'l':				 /* push planet into buf */
	      l = &planets[dist->tclose_pl];
	      tmp[0] = l->pl_name[0] - 'A' + 'a';
	      tmp[1] = l->pl_name[1];
	      tmp[2] = l->pl_name[2];
	      tmp[3] = '\0';
	      APPEND_CAP(pbuf1, cap, tmp);
	      cap = 0;
	      break;
	    case 'N':				 /* push planet into buf */
	      l = &planets[dist->tclose_pl];
	      APPEND(pbuf1, l->pl_name);
	      break;
	    case 'Z':				 /* push a 3 character team * 
						  * 
						  * * name into buf */
	      cap = 1;
	    case 'z':				 /* push a 3 character team * 
						  * 
						  * * name into buf */
	      l = &planets[dist->tclose_pl];
	      APPEND_CAP(pbuf1, cap, teamshort[l->pl_owner]);
	      cap = 0;
	      break;
	    case 't':				 /* push a team character * * 
						  * into buf */
	      l = &planets[dist->tclose_pl];
	      *pbuf1++ = teamlet[l->pl_owner];
	      break;
	    case 'T':				 /* push my team into buf */
	      *pbuf1++ = teamlet[sender->p_team];
	      break;
	    case 'r':				 /* push target into buf */
	      j = &players[dist->tclose_j];
	      *pbuf1++ = teamlet[j->p_team];
	      break;
	    case 'c':				 /* push my id char into buf */
	      *pbuf1++ = sender->p_mapchars[1];
	      break;
	    case 'W':				 /* push WTEMP flag into buf */

#ifdef RCM
	      if (dist->distype == rcm)		 /* whydead for RCM */
		{
		  APPEND(pbuf1, whydeadmess[dist->wtmp]);
		}
	      else
#endif

	      if (dist->wtmpflag)
		*pbuf1++ = '1';
	      else
		*pbuf1++ = '0';
	      break;
	    case 'E':				 /* push ETEMP flag into buf */
	      if (dist->etempflag)
		*pbuf1++ = '1';
	      else
		*pbuf1++ = '0';
	      break;
	    case 'K':
	      cap = 1;
	    case 'k':
	      if (cap)
		j = &players[dist->tclose_j];
	      else
		j = sender;

#ifdef RCM
	      if (dist->distype == rcm)
		{
		  APPEND_INT(pbuf1, dist->dam);
		  *pbuf1++ = '.';
		  itoapad(dist->shld, pbuf1, 1, 2);
		  pbuf1 += 2;
		}
	      else
		{
#endif

#ifdef nodef
		  if (j->p_ship.s_type == STARBASE)
		    sprintf(tmp, "%5.2f\0", j->p_stats.st_sbkills);
		  else
		    sprintf(tmp, "%5.2f\0", j->p_stats.st_kills + j->p_stats.st_tkills);
#else
		  sprintf(tmp, "%5.2f\0", j->p_kills);
#endif

		  APPEND(pbuf1, tmp);

#ifdef RCM
		}
#endif

	      break;

	    case 'U':				 /* push player name into buf 
						  * 
						  */
	      cap = 1;
	    case 'u':				 /* push player name into buf 
						  * 
						  */
	      j = &players[dist->tclose_j];
	      APPEND_CAP(pbuf1, cap, j->p_name);
	      cap = 0;
	      break;
	    case 'I':				 /* my player name into buf */
	      cap = 1;
	    case 'i':				 /* my player name into buf */
	      APPEND_CAP(pbuf1, cap, sender->p_name);
	      cap = 0;
	      break;
	    case 'S':				 /* push ship type into buf */

#ifndef SERVER
	      APPEND(pbuf1, classes[sender->p_ship.s_type]);
#else
	      APPEND(pbuf1, shiptypes[sender->p_ship.s_type]);
#endif

	      break;

#ifdef SERVER
	    case 'v':				 /* push average ping round * 
						  * 
						  * * trip time into buf */
	    case 'V':				 /* push ping stdev into buf */
	    case 'y':				 /* push packet loss into buf 
						  * 
						  */
	      *pbuf1++ = '0';
	    case 'M':				 /* push capitalized * *
						  * lastMessage into buf */
	    case 'm':				 /* push lastMessage into buf 
						  * 
						  */
	      break;
#else
	    case 'M':				 /* push capitalized * *
						  * lastMessage into buf */
	      cap = 1;
	    case 'm':				 /* push lastMessage into buf 
						  * 
						  */
	      APPEND_CAP(pbuf1, cap, lastMessage);
	      cap = 0;
	      break;

	    case 'v':				 /* push average ping round * 
						  * 
						  * * trip time into buf */
	      APPEND_INT(pbuf1, ping_av);
	      break;

	    case 'V':				 /* push ping stdev into buf */
	      APPEND_INT(pbuf1, ping_sd);
	      break;

	    case 'y':				 /* push packet loss into buf 
						  * 
						  */
	      /* this is the weighting formula used be socket.c ntserv */
	      APPEND_INT(pbuf1, (2 * ping_tloss_sc + ping_tloss_cs) / 3);
	      break;
#endif

	    case '*':				 /* push %} into buf */
	    case '}':				 /* push %} into buf */
	    case '{':				 /* push %{ into buf */
	    case '!':				 /* push %! into buf */
	    case '?':				 /* push %? into buf */
	    case '%':				 /* push %% into buf */
	      *pbuf1++ = '%';
	      *pbuf1++ = c;
	      break;
	    case '>':				 /* push tab stop */
	      c = '\0';
	      if (*pm >= '0' && *pm <= '9')
		c = (*pm++) - '0';
	      if (*pm >= '0' && *pm <= '9')
		c = c * 10 + ((*pm++) - '0');
	      if (c)
		{
		  *pbuf1++ = '%';
		  *pbuf1++ = '>';
		  *pbuf1++ = c;
		}
	      break;
	    default:
	      /* try to continue bad macro character is skipped entirely, the
	       * * * message will be parsed without whatever %. has occurred.
	       * - * * jn */
	      warning("Bad Macro character in distress!");
	      fprintf(stderr, "Unrecognizable special character in distress pass 1: %c\n", *(pm - 1));
	      break;
	    }
	}
      else
	{
	  *pbuf1++ = *pm++;
	}

    }

  *pbuf1 = '\0';

  /* second step is to evaluate tests, buf1->buf2 */
  testmacro(buf1, buf2, &index, &index2);
  buf2[index2] = '\0';

  if (index2 <= 0)
    {
      cry[0] = '\0';
      return (0);
    }

  index2 = 0;

  /* third step is to include conditional text, buf2->buf3 */
  condmacro(buf2, buf3, &index2, &index3, 1);

  if (index3 <= 0)
    {
      cry[0] = '\0';
      return (0);
    }

  buf3[index3] = '\0';

  cry[0] = '\0';
  strncat(cry, buf3, MSG_LEN - 1);

  return (index3);
}

void    testmacro(char *bufa, char *bufb, int *inda, int *indb)
{
  int     state = 0;
  char    c;

  if (*indb >= 10 * MAXMACLEN)
    return;
  /* maybe we should do something more "safe" here (and at other returns)? */


  while (bufa[*inda] && (*indb < 10 * MAXMACLEN))
    {
      if (state)
	{
	  switch (c = bufa[(*inda)++])
	    {
	    case '*':				 /* push %* into buf */
	    case '%':				 /* push %% into buf */
	    case '{':				 /* push %{ into buf */
	    case '}':				 /* push %} into buf */
	    case '!':				 /* push %! into buf */
	    case '>':				 /* push %>n into buf */
	      if (*indb < 10 * MAXMACLEN - 2)
		{
		  bufb[*indb] = '%';
		  (*indb)++;
		  bufb[*indb] = c;
		  (*indb)++;
		  if (c == '>')
		    bufb[(*indb)++] = bufa[(*inda)++];
		}
	      else
		return;				 /* we are full, so we are *
						  * * done */
	      state = 0;
	      continue;
	      break;

	    case '?':				 /* the dreaded conditional,
						  * * * evaluate it */
	      bufb[*indb] = '0' + solvetest(bufa, inda);
	      (*indb)++;
	      state = 0;
	      continue;
	      break;

	    default:
	      warning("Bad character in Macro!");
	      printf("Unrecognizable special character in macro pass2: %c  Trying to continue.\n",
		     bufa[(*inda) - 1]);
	      state = 0;
	      continue;
	      break;
	    }
	}

      if (bufa[*inda] == '%')
	{
	  state++;
	  (*inda)++;
	  continue;
	}

      state = 0;

      if (*indb < 10 * MAXMACLEN)
	{
	  bufb[*indb] = bufa[*inda];
	  (*inda)++;
	  (*indb)++;
	}
      else
	return;
    }
}

int     solvetest(char *bufa, int *inda)
{
  int     state = 0;
  char    bufh[10 * MAXMACLEN];
  char    bufc[10 * MAXMACLEN];
  int     indh = 0, indc = 0, i;
  char    operation;


  while (bufa[*inda] &&
	 bufa[*inda] != '<' &&
	 bufa[*inda] != '>' &&
	 bufa[*inda] != '=')
    {

      bufh[indh++] = bufa[(*inda)++];
    }
  bufh[indh] = '\0';

  operation = bufa[(*inda)++];

  while (bufa[*inda] &&
	 !(state &&
	   ((bufa[*inda] == '?') ||
	    (bufa[*inda] == '{'))))
    {

      if (state && (bufa[*inda] == '%' ||
		    bufa[*inda] == '!' ||
		    bufa[*inda] == '}'))
	{
	  bufc[indc++] = '%';
	}
      else if (bufa[*inda] == '%')
	{
	  state = 1;
	  (*inda)++;
	  continue;
	}

      state = 0;
      bufc[indc++] = bufa[(*inda)++];
    }
  bufc[indc] = '\0';

  if (bufa[*inda])
    (*inda)--;

  if (!operation)				 /* incomplete is truth, just
						  * * * ask Godel */
    return (1);

  switch (operation)
    {
    case '=':					 /* character by character *
						  * * equality */
      if (indc != indh)
	return (0);
      for (i = 0; i < indc; i++)
	{
	  if (bufc[i] != bufh[i])
	    return (0);
	}
      return (1);
      break;

    case '<':
      if (atoi(bufh) < atoi(bufc))
	return (1);
      else
	return (0);
      break;

    case '>':
      if (atoi(bufh) > atoi(bufc))
	return (1);
      else
	return (0);
      break;

    default:
      warning("Bad operation in Macro!");
      printf("Unrecognizable operation in macro pass3: %c  Trying to continue.\n",
	     operation);
      return (1);				 /* don't know what happened,
						  * * * pretend we do */
      break;
    }
}

int     condmacro(char *bufa, char *bufb, int *inda, int *indb, int flag)
{
  int     newflag, include;
  int     state = 0;


  if (*indb >= MAXMACLEN)
    return 0;

  include = flag;

  while (bufa[*inda] && (*indb < MAXMACLEN))
    {
      if (state)
	{
	  switch (bufa[(*inda)++])
	    {
	    case '}':				 /* done with this * *
						  * conditional, return */
	      return (0);
	      break;

	    case '{':				 /* handle new conditional */
	      if (*indb > 0)
		{
		  (*indb)--;
		  if (bufb[*indb] == '0')
		    newflag = 0;
		  else
		    newflag = 1;
		}
	      else				 /* moron starting with cond,
						  * * * assume true */
		newflag = 1;

	      if (include)
		condmacro(bufa, bufb, inda, indb, newflag);
	      else
		{
		  (*indb)++;
		  *inda = skipmacro(bufa, *inda);
		}

	      state = 0;
	      continue;
	      break;

	    case '!':				 /* handle not indicator */
	      if (flag)
		include = 0;
	      else
		include = 1;

	      state = 0;
	      continue;
	      break;

	    case '*':				 /* test for abort */
	      if (include)
		{
		  /* abort this macro */
		  bufb[0] = '\0';
		  *indb = 0;
		  return (0);
		}

	      state = 0;
	      continue;
	      break;

	    case '%':				 /* push % into buf */
	      if (include)
		{
		  if (*indb < MAXMACLEN)
		    {
		      bufb[*indb] = '%';
		      (*indb)++;
		    }
		  else
		    return (0);
		}
	      state = 0;
	      continue;

	    case '>':				 /* Generate Tab stop */
	      if (include)
		{
		  int     t = (int) bufa[(*inda)++];

		  for (; ((*indb < t) && (*indb < MAXMACLEN)); (*indb)++)
		    bufb[*indb] = ' ';
		}
	      state = 0;
	      continue;

	    default:
	      warning("Bad character in Macro!");
	      printf("Unrecognizable special character in macro pass4: %c  Trying to continue.\n",
		     bufa[(*inda) - 1]);
	    }
	}

      if (bufa[*inda] == '%')
	{
	  state++;
	  (*inda)++;
	  continue;
	}

      state = 0;


      if (include)
	{
	  if (*indb < MAXMACLEN)
	    {
	      bufb[*indb] = bufa[*inda];
	      (*inda)++;
	      (*indb)++;
	    }
	  else
	    return (0);
	}
      else
	(*inda)++;
    }
  return 0;
}

skipmacro(char *buf, int index)
{
  int     state = 0;
  int     end = 0;

  if (index == 0)
    index++;

  while (buf[index] && !end)
    {
      if (state)
	{
	  switch (buf[index++])
	    {
	    case '{':
	      index = skipmacro(buf, index);
	      continue;
	      break;
	    case '}':
	      end = 1;
	      continue;
	      break;
	    case '>':
	      index++;
	    case '!':
	    case '%':
	    case '*':
	      state = 0;
	      continue;
	      break;
	    default:
	      warning("Bad character in Macro!");
	      printf("Unrecognizable special character in macro pass5: %c  Trying to continue.\n",
		     buf[index - 1]);
	    }
	}

      if (buf[index] == '%')
	{
	  state++;
	  index++;
	  continue;
	}

      state = 0;
      index++;
    }

  return (index);
}


/* return a pointer to a capitalized copy of string s */
char   *
        strcap(char *s)
{
  static char buf[256];				 /* returns static */
  register char *t = buf;

  while (*s)
    {
      if (islower(*s))
	*t++ = toupper(*s++);
      else
	*t++ = *s++;
    }
  *t = 0;
  if (buf[255])
    {
      fprintf(stderr, "ERROR: String constant overwritten\n");
      return NULL;
    }
  return buf;
}
