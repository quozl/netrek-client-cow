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

#include "cowmain.h"
#include "defaults.h"
#include "defwin.h"
#include "docwin.h"
#include "helpwin.h"
#include "inform.h"
#include "interface.h"
#include "macrowin.h"
#include "map.h"
#include "netstatopt.h"
#include "option.h"
#include "pingstats.h"
#include "playback.h"
#include "playerlist.h"
#include "redraw.h"
#include "senddist.h"
#include "short.h"
#include "smessage.h"
#include "socket.h"
#include "spopt.h"
#include "tools.h"
#include "war.h"
#include "warning.h"
#include "udpopt.h"

#include "input.h"

static void detmine(void);
static void keyaction(W_Event * data);

int     detallow = 1;				 /* flag used to figure out * 

						  * 
						  * 
						  * * if we're allowing det */

#ifdef DOC_WIN
static int docline = 0;
static int xtrekrcline = 0;

#endif

int     opened_info = -2;	 /* counter for infowin popup, 6/1/93 LAB */

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
static void emptyKey(W_Event * data), Key32(W_Event * data), Key33(W_Event * data), Key34(W_Event * data),
        Key35(W_Event * data), Key36(W_Event * data), Key37(W_Event * data), Key38(W_Event * data), Key39(W_Event * data),
        Key40(W_Event * data), Key41(W_Event * data), Key42(W_Event * data), Key43(W_Event * data), Key44(W_Event * data),
        Key45(W_Event * data), Key46(W_Event * data), Key47(W_Event * data), Key48(W_Event * data), Key49(W_Event * data),
        Key50(W_Event * data), Key51(W_Event * data), Key52(W_Event * data), Key53(W_Event * data), Key54(W_Event * data),
        Key55(W_Event * data), Key56(W_Event * data), Key57(W_Event * data), Key58(W_Event * data), Key59(W_Event * data),
        Key60(W_Event * data), Key61(W_Event * data), Key62(W_Event * data), Key63(W_Event * data), Key64(W_Event * data),
        Key65(W_Event * data), Key66(W_Event * data), Key67(W_Event * data), Key68(W_Event * data), Key69(W_Event * data),
        Key70(W_Event * data), Key71(W_Event * data), Key72(W_Event * data),
        Key73(W_Event * data), Key74(W_Event * data), Key75(W_Event * data), Key76(W_Event * data),
        Key77(W_Event * data), Key78(W_Event * data), Key79(W_Event * data), Key80(W_Event * data), Key81(W_Event * data),
        Key82(W_Event * data), Key83(W_Event * data), Key84(W_Event * data), Key85(W_Event * data), Key86(W_Event * data),
        Key87(W_Event * data), Key88(W_Event * data), Key89(W_Event * data), Key90(W_Event * data),
        Key91(W_Event * data), Key92(W_Event * data), Key93(W_Event * data), Key94(W_Event * data), Key95(W_Event * data),
        Key96(W_Event * data), Key98(W_Event * data), Key99(W_Event * data), Key100(W_Event * data),
        Key101(W_Event * data), Key102(W_Event * data), Key103(W_Event * data), Key104(W_Event * data),
        Key105(W_Event * data), Key106(W_Event * data), Key107(W_Event * data),
        Key108(W_Event * data), Key110(W_Event * data), Key111(W_Event * data),
        Key112(W_Event * data), Key113(W_Event * data), Key114(W_Event * data), Key115(W_Event * data),
        Key116(W_Event * data), Key117(W_Event * data), Key118(W_Event * data), Key119(W_Event * data),
        Key120(W_Event * data), Key121(W_Event * data), Key122(W_Event * data), Key123(W_Event * data),
        Key124(W_Event * data), Key125(W_Event * data), Key126(W_Event * data), Key127(W_Event * data);
void Key109(W_Event * data);

/* control keys */
static void Key131(W_Event * data), Key144(W_Event * data), Key145(W_Event * data),
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
    void (*handler) (W_Event * data);
  }
key_handler_type;

