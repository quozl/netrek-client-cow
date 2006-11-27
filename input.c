
/* input.c
 *
 * $Log: input.c,v $
 * Revision 1.14  2006/09/19 10:20:39  quozl
 * ut06 full screen, det circle, quit on motd, add icon, add desktop file
 *
 * Revision 1.13  2006/05/22 13:12:48  quozl
 * add camera frame counter
 *
 * Revision 1.12  2006/05/16 06:25:25  quozl
 * some compilation fixes
 *
 * Revision 1.11  2002/06/22 04:43:24  tanner
 * Clean up of SDL code. #ifdef'd out functions not needed in SDL.
 *
 * Revision 1.10  2002/06/20 04:18:38  tanner
 * Merged COW_SDL_MIXER_BRANCH to TRUNK.
 *
 * Revision 1.7.2.1  2002/06/13 04:10:16  tanner
 * Wed Jun 12 22:52:13 2002  Bob Tanner  <tanner@real-time.com>
 *
 * 	* playback.c (pbmain):  Converted enter_ship.wav
 *
 * 	* input.c (Key113): Converted self_destruct.wav
 *
 * 	* input.c (Key109): Converted message.wav
 *
 * 	* local.c (DrawMisc): Converted warning.wav
 *
 * 	* local.c (DrawPlasmaTorps): Converted plasma_hit.wav
 *
 * 	* local.c (DrawTorps): Converted torp_hit.wav
 *
 * 	* sound.h: added EXPLOSION_OTHER_WAV, PHASER_OTHER_WAV,
 * 	FIRE_TORP_OTHER. and the code to load these new sounds.
 *
 * 	* local.c (DrawShips): Converted cloak.wav, uncloak.wav,
 * 	shield_down.wav, shield_up.wav, explosion.wav,
 * 	explosion_other.wav, phaser.wav, phaser_other.wav
 *
 * 	* cowmain.c (cowmain): Converted enter_ship.wav and engine.wav
 *
 * 	* sound.c: added isDirectory to check that the sounddir is
 * 	actually a directory.
 *
 * Tue Jun 11 01:10:51 2002  Bob Tanner  <tanner@real-time.com>
 *
 * 	* system.mk.in: Added SDL_CFLAGS, SDL_CONFIG, SDL_LIBS,
 * 	SDL_MIXER_LIBS
 *
 * 	* sound.c: Added HAVE_SDL wrapper, initialization of SDL system,
 * 	opening of audio device, and loading of 17 cow sounds.
 *
 * 	* cowmain.c (cowmain): HAVE_SDL wrapper to Init_Sound using SDL. I
 * 	moved the Init_Sound method to right after readdefaults() so the
 * 	intro can start playing ASAP.
 *
 * 	* configure.in: Added AC_CANONICAL_SYSTEM, added check for SDL,
 * 	add check for SDL_mixer.
 *
 * 	* config.h.in: add HAVE_SDL
 *
 * 	* spike: See spike/README for details
 *
 * Revision 1.8  2002/06/13 03:58:41  tanner
 * The changes for sound are mostly isolated in local.c, just a few other changes
 * in the commit.
 *
 * 	* playback.c (pbmain):  Converted enter_ship.wav
 *
 * 	* input.c (Key113): Converted self_destruct.wav
 *
 * 	* input.c (Key109): Converted message.wav
 *
 * Revision 1.7  2001/08/21 20:52:15  siegl
 *
 * mouse wheel support
 *
 * Revision 1.6  2001/06/12 02:48:49  quozl
 * add single-step playback keys
 *
 * Revision 1.5  2001/04/28 04:07:45  quozl
 * Redirect keystrokes that would otherwise be
 * ignored to be processed in the tactical window.  Novice users find
 * it painful to have to put the cursor unnecessarily into the
 * tactical window before pressing a key.
 *
 * Revision 1.4  2000/05/19 14:24:52  jeffno
 * Improvements to playback.
 * - Can jump to any point in recording.
 * - Can lock on to cloaked players.
 * - Tactical/galactic repaint when paused.
 * - Can lock on to different players when recording paused.
 *
 * Revision 1.3  1999/08/05 16:44:06  siegl
 * fix B,V core dump with rabbitears
 *
 * Revision 1.2  1999/01/31 16:38:17  siegl
 * Hockey rink background XPM on galactic map in hockey mode.
 *
 * Revision 1.1.1.1  1998/11/01 17:24:10  siegl
 * COW 3.0 initial revision
 * */
#include <setjmp.h>
#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>
#include INC_SYS_TIME
#include INC_SYS_SELECT
#include INC_STRINGS
#include <signal.h>
#include <errno.h>

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"
#include "playerlist.h"
#include "spopt.h"
#include "map.h"

int     detallow = 1;				 /* flag used to figure out * 

						  * 
						  * 
						  * * if we're allowing det */

#ifdef DOC_WIN
static int docline = 0;
static int xtrekrcline = 0;

#endif

int     opened_info = -2;			 /* counter for infowin *

						  * 
						  * * popup, 6/1/93 LAB */

void    doMacro(W_Event *);

#ifdef THREADED
void    input2();
extern jmp_buf env;

#endif

struct obtype *gettarget(W_Window ww, int x, int y, int targtype), *target;
unsigned char key = ' ';

/* this used to be 177 for an unknown reason...I think it may * have included
 * various control characters.  We don't support * those anyway right?? - jn */
#define MAXKEY 224
#define MAXASCII 128

/* this method of avoiding a massive switch should represent * massive
 * performance gains... instead of having to test * n/2 times for n different
 * keys, the key is run directly * via an array of input functions whose
 * index happens * to coorespond to the value of input char. - jn */
int     emptyKey(void), Key32(void), Key33(void), Key34(W_Event * data),
        Key35(void), Key36(void), Key37(void), Key38(void), Key39(void),
        Key40(void), Key41(W_Event * data), Key42(void), Key43(void), Key44(void),
        Key45(void), Key46(void), Key47(void), Key48(void), Key49(void),
        Key50(void), Key51(void), Key52(void), Key53(void), Key54(void),
        Key55(void), Key56(void), Key57(void), Key58(void), Key59(W_Event * data),
        Key60(void), Key61(void), Key62(void), Key63(void), Key64(void),
        Key65(W_Event * data), Key66(void), Key67(void), Key68(void), Key69(W_Event * data),
        Key70(W_Event * data), Key71(W_Event * data), Key72(W_Event * data),
        Key73(W_Event * data), Key74(W_Event * data), Key75(void), Key76(void),
        Key77(W_Event * data), Key78(void), Key79(void), Key80(void), Key81(void),
        Key82(void), Key83(void), Key84(W_Event * data), Key85(void), Key86(void),
        Key87(W_Event * data), Key88(void), Key89(W_Event * data), Key90(W_Event * data),
        Key91(void), Key92(void), Key93(void), Key94(W_Event * data), Key95(W_Event * data),
        Key96(void), Key97(W_Event * data), Key98(void), Key99(void), Key100(void),
        Key101(void), Key102(W_Event * data), Key103(W_Event * data), Key104(void),
        Key105(W_Event * data), Key106(W_Event * data), Key107(W_Event * data),
        Key108(W_Event * data), Key109(void), Key110(W_Event * data), Key111(void),
        Key112(W_Event * data), Key113(void), Key114(void), Key115(void),
        Key116(W_Event * data), Key117(void), Key118(W_Event * data), Key119(void),
        Key120(void), Key121(W_Event * data), Key122(void), Key123(void),
        Key124(void), Key125(void), Key126(W_Event * data), Key127(W_Event * data);

