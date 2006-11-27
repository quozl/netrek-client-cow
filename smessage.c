
/* smessage.c
 *
 * $Log: smessage.c,v $
 * Revision 1.2  2006/05/22 13:11:58  quozl
 * fix compilation warnings
 *
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 * */
#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <ctype.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "smessage.h"

static int lcount;
static int HUDoffset;
static char buf[80];
static char cursor = '_';
static char mbuf[80];
char   *getaddr(char who), *getaddr2(int flags, int recip);

/* XFIX */
#define BLANKCHAR(col, n) W_ClearArea(messagew, 5+W_Textwidth*(col), 5, \
    W_Textwidth * (n), W_Textheight);
#define DRAWCURSOR(col) W_WriteText(messagew, 5+W_Textwidth*(col), 5, \
    textColor, &cursor, 1, W_RegularFont);
#define TESTCAP(c,s)    (c ? strcap(s) : s)

/* Routines to handle multi-window messaging */
void    DisplayMessage()
{

#ifdef XTRA_MESSAGE_UI
  if (HUDoffset)
    W_WriteText(w, 5, HUDoffset, textColor,
		outmessage, strlen(outmessage), W_RegularFont);
#endif

  W_WriteText(messagew, 5, 5, textColor,
	      outmessage, strlen(outmessage), W_RegularFont);
}
void    AddChar(char *twochar)
{

#ifdef XTRA_MESSAGE_UI
  if (HUDoffset)
    W_WriteText(w, 5 + W_Textwidth * lcount, HUDoffset, textColor,
		twochar, 2, W_RegularFont);
#endif

  W_WriteText(messagew, 5 + W_Textwidth * lcount, 5, textColor,
	      twochar, 2, W_RegularFont);
}
void    BlankChar(int HUDoffsetcol, int len)
{

#ifdef XTRA_MESSAGE_UI
  if (HUDoffset)
    W_ClearArea(w, 5 + W_Textwidth * (HUDoffsetcol), HUDoffset,
		W_Textwidth * (len), W_Textheight);
#endif

  W_ClearArea(messagew, 5 + W_Textwidth * (HUDoffsetcol), 5,
	      W_Textwidth * (len), W_Textheight);
}
void    DrawCursor(int col)
{

#ifdef XTRA_MESSAGE_UI
  if (HUDoffset)
    W_WriteText(w, 5 + W_Textwidth * (col), HUDoffset,
		textColor, &cursor, 1, W_RegularFont);
#endif

  W_WriteText(messagew, 5 + W_Textwidth * (col), 5,
	      textColor, &cursor, 1, W_RegularFont);
}