key_handler_type key_handlers[MAXKEY] =
{
  { (void (*)(W_Event *))emptyKey },					 /* \0 */
  { (void (*)(W_Event *))emptyKey },					 /* 1 */
  { (void (*)(W_Event *))emptyKey },					 /* 2 */
  { (void (*)(W_Event *))emptyKey },					 /* 3 */
  { (void (*)(W_Event *))emptyKey },					 /* 4 */
  { (void (*)(W_Event *))emptyKey },					 /* 5 */
  { (void (*)(W_Event *))emptyKey },					 /* 6 */
  { (void (*)(W_Event *))emptyKey },					 /* 7 */
  { (void (*)(W_Event *))emptyKey },					 /* 8 */
  { (void (*)(W_Event *))emptyKey },					 /* 9 */
  { (void (*)(W_Event *))emptyKey },					 /* 10 */
  { (void (*)(W_Event *))emptyKey },					 /* 11 */
  { (void (*)(W_Event *))emptyKey },					 /* 12 */
  { (void (*)(W_Event *))emptyKey },					 /* 13 */
  { (void (*)(W_Event *))emptyKey },					 /* 14 */
  { (void (*)(W_Event *))emptyKey },					 /* 15 */
  { (void (*)(W_Event *))emptyKey },					 /* 16 */
  { (void (*)(W_Event *))emptyKey },					 /* 17 */
  { (void (*)(W_Event *))emptyKey },					 /* 18 */
  { (void (*)(W_Event *))emptyKey },					 /* 19 */
  { (void (*)(W_Event *))emptyKey },					 /* 20 */
  { (void (*)(W_Event *))emptyKey },					 /* 21 */
  { (void (*)(W_Event *))emptyKey },					 /* 22 */
  { (void (*)(W_Event *))emptyKey },					 /* 23 */
  { (void (*)(W_Event *))emptyKey },					 /* 24 */
  { (void (*)(W_Event *))emptyKey },					 /* 25 */
  { (void (*)(W_Event *))emptyKey },					 /* 26 */
  { (void (*)(W_Event *))emptyKey },					 /* 27 */
  { (void (*)(W_Event *))emptyKey },					 /* 28 */
  { (void (*)(W_Event *))emptyKey },					 /* 29 */
  { (void (*)(W_Event *))emptyKey },					 /* 30 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */

  { (void (*)(W_Event *))Key32 },					 /* space */
  { (void (*)(W_Event *))Key33 },					 /* ! */
  { (void (*)(W_Event *))Key34 },					 /* " */
  { (void (*)(W_Event *))Key35 },					 /* # */
  { (void (*)(W_Event *))Key36 },					 /* $ */
  { (void (*)(W_Event *))Key37 },					 /* % */
  { (void (*)(W_Event *))Key38 },					 /* & */
  { (void (*)(W_Event *))Key39 },					 /* ' */
  { (void (*)(W_Event *))Key40 },					 /* ( */
  { (void (*)(W_Event *))Key41 },					 /* ) */
  { (void (*)(W_Event *))Key42 },					 /* * */
  { (void (*)(W_Event *))Key43 },					 /* + */
  { (void (*)(W_Event *))Key44 },					 /* , */
  { (void (*)(W_Event *))Key45 },					 /* - */
  { (void (*)(W_Event *))Key46 },					 /* . */
  { (void (*)(W_Event *))Key47 },					 /* / */
  { (void (*)(W_Event *))Key48 },					 /* 0 */
  { (void (*)(W_Event *))Key49 },					 /* 1 */
  { (void (*)(W_Event *))Key50 },					 /* 2 */
  { (void (*)(W_Event *))Key51 },					 /* 3 */
  { (void (*)(W_Event *))Key52 },					 /* 4 */
  { (void (*)(W_Event *))Key53 },					 /* 5 */
  { (void (*)(W_Event *))Key54 },					 /* 6 */
  { (void (*)(W_Event *))Key55 },					 /* 7 */
  { (void (*)(W_Event *))Key56 },					 /* 8 */
  { (void (*)(W_Event *))Key57 },					 /* 9 */
  { (void (*)(W_Event *))Key58 },					 /* : */
  { (void (*)(W_Event *))Key59 },					 /* ; */
  { (void (*)(W_Event *))Key60 },					 /* < */
  { (void (*)(W_Event *))Key61 },					 /* = */
  { (void (*)(W_Event *))Key62 },					 /* > */
  { (void (*)(W_Event *))Key63 },					 /* ?  -  you know this is *
						  * * boring as hell... */
  { (void (*)(W_Event *))Key64 },					 /* @ */
  { (void (*)(W_Event *))Key65 },					 /* A */
  { (void (*)(W_Event *))Key66 },					 /* B */
  { (void (*)(W_Event *))Key67 },					 /* C */
  { (void (*)(W_Event *))Key68 },					 /* D */
  { (void (*)(W_Event *))Key69 },					 /* E */
  { (void (*)(W_Event *))Key70 },					 /* F */
  { (void (*)(W_Event *))Key71 },					 /* G */
  { (void (*)(W_Event *))Key72 },					 /* H */
  { (void (*)(W_Event *))Key73 },					 /* I */
  { (void (*)(W_Event *))Key74 },					 /* J */
  { (void (*)(W_Event *))Key75 },					 /* K */
  { (void (*)(W_Event *))Key76 },					 /* L */
  { (void (*)(W_Event *))Key77 },					 /* M */
  { (void (*)(W_Event *))Key78 },					 /* N */
  { (void (*)(W_Event *))Key79 },					 /* O */
  { (void (*)(W_Event *))Key80 },					 /* P */
  { (void (*)(W_Event *))Key81 },					 /* Q */
  { (void (*)(W_Event *))Key82 },					 /* R */
  { (void (*)(W_Event *))Key83 },					 /* S */
  { (void (*)(W_Event *))Key84 },					 /* T */
  { (void (*)(W_Event *))Key85 },					 /* U */
  { (void (*)(W_Event *))Key86 },					 /* V */
  { (void (*)(W_Event *))Key87 },					 /* W */
  { (void (*)(W_Event *))Key88 },					 /* X */
  { (void (*)(W_Event *))Key89 },					 /* Y */
  { (void (*)(W_Event *))Key90 },					 /* Z */
  { (void (*)(W_Event *))Key91 },					 /* [ */
  { (void (*)(W_Event *))Key92 },					 /* \ */
  { (void (*)(W_Event *))Key93 },					 /* ]  -  ascii is fucked... */
  { (void (*)(W_Event *))Key94 },					 /* ^ */
  { (void (*)(W_Event *))Key95 },					 /* _ */
  { (void (*)(W_Event *))Key96 },					 /* ` */
  { (void (*)(W_Event *))emptyKey },					 /* a */
  { (void (*)(W_Event *))Key98 },					 /* b */
  { (void (*)(W_Event *))Key99 },					 /* c */
  { (void (*)(W_Event *))Key100 },					 /* d */
  { (void (*)(W_Event *))Key101 },					 /* e */
  { (void (*)(W_Event *))Key102 },					 /* f */
  { (void (*)(W_Event *))Key103 },					 /* g */
  { (void (*)(W_Event *))Key104 },					 /* h */
  { (void (*)(W_Event *))Key105 },					 /* i */
  { (void (*)(W_Event *))Key106 },					 /* j */
  { (void (*)(W_Event *))Key107 },					 /* k */
  { (void (*)(W_Event *))Key108 },					 /* l */
  { (void (*)(W_Event *))Key109 },					 /* m */
  { (void (*)(W_Event *))Key110 },					 /* n */
  { (void (*)(W_Event *))Key111 },					 /* o */
  { (void (*)(W_Event *))Key112 },					 /* p */
  { (void (*)(W_Event *))Key113 },					 /* q */
  { (void (*)(W_Event *))Key114 },					 /* r */
  { (void (*)(W_Event *))Key115 },					 /* s */
  { (void (*)(W_Event *))Key116 },					 /* t */
  { (void (*)(W_Event *))Key117 },					 /* u */
  { (void (*)(W_Event *))Key118 },					 /* v */
  { (void (*)(W_Event *))Key119 },					 /* w */
  { (void (*)(W_Event *))Key120 },					 /* x */
  { (void (*)(W_Event *))Key121 },					 /* y */
  { (void (*)(W_Event *))Key122 },					 /* z */
  { (void (*)(W_Event *))Key123 },					 /* { */
  { (void (*)(W_Event *))Key124 },					 /* | */
  { (void (*)(W_Event *))Key125 },					 /* } - my wife was once * *
						  * bitten by a lhama */
  { (void (*)(W_Event *))Key126 },					 /* ~ */
  { (void (*)(W_Event *))Key127 },					 /* delete */
  { (void (*)(W_Event *))emptyKey },					 /* 128 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))Key131 },					 /* ^# */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))Key144 },					 /* ^0 */
  { (void (*)(W_Event *))Key145 },					 /* ^1 */
  { (void (*)(W_Event *))Key146 },					 /* ^2 */
  { (void (*)(W_Event *))Key147 },					 /* ^3 */
  { (void (*)(W_Event *))Key148 },					 /* ^4 */
  { (void (*)(W_Event *))Key149 },					 /* ^5 */
  { (void (*)(W_Event *))Key150 },					 /* ^6 */
  { (void (*)(W_Event *))Key151 },					 /* ^7 */
  { (void (*)(W_Event *))Key152 },					 /* ^8 */
  { (void (*)(W_Event *))Key153 },					 /* ^9 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))Key160 },					 /* ^@ */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))Key162 },					 /* ^B */
  { (void (*)(W_Event *))Key163 },					 /* ^C */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))Key175 },					 /* ^O */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))Key180 },					 /* ^T */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))Key194 },					 /* ^b */
  { (void (*)(W_Event *))Key195 },					 /* ^c */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))Key197 },					 /* ^e */
  { (void (*)(W_Event *))Key198 },					 /* ^f */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))Key200 },					 /* ^h */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))Key204 },					 /* ^l */
  { (void (*)(W_Event *))Key205 },					 /* ^m */
  { (void (*)(W_Event *))Key206 },					 /* ^n */
  { (void (*)(W_Event *))Key207 },					 /* ^o */
  { (void (*)(W_Event *))Key208 },					 /* ^p */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))Key212 },					 /* ^t */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 31 */
  { (void (*)(W_Event *))emptyKey },					 /* 223 */
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