/* control keys */
int     Key131(W_Event * data), Key144(W_Event * data), Key145(W_Event * data),
        Key146(W_Event * data), Key147(W_Event * data), Key148(W_Event * data),
        Key149(W_Event * data), Key150(W_Event * data), Key151(W_Event * data),
        Key152(W_Event * data), Key153(W_Event * data), Key160(W_Event * data),
        Key162(W_Event * data), Key163(W_Event * data), Key175(W_Event * data),
        Key180(W_Event * data), Key194(W_Event * data), Key195(W_Event * data),
        Key197(W_Event * data), Key198(W_Event * data), Key200(W_Event * data),
        Key206(W_Event * data), Key207(W_Event * data), Key204(W_Event * data),
        Key205(W_Event * data), Key208(W_Event * data), Key212(W_Event * data);

typedef struct
  {
    int     (*handler) ();
  }

key_handler_type;

key_handler_type key_handlers[MAXKEY] =
{
  emptyKey,					 /* \0 */
  emptyKey,					 /* 1 */
  emptyKey,					 /* 2 */
  emptyKey,					 /* 3 */
  emptyKey,					 /* 4 */
  emptyKey,					 /* 5 */
  emptyKey,					 /* 6 */
  emptyKey,					 /* 7 */
  emptyKey,					 /* 8 */
  emptyKey,					 /* 9 */
  emptyKey,					 /* 10 */
  emptyKey,					 /* 11 */
  emptyKey,					 /* 12 */
  emptyKey,					 /* 13 */
  emptyKey,					 /* 14 */
  emptyKey,					 /* 15 */
  emptyKey,					 /* 16 */
  emptyKey,					 /* 17 */
  emptyKey,					 /* 18 */
  emptyKey,					 /* 19 */
  emptyKey,					 /* 20 */
  emptyKey,					 /* 21 */
  emptyKey,					 /* 22 */
  emptyKey,					 /* 23 */
  emptyKey,					 /* 24 */
  emptyKey,					 /* 25 */
  emptyKey,					 /* 26 */
  emptyKey,					 /* 27 */
  emptyKey,					 /* 28 */
  emptyKey,					 /* 29 */
  emptyKey,					 /* 30 */
  emptyKey,					 /* 31 */

  Key32,					 /* space */
  Key33,					 /* ! */
  Key34,					 /* " */
  Key35,					 /* # */
  Key36,					 /* $ */
  Key37,					 /* % */
  Key38,					 /* & */
  Key39,					 /* ' */
  Key40,					 /* ( */
  Key41,					 /* ) */
  Key42,					 /* * */
  Key43,					 /* + */
  Key44,					 /* , */
  Key45,					 /* - */
  Key46,					 /* . */
  Key47,					 /* / */
  Key48,					 /* 0 */
  Key49,					 /* 1 */
  Key50,					 /* 2 */
  Key51,					 /* 3 */
  Key52,					 /* 4 */
  Key53,					 /* 5 */
  Key54,					 /* 6 */
  Key55,					 /* 7 */
  Key56,					 /* 8 */
  Key57,					 /* 9 */
  Key58,					 /* : */
  Key59,					 /* ; */
  Key60,					 /* < */
  Key61,					 /* = */
  Key62,					 /* > */
  Key63,					 /* ?  -  you know this is *
						  * * boring as hell... */
  Key64,					 /* @ */
  Key65,					 /* A */
  Key66,					 /* B */
  Key67,					 /* C */
  Key68,					 /* D */
  Key69,					 /* E */
  Key70,					 /* F */
  Key71,					 /* G */
  Key72,					 /* H */
  Key73,					 /* I */
  Key74,					 /* J */
  Key75,					 /* K */
  Key76,					 /* L */
  Key77,					 /* M */
  Key78,					 /* N */
  Key79,					 /* O */
  Key80,					 /* P */
  Key81,					 /* Q */
  Key82,					 /* R */
  Key83,					 /* S */
  Key84,					 /* T */
  Key85,					 /* U */
  Key86,					 /* V */
  Key87,					 /* W */
  Key88,					 /* X */
  Key89,					 /* Y */
  Key90,					 /* Z */
  Key91,					 /* [ */
  Key92,					 /* \ */
  Key93,					 /* ]  -  ascii is fucked... */
  Key94,					 /* ^ */
  Key95,					 /* _ */
  Key96,					 /* ` */
  Key97,					 /* a */
  Key98,					 /* b */
  Key99,					 /* c */
  Key100,					 /* d */
  Key101,					 /* e */
  Key102,					 /* f */
  Key103,					 /* g */
  Key104,					 /* h */
  Key105,					 /* i */
  Key106,					 /* j */
  Key107,					 /* k */
  Key108,					 /* l */
  Key109,					 /* m */
  Key110,					 /* n */
  Key111,					 /* o */
  Key112,					 /* p */
  Key113,					 /* q */
  Key114,					 /* r */
  Key115,					 /* s */
  Key116,					 /* t */
  Key117,					 /* u */
  Key118,					 /* v */
  Key119,					 /* w */
  Key120,					 /* x */
  Key121,					 /* y */
  Key122,					 /* z */
  Key123,					 /* { */
  Key124,					 /* | */
  Key125,					 /* } - my wife was once * *
						  * bitten by a lhama */
  Key126,					 /* ~ */
  Key127,					 /* delete */
  emptyKey,					 /* 128 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  Key131,					 /* ^# */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  Key144,					 /* ^0 */
  Key145,					 /* ^1 */
  Key146,					 /* ^2 */
  Key147,					 /* ^3 */
  Key148,					 /* ^4 */
  Key149,					 /* ^5 */
  Key150,					 /* ^6 */
  Key151,					 /* ^7 */
  Key152,					 /* ^8 */
  Key153,					 /* ^9 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  Key160,					 /* ^@ */
  emptyKey,					 /* 31 */
  Key162,					 /* ^B */
  Key163,					 /* ^C */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  Key175,					 /* ^O */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  Key180,					 /* ^T */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  Key194,					 /* ^b */
  Key195,					 /* ^c */
  emptyKey,					 /* 31 */
  Key197,					 /* ^e */
  Key198,					 /* ^f */
  emptyKey,					 /* 31 */
  Key200,					 /* ^h */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  Key204,					 /* ^l */
  Key205,					 /* ^m */
  Key206,					 /* ^n */
  Key207,					 /* ^o */
  Key208,					 /* ^p */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  Key212,					 /* ^t */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 31 */
  emptyKey,					 /* 223 */
};

unsigned char
        getctrlkey(unsigned char **s)
{
  unsigned char *str = *s;
  unsigned char c;

  /* Character is control key. */
  if (*str == '^')
    {
      str++;
      /* check for '^' key being specified with "^^" */
      if (*str != '^')
	c = *str + 96;
      else
	c = *str;
    }
  else
    c = *str;
  str++;
  *s = str;
  return (c);
}

extern struct shipdef *myshipdef;