void    smessage(char ichar)
{
  int     i;
  char   *getaddr(char who);
  char    twochar[2];
  static char addr, *addr_str, *pm;

  if (messpend == 0)
    {
      messpend = 1;

#ifdef XTRA_MESSAGE_UI
      /* Figure out where to put the message on the local */
      switch (messageHUD)
	{
	case 1:
	  HUDoffset = 5;
	  break;
	case 2:
	  HUDoffset = W_WindowHeight(w) - W_Textheight - 5;
	  break;
	default:
	  HUDoffset = 0;
	}
#endif

      /* Put the proper recipient in the window */

#ifdef TOOLS
      if (keys[0] != '\0')
	{
	  if (pm = INDEX((char *) keys, ichar))
	    ichar = macroKeys[((int) pm) - ((int) keys)].dest;
	}
#endif

      if ((ichar == 't') || (ichar == 'T'))
	addr = teamlet[me->p_team];
      else
	addr = ichar;
      addr_str = getaddr(addr);
      if (addr_str == 0)
	{
	  /* print error message */
	  messpend = 0;
	  message_off();
	  return;
	}
      strcat(outmessage, addr_str);
      lcount = ADDRLEN;
      DrawCursor(ADDRLEN);
      while (strlen(outmessage) < ADDRLEN)
	{
	  strcat(outmessage, " ");
	}
      strcat(outmessage, "_");

      /* Display the header */
      DisplayMessage();

      return;
    }

  if (ichar == ((char) ('h' + 96)) || ichar == ((char) ('H' + 96)))
    ichar = '\b';
  else if (ichar == ((char) ('[' + 96)))
    ichar = '\033';
  else if (ichar == ((char) ('m' + 96)) || ichar == ((char) ('M' + 96)))
    ichar = '\r';
  else if (ichar == ((char) ('j' + 96)) || ichar == ((char) ('J' + 96)))
    ichar = '\r';
  else if (ichar == ((char) ('u' + 96)) || ichar == ((char) ('U' + 96)))
    ichar = 23;

  switch ((unsigned char) ichar & ~(0x80))
    {
    case '\b':					 /* character erase */
    case '\177':
      if (--lcount < ADDRLEN)
	{
	  lcount = ADDRLEN;
	  break;
	}
      BlankChar(lcount + 1, 1);
      DrawCursor(lcount);
      outmessage[lcount + 1] = '\0';
      outmessage[lcount] = cursor;
      break;

    case '\033':				 /* abort message */
      BlankChar(0, lcount + 1);
      mdisplayed = 0;
      messpend = 0;
      message_off();
      for (i = 0; i < 80; i++)
	{
	  outmessage[i] = '\0';
	}
      break;

    case 23:
      while (--lcount >= ADDRLEN)
	{
	  BlankChar(lcount + 1, 1);
	  DrawCursor(lcount);
	  outmessage[lcount + 1] = '\0';
	  outmessage[lcount] = cursor;
	}
      lcount = ADDRLEN;
      break;
    case '\r':					 /* send message */
      buf[lcount - ADDRLEN] = '\0';
      messpend = 0;
      for (i = 0; i < 80; i++)
	{
	  outmessage[i] = '\0';
	}
      switch (addr)
	{
	case 'A':
	  pmessage(buf, 0, MALL);
	  break;
	case 'F':
	  pmessage(buf, FED, MTEAM);
	  break;
	case 'R':
	  pmessage(buf, ROM, MTEAM);
	  break;
	case 'K':
	  pmessage(buf, KLI, MTEAM);
	  break;
	case 'O':
	  pmessage(buf, ORI, MTEAM);
	  break;
	case 'G':
	  pmessage(buf, 0, MGOD);
	  break;

#ifdef TOOLS
	case '!':
	  pmessage(buf, 0, MTOOLS);
	  break;
#endif

	case 'M':
	  pmessage(buf, 0, MMOO);
	  break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  if (players[addr - '0'].p_status == PFREE)
	    {
	      warning("That player left the game. message not sent.");
	      return;
	    }
	  pmessage(buf, addr - '0', MINDIV);
	  break;
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':
	  if (players[addr - 'a' + 10].p_status == PFREE)
	    {
	      warning("That player left the game. message not sent.");
	      return;
	    }
	  pmessage(buf, addr - 'a' + 10, MINDIV);
	  break;
	default:
	  warning("Not legal recipient");
	}
      BlankChar(0, lcount + 1);
      mdisplayed = 0;
      lcount = 0;
      break;

    default:					 /* add character */
      if (lcount >= 79)
	{
	  W_Beep();
	  break;
	}
      if (iscntrl((unsigned char) ichar & ~(0x80)))
	break;
      twochar[0] = ichar;
      twochar[1] = cursor;
      AddChar(twochar);
      outmessage[lcount] = ichar;
      outmessage[lcount + 1] = cursor;
      buf[(lcount++) - ADDRLEN] = ichar;
      break;
    }
}

pmessage(char *str, int recip, int group)
{
  char    newbuf[100];

  /* message length failsafe and last message saving - jn 6/17/93 */
  lastMessage[0] = '\0';
  strncat(lastMessage, str, 79);
  str = lastMessage;

  switch (group)
    {

#ifdef TOOLS
    case MTOOLS:
      sendTools(str);
      break;
#endif

    case MMOO:
      strcpy(defaultsFile, str);
      sprintf(mbuf, "changing defaultsFile to %s", str);
      warning(mbuf);
      break;
    default:
      sendMessage(str, group, recip);
    }

  if ((group == MTEAM && recip != me->p_team) ||
      (group == MINDIV && recip != me->p_no))
    {
      sprintf(newbuf, "%s  %s", getaddr2(group, recip), str);
      newbuf[79] = 0;
      dmessage(newbuf, group, me->p_no, recip);
    }
  message_off();
}