void initkeymap(void)
{
  unsigned char *str;

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
      else if (!strcmpi((char *) str, "TAB"))
	{
	  p = (unsigned char *) "^i";
	  mystats->st_keymap[getctrlkey(&p) - 32] = 'X';
	}
      else if (!strcmpi((char *) str, "ESC"))
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

void
detsetallow(int _dummy)
{
  detallow = 1;
}

#ifdef THREADED
void input()
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

void input2()
#else
void input()
#endif
{
  fd_set  readfds;
#ifndef HAVE_WIN32
  int     xsock = W_Socket();
#endif
  struct timeval timeout;
  int     retval;
  int     flush = 0;
  int     stall = 100;

  while (1) {
    FD_ZERO(&readfds);
#ifndef HAVE_WIN32
    FD_SET(xsock, &readfds);
    if (xsock > max_fd) fprintf(stderr, "input: xsock > max_fd\n");
#endif
    if (!serverDead) {
      FD_SET(sock, &readfds);
      if (sock > max_fd) fprintf(stderr, "input: sock > max_fd\n");
      if (udpSock >= 0) {
        FD_SET(udpSock, &readfds);
        if (sock > max_fd)
          fprintf(stderr, "input: udpSock > max_fd\n");
      }
    } else {
      warning("Lost connection to server, press q to quit.");
      redrawall = 1;
      redraw();
      flush++;
    }

    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
    retval = SELECT(max_fd, &readfds, 0, 0, &timeout);
    if (retval == 0) {
#ifndef HAVE_WIN32
      /* With Xlib/XCB, X events might not cause the X socket to be readable */
      if (W_EventsQueuedCk()) {
        while (W_EventsQueuedCk())
	  process_event();
        flush++;
      }
#endif
      stall--;
      if (stall < 0) {
	warning("Stall in data stream from server!");
	redrawall = 1;
	redraw();
	flush++;
	stall = 100;
      }
    } else if (retval > 0) {
#ifndef THREADED
#ifndef HAVE_WIN32
      /* X events may cause the X socket to be readable */
      if (FD_ISSET(xsock, &readfds)) {
	while (W_EventsQueuedCk())
	  process_event();
	flush++;
      }
#else
      if (W_EventsPending()) {
	process_event();
	flush++;
      }
#endif /* !HAVE_WIN32 */
#endif /* !THREADED */
      /* read from server */
      if (FD_ISSET(sock, &readfds) ||
	  (udpSock >= 0 && FD_ISSET(udpSock, &readfds))) {
	stall = 100;
	intrupt(&readfds);
	flush++;
	if (serverDead) {
	  warning("Lost connection to server!");
	}

        /* manage expiry of info window every server update */
        if (keepInfo > 0 && opened_info != -2) { /* 6/1/93 LAB */
          opened_info--;
          if (opened_info < 0 && infomapped) destroyInfo();
        }

      }
    }

    if (flush) {
      W_Flush();
      flush = 0;
    }
  }
}

int process_event(void)
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

	  if (data.Window == messagew)
	    break;
	  if (serverDead) {
	    if (data.key == 'q') terminate(EXIT_DISCONNECTED);
	  }
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
	  else if (data.Window == messagew) {
	    DisplayMessage();
	  }

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

	case W_EV_CLOSED:
	  if (data.Window == baseWin) {
	    fprintf(stderr, "you quit, by closing the play window in play\n");
	    fastQuit = 1;
	    sendQuitReq();
	  }
	  break;

	default:
	  break;
	}
    }
  while (W_EventsQueued());
  return loop;
}