initkeymap(void)
{
  unsigned char *str;
  unsigned char c1, c2;

  /* in the future let me strongly recommed we move keymap * completely * *
   * outside of the stats structure. - jn */


  if ((str = myshipdef->keymap) != NULL)
    {
      while (*str != '\0')
	{
	  if (*str >= 32 && *str < 128)
	    {
	      mystats->st_keymap[*str - 32] = *(str + 1);
	    }
	  str += 2;
	}
    }

  /* See if we can get macroKey to work. What a hack -SAC */
  if ((str = (unsigned char *) getdefault("macroKey")) != NULL)
    {
      unsigned char   *p;

      if (strlen((char *)str) == 1)
	{
	  /* This is a little pointless, but it'll preform as per * the *
	   * documentation */
	  mystats->st_keymap[*str - 32] = 'X';
	}
      else if (!strcmpi(str, "TAB"))
	{
	  p = (unsigned char *) "^i";
	  mystats->st_keymap[getctrlkey(&p) - 32] = 'X';
	}
      else if (!strcmpi(str, "ESC"))
	{
	  p = (unsigned char *) "^[";
	  mystats->st_keymap[getctrlkey(&p) - 32] = 'X';
	}
    }

  if ((str = myshipdef->ckeymap) != NULL)
    {
      unsigned char c1, c2;

      while (*str != '\0')
	{

	  if (*str >= 32 && *str < MAXASCII)
	    {
	      c1 = getctrlkey(&str) - 32;
	      c2 = getctrlkey(&str);
	      mystats->st_keymap[c1] = c2;
	    }

	}
    }

#ifdef MOUSE_AS_SHIFT
  if ((str = (unsigned char *) getdefault("b1keymap")) != NULL)
    {
      b1_as_shift = 1;
      while (*str != '\0')
	{
	  if (*str >= 32 && *str < 176)
	    {
	      mystats->st_keymap[*str - 32 + 192] = *(str + 1);
	    }
	  str += 2;
	}
    }

  if ((str = (unsigned char *) getdefault("b2keymap")) != NULL)
    {
      b2_as_shift = 1;
      while (*str != '\0')
	{
	  if (*str >= 32 && *str < 176)
	    {
	      mystats->st_keymap[*str - 32 + 288] = *(str + 1);
	    }
	  str += 2;
	}
    }

  if ((str = (unsigned char *) getdefault("b3keymap")) != NULL)
    {
      b3_as_shift = 1;
      while (*str != '\0')
	{
	  if (*str >= 32 && *str < 176)
	    {
	      mystats->st_keymap[*str - 32 + 384] = *(str + 1);
	    }
	  str += 2;
	}
    }
#endif

  /* note: not stored on server */
  if ((str = myshipdef->buttonmap) != NULL)
    {
      while (*str != '\0')
	{
	  switch (*str++)
	    {
	    case '1':
	      buttonmap[1] = getctrlkey(&str);
	      break;
	    case '2':
	      buttonmap[2] = getctrlkey(&str);
	      break;
	    case '3':
	      buttonmap[3] = getctrlkey(&str);
	      break;

	    case 'd':
	      buttonmap[4] = getctrlkey(&str);
	      break;
	    case 'e':
	      buttonmap[5] = getctrlkey(&str);
	      break;
	    case 'f':
	      buttonmap[6] = getctrlkey(&str);
	      break;
	    case 'g':
	      buttonmap[7] = getctrlkey(&str);
	      break;

#ifdef SHIFTED_MOUSE
	    case '4':
	      buttonmap[9] = getctrlkey(&str);
	      break;
	    case '5':
	      buttonmap[10] = getctrlkey(&str);
	      break;
	    case '6':
	      buttonmap[11] = getctrlkey(&str);
	      break;

	    case 'h':
	      buttonmap[12] = getctrlkey(&str);
	      break;
	    case 'i':
	      buttonmap[13] = getctrlkey(&str);
	      break;
	    case 'j':
	      buttonmap[14] = getctrlkey(&str);
	      break;
	    case 'k':
	      buttonmap[15] = getctrlkey(&str);
	      break;


	    case '7':
	      buttonmap[17] = getctrlkey(&str);
	      break;
	    case '8':
	      buttonmap[18] = getctrlkey(&str);
	      break;
	    case '9':
	      buttonmap[19] = getctrlkey(&str);
	      break;

	    case 'l':
	      buttonmap[20] = getctrlkey(&str);
	      break;
	    case 'm':
	      buttonmap[21] = getctrlkey(&str);
	      break;
	    case 'n':
	      buttonmap[22] = getctrlkey(&str);
	      break;
	    case 'o':
	      buttonmap[23] = getctrlkey(&str);
	      break;

	    case 'a':
	      buttonmap[25] = getctrlkey(&str);
	      break;
	    case 'b':
	      buttonmap[26] = getctrlkey(&str);
	      break;
	    case 'c':
	      buttonmap[27] = getctrlkey(&str);
	      break;

	    case 'p':
	      buttonmap[28] = getctrlkey(&str);
	      break;
	    case 'q':
	      buttonmap[29] = getctrlkey(&str);
	      break;
	    case 'r':
	      buttonmap[30] = getctrlkey(&str);
	      break;
	    case 's':
	      buttonmap[31] = getctrlkey(&str);
	      break;

#endif

	    default:
	      fprintf(stderr, "%c ignored in buttonmap\n", *(str - 1));
	      break;
	    }
	}
    }

}

initinput(void)
{
  /* Nothing doing... */
}

RETSIGTYPE
detsetallow(int _dummy)
{
  detallow = 1;
}

#ifdef THREADED
input()
{
  W_Event event;

  /* Under Windows we spawn off a separate thread to handle network *
   * interaction; this is because it would require some awkward rewrites to * 
   * integrate event detection and handling into the select() mechanism. It * 
   * probably also increases performance */
  THREAD(input2);

  while (1)
    {
      if (!W_WaitForEvent())			 /* W_WaitForEvent returns 0
						  * * if W_TerminateWait is * 
						  * called */
	break;
      process_event();
    }

  printf("Resetting game\n");

  while (W_EventsPending())
    W_NextEvent(&event);

  longjmp(env, 0);
}

void    input2()
#else
input()
#endif
{
  fd_set  readfds;

#ifndef HAVE_WIN32
  register
  int     xsock = W_Socket();			 /* put it in a register */

#endif
  register
  int     doflush = 0;

  while (1)
    {

      if (keepInfo > 0 && opened_info != -2 &&	 /* 6/1/93 LAB */
	  opened_info < 0 && infomapped)
	destroyInfo();

      if (keepInfo > 0 && opened_info != -2)
	opened_info--;

      FD_ZERO(&readfds);

#ifndef HAVE_WIN32
      FD_SET(xsock, &readfds);
#endif

      FD_SET(sock, &readfds);

      if (udpSock >= 0)
	FD_SET(udpSock, &readfds);

      /* Skip read from server if select results in error. */
      if (SELECT(max_fd, &readfds, 0, 0, 0) > 0)
	{

#ifndef THREADED

#ifndef HAVE_WIN32
	  if (FD_ISSET(xsock, &readfds))
#else
	  if (W_EventsPending())
#endif /* !HAVE_WIN32 */

	    {
	      process_event();
	      /* NOTE: we're no longer calling XPending(), need this */
	      doflush = 1;
	    }
#endif /* !THREADED */

	  if (FD_ISSET(sock, &readfds) ||
	      (udpSock >= 0 && FD_ISSET(udpSock, &readfds)))
	    {
	      intrupt(&readfds);
	      /* NOTE: we're no longer calling XPending(), need this */
	      doflush = 1;
	      if (isServerDead())
		{
		  printf("Whoops!  We've been ghostbusted!\n");
		  printf("Pray for a miracle!\n");
		  /* UDP fail-safe */
		  commMode = commModeReq = COMM_TCP;
		  commSwitchTimeout = 0;
		  if (udpSock >= 0)
		    closeUdpConn();
		  if (udpWin)
		    {
		      udprefresh(UDP_CURRENT);
		      udprefresh(UDP_STATUS);
		    }
		  connectToServer(nextSocket);
		  printf("Yea!  We've been resurrected!\n");
		}
	    }

	}

      /* NOTE: we're no longer calling XPending(), need this */
      if (doflush)
	{
	  W_Flush();
	  doflush = 0;
	}
    }
}