char   *
        getaddr(char who)
{

  switch (who)
    {
    case 'A':
      return (getaddr2(MALL, 0));
    case 'F':
      return (getaddr2(MTEAM, FED));
    case 'R':
      return (getaddr2(MTEAM, ROM));
    case 'K':
      return (getaddr2(MTEAM, KLI));
    case 'O':
      return (getaddr2(MTEAM, ORI));
    case 'G':
      return (getaddr2(MGOD, 0));
    case 'M':
      return (getaddr2(MMOO, 0));

#ifdef TOOLS
    case '!':
      return (getaddr2(MTOOLS, 0));
#endif

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      if (players[who - '0'].p_status == PFREE)
	{
	  warning("Slot is not alive.");
	  return 0;
	}
      return (getaddr2(MINDIV, who - '0'));
      break;
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
      if (who - 'a' + 10 > MAXPLAYER)
	{
	  warning("Player is not in game");
	  return (0);
	}
      if (players[who - 'a' + 10].p_status == PFREE)
	{
	  warning("Slot is not alive.");
	  return 0;
	}

      return (getaddr2(MINDIV, who - 'a' + 10));
      break;
    default:
      warning("Not legal recipient");
      return (0);
    }
}

char   *
        getaddr2(int flags, int recip)
{
  static char addrmesg[ADDRLEN];

  (void) sprintf(addrmesg, " %c%c->", teamlet[me->p_team], shipnos[me->p_no]);
  switch (flags)
    {
    case MALL:
      (void) sprintf(&addrmesg[5], "ALL");
      break;
    case MTEAM:
      (void) sprintf(&addrmesg[5], teamshort[recip]);
      break;
    case MINDIV:
      if (maskrecip)
	{
	  (void) sprintf(&addrmesg[5], "?? ");
	  maskrecip = 0;
	}
      else
	{
	  /* printf("smessage:getaddr2 recip=%d\n",recip); */
	  (void) sprintf(&addrmesg[5], "%c%c ",
			 teamlet[players[recip].p_team], shipnos[recip]);
	}

      break;
    case MGOD:
      (void) sprintf(&addrmesg[5], "GOD");
      break;

#ifdef TOOLS
    case MTOOLS:
      (void) sprintf(addrmesg, "COW: sh>");
      break;
#endif

    case MMOO:
      (void) sprintf(&addrmesg[5], "MOO");
      break;
    }
  return (addrmesg);
}

message_on(void)
{
  messageon = 1;
  W_DefineTextCursor(w);
  W_DefineTextCursor(mapw);

#ifdef XTRA_MESSAGE_UI
  messMouseDelta = 0;
  if (lcount)
    DisplayMessage();
#endif
}

message_off(void)
{
  messageon = 0;
  W_DefineLocalcursor(w);
  W_DefineMapcursor(mapw);
}

#ifdef XTRA_MESSAGE_UI
message_hold(void)
{
  char    twochar[2] =
  {'#', ' '};

  AddChar(twochar);
  message_off();
}
#endif

/* Used in NEWMACRO, useful elsewhere also */
int
        getgroup(char addr, int *recip)
{
  *recip = 0;

  switch (addr)
    {
    case 'A':
      *recip = 0;
      return (MALL);
      break;
    case 'F':
      *recip = FED;
      return (MTEAM);
      break;
    case 'R':
      *recip = ROM;
      return (MTEAM);
      break;
    case 'K':
      *recip = KLI;
      return (MTEAM);
      break;
    case 'O':
      *recip = ORI;
      return (MTEAM);
      break;
    case 'G':
      *recip = 0;
      return (MGOD);
      break;

#ifdef TOOLS
    case '!':
      *recip = 0;
      return (MTOOLS);
      break;
#endif

    case 'M':
      *recip = 0;
      return (MMOO);
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      if (players[addr - '0'].p_status == PFREE)
	{
	  warning("That player left the game. message not sent.");
	  return 0;
	}
      *recip = addr - '0';
      return (MINDIV);
      break;
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
      if (players[addr - 'a' + 10].p_status == PFREE)
	{
	  warning("That player left the game. message not sent.");
	  return 0;
	}
      *recip = addr - 'a' + 10;
      return (MINDIV);
      break;
    default:
      warning("Not legal recipient");
    }
  return 0;
}


pnbtmacro(int c)
{
  switch (macro[c].who)
    {
    case 'A':
      pmessage(macro[c].string, 0, MALL);
      break;
    case 'F':
      pmessage(macro[c].string, FED, MTEAM);
      break;
    case 'R':
      pmessage(macro[c].string, ROM, MTEAM);
      break;
    case 'K':
      pmessage(macro[c].string, KLI, MTEAM);
      break;
    case 'O':
      pmessage(macro[c].string, ORI, MTEAM);
      break;
    case 'T':
      pmessage(macro[c].string, me->p_team, MTEAM);
      break;
    }
}