static void keyaction(W_Event * data)
{
  fastQuit = 0;					 /* any event, cancel * *
						  * fastquit! */

  /* remap events in other windows to local window */
  if (data->Window != mapw && data->Window != w && data->Window != infow
      && data->Window != scanw)
    {
      data->Window = w;
      data->x = data->y = TWINSIDE / 2;
    }

  key = data->key;

  /* remap events in information window to the surrounding game window */
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
      fprintf(stderr, "input.c: keyaction() key %d outside range\n", key);
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
void mkeyaction(W_Event * data)
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

void buttonaction(W_Event * data)
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
      && data->Window != infow)
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
	  opened_info = keepInfo * server_ups / 5;
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

int getcourse(W_Window ww, int x, int y)
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

static void detmine(void)
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

void lockPlanetOrBase(W_Window ww, int x, int y)
/* special version of gettarget, 6/1/93 LAB */
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


static void emptyKey(W_Event * data)
{
  fprintf(stderr, "input.c: emptyKey %d\n", key);
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
  int targettype;
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
				       targettype | TARG_CLOAK);
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

static void Key32(W_Event * data)
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
  if (udpWin)
    udpdone();
}

static void Key33(W_Event * data)
{
  set_speed(11);
}

static void Key34(W_Event * data)
{
  int ok = W_FullScreenToggle(baseWin);
  switch (ok) {
  case -1:
    warning("Full screen mode was not built in this program.");
    break;
  case 0:
    warning("Full screen mode toggled.");
    break;
  case 1:
    warning("Full screen mode not available here.");
    break;
  }
}