process_event(void)
{
  W_Event data;
  int     loop = 0;
  W_Callback handler;

  do
    {
      loop++;

      if (!W_SpNextEvent(&data))
	continue;				 /* continues at loop bottom */

      switch ((int) data.type)
	{
	case W_EV_KEY:
	  if ((handler = W_GetWindowKeyDownHandler(data.Window)) != NULL)
	    (*handler) (&data);

#ifdef DOC_WIN
	  else if (data.Window == docwin)
	    switch (data.key)
	      {
	      case 'f':
		docline += 28;

		if (docline >= maxdoclines)
		  docline = maxdoclines - 28;

		showdocs(docline);
		break;
	      case 'b':
		docline -= 28;

		if (docline < 0)
		  docline = 0;

		showdocs(docline);
		break;
	      case 'F':
		docline += 4;

		if (docline >= maxdoclines)
		  docline = maxdoclines - 28;

		showdocs(docline);
		break;
	      case 'B':
		docline -= 4;

		if (docline < 0)
		  docline = 0;

		showdocs(docline);
		break;
	      default:
		data.Window = w;
		keyaction(&data);
		break;
	      }
	  else if (data.Window == xtrekrcwin)
	    switch (data.key)
	      {
	      case 'f':
		xtrekrcline += 28;

		if (xtrekrcline >= maxxtrekrclines)
		  xtrekrcline = maxxtrekrclines - 28;

		showxtrekrc(xtrekrcline);
		break;
	      case 'b':
		xtrekrcline -= 28;

		if (xtrekrcline < 0)
		  xtrekrcline = 0;

		showxtrekrc(xtrekrcline);
		break;
	      case 'F':
		xtrekrcline += 4;

		if (xtrekrcline >= maxxtrekrclines)
		  xtrekrcline = maxxtrekrclines - 28;

		showxtrekrc(xtrekrcline);
		break;
	      case 'B':
		xtrekrcline -= 4;

		if (xtrekrcline < 0)
		  xtrekrcline = 0;

		showxtrekrc(xtrekrcline);
		break;
	      default:
		data.Window = w;
		keyaction(&data);
		break;
	      }
#endif

	  else if (messageon)
	    smessage(data.key);
	  else
	    keyaction(&data);
	  break;

#ifdef MOTION_MOUSE
	case W_EV_CM_BUTTON:
#endif

	case W_EV_BUTTON:
	  if ((handler = W_GetWindowButtonHandler(data.Window)) != NULL)
	    (*handler) (&data);
	  else
	    buttonaction(&data);
	  break;

	case W_EV_EXPOSE:
	  if ((handler = W_GetWindowExposeHandler(data.Window)) != NULL)
	    (*handler) (&data);
	  else if (data.Window == mapw)
	    redrawall = 1;
	  else if (data.Window == warnw)
	    W_ClearWindow(warnw);
	  else if (data.Window == messagew)
	    W_ClearWindow(messagew);

#ifdef XTREKRC_HELP
	  else if (defWin && (data.Window == defWin))
	    showdef();
#endif

#ifdef DOC_WIN
	  else if (docwin && (data.Window == docwin))
	    showdocs(docline);
	  else if (xtrekrcwin && (data.Window == xtrekrcwin))
	    showxtrekrc(xtrekrcline);
#endif

	  break;

	default:
	  break;
	}
    }
  while (W_EventsQueued());
  return loop;
}


keyaction(W_Event * data)
{
  char    mbuf[80];
  char    buf[80];
  char   *t;
  int     targettype;


#ifdef NBT
  int     c;
  char    who;
  int     found;

#endif

  fastQuit = 0;					 /* any event, cancel * *
						  * fastquit! */

  if (data->Window != mapw && data->Window != w && data->Window != infow
      && data->Window != scanw)
    {
      /* redirect any orphaned keystrokes to main window */
      data->Window = w;
      data->x = data->y = TWINSIDE / 2;
    }

  key = data->key;

  if (data->Window == infow)
    {
      int     x, y;

      if (findMouseInWin(&x, &y, w))
	{					 /* local window */
	  data->Window = w;
	  data->x = x;
	  data->y = y;
	}
      else if (findMouseInWin(&x, &y, mapw))
	{					 /* galactic window */
	  data->Window = mapw;
	  data->x = x;
	  data->y = y;
	}
    }


  /* this may represent a considerable efficiency improvement */
  /* removes the need for an INDEX and a couple tests */
  if (localflags & (PFREFIT))
    {
      switch (key)
	{
	case 'c':
	  sendRefitReq(CRUISER);
	  localflags &= ~(PFREFIT);
	  return;
	  break;
	case 'o':
	  sendRefitReq(STARBASE);
	  localflags &= ~(PFREFIT);
	  return;
	  break;
	case 'a':
	  sendRefitReq(ASSAULT);
	  localflags &= ~(PFREFIT);
	  return;
	  break;
	case 'd':
	  sendRefitReq(DESTROYER);
	  localflags &= ~(PFREFIT);
	  return;
	  break;
	case 'g':
	  sendRefitReq(SGALAXY);
	  localflags &= ~(PFREFIT);
	  return;
	  break;
	case 'b':
	  sendRefitReq(BATTLESHIP);
	  localflags &= ~(PFREFIT);
	  return;
	  break;
	case 's':
	  sendRefitReq(SCOUT);
	  localflags &= ~(PFREFIT);
	  return;
	  break;
	case '*':
	  sendRefitReq(ATT);
	  localflags &= ~(PFREFIT);
	  return;
	  break;
	default:
	  localflags &= ~(PFREFIT);
	  return;
	  break;
	}
    }

#ifdef RECORDGAME
  if (playback)
    switch(key)
      {
      case 0x8:
      case 0xd:
	pbsetspeed(key);
	return;
      }
#endif

  if (key >= 32 && key < MAXKEY)
    {
      key = mystats->st_keymap[key - 32];
    }
  else
    {
      W_Beep();
      return;
    }

#ifdef RECORDGAME
  /* If playing a recorded game, do not use regular keys. */
  /* What follows is a hardcoded list of commands */
  if (playback)
    {
      extern int pb_sequence_count;
      struct obtype *target;

      switch (key)
	{
	case 'Q':
	case 'q':
	  /* Instant Quit */
	  terminate(0);
	  break;
	case ' ':
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
	case '!':
	case '#':
	case '%':
	case '(':
	case ')':
	case '<':
	case '>':
	case '@':
	case 'R':
        case 'j':
        case 'J':
        case 's':
	  pbsetspeed(key);
	  return;
	  break;
	case 'l':
	  target = gettarget(data->Window, data->x, data->y, TARG_PLAYER | TARG_CLOAK);
	  pblockplayer(target->o_num);
	  return;
	  break;
	case ';':
	  target = gettarget(data->Window, data->x, data->y, TARG_PLANET);
	  pblockplanet(target->o_num);
	  return;
	  break;
        case '=':
          printf("sequence #%d\n", pb_sequence_count);
          return;
          break;
	case 'I':
	case 'i':
	case '?':
	  /* Do the normal function */
	  /* Used for commands that do not try to send packets */
	  break;
	default:
	  /* If key is not in above list dont run it. */
	  return;
	}
    }
#endif


  /* suggestion:  later we can add an option removing this to the emptyKey()
   * * * function.  This would improve input efficiency considerably when * * 
   * "singleMacro" is non-NULL. - jn */

  if ((!MacroMode)
      && singleMacro[0] != '\0'
      && INDEX((char *) singleMacro, data->key))
    {
      MacroMode = 1;
      MacroNum = -1;
    }

  if (MacroMode)
    {
      doMacro(data);
      return;
    }

#ifdef MOTION_MOUSE
  if ((data->type == W_EV_CM_BUTTON) &&		 /* KOC - 10/18/95     */
      (!motion_mouse_enablable) &&		 /* Hack for            */
      (key != 107))				 /* continuous_steer    */
    return;
#endif

  (*(key_handlers[key].handler)) (data);
}

#ifdef MOUSE_AS_SHIFT
mkeyaction(W_Event * data)
{
  unsigned char key = data->key;

  fastQuit = 0;					 /* any event, cancel * *
						  * fastquit! */

  if (!INDEX("sbogadc", key) || !(localflags & PFREFIT))
    {
      if (key >= 32 && key < 176)
	{
	  int     offset;

	  switch (data->modifier)
	    {
	    case W_LBUTTON:
	      offset = 192;
	      break;

	    case W_MBUTTON:
	      offset = 288;
	      break;

	    case W_RBUTTON:
	      offset = 384;
	      break;
	    }

	  key = mystats->st_keymap[key - 32 + offset];
	}
    }

  data->key = key;
  keyaction(data);
}
#endif

buttonaction(W_Event * data)
{
  unsigned char course;
  struct obtype *gettarget(W_Window ww, int x, int y, int targtype);

  if (messageon)
    message_off();				 /* ATM */

  fastQuit = 0;					 /* any event, cancel * *
						  * fastquit! */

#ifdef RECORDGAME
  /* While playing back recorded games, ignore the mouse */
  if (playback)
    return;
#endif

#ifdef SHORT_PACKETS
  if (data->Window == reviewWin)
    {
      if (recv_mesg)
	{
	  sendShortReq(SPK_MOFF);
	}
      else
	{
	  sendShortReq(SPK_MON);
	}
      return;
    }
#endif


  if (data->Window != w && data->Window != mapw
      && data->Window != infow
      && data->Window != scanwin)
    return;

#ifdef SHIFTED_MOUSE
  if (data->key >= W_LBUTTON && data->key < W_BUTTON_RANGE)
#else
  if (data->key > 0 && data->key <= 3)
#endif

    {
      if (buttonmap[data->key] != '\0')
	{
	  data->key = buttonmap[data->key];
	  keyaction(data);
	  return;
	}
    }

#ifdef MOTION_MOUSE
  if ((data->type == W_EV_CM_BUTTON) &&		 /* KOC - 10/18/95     */
      (!motion_mouse_enablable) &&		 /* Hack for            */
      (data->key != W_RBUTTON))			 /* continuous_steer    */
    return;
#endif

  if (data->Window == infow)
    {
      int     x, y;

      if (findMouseInWin(&x, &y, w))
	{					 /* local window */
	  data->Window = w;
	  data->x = x;
	  data->y = y;
	}
      else if (findMouseInWin(&x, &y, mapw))
	{					 /* galactic window */
	  data->Window = mapw;
	  data->x = x;
	  data->y = y;
	}
    }

  if (data->key == W_RBUTTON)
    {
      course = getcourse(data->Window, data->x, data->y);
      set_course(course);
    }
  else if (data->key == W_LBUTTON)
    {
      course = getcourse(data->Window, data->x, data->y);
      sendTorpReq(course);
    }
  else if (data->key == W_MBUTTON)
    {
      course = getcourse(data->Window, data->x, data->y);
      sendPhaserReq(course);
    }

#ifdef NODEF /* SHIFTED_MOUSE - no defaults if not set */
  else if (data->key == W_RBUTTON2)
    {
      set_speed(me->p_ship.s_maxspeed / 2);
      localflags &= ~(PFREFIT);
    }
  else if (data->key == W_LBUTTON2)
    {
      set_speed(99);				 /* Max speed... */
      localflags &= ~(PFREFIT);
    }
  else if (data->key == W_MBUTTON2)
    {
      detmine();
    }
  else if (data->key == W_RBUTTON3)
    {
      if (!infomapped)
	{
	  inform(data->Window, data->x, data->y, 'i');
	  opened_info = keepInfo;		 /* 5/31/93 LAB */
	}
      else
	{
	  destroyInfo();
	  opened_info = -2;
	}
    }
  else if (data->key == W_LBUTTON3)
    {
      shield_tog();
    }
  else if (data->key == W_MBUTTON3)
    {
      cloak();
    }
  else if (data->key == W_RBUTTON4)
    {
      lockPlanetOrBase(data->Window, data->x, data->y);
    }
  else if (data->key == W_LBUTTON4)
    {
      struct obtype *gettarget(W_Window ww, int x, int y, int targtype),
             *target;

      if (me->p_flags & (PFTRACT | PFPRESS))
	sendTractorReq(0, me->p_no);
      target = gettarget(data->Window, data->x, data->y, TARG_PLAYER);
      me->p_tractor = target->o_num;
      sendTractorReq(1, target->o_num);
    }
  else if (data->key == W_MBUTTON4)
    {
      struct obtype *gettarget(W_Window ww, int x, int y, int targtype),
             *target;

      if (me->p_flags & (PFTRACT | PFPRESS))
	sendRepressReq(0, me->p_no);
      target = gettarget(data->Window, data->x, data->y, TARG_PLAYER);
      me->p_tractor = target->o_num;
      sendRepressReq(1, target->o_num);
    }
#endif
}

getcourse(W_Window ww, int x, int y)
{
  if (ww == mapw)
    {
      int     me_x, me_y;

      me_x = me->p_x * GWINSIDE / GWIDTH;
      me_y = me->p_y * GWINSIDE / GWIDTH;
      return ((unsigned char) nint((atan2((double) (x - me_x),
			     (double) (me_y - y)) / 3.14159 * 128.) + 0.5));
    }
  else
    return ((unsigned char) nint((atan2((double) (x - TWINSIDE / 2),
					(double) (TWINSIDE / 2 - y))
				  / 3.14159 * 128.) + 0.5));
}

scan(W_Window w, int x, int y)
{
}

detmine(void)
{
  register int i;

  for (i = 0; i < MAXTORP; i++)
    {
      if (torps[i + (me->p_no * MAXTORP)].t_status == TMOVE ||
	  torps[i + (me->p_no * MAXTORP)].t_status == TSTRAIGHT)
	{
	  sendDetMineReq(i + (me->p_no * MAXTORP));

#ifdef SHORT_PACKETS
	  if (recv_short)
	    break;				 /* Let the server det for me 
						  * 
						  */
#endif
	}
    }
}

lockPlanetOrBase(W_Window ww, int x, int y)	 /* special version of * *
						  * gettarget, 6/1/93 LAB */


{
  register int i;
  register struct player *j;
  register struct planet *k;
  int     g_x, g_y;
  double  dist, closedist;
  register int targtyp, targnum;

  if (ww == mapw)
    {
      g_x = x * GWIDTH / GWINSIDE;
      g_y = y * GWIDTH / GWINSIDE;
    }
  else
    {
      g_x = me->p_x + ((x - TWINSIDE / 2) * SCALE);
      g_y = me->p_y + ((y - TWINSIDE / 2) * SCALE);
    }
  closedist = GWIDTH;

  for (i = 0, k = &planets[i]; i < MAXPLANETS; i++, k++)
    {
      dist = hypot((double) (g_x - k->pl_x), (double) (g_y - k->pl_y));
      if (dist < closedist)
	{
	  targtyp = PLANETTYPE;
	  targnum = i;
	  closedist = dist;
	}
    }

  for (i = 0, j = &players[i]; i < MAXPLAYER; i++, j++)
    {
      if (j->p_status != PALIVE)
	continue;
      if (j->p_flags & PFCLOAK)
	continue;
      if (j == me)
	continue;
      if ((j->p_ship.s_type == STARBASE) && (j->p_team == me->p_team))
	{
	  dist = hypot((double) (g_x - j->p_x), (double) (g_y - j->p_y));
	  if (dist < closedist)
	    {
	      targtyp = PLAYERTYPE;
	      targnum = i;
	      closedist = dist;
	    }
	}
    }

  if (targtyp == PLAYERTYPE)
    {
      sendPlaylockReq(targnum);
      me->p_playerl = targnum;
    }
  else
    {
      sendPlanlockReq(targnum);
      me->p_planet = targnum;
    }

}