static void Key35(W_Event * data)
{
  set_speed(me->p_ship.s_maxspeed / 2);
}

static void Key36(W_Event * data)
{
  sendTractorReq(0, me->p_no);
}

static void Key37(W_Event * data)
{
  set_speed(99);				 /* Max speed... */
}

static void Key38(W_Event * data)
{
  char    mbuf[80];

  if (strlen(defaultsFile) > 0)
    {
      strcpy(mbuf, "Re-reading ");
      strncat(mbuf, defaultsFile, sizeof(mbuf) - strlen(mbuf) - 1);

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

static void Key39(W_Event * data)
{
  sendUdpReq(COMM_UPDATE);
}

static void Key40(W_Event * data)
{
  set_speed(10);
}

static void Key41(W_Event * data)
{
  set_speed(10);
}

static void Key42(W_Event * data)
{
  sendPractrReq();
}

static void Key43(W_Event * data)
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

static void Key44(W_Event * data)
{
  if (W_IsMapped(pStats))
    {
      W_UnmapWindow(pStats);
    }
  else
    {
      W_MapWindow(pStats);
      redrawPStats(NULL);
    }
}

static void Key45(W_Event * data)
{
#ifdef SHORT_PACKETS
  sendShortReq(SPK_SALL);
#endif
}

static void Key46(W_Event * data)
{
  if (netstatWin != NULL && W_IsMapped(netstatWin))
    nsdone();
  else
    nswindow();
}

static void Key47(W_Event * data)
{
  sortPlayers = !sortPlayers;
  RedrawPlayerList(NULL);
}

static void Key48(W_Event * data)
{
  set_speed(0);
}

static void Key49(W_Event * data)
{
  set_speed(1);
}

static void Key50(W_Event * data)
{
  set_speed(2);
}

static void Key51(W_Event * data)
{
  set_speed(3);
}

static void Key52(W_Event * data)
{
  set_speed(4);
}

static void Key53(W_Event * data)
{
  set_speed(5);
}

static void Key54(W_Event * data)
{
  set_speed(6);
}

static void Key55(W_Event * data)
{
  set_speed(7);
}

static void Key56(W_Event * data)
{
  set_speed(8);
}

static void Key57(W_Event * data)
{
  set_speed(9);
}

static void Key58(W_Event * data)
{
  logmess = !logmess;
  if (logmess)
    warning("Message logging is ON");
  else
    warning("Message logging is OFF");
}

static void Key59(W_Event * data)
{
  lockPlanetOrBase(data->Window, data->x, data->y);
}

/* < */
static void Key60(W_Event * data)
{
  set_speed(me->p_speed - 1);
}

static void Key61(W_Event * data)
{
  /* UDP: request for full update */
  sendUdpReq(COMM_UPDATE);
}

/* > */
static void Key62(W_Event * data)
{
  set_speed(me->p_speed + 1);
}

static void Key63(W_Event * data)
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

static void Key64(W_Event * data)
{
  set_speed(12);
}

static void Key65(W_Event * data)
{
  /* W_ShowBitmaps(); */
  emptyKey(data);
}

static void Key66(W_Event * data)
{
  showgalactic++;
  if (showgalactic > 4)
    showgalactic = 0;

  redrawall = 2;
}

static void Key67(W_Event * data)
{
  sendCoupReq();
  W_CameraSnap(w);
}

static void Key68(W_Event * data)
{
  detmine();

#ifdef AUTOKEY
  if (autoKey)
    autoKeyAllOff();				 /* xx */
#endif
}

/* E */
static void Key69(W_Event * data)
{
  emergency(generic, data);
}

/* F */
static void Key70(W_Event * data)
{
  emergency(carrying, data);
}

static void Key71(W_Event * data)
{
  emptyKey(data);
}

static void Key72(W_Event * data)
{
  emptyKey(data);
}

static void Key73(W_Event * data)
{
  /* I = get extended information */
  if (!infomapped)
    {
      inform(data->Window, data->x, data->y, key);
      opened_info = keepInfo * server_ups / 5;
    }
  else
    {
      destroyInfo();
      opened_info = -2;
    }
}

static void Key74(W_Event * data)
{
  emptyKey(data);
}

static void Key75(W_Event * data)
{
  emptyKey(data);
}

static void Key76(W_Event * data)
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

static void Key77(W_Event * data)
{
#ifdef TOOLS
  showToolsWin();
#else
  emptyKey(data);
#endif
}

static void Key78(W_Event * data)
{
  /* N = Toggle Name mode */
  namemode = !namemode;
  if (optionWin)
    optionredrawoption(&namemode);
}

static void Key79(W_Event * data)
{
  if (optionWin != NULL && W_IsMapped(optionWin))
    optiondone();
  else
    optionwindow();
}

static void Key80(W_Event * data)
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

static void Key81(W_Event * data)
{
#ifdef SOUND
  Play_Sound(SELF_DESTRUCT_SOUND);
#endif
  sendQuitReq();
}

static void Key82(W_Event * data)
{
  sendRepairReq(1);
}

static void Key83(W_Event * data)
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

static void Key84(W_Event * data)
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

static void Key85(W_Event * data)
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
static void Key86(W_Event * data)
{
  showlocal++;
  if (showlocal > 4)
    showlocal = 0;
}

static void Key87(W_Event * data)
{
  emptyKey(data);
}

static void Key88(W_Event * data)
{
  macro_on();
}

static void Key89(W_Event * data)
{
  emptyKey(data);
}

static void Key90(W_Event * data)
{
  emptyKey(data);
}

static void Key91(W_Event * data)
{
  shield_down();
}

static void Key92(W_Event * data)
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

static void Key93(W_Event * data)
{
  shield_up();
}

static void Key94(W_Event * data)
{
  if (me->p_flags & (PFTRACT | PFPRESS))
    sendRepressReq(0, me->p_no);
  target = gettarget(data->Window, data->x, data->y, TARG_PLAYER);
  me->p_tractor = target->o_num;
  sendRepressReq(1, target->o_num);
}

static void Key95(W_Event * data)
{
  if (me->p_flags & (PFTRACT | PFPRESS))
    sendTractorReq(0, me->p_no);
  target = gettarget(data->Window, data->x, data->y, TARG_PLAYER);
  me->p_tractor = target->o_num;
  sendTractorReq(1, target->o_num);
}

static void Key96(W_Event * data)
{
#ifdef SHORT_PACKETS
  if (spWin != NULL && W_IsMapped(spWin))
    spdone();
  else
    spwindow();
#endif
}

static void Key98(W_Event * data)
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

static void Key99(W_Event * data)
{
  cloak();
}

static void Key100(W_Event * data)
{
  static unsigned long lastdet = 0;
  unsigned long curtime;

#ifdef AUTOKEY
  if (autoKey)
    autoKeyDetReqOn();
  else
    sendDetonateReq();
#else
  /* want to limit these to one per server frame */
  curtime = ustime();
  if (curtime >= lastdet + (1000000 / server_fps))
    {
      sendDetonateReq();
      lastdet = curtime;
    }
#endif /* AUTOKEY */
  detCircle = 1;
}

static void Key101(W_Event * data)
{
  if (me->p_flags & PFDOCKOK)
    sendDockingReq(0);
  else
    sendDockingReq(1);
}

static void Key102(W_Event * data)
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

static void Key103(W_Event * data)
{
  emptyKey(data);
}

static void Key104(W_Event * data)
{
  /* h = Map help window */
  if (W_IsMapped(helpWin))
    {
      W_UnmapWindow(helpWin);
    }
  else
    {
      fillhelp(NULL);
      W_MapWindow(helpWin);
    }
  if (optionWin)
    optionredrawtarget(helpWin);
}

static void Key105(W_Event * data)
{
  if (!infomapped)
    {
      inform(data->Window, data->x, data->y, key);
      opened_info = keepInfo * server_ups / 5;
    }
  else
    {
      destroyInfo();
      opened_info = -2;
    }
}

static void Key106(W_Event * data)
{
  emptyKey(data);
}

static void Key107(W_Event * data)
{
  unsigned char course;

  /* Observers can't move.  Also incorrectly removes the lock flag even though
     you are still locked */
  if (me->p_flags & PFOBSERV) {
    warning("Course change ignored while observing!");
    return;
  }

  course = getcourse(data->Window, data->x, data->y);
  set_course(course);
  me->p_flags &= ~(PFPLOCK | PFPLLOCK);
}

static void Key108(W_Event * data)
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

void Key109(W_Event * data)
{
#ifdef SOUND
  Play_Sound(MESSAGE_SOUND);
#endif
  message_on();
}

static void Key110(W_Event * data)
{
  emptyKey(data);
}

static void Key111(W_Event * data)
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

static void Key112(W_Event * data)
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

static void Key113(W_Event * data)
{
#ifdef SOUND
  Play_Sound(SELF_DESTRUCT_SOUND);
#endif

  fastQuit = 1;
  sendQuitReq();
}

/* r */
static void Key114(W_Event * data)
{
  localflags |= PFREFIT;
  warning("s=scout, d=destroyer, c=cruiser, b=battleship, a=assault, g=galaxy, o=starbase/outpost");
}

static void Key115(W_Event * data)
{
  shield_tog();
}

static void Key116(W_Event * data)
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

static void Key117(W_Event * data)
{
  shield_tog();
}

static void Key118(W_Event * data)
{
  emptyKey(data);
}

static void Key119(W_Event * data)
{
  /* w = map war stuff */
  if (W_IsMapped(war))
    W_UnmapWindow(war);
  else
    warwindow();
}

static void Key120(W_Event * data)
{
#ifdef AUTOKEY
  if (autoKey)
    autoKeyBeamDownReqOn();
  else
#endif
    beam_down();
}

static void Key121(W_Event * data)
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

static void Key122(W_Event * data)
{
#ifdef AUTOKEY
  if (autoKey)
    autoKeyBeamUpReqOn();
  else
#endif
    beam_up();
}

static void Key123(W_Event * data)
{
  cloak_on();
}

static void Key124(W_Event * data)
{
#ifdef SHORT_PACKETS
  sendShortReq(SPK_ALL);
#endif
}

static void Key125(W_Event * data)
{
  cloak_off();
}

/* ~ */
static void Key126(W_Event * data) {

#ifdef SOUND
  if ((soundWin != NULL) && W_IsMapped(soundWin))
    sounddone();
  else
    soundwindow();
#else
  emptyKey(data);
#endif
}

static void Key127(W_Event * data)
{
  emptyKey(data);
}

/* ^T */
static void Key180(W_Event * data)
{
  emergency(take, data);
}

/* ^t */
static void Key212(W_Event * data)
{
  emergency(take, data);
}

/* ^o */
static void Key207(W_Event * data)
{
  emergency(ogg, data);
}

/* ^b */
static void Key194(W_Event * data)
{
  emergency(bomb, data);
}

/* ^c */
static void Key195(W_Event * data)
{
  emergency(space_control, data);
}

/* ^1 */
static void Key145(W_Event * data)
{
  emergency(save_planet, data);
}

/* ^2 */
static void Key146(W_Event * data)
{
  emergency(base_ogg, data);
}

/* ^3 */
static void Key147(W_Event * data)
{
  emergency(help1, data);
}

/* ^4 */
static void Key148(W_Event * data)
{
  emergency(help2, data);
}

/* ^e */
static void Key197(W_Event * data)
{
  emergency(escorting, data);
}

/* ^p */
static void Key208(W_Event * data)
{
  emergency(ogging, data);
}

/* ^m */
static void Key205(W_Event * data)
{
  emergency(bombing, data);
}

/* ^l */
static void Key204(W_Event * data)
{
  emergency(controlling, data);
}


/* ^O */
static void Key175(W_Event * data)
{
  emergency(ogging, data);
}

/* ^B */
static void Key162(W_Event * data)
{
  emergency(bombing, data);
}

/* ^C */
static void Key163(W_Event * data)
{
  emergency(controlling, data);
}

/* ^5 */
static void Key149(W_Event * data)
{
  emergency(asw, data);
}

/* ^6 */
static void Key150(W_Event * data)
{
  emergency(asbomb, data);
}

/* ^7 */
static void Key151(W_Event * data)
{
  emergency(doing1, data);
}

/* ^8 */
static void Key152(W_Event * data)
{
  emergency(doing2, data);
}

/* ^f */
static void Key198(W_Event * data)
{
  emergency(free_beer, data);
}

/* ^n */
static void Key206(W_Event * data)
{
  emergency(no_gas, data);
}

/* ^h */
static void Key200(W_Event * data)
{
  emergency(crippled, data);
}

/* ^9 */
static void Key153(W_Event * data)
{
  emergency(pickup, data);
}

/* ^0 */
static void Key144(W_Event * data)
{
  emergency(pop, data);
}

/* ^@ */
static void Key160(W_Event * data)
{
  emergency(other1, data);
}

/* ^# */
static void Key131(W_Event * data)
{
  emergency(other2, data);
}