emptyKey(void)
{
  W_Beep();
}


void    macro_on(void)
{
  warning("In Macro Mode");
  MacroMode = 1;
  MacroNum = -1;
}


void    doMacro(W_Event * data)
{
  struct obtype *gettarget(W_Window ww, int x, int y, int targtype), *target;
  int     targettype;
  struct dmacro_list *dm;
  enum dist_type i;

#ifdef NBT
  int     c;
  char    who;
  int     found = 0;

#endif

  warning(" ");					 /* We are here now, so turn
						  * * off macro mode */
  MacroMode = 0;


#ifdef NBT
  if (data->key == '?')
    {
      showMacroWin();
      return;
    }


  /* user defined macros *OVERRIDE* receiver configurable distresses.  */

  if (UseNewMacro)
    {
      /* sorry guys, I only program in kludge - jn 6/3/93 */

      if (MacroNum > -1)
	{					 /* macro identified, who to? 
						  * 
						  */
	  if (MacroNum >= MAX_MACRO)
	    fprintf(stderr, "Unknown Macro Num!  There is a macro bug!!\n");

	  if (!pmacro(MacroNum, data->key, data))
	    W_Beep();

	  MacroNum = -1;
	  return;
	}
    }


  for (c = 0; c < macrocnt; ++c)
    {
      if (macro[c].key == data->key)
	{
	  if (!UseNewMacro)
	    {
	      if (rejectMacro && macro[c].type != NBTM)
		warning("NEWMACROs not allowed at this server!!");
	      else
		pnbtmacro(c);

	      return;
	    }


	  /* Use New Macro */

	  switch (macro[c].type)
	    {
	    case NBTM:

	      pnbtmacro(c);
	      return;
	      break;

	    case NEWM:

	      warning("Send macro to which player?");
	      MacroNum = c;
	      MacroMode = 1;			 /* Need another key */
	      return;
	      break;

	    case NEWMMOUSE:
	      {
		/* first translate into who, then send */
		switch (macro[c].who)
		  {
		    struct player *j;
		    struct planet *l;

		  case MACRO_FRIEND:
		  case MACRO_ENEMY:
		  case MACRO_PLAYER:

		    targettype = TARG_PLAYER;
		    if (macro[c].who == MACRO_ENEMY)
		      targettype |= TARG_ENEMY;
		    else if (macro[c].who == MACRO_FRIEND)
		      targettype |= TARG_FRIEND;

		    target = gettarget(data->Window, data->x, data->y,
				       TARG_PLAYER | TARG_CLOAK);
		    if (target->o_type == PLAYERTYPE)
		      {
			j = &players[target->o_num];
			if (j->p_flags & PFCLOAK)
			  maskrecip = 1;
			who = j->p_mapchars[1];
		      }
		    else
		      {
			who = me->p_mapchars[1];
			warning("Can only send a message to a player");
		      }
		    break;

		  case MACRO_TEAM:
		    target = gettarget(data->Window, data->x, data->y,
				       TARG_PLAYER | TARG_PLANET);
		    if (target->o_type == PLANETTYPE)
		      {
			l = &planets[target->o_num];
			who = teamlet[l->pl_owner];
		      }
		    else if (target->o_type == PLAYERTYPE)
		      {
			j = &players[target->o_num];
			who = j->p_mapchars[0];
		      }
		    else
		      {
			who = me->p_mapchars[1];
			warning("Player or planet only please");
		      }
		    break;

		  default:
		    who = me->p_mapchars[1];
		    break;
		  }

		if (!pmacro(c, who, data))
		  W_Beep();

		return;
		break;
	      }

#ifdef MULTILINE_MACROS
	    case NEWMULTIM:

	      if (!pmacro(c, macro[c].who, data))
		W_Beep();

	      found = 1;
	      break;				 /* Loop again */
#endif

	    case NEWMSPEC:

	      if (!pmacro(c, macro[c].who, data))
		W_Beep();
	      return;
	      break;


	    default:
	      fprintf(stderr, "Unknown Macro Type!  Jeff's a twink!!\n");
	      warning("Unknown macro type (eg There is a macro bug)");
	      return;
	      break;
	    }
	}
    }

  if (found)
    return;


#ifdef DIST_KEY_NAME
  /* scan for distress call here */

  for (i = take; distmacro[i].name; i++)
    {
      if (distmacro[i].c == data->key)
	{
	  emergency(i, data);
	  return;
	}
    }
#endif


  warning("Unknown macro");
  W_Beep();
#endif
}


Key32(void)
{
  /* ' ' = clear special windows */
  W_UnmapWindow(planetw);
  W_UnmapWindow(rankw);
  if (infomapped)
    destroyInfo();
  W_UnmapWindow(helpWin);

#ifdef NBT
  W_UnmapWindow(macroWin);
#endif

#ifdef XTREKRC_HELP
  if (defWin)
    W_UnmapWindow(defWin);
#endif

  W_UnmapWindow(war);
  if (optionWin)
    optiondone();
  if (scanmapped)
    {
      W_UnmapWindow(scanwin);
      scanmapped = 0;
    }
  if (udpWin)
    udpdone();

}

Key33(void)
{
  set_speed(11);
}

Key34(W_Event * data)
{
  W_FullScreenToggle(baseWin);
}

Key35(void)
{
  set_speed(me->p_ship.s_maxspeed / 2);
}

Key36(void)
{
  sendTractorReq(0, me->p_no);
}

Key37(void)
{
  set_speed(99);				 /* Max speed... */
}

Key38(void)
{
  reread_defaults();
}

Key39(void)
{

#ifdef HAVE_XPM
  W_GalacticBgd(MAP_PIX);
  W_LocalBgd(LOCAL_PIX);
  sendUdpReq(COMM_UPDATE);
#else
  emptyKey();
#endif
}

Key40(void)
{
  set_speed(10);
}

Key41(W_Event * data)
{
  set_speed(10);
}

Key42(void)
{
  sendPractrReq();
}

Key43(void)
{
  /* UDP: pop up UDP control window */
  if (udpWin != NULL && W_IsMapped(udpWin))
    udpdone();
  else
    {
      char    buf[80];

      udpwindow();
      sprintf(buf, "UDP client version %.1f",
	      (float) UDPVERSION / 10.0);
      warning(buf);
    }

}

Key44(void)
{
  if (W_IsMapped(pStats))
    {
      W_UnmapWindow(pStats);
    }
  else
    {
      W_MapWindow(pStats);
      redrawPStats();
    }

}

Key45(void)
{

#ifdef SHORT_PACKETS
  sendShortReq(SPK_SALL);
#endif

}

Key46(void)
{
  if (netstatWin != NULL && W_IsMapped(netstatWin))
    nsdone();
  else
    nswindow();
}

Key47(void)
{
  sortPlayers = !sortPlayers;
  RedrawPlayerList();
}

Key48(void)
{
  set_speed(0);
}

Key49(void)
{
  set_speed(1);
}

Key50(void)
{
  set_speed(2);
}

Key51(void)
{
  set_speed(3);
}

Key52(void)
{
  set_speed(4);
}

Key53(void)
{
  set_speed(5);
}

Key54(void)
{
  set_speed(6);
}

Key55(void)
{
  set_speed(7);
}

Key56(void)
{
  set_speed(8);
}

Key57(void)
{
  set_speed(9);
}

Key58(void)
{
  logmess = !logmess;
  if (logmess)
    warning("Message logging is ON");
  else
    warning("Message logging is OFF");

}

Key59(W_Event * data)
{
  lockPlanetOrBase(data->Window, data->x, data->y);

}

/* < */
Key60(void)
{
  set_speed(me->p_speed - 1);
}

Key61(void)
{
  /* UDP: request for full update */
  sendUdpReq(COMM_UPDATE);
}

/* > */
Key62(void)
{
  set_speed(me->p_speed + 1);
}

Key63(void)
{
  if (W_IsMapped(phaserwin))
    phaserWindow = 1;
  if (!W_IsMapped(reviewWin))
    {
      if (W_IsMapped(messwa))
	{
	  W_UnmapWindow(messwa);
	  W_UnmapWindow(phaserwin);
	  W_UnmapWindow(messwt);
	  W_UnmapWindow(messwi);
	  W_UnmapWindow(messwk);
	}
      else
	{
	  W_MapWindow(reviewWin);
	}
    }
  else
    {
      W_UnmapWindow(reviewWin);
      W_MapWindow(messwa);
      W_MapWindow(messwt);
      W_MapWindow(messwi);
      W_MapWindow(messwk);
      if (phaserWindow)
	W_MapWindow(phaserwin);
      if (W_IsMapped(statwin))
	{
	  W_UnmapWindow(statwin);
	  W_MapWindow(statwin);
	}
    }
  if (optionWin)
    {
      optionredrawtarget(reviewWin);
      optionredrawtarget(messwa);
      optionredrawtarget(phaserwin);
      optionredrawtarget(messwt);
      optionredrawtarget(messwi);
      optionredrawtarget(messwk);
    }
}

Key64(void)
{
  set_speed(12);
}

Key65(W_Event * data)
{
  /* W_ShowBitmaps(); */
  emptyKey();
}

Key66(void)
{
  showgalactic++;
  if (showgalactic > 4)
    showgalactic = 0;

  redrawall = 2;
}

Key67(void)
{
  sendCoupReq();
  camera_snap();
}

Key68(void)
{
  detmine();

#ifdef AUTOKEY
  if (autoKey)
    autoKeyAllOff();				 /* xx */
#endif
}

/* E */
Key69(W_Event * data)
{
  emergency(generic, data);
}

/* F */
Key70(W_Event * data)
{
  emergency(carrying, data);
}

Key71(W_Event * data)
{
  emptyKey();
}

Key72(W_Event * data)
{
  emptyKey();
}

Key73(W_Event * data)
{
  /* I = get extended information */
  if (!infomapped)
    {
      inform(data->Window, data->x, data->y, key);
      opened_info = keepInfo;			 /* 5/31/93 LAB */
    }
  else
    {
      destroyInfo();
      opened_info = -2;
    }

}

Key74(W_Event * data)
{
  emptyKey();
}

Key75(void)
{

#ifdef HAVE_XPM
  W_GalacticBgd(GREET_PIX);
#endif
}

Key76(void)
{
  if (W_IsMapped(playerw))
    {
      W_UnmapWindow(playerw);
    }
  else
    {
      W_MapWindow(playerw);
    }

}

Key77(W_Event * data)
{

#ifdef TOOLS
  showToolsWin();
#else
  emptyKey();
#endif
}

Key78(void)
{
  /* N = Toggle Name mode */
  namemode = !namemode;
  if (optionWin)
    optionredrawoption(&namemode);

}

Key79(void)
{
  if (optionWin != NULL && W_IsMapped(optionWin))
    optiondone();
  else
    optionwindow();
}

Key80(void)
{
  if (W_IsMapped(planetw))
    {
      W_UnmapWindow(planetw);
    }
  else
    {
      W_MapWindow(planetw);
    }

}

Key81(void)
{

#ifdef SOUND
  Play_Sound(SELF_DESTRUCT_SOUND);
#endif

  sendQuitReq();

}

Key82(void)
{
  sendRepairReq(1);
}

Key83(void)
{
  if (W_IsMapped(statwin))
    {
      W_UnmapWindow(statwin);
    }
  else
    {
      W_MapWindow(statwin);
    }

}

Key84(W_Event * data)
{
  if (me->p_flags & (PFTRACT | PFPRESS))
    {
      sendTractorReq(0, me->p_no);
      return;
    }
  target = gettarget(data->Window, data->x, data->y, TARG_PLAYER);
  me->p_tractor = target->o_num;
  if (key == 'T')
    {
      sendTractorReq(1, target->o_num);
    }
  else
    {
      sendRepressReq(1, target->o_num);
    }
}

Key85(void)
{
  if (W_IsMapped(rankw))
    {
      W_UnmapWindow(rankw);
    }
  else
    {
      W_MapWindow(rankw);
    }

}

/* I really should get paid for this... */
Key86(void)
{
  showlocal++;
  if (showlocal > 4)
    showlocal = 0;

}

Key87(W_Event * data)
{
  emptyKey();
}

Key88(void)
{
  macro_on();
}

Key89(W_Event * data)
{
  emptyKey();
}

Key90(W_Event * data)
{
  emptyKey();
}

Key91(void)
{
  shield_down();
}

Key92(void)
{
  if (netstat)
    {
      if (lMeter != NULL && W_IsMapped(lMeter))
	W_UnmapWindow(lMeter);
      else
	W_MapWindow(lMeter);
    }
  else
    {
      warning("Network stats are not being collected!");
    }

}

Key93(void)
{
  shield_up();
}

Key94(W_Event * data)
{
  if (me->p_flags & (PFTRACT | PFPRESS))
    sendRepressReq(0, me->p_no);
  target = gettarget(data->Window, data->x, data->y, TARG_PLAYER);
  me->p_tractor = target->o_num;
  sendRepressReq(1, target->o_num);
}

Key95(W_Event * data)
{
  if (me->p_flags & (PFTRACT | PFPRESS))
    sendTractorReq(0, me->p_no);
  target = gettarget(data->Window, data->x, data->y, TARG_PLAYER);
  me->p_tractor = target->o_num;
  sendTractorReq(1, target->o_num);
}

Key96(void)
{

#ifdef SHORT_PACKETS
  if (spWin != NULL && W_IsMapped(spWin))
    spdone();
  else
    spwindow();
#endif

}

Key97(W_Event * data)
{
  if (!W_IsMapped(scanwin))
    {
      scan(data->Window, data->x, data->y);
    }
  else
    {
      if (scanmapped)
	W_UnmapWindow(scanwin);
      scanmapped = 0;
    }
}

Key98(void)
{

#ifdef AUTOKEY
  if (autoKey && !(localflags & PFREFIT))
    autoKeyBombReqOn();
  else
    bomb_planet();
#else
  bomb_planet();
#endif /* AUTOKEY */

}

Key99(void)
{
  cloak();

}

Key100(void)
{
  static unsigned long lastdet = 0;
  unsigned long curtime;

#ifdef AUTOKEY
  if (autoKey)
    autoKeyDetReqOn();
  else
    sendDetonateReq();
#else
  /* want to limit these to one per update */
  curtime = mstime();
  if (curtime >= lastdet + 100)			 /* Allow one per 100 ms */
    {
      sendDetonateReq();
      lastdet = curtime;
    }
#endif /* AUTOKEY */
  detCircle = 1;

}

Key101(void)
{
  if (me->p_flags & PFDOCKOK)
    sendDockingReq(0);
  else
    sendDockingReq(1);
}

Key102(W_Event * data)
{
  unsigned char course;

  /* f = launch plasma torpedos */

#ifdef AUTOKEY
  if (autoKey)
    autoKeyPlasmaReqOn();
  else
    {
      course = getcourse(data->Window, data->x, data->y);
      sendPlasmaReq(course);
    }
#else
  course = getcourse(data->Window, data->x, data->y);
  sendPlasmaReq(course);
#endif /* AUTOKEY */

}

Key103(W_Event * data)
{
  emptyKey();
}

Key104(void)
{
  /* h = Map help window */
  if (W_IsMapped(helpWin))
    {
      W_UnmapWindow(helpWin);
    }
  else
    {
      fillhelp();
      W_MapWindow(helpWin);
    }
  if (optionWin)
    optionredrawtarget(helpWin);

}

Key105(W_Event * data)
{
  if (!infomapped)
    {
      inform(data->Window, data->x, data->y, key);
      opened_info = keepInfo;			 /* 5/31/93 LAB */
    }
  else
    {
      destroyInfo();
      opened_info = -2;
    }
}

Key106(W_Event * data)
{
  emptyKey();
}

Key107(W_Event * data)
{
  unsigned char course;

  course = getcourse(data->Window, data->x, data->y);
  set_course(course);
  me->p_flags &= ~(PFPLOCK | PFPLLOCK);
}

Key108(W_Event * data)
{
  target = gettarget(data->Window, data->x, data->y,
		     TARG_PLAYER | TARG_PLANET);
  if (target->o_type == PLAYERTYPE)
    {
      sendPlaylockReq(target->o_num);
      me->p_playerl = target->o_num;

    }
  else
    {						 /* It's a planet */
      sendPlanlockReq(target->o_num);
      me->p_planet = target->o_num;
    }
}

Key109(void)
{

#ifdef SOUND
#if defined(HAVE_SDL)
  Play_Sound(MESSAGE_WAV);
#else
  Play_Sound(MESSAGE_SOUND);
#endif
#endif

  message_on();
}

Key110(W_Event * data)
{
  emptyKey();
}

Key111(void)
{

#ifdef AUTOKEY
  if (autoKey)
    autoKeyOrbitReqOn();
  else
    sendOrbitReq(1);
#else
  sendOrbitReq(1);
#endif /* AUTOKEY */

}

Key112(W_Event * data)
{
  unsigned char course;

#ifdef AUTOKEY
  if (autoKey)
    autoKeyPhaserReqOn();
  else
    {
      course = getcourse(data->Window, data->x, data->y);
      sendPhaserReq(course);
    }
#else
  course = getcourse(data->Window, data->x, data->y);
  sendPhaserReq(course);
#endif /* AUTOKEY */

}

Key113(void)
{

#ifdef SOUND
#if defined(HAVE_SDL)
  Play_Sound(SELF_DESTRUCT_WAV);
#else
  Play_Sound(SELF_DESTRUCT_SOUND);
#endif
#endif

  fastQuit = 1;
  sendQuitReq();
}

/* r */
Key114(void)
{
  localflags |= PFREFIT;
  warning("s=scout, d=destroyer, c=cruiser, b=battleship, a=assault, g=galaxy, o=starbase/outpost");
}

Key115(void)
{
  shield_tog();
}

Key116(W_Event * data)
{
  unsigned char course;

#ifdef AUTOKEY
  if (autoKey)
    autoKeyTorpReqOn();
  else
    {
      course = getcourse(data->Window, data->x, data->y);
      sendTorpReq(course);
    }
#else
  course = getcourse(data->Window, data->x, data->y);
  sendTorpReq(course);
#endif /* AUTOKEY */
}

Key117(void)
{
  shield_tog();
}

Key118(W_Event * data)
{
  emptyKey();
}

Key119(void)
{
  /* w = map war stuff */
  if (W_IsMapped(war))
    W_UnmapWindow(war);
  else
    warwindow();

}

Key120(void)
{

#ifdef AUTOKEY
  if (autoKey)
    autoKeyBeamDownReqOn();
  else
#endif

    beam_down();

}

Key121(W_Event * data)
{
  if (me->p_flags & (PFTRACT | PFPRESS))
    {
      sendTractorReq(0, me->p_no);
      return;
    }
  target = gettarget(data->Window, data->x, data->y, TARG_PLAYER);
  me->p_tractor = target->o_num;
  if (key == 'T')
    {
      sendTractorReq(1, target->o_num);
    }
  else
    {
      sendRepressReq(1, target->o_num);
    }

}

Key122(void)
{

#ifdef AUTOKEY
  if (autoKey)
    autoKeyBeamUpReqOn();
  else
#endif

    beam_up();

}

Key123(void)
{
  cloak_on();
}

Key124(void)
{

#ifdef SHORT_PACKETS
  sendShortReq(SPK_ALL);
#endif

}

Key125(void)
{
  cloak_off();
}

/* ~ */
Key126(W_Event * data) {

#ifdef SOUND
  if ((soundWin != NULL) && W_IsMapped(soundWin))
    sounddone();
  else
    soundwindow();
#else
  emptyKey();
#endif
}

Key127(W_Event * data)
{
  emptyKey();
}

reread_defaults(void)
{
  char    mbuf[80];

  if (strlen(defaultsFile) > 0)
    {
      sprintf(mbuf, "Re-reading  %s", defaultsFile);

      warning(mbuf);
      initDefaults(defaultsFile);
      resetdefaults();
      initkeymap();
    }
  else
    {
      warning("no default file found");
    }
}

/* ^T */
Key180(W_Event * data)
{
  emergency(take, data);
}

/* ^t */
Key212(W_Event * data)
{
  emergency(take, data);
}

/* ^o */
Key207(W_Event * data)
{
  emergency(ogg, data);
}

/* ^b */
Key194(W_Event * data)
{
  emergency(bomb, data);
}

/* ^c */
Key195(W_Event * data)
{
  emergency(space_control, data);
}

/* ^1 */
Key145(W_Event * data)
{
  emergency(save_planet, data);
}

/* ^2 */
Key146(W_Event * data)
{
  emergency(base_ogg, data);
}

/* ^3 */
Key147(W_Event * data)
{
  emergency(help3, data);
}

/* ^4 */
Key148(W_Event * data)
{
  emergency(help4, data);
}

/* ^e */
Key197(W_Event * data)
{
  emergency(escorting, data);
}

/* ^p */
Key208(W_Event * data)
{
  emergency(ogging, data);
}

/* ^m */
Key205(W_Event * data)
{
  emergency(bombing, data);
}

/* ^l */
Key204(W_Event * data)
{
  emergency(controlling, data);
}


/* ^O */
Key175(W_Event * data)
{
  emergency(ogging, data);
}

/* ^B */
Key162(W_Event * data)
{
  emergency(bombing, data);
}

/* ^C */
Key163(W_Event * data)
{
  emergency(controlling, data);
}

/* ^5 */
Key149(W_Event * data)
{
  emergency(asw, data);
}

/* ^6 */
Key150(W_Event * data)
{
  emergency(asbomb, data);
}

/* ^7 */
Key151(W_Event * data)
{
  emergency(doing3, data);
}

/* ^8 */
Key152(W_Event * data)
{
  emergency(doing4, data);
}

/* ^f */
Key198(W_Event * data)
{
  emergency(free_beer, data);
}

/* ^n */
Key206(W_Event * data)
{
  emergency(no_gas, data);
}

/* ^h */
Key200(W_Event * data)
{
  emergency(crippled, data);
}

/* ^9 */
Key153(W_Event * data)
{
  emergency(pickup, data);
}

/* ^0 */
Key144(W_Event * data)
{
  emergency(pop, data);
}

/* ^@ */
Key160(W_Event * data)
{
  emergency(other1, data);
}

/* ^# */
Key131(W_Event * data)
{
  emergency(other2, data);
}
