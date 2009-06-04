/* x11window.c
 *
 * Kevin P. Smith  6/11/89 Much modified by Jerry Frain and Joe Young
 *
 */

#define DEBUG 0

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include INC_SYS_SELECT
#include INC_STRINGS
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include INC_SYS_TIMEB
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "teams.bitmap"
#include "mapcursor.bitmap"
#include "localcursor.bitmap"
#include "smessage.h"
#include "defaults.h"
#include "x11window.h"
#include "x11sprite.h"
#include "camera.h"

extern void terminate(int error);

/* XFIX speedup */
#define MAXCACHE        128
#define MAX_TEXT_WIDTH	100

/* changes too good to risk leaving out, by Richard Caley (rjc@cstr.ed.ac.uk) */
/* Was #ifdef RJC, but now they're just part of the code                    */

#ifdef SMALL_SCREEN
#define NORMAL_FONT     "5x7"
#define BOLD_FONT       "5x7"
#define ITALIC_FONT     "5x7"
#define IND_FONT        "5x7"
#else
#define NORMAL_FONT     "6x10"
#define BOLD_FONT       "-*-clean-bold-r-normal--10-100-75-75-c-60-*"
#define ITALIC_FONT     "-*-clean-bold-r-normal--10-100-75-75-c-60-*"
#define IND_FONT        "-*-clean-bold-r-normal--10-100-75-75-c-60-*"
#endif

#define BIG_FONT        "-*-lucidatypewriter-*-*-*-*-40-*-*-*-*-*-*-*"
int     forceMono = 0;


#ifdef BEEPLITE
#define TTS_FONT        "10x20"
extern void init_tts(void);

#endif

#define G_SET_WIDTH     0x1
#define G_SET_HEIGHT    0x2
#define G_SET_X         0x4
#define G_SET_Y         0x8


static char *_nfonts[] =
{
  NORMAL_FONT,
  "-*-clean-medium-r-normal--10-100-75-75-c-60-*",
  "fixed",
  NULL,
};
static char *_bfonts[] =
{
  BOLD_FONT,
  "-*-clean-bold-r-normal--10-100-75-75-c-60-*",
  "fixed",
  NULL,
};
static char *_ifonts[] =
{
  ITALIC_FONT,
  "-*-clean-bold-r-normal--10-100-75-75-c-60-*",
  "fixed",
  NULL,
};
static char *_bgfonts[] =
{
  BIG_FONT,
  "fixed",
  "fixed",
  NULL,
};
XFontStruct *find_font(char *oldf, char **fonts);

#define FONTS 4
#define BITGC 4

#define WHITE   0
#define BLACK   1
#define RED     2
#define GREEN   3
#define YELLOW  4
#define CYAN    5
#define GREY    6

#ifdef RACE_COLORS
#define C_ROM   7
#define C_KLI   8
#define C_FED   9
#define C_ORI   10
#define C_IND   11
#endif

#ifdef RACE_COLORS
#define COLORS  16
#define PLANES  4
#else
#define COLORS  8
#define PLANES  3
#endif

#define RaceDefaultOffset (C_ROM - RED)

extern int takeNearest;

static int zero = 0;
static int one = 1;
static int two = 2;
static int three = 3;


int     W_FastClear = 0;
Window  W_Root;
Colormap W_Colormap;
int     W_Screen;

Visual *W_Visual;

W_Font  W_BigFont = (W_Font) & zero, W_RegularFont = (W_Font) & one;
W_Font  W_HighlightFont = (W_Font) & two, W_UnderlineFont = (W_Font) & three;
Display *W_Display;
W_Color W_White = WHITE, W_Black = BLACK, W_Red = RED, W_Green = GREEN;
W_Color W_Yellow = YELLOW, W_Cyan = CYAN, W_Grey = GREY;

#ifdef RACE_COLORS
W_Color W_Ind = C_IND, W_Fed = C_FED, W_Rom = C_ROM, W_Kli = C_KLI, W_Ori = C_ORI;

#endif
int     W_BigTextwidth, W_BigTextheight, W_Textwidth, W_Textheight;
char   *getdefault(char *str);

int     W_in_message = 0;			 /* jfy -- for Jerry's warp * 

						  * 
						  * 
						  * * message hack */

/* TTS: moved this out so we can use the 8th color */

static unsigned long planes[PLANES];

/* Scrollable message windows */
#define SCROLL_THUMB_WIDTH	5
static int scrollbar = 1;
static int scroll_thumb_width = SCROLL_THUMB_WIDTH;
static GC scroll_thumb_gc;
static Pixmap scroll_thumb_pixmap;
static int scroll_lines = 100;			 /* save 100 lines */

Atom wm_protocols, wm_delete_window;

extern W_Window baseWin;
static XClassHint class_hint =
{
  "netrek", "Netrek",
};

static XWMHints wm_hint =
{
  InputHint | StateHint,
  True,
  NormalState,
  None,
  None,
  0, 0,
  None,
  None,
};

#ifdef WINDOWMAKER
  char **wm_argv;
  int wm_argc;
#endif

static W_Event W_myevent;
static int W_isEvent = 0; /* an event is being held here for the caller */

struct fontInfo
  {
    int     baseline;
  };

struct colors
  {
    char   *name;
    GC      contexts[FONTS + 1];
    GC      insens_contexts[FONTS + 1];
    Pixmap  pixmap;
    int     pixelValue;
  };

Pixmap  insens_tile;

struct icon
  {
    Window  window;
    Pixmap  bitmap;
    int     width, height;
    Pixmap  pixmap;
  };

#define WIN_GRAPH       1
#define WIN_TEXT        2
#define WIN_MENU        3
#define WIN_SCROLL      4

static void changeMenuItem(struct window *win, int col, int n, char *str, W_Color color);
static void scrollUp(struct window *win, int y);
static void scrollDown(struct window *win, int y);
static void scrollPosition(struct window *win, int y);
static void scrollTo(struct window *win, struct scrollingWindow *sw, int topline);
static void scrollScrolling(W_Event * wevent);
static void configureScrolling(struct window *win, int x, int y, int width, int height);
static void AddToScrolling(struct window *win, W_Color color, W_Font font, char *str, int len);
static void drawThumb(struct window *win, struct scrollingWindow *sw);
static void redrawScrolling(struct window *win);
static int checkGeometry(char *name, int *x, int *y, int *width, int *height);

struct stringList
  {
    char    string[MAX_TEXT_WIDTH];
    W_Color color;
    W_Font  font;
    struct stringList *next, *prev;
  };

struct menuItem
  {
    int     column;
    char   *string;
    W_Color color;
  };

struct colors colortable[] =
{
  {"white"},
  {"black"},
  {"red"},
  {"green"},
  {"yellow"},
  {"cyan"},
  {"dark grey"}

#ifdef RACE_COLORS
  ,
  {"Rom"},
  {"Kli"},
  {"Fed"},
  {"Ori"},
  {"Ind"}
#endif

};

struct windowlist
  {
    struct window *window;
    struct windowlist *next;
  };

#define HASHSIZE 101
#define hash(x) (((int) (x)) % HASHSIZE)

struct windowlist *hashtable[HASHSIZE];
struct fontInfo fonts[FONTS];

struct window *newWindow(Window window, int type);
struct window *findWindow(Window window);

/* char *malloc (size_t); */
short  *x11tox10bits();

struct window myroot;

#define NCOLORS (sizeof(colortable)/sizeof(colortable[0]))
#define W_Void2Window(win) ((win) ? ((struct window *) (win)) : (&myroot))
#define W_Window2Void(window) ((W_Window) (window))
#define W_Void2Icon(bit) ((struct icon *) (bit))
#define W_Icon2Void(bit) ((W_Icon) (bit))
#define fontNum(font) (*((int *) font))
#define TILESIDE 16

#define WIN_EDGE 5				 /* border on l/r edges of *
						  * * text windows */
#define MENU_PAD 6				 /* border on t/b edges of *
						  * * text windows */
#define MENU_BAR 1				 /* width of menu bar */

static char gray[] =
{
  0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,
  0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,
  0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,
  0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55
};

static char striped[] =
{
  0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
  0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f,
  0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
  0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0
};

static char solid[] =
{
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

int full_screen_default, full_screen_enabled;

#ifdef FULLSCREEN
static void video_mode_off(void);
static int video_mode_initialise(void);
static void video_mode_on(void);
static void view_port_warp(W_Window window);
static void pointer_grab_on(W_Window window);
static void pointer_grab_off(W_Window window);
static void kde_fullscreen_on(W_Window window);
static void kde_fullscreen_off(W_Window window);
#endif

/* X debugging */
int
        _myerror(Display * d, XErrorEvent * e)
{
  fprintf(stderr, "netrek: x11window: _myerror\n");
  abort();
}

void pastebuffer(void)
{
  int     nbytes, x;
  char   *buff, c;

  buff = XFetchBuffer(W_Display, &nbytes, 0);
  for (x = 0; x < nbytes; x++)
    {
      c = buff[x];
      smessage(c);
    }
}

static long
        WMXYHintMode_default(void)
{
  static int fetched = 0;
  static long WMXYHM_default;
  char   *hm_default_string;

  if (!fetched)
    {
      hm_default_string = getdefault("WMXYHintMode");
      if (!hm_default_string || strcmp(hm_default_string, "USPosition") == 0)
	WMXYHM_default = USPosition;
      else
	WMXYHM_default = PPosition;
      fetched = 1;
    }
  return WMXYHM_default;
}


void
        W_Initialize(char *str)
{
  int     i;

#if DEBUG > 0
  printf("Initializing...\n");
#endif

  for (i = 0; i < HASHSIZE; i++)
    {
      hashtable[i] = NULL;
    }
  if ((W_Display = XOpenDisplay(str)) == NULL)
    {
      fprintf(stderr, "I can't open your display, twink!\n");
      terminate(1);
    }

  /* prevent X socket from being copied to forked exec'd process */
  if (fcntl(ConnectionNumber(W_Display), F_SETFD, FD_CLOEXEC) < 0)
    {
      fprintf(stderr, "failed to set the X socket to close on exec(),\n"
              "fcntl F_SETFD FD_CLOEXEC failure,\n'%s'",
              strerror(errno));
    }

  // uncomment this to synchronise display for testing
  // XSynchronize(W_Display, True);

  // uncomment this to enable a fatal error handler
  // XSetErrorHandler(_myerror);

  wm_protocols = XInternAtom(W_Display, "WM_PROTOCOLS", False);
  wm_delete_window = XInternAtom(W_Display, "WM_DELETE_WINDOW", False);

  W_Root = DefaultRootWindow(W_Display);
  W_Visual = DefaultVisual(W_Display, DefaultScreen(W_Display));
  W_Screen = DefaultScreen(W_Display);
  W_Colormap = DefaultColormap(W_Display, W_Screen);
  myroot.window = W_Root;
  myroot.type = WIN_GRAPH;
  GetFonts();
  GetColors();

  /* display scroll thumb */
  scrollbar = booleanDefault("ScrollBar", scrollbar);
  scroll_lines = intDefault("ScrollSaveLines", scroll_lines);
  scroll_thumb_width = intDefault("ScrollBarWidth", scroll_thumb_width);

#ifdef BEEPLITE
  init_tts();
#endif
}

void W_GetPixmaps(W_Window t, W_Window g)
{
  GetPixmaps(W_Display, &myroot, t, g);
}

/* Make sure the font will work, ie: that it fits in the 6x10 character cell
 * that we expect. */
void checkFont(XFontStruct * fontinfo, char *fontname)
{

#ifndef SMALL_SCREEN
  if (fontinfo->max_bounds.width != 6 ||
      fontinfo->min_bounds.width != 6 ||
      fontinfo->descent + fontinfo->ascent != 10 ||
      fontinfo->min_bounds.lbearing < 0 ||
      fontinfo->max_bounds.rbearing > 6 ||
      fontinfo->max_bounds.ascent > 8 ||
      fontinfo->max_bounds.descent > 2)
    {
      fprintf(stderr, "Warning: font '%s'\ndoes not conform to 6x10 character cell rules.\n", fontname);
    }
#endif
}

void GetFonts(void)
{
  Font    regular, italic, bold, big;
  int     i;
  XGCValues values;
  XFontStruct *fontinfo;
  char   *fontname;
  int     black, white;

  fontname = getdefault("font");
  if (fontname == NULL)
    fontname = NORMAL_FONT;
  fontinfo = XLoadQueryFont(W_Display, fontname);
  if (fontinfo == NULL)
    {
      fontinfo = find_font(fontname, _nfonts);
    }
  if (fontinfo == NULL)
    {
      printf("netrek: Can't find any fonts!\n");
      terminate(1);
    }
  checkFont(fontinfo, fontname);
  regular = fontinfo->fid;
  W_Textwidth = fontinfo->max_bounds.width;
  W_Textheight = fontinfo->max_bounds.descent + fontinfo->max_bounds.ascent;
  fonts[1].baseline = fontinfo->max_bounds.ascent;

  fontname = getdefault("boldfont");
  if (fontname == NULL)
    fontname = BOLD_FONT;
  fontinfo = XLoadQueryFont(W_Display, fontname);
  if (fontinfo == NULL)
    {
      fontinfo = find_font(fontname, _bfonts);
    }
  if (fontinfo == NULL)
    {
      bold = regular;
      fonts[2].baseline = fonts[1].baseline;
    }
  else
    {
      checkFont(fontinfo, fontname);
      bold = fontinfo->fid;
      fonts[2].baseline = fontinfo->max_bounds.ascent;
      if (fontinfo->max_bounds.width > W_Textwidth)
	W_Textwidth = fontinfo->max_bounds.width;
      if (fontinfo->max_bounds.descent + fontinfo->max_bounds.ascent > W_Textheight)
	W_Textheight = fontinfo->max_bounds.descent + fontinfo->max_bounds.ascent;
    }

  fontname = getdefault("italicfont");
  if (fontname == NULL)
    fontname = ITALIC_FONT;
  fontinfo = XLoadQueryFont(W_Display, fontname);
  if (fontinfo == NULL)
    {
      fontinfo = find_font(fontname, _ifonts);
    }
  if (fontinfo == NULL)
    {
      italic = regular;
      fonts[3].baseline = fonts[1].baseline;
    }
  else
    {
      checkFont(fontinfo, fontname);
      italic = fontinfo->fid;
      fonts[3].baseline = fontinfo->max_bounds.ascent;
      if (fontinfo->max_bounds.width > W_Textwidth)
	W_Textwidth = fontinfo->max_bounds.width;
      if (fontinfo->max_bounds.descent + fontinfo->max_bounds.ascent > W_Textheight)
	W_Textheight = fontinfo->max_bounds.descent + fontinfo->max_bounds.ascent;
    }

  fontname = getdefault("bigfont");
  if (fontname == NULL)
    fontname = BIG_FONT;
  fontinfo = XLoadQueryFont(W_Display, fontname);
  if (fontinfo == NULL)
    {
      fontinfo = find_font(fontname, _bgfonts);
    }
  if (fontinfo == NULL)
    {
      big = regular;
      fonts[0].baseline = fonts[1].baseline;
      W_BigTextwidth = W_Textwidth;
      W_BigTextheight = W_Textheight;
    }
  else
    {
      big = fontinfo->fid;
      fonts[0].baseline = fontinfo->max_bounds.ascent;
      W_BigTextwidth = fontinfo->max_bounds.width;
      W_BigTextheight = fontinfo->max_bounds.descent + fontinfo->max_bounds.ascent;
    }
  white = WhitePixel(W_Display, W_Screen);
  black = BlackPixel(W_Display, W_Screen);
  insens_tile = XCreatePixmapFromBitmapData(W_Display, W_Root, gray,
       TILESIDE, TILESIDE, black, white, DefaultDepth(W_Display, W_Screen));
  for (i = 0; i < NCOLORS; i++)
    {
      values.font = big;
      colortable[i].contexts[0] = XCreateGC(W_Display, W_Root, GCFont, &values);
      XSetGraphicsExposures(W_Display, colortable[i].contexts[0], False);
      values.font = regular;
      colortable[i].contexts[1] = XCreateGC(W_Display, W_Root, GCFont, &values);
      XSetGraphicsExposures(W_Display, colortable[i].contexts[1], False);

      values.fill_style = FillTiled;
      values.tile = insens_tile;
      colortable[i].insens_contexts[1] = XCreateGC(W_Display, W_Root,
				    GCFont | GCFillStyle | GCTile, &values);
      XSetGraphicsExposures(W_Display, colortable[i].insens_contexts[1],
			    False);
      values.font = bold;
      colortable[i].contexts[2] = XCreateGC(W_Display, W_Root, GCFont, &values);
      XSetGraphicsExposures(W_Display, colortable[i].contexts[2], False);
      values.font = italic;
      colortable[i].contexts[3] = XCreateGC(W_Display, W_Root, GCFont, &values);
      XSetGraphicsExposures(W_Display, colortable[i].contexts[3], False);
      {
	char dl[] = {1, 8};

	XSetLineAttributes(W_Display, colortable[i].contexts[3],
			   0, LineOnOffDash, CapButt, JoinMiter);
	XSetDashes(W_Display, colortable[i].contexts[3], 0, dl, 2);
      }
      values.function = GXor;
      colortable[i].contexts[BITGC] = XCreateGC(W_Display, W_Root, GCFunction,
						&values);
      XSetGraphicsExposures(W_Display, colortable[i].contexts[BITGC], False);
    }
}
XFontStruct *
        find_font(char *oldf, char **fonts)
{
  XFontStruct *fi;
  char  **f;

  fprintf(stderr, "netrek: Can't find font %s.  Trying others...\n",
	  oldf);
  for (f = fonts; *f; f++)
    {
      if (strcmp(*f, oldf) != 0)
	{
	  if ((fi = XLoadQueryFont(W_Display, *f)))
	    return fi;
	}
    }
  return NULL;
}

static unsigned short extrared[COLORS] =
{0x00, 0x20, 0x40, 0x60, 0x80, 0xa0, 0xb0, 0xc0};
static unsigned short extragreen[COLORS] =
{0x40, 0x60, 0x80, 0xa0, 0xb0, 0xc0, 0x00, 0x20};
static unsigned short extrablue[COLORS] =
{0x80, 0xa0, 0xb0, 0xc0, 0x00, 0x20, 0x40, 0x60};


void GetColors(void)
{
  int     i, j;
  XGCValues values;
  XColor  foo;
  int     white, black;
  unsigned long pixel;
  char    defaultstring[100];
  char   *defaults;
  unsigned long extracolors[COLORS];
  XColor  colordef;


  forceMono = booleanDefault("forcemono", forceMono);	/* 11/14/91 TC */

  if ((DisplayCells(W_Display, W_Screen) <= 2) || forceMono)
    {
      white = WhitePixel(W_Display, W_Screen);
      black = BlackPixel(W_Display, W_Screen);
      for (i = 0; i < NCOLORS; i++)
	{
	  if (i != W_Black)
	    {
	      colortable[i].pixelValue = white;
	    }
	  else
	    {
	      colortable[i].pixelValue = black;
	    }
	  if (i == W_Red)
	    {
	      colortable[i].pixmap = XCreatePixmapFromBitmapData(W_Display,
			  W_Root, striped, TILESIDE, TILESIDE, white, black,
					 DefaultDepth(W_Display, W_Screen));
	    }
	  else if (i == W_Yellow)
	    {
	      colortable[i].pixmap = XCreatePixmapFromBitmapData(W_Display,
			     W_Root, gray, TILESIDE, TILESIDE, white, black,
					 DefaultDepth(W_Display, W_Screen));
	    }
	  else
	    {
	      colortable[i].pixmap = XCreatePixmapFromBitmapData(W_Display,
					  W_Root, solid, TILESIDE, TILESIDE,
						   colortable[i].pixelValue,
						   colortable[i].pixelValue,
					 DefaultDepth(W_Display, W_Screen));
	    }
	  /* We assume white is 0 or 1, and black is 0 or 1. We adjust * *
	   * graphics function based upon who is who. */
	  if (white == 0)
	    {					 /* Black is 1 */
	      XSetFunction(W_Display, colortable[i].contexts[BITGC],
			   GXand);
	    }
	}
    }
  else if (W_Visual->class == PseudoColor)
    {
      if (!XAllocColorCells(W_Display, W_Colormap, False, planes, PLANES,
			    &pixel, 1) && !takeNearest)
	{
	  /* couldn't allocate 3 planes, make a new colormap */
	  W_Colormap = XCreateColormap(W_Display, W_Root, W_Visual, AllocNone);

	  if (!XAllocColorCells(W_Display, W_Colormap, False, planes, PLANES,
				&pixel, 1))
	    {
	      fprintf(stderr, "Cannot create new colormap\n");
	      terminate(1);
	    }
	  /* and fill it with at least 8 more colors so when mouse is inside
	   * * * netrek windows, use might be able to see his other windows */
	  if (XAllocColorCells(W_Display, W_Colormap, False, NULL, 0,
			       extracolors, COLORS))
	    {
	      colordef.flags = DoRed | DoGreen | DoBlue;

	      for (i = 0; i < COLORS; i++)
		{
		  colordef.pixel = extracolors[i];
		  colordef.red = extrared[i] << 8;
		  colordef.green = extragreen[i] << 8;
		  colordef.blue = extrablue[i] << 8;
		  XStoreColor(W_Display, W_Colormap, &colordef);
		}
	    }
	}

      for (i = 0; i < NCOLORS; i++)
	{
	  sprintf(defaultstring, "color.%s", colortable[i].name);
	  defaults = getdefault(defaultstring);

	  if (defaults == NULL)
	    {

#ifdef RACE_COLORS
	      if (i > GREY)
		{
		  /* The default colour from the ROMS is the colour defined * 
		   * 
		   * * to be RED and not the colour which is actually RED. */

		  sprintf(defaultstring, "color.%s",
			  colortable[i - RaceDefaultOffset].name);
		  defaults = getdefault(defaultstring);

		  if (defaults == NULL)
		    defaults = colortable[i - RaceDefaultOffset].name;
		}
	      else
#endif

		defaults = colortable[i].name;
	    }

	  XParseColor(W_Display, W_Colormap, defaults, &foo);

	  switch (i)
	    {

#ifndef RACE_COLORS
	    case WHITE:
	      foo.pixel = pixel | planes[0] | planes[1] | planes[2];
	      break;
	    case BLACK:
	      foo.pixel = pixel;
	      break;
	    case RED:
	      foo.pixel = pixel | planes[0];
	      break;
	    case CYAN:
	      foo.pixel = pixel | planes[1];
	      break;
	    case YELLOW:
	      foo.pixel = pixel | planes[2];
	      break;
	    case GREY:
	      foo.pixel = pixel | planes[0] | planes[1];
	      break;
	    case GREEN:
	      foo.pixel = pixel | planes[1] | planes[2];
	      break;
#else
	      /* 
	       * Choose colors so that when two ships overlap, things look
	       * ok.  When players overlab, the bits are ORed together. */

	      /* Background color */
	    case BLACK:
	      foo.pixel = pixel;
	      break;

	      /* Alert colors (sum to grey usually) */
	    case RED:
	      foo.pixel = pixel | planes[1] | planes[2];
	      break;
	    case CYAN:
	      foo.pixel = pixel | planes[1] | planes[3];
	      break;
	    case YELLOW:
	      foo.pixel = pixel | planes[2] | planes[3];
	      break;
	    case GREEN:
	      foo.pixel = pixel | planes[1];
	      break;
	    case GREY:
	      foo.pixel = pixel | planes[1] | planes[2] | planes[3];
	      break;

	      /* Your color */
	    case WHITE:
	      foo.pixel = pixel | planes[0];
	      break;

	      /* The color of other ships should dominate over your color * * 
	       * and should sum to C_IND where possible. */
	    case C_FED:
	      foo.pixel = pixel | planes[0] | planes[1] | planes[2];
	      break;
	    case C_ROM:
	      foo.pixel = pixel | planes[0] | planes[1] | planes[3];
	      break;
	    case C_KLI:
	      foo.pixel = pixel | planes[0] | planes[2] | planes[3];
	      break;
	    case C_ORI:
	      foo.pixel = pixel | planes[0] | planes[1];
	      break;
	    case C_IND:
	      foo.pixel = pixel | planes[0] | planes[1] | planes[2] | planes[3];
	      break;
#endif
	    }
	  if (takeNearest)
	    XAllocColor(W_Display, W_Colormap, &foo);
	  else
	    XStoreColor(W_Display, W_Colormap, &foo);
	  colortable[i].pixelValue = foo.pixel;
	  colortable[i].pixmap = XCreatePixmapFromBitmapData(W_Display,
		    W_Root, solid, TILESIDE, TILESIDE, foo.pixel, foo.pixel,
					 DefaultDepth(W_Display, W_Screen));
	}
    }
  else if (W_Visual->class >= TrueColor)
    {
      /* Stuff added by sheldon@iastate.edu 5/28/93 This is supposed to * *
       * detect a TrueColor display, and then do a lookup of the colors in *
       * * default colormap, instead of creating new colormap. */
      for (i = 0; i < NCOLORS; i++)
	{
	  sprintf(defaultstring, "color.%s", colortable[i].name);

	  defaults = getdefault(defaultstring);
	  if (defaults == NULL)
	    {

#ifdef RACE_COLORS
	      if (i > GREY)
		{
		  /* The default color from the ROMS is the color defined to
		   * * * be RED and not the color which is actually RED. */

		  sprintf(defaultstring, "color.%s",
			  colortable[i - RaceDefaultOffset].name);
		  defaults = getdefault(defaultstring);

		  if (defaults == NULL)
		    defaults = colortable[i - RaceDefaultOffset].name;
		}
	      else
#endif

		defaults = colortable[i].name;
	    }


	  XParseColor(W_Display, W_Colormap, defaults, &foo);
	  XAllocColor(W_Display, W_Colormap, &foo);
	  colortable[i].pixelValue = foo.pixel;
	  colortable[i].pixmap = XCreatePixmapFromBitmapData(W_Display,
		    W_Root, solid, TILESIDE, TILESIDE, foo.pixel, foo.pixel,
					 DefaultDepth(W_Display, W_Screen));
	}
    }
  else
    {

      fprintf(stderr, "Don't know how to deal with a Class %d Visual\n",
	      W_Visual->class);
      fprintf(stderr, "Your X server is not supported at %d bpp!\n",
	      DefaultDepth(W_Display, W_Screen));
      terminate(1);
    }
  for (i = 0; i < NCOLORS; i++)
    {
      for (j = 0; j < FONTS + 1; j++)
	{
	  XSetForeground(W_Display, colortable[i].contexts[j],
			 colortable[i].pixelValue);
	  XSetBackground(W_Display, colortable[i].contexts[j],
			 colortable[W_Black].pixelValue);
	}
    }

  if (scrollbar)
    {
      scroll_thumb_pixmap = XCreatePixmapFromBitmapData(W_Display,
					   W_Root, gray, TILESIDE, TILESIDE,
					     colortable[W_White].pixelValue,
					     colortable[W_Black].pixelValue,
					 DefaultDepth(W_Display, W_Screen));

      values.fill_style = FillTiled;
      values.tile = scroll_thumb_pixmap;

      scroll_thumb_gc = XCreateGC(W_Display, W_Root,
				  GCFillStyle | GCTile, &values);
    }
}

void W_RenameWindow(W_Window window, char *str)
{
  struct window *win = W_Void2Window(window);
  XStoreName(W_Display, win->window, str);
}

W_Window
W_MakeWindow(char *name, int x, int y, int width, int height, W_Window parent, int border, W_Color color)
{
  struct window *newwin;
  Window  wparent;
  XSetWindowAttributes attrs;
  char   *window_title = "Netrek", title_buff[257];
  XSizeHints *sz_hints;
  int     gcheck_result;

#if DEBUG > 0
  printf("New window...\n");
#endif

  gcheck_result = checkGeometry(name, &x, &y, &width, &height);
  checkParent(name, &parent);
  wparent = W_Void2Window(parent)->window;
  attrs.border_pixel = colortable[color].pixelValue;
  attrs.event_mask = KeyPressMask | ButtonPressMask | ExposureMask | LeaveWindowMask;

#ifdef AUTOKEY
  attrs.event_mask |= KeyReleaseMask;
#endif /* AUTOKEY */

#ifdef MOTION_MOUSE
  attrs.event_mask |= ButtonMotionMask;
#endif

  if (strcmp(name, "netrek_icon") == 0)		 /* icon should not select *
						  * for input */
    attrs.event_mask = ExposureMask;
  attrs.background_pixel = colortable[W_Black].pixelValue;
  attrs.do_not_propagate_mask = KeyPressMask | ButtonPressMask | ExposureMask;
  newwin = newWindow(
	      XCreateWindow(W_Display, wparent, x, y, width, height, border,
			    CopyFromParent, InputOutput, CopyFromParent,
			    CWBackPixel | CWEventMask | CWBorderPixel,
			    &attrs),
		      WIN_GRAPH);

  if (!strcmp(name, "netrek")) {
    if (full_screen_enabled) {
#ifdef FULLSCREEN
      kde_fullscreen_on(W_Window2Void(newwin));
#endif
    }
  }

  /* top window */
  sz_hints = XAllocSizeHints();
  if (strcmp(name, "netrek") == 0 || strcmp(name, "wait") == 0 ||
      strcmp(name, "waitmotd") == 0)
    {
      if (!title && serverName)
	{
	  if (strcmp(name, "wait") == 0)
	    strcpy(title_buff, serverName);
	  else if (strcmp(name, "waitmotd") == 0)
	    strcpy(title_buff, "Motd-[f]forward, [b]back, [space]unmap");
	  else
	    sprintf(title_buff, "Netrek  @  %s", serverName);
	  window_title = title_buff;
	}
      else
	/* but title on command line will override */ if (title)
	window_title = title;
      sz_hints->min_width = width;
      sz_hints->max_width = width;
      sz_hints->min_height = height;
      sz_hints->max_height = height;
      sz_hints->flags = PMinSize | PMaxSize;

#ifndef SMALL_SCREEN
/* remove this check for SMALL_SCREEN;
 * otherwise root window may not be aligned with upper-left corner of screen */
      if (gcheck_result & G_SET_X || gcheck_result & G_SET_Y)
#endif

	sz_hints->flags |= WMXYHintMode_default();
    }
  else
    {
      window_title = name;
      if (gcheck_result & G_SET_X || gcheck_result & G_SET_Y)
	sz_hints->flags |= WMXYHintMode_default();
    }
  XStoreName(W_Display, newwin->window, window_title);
  XSetWMNormalHints(W_Display, newwin->window, sz_hints);
  class_hint.res_name = name;
  XSetClassHint(W_Display, newwin->window, &class_hint);
  XSetWMHints(W_Display, newwin->window, &wm_hint);
  newwin->name = strdup(name);
  newwin->width = width;
  newwin->height = height;
  if (wparent != W_Root)
    if (checkMapped(name))
      W_MapWindow(W_Window2Void(newwin));

#if DEBUG > 0
  printf("New graphics window %d, child of %d\n", newwin, parent);
#endif

  XSetWindowColormap(W_Display, newwin->window, W_Colormap);
  return W_Window2Void(newwin);
}

void
        W_ChangeBorder(W_Window window, int color)
{

#if DEBUG > 2
  printf("Changing border of %d\n", window);
#endif

  /* fix inexplicable color bug */
  if ((DisplayCells(W_Display, W_Screen) <= 2) || forceMono)

    XSetWindowBorderPixmap(W_Display, W_Void2Window(window)->window,
			   colortable[color].pixmap);
  else
    XSetWindowBorder(W_Display, W_Void2Window(window)->window,
		     colortable[color].pixelValue);


}

void
        W_MapWindow(W_Window window)
{
  struct window *win;

#if DEBUG > 2
  printf("Mapping %d\n", window);
#endif

  win = W_Void2Window(window);
  if (win->mapped)
    return;
  win->mapped = 1;
  XMapRaised(W_Display, win->window);
}

void
        W_UnmapWindow(W_Window window)
{
  struct window *win;

#if DEBUG > 2
  printf("UnMapping %d\n", window);
#endif

  win = W_Void2Window(window);
  if (win->mapped == 0)
    return;
  win->mapped = 0;
  XUnmapWindow(W_Display, win->window);
}

int
        W_IsMapped(W_Window window)
{
  struct window *win;

  win = W_Void2Window(window);
  if (win == NULL)
    return 0;
  return win->mapped;
}

void
        W_FillArea(W_Window window, int x, int y, int width, int height, W_Color color)
{
  struct window *win;

#if DEBUG > 2
  printf("Clearing (%d %d) x (%d %d) with %d on %d\n", x, y, width, height,
	 color, window);
#endif

  win = W_Void2Window(window);
  switch (win->type)
    {
    case WIN_GRAPH:
      XFillRectangle(W_Display, win->window, colortable[color].contexts[0],
		     x, y, width, height);
      break;
    case WIN_SCROLL:
      XFillRectangle(W_Display, win->window, colortable[color].contexts[0],
		     WIN_EDGE + x * W_Textwidth,
		     MENU_PAD + y * W_Textheight,
		     width * W_Textwidth, height * W_Textheight);
      break;
    default:
      XFillRectangle(W_Display, win->window, colortable[color].contexts[0],
		     WIN_EDGE + x * W_Textwidth, MENU_PAD + y * W_Textheight,
		     width * W_Textwidth, height * W_Textheight);
    }
}

/* XFIX */

static XRectangle _rcache[MAXCACHE];
static int _rcache_index;

static void
        FlushClearAreaCache(Window win)
{
  XFillRectangles(W_Display, win, colortable[backColor].contexts[0],
		  _rcache, _rcache_index);
  _rcache_index = 0;
}

/* local window only */
void
        W_CacheClearArea(W_Window window, int x, int y, int width, int height)
{
  Window  win = W_Void2Window(window)->window;
  register XRectangle *r;

  if (_rcache_index == MAXCACHE)
    FlushClearAreaCache(win);

  r = &_rcache[_rcache_index++];
  r->x = (short) x;
  r->y = (short) y;
  r->width = (unsigned short) width;
  r->height = (unsigned short) height;
}

void
        W_FlushClearAreaCache(W_Window window)
{
  Window  win = W_Void2Window(window)->window;

  if (_rcache_index)
    FlushClearAreaCache(win);
}

/* XFIX: clears now instead of filling. */
void
        W_ClearArea(W_Window window, int x, int y, int width, int height)
{
  struct window *win;

  win = W_Void2Window(window);
  switch (win->type)
    {
    case WIN_GRAPH:
      /* XFIX: changed */
      XClearArea(W_Display, win->window, x, y, width, height, False);
      break;
    case WIN_SCROLL:
      XClearArea(W_Display, win->window, WIN_EDGE + x * W_Textwidth,
		 MENU_PAD + y * W_Textheight,
		 width * W_Textwidth, height * W_Textheight, False);
      break;
    default:
      /* XFIX: changed */
      XClearArea(W_Display, win->window, WIN_EDGE + x * W_Textwidth,
		 MENU_PAD + y * W_Textheight, width * W_Textwidth, height * W_Textheight, False);
      break;
    }
}

void
        W_ClearWindow(W_Window window)
{

#if DEBUG > 2
  printf("Clearing %d\n", window);
#endif

  XClearWindow(W_Display, W_Void2Window(window)->window);
}

int
	W_Pending(void)
{
  return XPending(W_Display);
}

int
        W_EventsPending(void)
{
  if (W_isEvent)
    return 1;
  while (XPending(W_Display))
    {
      if (W_SpNextEvent(&W_myevent))
	{
	  W_isEvent = 1;
	  return 1;
	}
    }
  return 0;
}

void
        W_NextEvent(W_Event * wevent)
{
  if (W_isEvent)
    {
      *wevent = W_myevent;
      W_isEvent = 0;
      return;
    }
  while (W_SpNextEvent(wevent) == 0);
}

static unsigned char sym_to_key(int sym)
{
  switch (sym) {
  case XK_Up: return W_Key_Up;
  case XK_Down: return W_Key_Down;
  }
  return 0;
}

int
        W_SpNextEvent(W_Event * wevent)
{
  XEvent  event;
  XKeyEvent *key;
  XButtonEvent *button;
  XExposeEvent *expose;
  XConfigureEvent *configure;

#ifdef MOTION_MOUSE
  XMotionEvent *motion;
  static int prev_x, prev_y;
  int     thresh;
#endif

#ifdef CONTROL_KEY
  int     control_key = 0;
#endif
  unsigned char ch;
  int nch;
  struct window *win;
  KeySym  sym;

#if DEBUG > 1
  printf("event");
#endif

  key = (XKeyEvent *) & event;
  button = (XButtonEvent *) & event;
  expose = (XExposeEvent *) & event;
  configure = (XConfigureEvent *) & event;

#ifdef MOTION_MOUSE
  motion = (XMotionEvent *) & event;
#endif

  for (;;)
    {
      XNextEvent(W_Display, &event);

#if DEBUG > 1
      printf(", read type=%d\n", event.type);
#endif
      win = findWindow(key->window);
      if (win == NULL)
	return 0;
      if (key->send_event == True && event.type != ClientMessage)
	return 0; /* event sent by another client */
      if ((event.type == KeyPress || event.type == ButtonPress) &&
	  win->type == WIN_MENU)
	{
	  if (key->y % (W_Textheight + MENU_PAD * 2 + MENU_BAR) >=
	      W_Textheight + MENU_PAD * 2)
	    return 0;
	  key->y = key->y / (W_Textheight + MENU_PAD * 2 + MENU_BAR);
	}
      switch ((int) event.type)
	{
	case ClientMessage:
	  if (event.xclient.message_type == wm_protocols &&
	      event.xclient.data.l[0] == wm_delete_window)
	    {
	      W_UnmapWindow(W_Window2Void(win));
	      wevent->type = W_EV_CLOSED;
	      wevent->Window = W_Window2Void(win);
	      wevent->key = wevent->x = wevent->y = 0;
	      return 1;
	    }
	  break;

	case LeaveNotify: /* for message window -- jfy */
	  if (win == (struct window *) messagew)
	    {
	      W_in_message = 0;
	    }
	  return 0;
	  break;
	case KeyPress:
	  if ((key->state & LockMask) && !(key->state & ShiftMask) &&
	      (ignoreCaps))
	    {
	      printf("Got a capslock!\n");
	      key->state = key->state & ~LockMask;
	    }

#ifdef CONTROL_KEY
	  if (key->state & ControlMask && use_control_key)
	    {
	      control_key = 1;
	      key->state = key->state & ~ControlMask;
	    }
	  else
	    control_key = 0;
#endif

	  nch = XLookupString(key, (char *) &ch, 1, &sym, NULL);
	  if (nch == 0) {
	    ch = sym_to_key(sym);
	    if (ch == 0) return 0;
	  }
#ifdef MOUSE_AS_SHIFT
	  if (mouse_as_shift)
	    {
	      if (key->state & Button1Mask)
		{
		  wevent->modifier = W_LBUTTON;
		  wevent->type = W_EV_MKEY;
		}
	      else if (key->state & Button2Mask)
		{
		  wevent->modifier = W_MBUTTON;
		  wevent->type = W_EV_MKEY;
		}
	      else if (key->state & Button3Mask)
		{
		  wevent->modifier = W_RBUTTON;
		  wevent->type = W_EV_MKEY;
		}
	      else
		{
		  wevent->type = W_EV_KEY;
		}
	    }
	  else
	    wevent->type = W_EV_KEY;
#else
	  wevent->type = W_EV_KEY;
#endif

	  wevent->Window = W_Window2Void(win);
	  wevent->x = key->x;
	  wevent->y = key->y;
	  
#ifdef CONTROL_KEY
	  if (control_key)
	    wevent->key = (unsigned char) (ch + 96);
	  else
	    wevent->key = ch;
#else
	  wevent->key = ch;
#endif
	  
	  return 1;
	  break;

#ifdef AUTOKEY
	case KeyRelease:
	  if (XLookupString(key, &ch, 1, NULL, NULL) > 0)
	    {
	      wevent->type = W_EV_KEY_OFF;
	      wevent->Window = W_Window2Void(win);
	      wevent->x = key->x;
	      wevent->y = key->y;
	      wevent->key = ch;
	      return 1;
	    }
	  return 0;
	  break;
#endif /* AUTOKEY */

	case ButtonPress:
	  wevent->type = W_EV_BUTTON;
	  wevent->Window = W_Window2Void(win);

#ifdef MOTION_MOUSE
	  prev_x = wevent->x = button->x;
	  prev_y = wevent->y = button->y;
#else
	  wevent->x = button->x;
	  wevent->y = button->y;
#endif


#ifdef MOUSE_AS_SHIFT
	  if (mouse_as_shift &&
	      (wevent->Window == mapw || wevent->Window == w))
	    switch (button->button & 0xf)
	      {
	      case Button3:
		if (b3_as_shift)
		  return 0;
		break;
	      case Button1:
		if (b1_as_shift)
		  return 0;
		break;
	      case Button2:
		if (b2_as_shift)
		  return 0;
		break;
	      }
#endif

	  switch (button->button & 0xf)
	    {
	    case Button3:
	      wevent->key = W_RBUTTON;
	      break;
	    case Button1:
	      wevent->key = W_LBUTTON;
	      break;
	    case Button2:
	      wevent->key = W_MBUTTON;
	      break;
#ifdef Button4
	    case Button4:
	      wevent->key = W_WUBUTTON;
	      break;
#endif
#ifdef Button5
	    case Button5:
	      wevent->key = W_WDBUTTON;
	      break;
#endif
#ifdef Button6
	    case Button6:
	      wevent->key = W_X1BUTTON;
	      break;
#endif
#ifdef Button7
	    case Button7:
	      wevent->key = W_X2BUTTON;
	      break;
#endif
	    }

#ifdef SHIFTED_MOUSE
	  if (extended_mouse)
	    {
	      if (button->state & (ControlMask | ShiftMask))
		{

		  if (button->state & ShiftMask)
		    {
		      wevent->key |= W_SHIFT_BUTTON;
		    }

		  if (button->state & ControlMask)
		    {
		      wevent->key |= W_CTRL_BUTTON;
		    }

		  return 1;
		}

	    }
#endif

	  if (win->type == WIN_SCROLL)
	    {
	      scrollScrolling(wevent);
	      return 0;
	    }
	  return 1;

#ifdef MOTION_MOUSE
	case MotionNotify:
	  if (!motion_mouse ||
	      (!motion_mouse_enablable && !motion_mouse_steering))
	    return 0;
	  wevent->type = W_EV_CM_BUTTON;
	  wevent->Window = W_Window2Void(win);

	  thresh = abs(prev_x - motion->x) + abs(prev_y - motion->y);

	  if (thresh < user_motion_thresh)
	    return 0;

	  prev_x = wevent->x = motion->x;
	  prev_y = wevent->y = motion->y;


#ifdef MOUSE_AS_SHIFT
	  if (mouse_as_shift &&
	      (wevent->Window == mapw || wevent->Window == w))
	    switch (button->button & 0xf)
	      {
	      case Button3:
		if (b3_as_shift)
		  return 0;
		break;
	      case Button1:
		if (b1_as_shift)
		  return 0;
		break;
	      case Button2:
		if (b2_as_shift)
		  return 0;
		break;
	      }
#endif

	  switch (button->button & 0xf)
	    {
	    case Button3:
	      wevent->key = W_RBUTTON;
	      break;
	    case Button1:
	      wevent->key = W_LBUTTON;
	      break;
	    case Button2:
	      wevent->key = W_MBUTTON;
	      break;
#ifdef Button4
	    case Button4:
	      wevent->key = W_WUBUTTON;
	      break;
#endif
#ifdef Button5
	    case Button5:
	      wevent->key = W_WDBUTTON;
	      break;
#endif
#ifdef Button6
	    case Button6:
	      wevent->key = W_X1BUTTON;
	      break;
#endif
#ifdef Button7
	    case Button7:
	      wevent->key = W_X2BUTTON;
	      break;
#endif
	    }

#ifdef SHIFTED_MOUSE
	  if (extended_mouse)
	    {
	      if (button->state & (ControlMask | ShiftMask))
		{

		  if (button->state & ShiftMask)
		    {
		      wevent->key |= W_SHIFT_BUTTON;
		    }

		  if (button->state & ControlMask)
		    {
		      wevent->key |= W_CTRL_BUTTON;
		    }

		  return 1;
		}
	    }
#endif

	  return 1;
#endif

	case Expose:
	  if (expose->count != 0)
	    return 0;
	  if (win->type == WIN_SCROLL)
	    {
	      configureScrolling(win, expose->x, expose->y,
				 expose->width, expose->height);
	      redrawScrolling(win);
	      return 0;
	    }
	  if (win->type == WIN_MENU)
	    {
	      redrawMenu(win);
	      return 0;
	    }

	  wevent->type = W_EV_EXPOSE;
	  wevent->Window = W_Window2Void(win);
	  return 1;
	case ConfigureNotify:
	  configureScrolling(win, configure->x, configure->y,
			     configure->width, configure->height);
	  break;
	default:
	  return 0;
	  break;
	}
    }
}

void
        W_MakeLine(W_Window window, int x0, int y0, int x1, int y1, W_Color color)
{
  Window  win;

#if DEBUG > 3
  printf("Line on %d\n", window);
#endif

  win = W_Void2Window(window)->window;
  XDrawLine(W_Display, win, colortable[color].contexts[0], x0, y0, x1, y1);
}

/* XFIX */

static XSegment _lcache[NCOLORS][MAXCACHE];
static int _lcache_index[NCOLORS];

static void
        FlushLineCache(Window win, int color)
{
  XDrawSegments(W_Display, win, colortable[color].contexts[0],
		_lcache[color], _lcache_index[color]);
  _lcache_index[color] = 0;
}

/* for local window only */
void
        W_CacheLine(W_Window window, int x0, int y0, int x1, int y1, int color)
{
  Window  win = W_Void2Window(window)->window;
  register XSegment *s;

  if (_lcache_index[color] == MAXCACHE)
    FlushLineCache(win, color);

  s = &_lcache[color][_lcache_index[color]++];
  s->x1 = (short) x0;
  s->y1 = (short) y0;
  s->x2 = (short) x1;
  s->y2 = (short) y1;
}

void
        W_FlushLineCaches(W_Window window)
{
  Window  win = W_Void2Window(window)->window;
  int i;

  for (i = 0; i < NCOLORS; i++)
    {
      if (_lcache_index[i])
	FlushLineCache(win, i);
    }
}

void
        W_MakeTractLine(W_Window window, int x0, int y0, int x1, int y1, W_Color color)
{
  Window  win;

#if DEBUG > 3
  printf("Line on %d\n", window);
#endif

  win = W_Void2Window(window)->window;
  XDrawLine(W_Display, win, colortable[color].contexts[3], x0, y0, x1, y1);
}

void
        W_MakePhaserLine(W_Window window, int x0, int y0, int x1, int y1, W_Color color)
{
  Window  win;

#if DEBUG > 3
  printf("Line on %d\n", window);
#endif

  win = W_Void2Window(window)->window;
  XDrawLine(W_Display, win, colortable[color].contexts[1], x0, y0, x1, y1);
}

void W_WriteCircle (W_Window window,
                    int x,
                    int y,
                    int r,
                    W_Color color)
{
  struct window *win = W_Void2Window(window);

  XSetForeground(W_Display, colortable[color].contexts[0],
		 colortable[color].pixelValue);
  XDrawArc(W_Display, win->window, colortable[color].contexts[0],
	   x, y, r, r, 0, 23040);
}

void
        W_WriteTriangle(W_Window window, int x, int y, int s, int t, W_Color color)
{
  struct window *win = W_Void2Window(window);
  XPoint  points[4];

  if (t == 0)
    {
      points[0].x = x;
      points[0].y = y;
      points[1].x = x + s;
      points[1].y = y - s;
      points[2].x = x - s;
      points[2].y = y - s;
      points[3].x = x;
      points[3].y = y;
    }
  else
    {
      points[0].x = x;
      points[0].y = y;
      points[1].x = x + s;
      points[1].y = y + s;
      points[2].x = x - s;
      points[2].y = y + s;
      points[3].x = x;
      points[3].y = y;
    }


  XDrawLines(W_Display, win->window, colortable[color].contexts[0],
	     points, 4, CoordModeOrigin);
}

void
        W_WriteText(W_Window window, int x, int y, W_Color color, char *str, int len, W_Font font)
{
  struct window *win;
  int     addr;

#if DEBUG > 3
  printf("Text for %d @ (%d, %d) in %d: [%s]\n", window, x, y, color, str);
#endif

  if (font == 0)
    font = W_RegularFont;
  win = W_Void2Window(window);

  switch (win->type)
    {
    case WIN_GRAPH:
      addr = fonts[fontNum(font)].baseline;
      if (len < 0) len = strlen(str);
      XDrawImageString(W_Display, win->window,

#ifdef SHORT_PACKETS
		       win->insensitive ?
		       colortable[color].insens_contexts[1] :
		       colortable[color].contexts[fontNum(font)],
#else
		       colortable[color].contexts[fontNum(font)],
#endif

		       x, y + addr, str, len);
      break;
    case WIN_SCROLL:
      XCopyArea(W_Display, win->window, win->window,
	 colortable[W_White].contexts[0], WIN_EDGE, MENU_PAD + W_Textheight,
		win->width * W_Textwidth, (win->height - 1) * W_Textheight,
		WIN_EDGE, MENU_PAD);
      XClearArea(W_Display, win->window,
		 WIN_EDGE, MENU_PAD + W_Textheight * (win->height - 1),
		 W_Textwidth * win->width, W_Textheight, False);
      if (len < 0) len = strlen(str);
      XDrawImageString(W_Display, win->window,

#ifdef SHORT_PACKETS
		       win->insensitive ?
		       colortable[color].insens_contexts[1] :
		       colortable[color].contexts[fontNum(font)],
#else
		       colortable[color].contexts[fontNum(font)],
#endif

		       WIN_EDGE, MENU_PAD + W_Textheight * (win->height - 1) + fonts[fontNum(font)].baseline,
		       str, len);
      AddToScrolling(win, color, font, str, len);
      break;
    case WIN_MENU:
      changeMenuItem(win, x, y, str, color);
      break;
    default:
      addr = fonts[fontNum(font)].baseline;
      if (len < 0) len = strlen(str);
      XDrawImageString(W_Display, win->window,

#ifdef SHORT_PACKETS
		       win->insensitive ?
		       colortable[color].insens_contexts[1] :
		       colortable[color].contexts[fontNum(font)],
#else
		       colortable[color].contexts[fontNum(font)],
#endif

	     x * W_Textwidth + WIN_EDGE, MENU_PAD + y * W_Textheight + addr,
		       str, len);
      break;
    }
}

void
        W_MaskText(W_Window window, int x, int y, W_Color color, char *str, int len, W_Font font)
{
  struct window *win;
  int     addr;

  addr = fonts[fontNum(font)].baseline;

#if DEBUG > 3
  printf("TextMask for %d @ (%d, %d) in %d: [%s]\n", window, x, y, color, str);
#endif

  win = W_Void2Window(window);
  XDrawString(W_Display, win->window,
	  colortable[color].contexts[fontNum(font)], x, y + addr, str, len);
}

W_Icon
W_StoreBitmap(int width, int height, char *data, W_Window window)
{
  struct icon *newicon;
  struct window *win;

#if DEBUG > 0
  printf("Storing bitmap for %d (%d x %d)\n", window, width, height);
  fflush(stdout);
#endif

  win = W_Void2Window(window);
  newicon = (struct icon *) malloc(sizeof(struct icon));

  newicon->width = width;
  newicon->height = height;
  newicon->bitmap = XCreateBitmapFromData(W_Display, win->window,
					  data, width, height);

#ifdef nodef
  /* XFIX: changed to Pixmap */
  white = WhitePixel(W_Display, W_Screen);
  black = BlackPixel(W_Display, W_Screen);
  newicon->bitmap = XCreatePixmapFromBitmapData(W_Display, W_Root, data,
						width, height, white, black,
						DefaultDepth(W_Display,
							     W_Screen));
#endif /* nodef */

  newicon->window = win->window;
  newicon->pixmap = 0;
  return W_Icon2Void(newicon);
}

void
        W_WriteBitmap(int x, int y, W_Icon bit, W_Color color)
{
  struct icon *icon;

  icon = W_Void2Icon(bit);

#if DEBUG > 4
  printf("Writing bitmap to %d\n", icon->window);
#endif

  XCopyPlane(W_Display, icon->bitmap, icon->window,
	 colortable[color].contexts[BITGC], 0, 0, icon->width, icon->height,
	     x, y, 1);

#ifdef nodef
  /* XFIX : copyarea */
  XCopyArea(W_Display, icon->bitmap, icon->window,
	 colortable[color].contexts[BITGC], 0, 0, icon->width, icon->height,
	    x, y);
#endif
}


void
        W_TileWindow(W_Window window, W_Icon bit)
{
  Window  win;
  struct icon *icon;

#if DEBUG > 4
  printf("Tiling window %d\n", window);
#endif

  icon = W_Void2Icon(bit);
  win = W_Void2Window(window)->window;

  if (icon->pixmap == 0)
    {
      icon->pixmap = XCreatePixmap(W_Display, W_Root,
	      icon->width, icon->height, DefaultDepth(W_Display, W_Screen));
      XCopyPlane(W_Display, icon->bitmap, icon->pixmap,
	   colortable[W_White].contexts[0], 0, 0, icon->width, icon->height,
		 0, 0, 1);
    }
  XSetWindowBackgroundPixmap(W_Display, win, icon->pixmap);
  XClearWindow(W_Display, win);

  /* if (icon->pixmap==0) { icon->pixmap=XMakePixmap(icon->bitmap, * *
   * colortable[W_White].pixelValue, colortable[W_Black].pixelValue); } * *
   * XChangeBackground(win, icon->pixmap); XClear(win); */
}

void
        W_UnTileWindow(W_Window window)
{
  Window  win;

#if DEBUG > 4
  printf("Untiling window %d\n", window);
#endif

  win = W_Void2Window(window)->window;

  XSetWindowBackground(W_Display, win, colortable[W_Black].pixelValue);
  XClearWindow(W_Display, win);
}

W_Window
W_MakeTextWindow(char *name, int x, int y, int width, int height, W_Window parent, int border)
{
  struct window *newwin;
  Window  wparent;
  XSetWindowAttributes attrs;
  XSizeHints *sz_hints;
  int     gcheck_result;

#if DEBUG > 0
  printf("New window...\n");
#endif

  gcheck_result = checkGeometry(name, &x, &y, &width, &height);
  checkParent(name, &parent);
  attrs.border_pixel = colortable[W_White].pixelValue;
  attrs.event_mask = ExposureMask;

#ifdef AUTOKEY
  attrs.event_mask |= KeyReleaseMask;
#endif /* AUTOKEY */

#ifdef SHORT_PACKETS
  attrs.event_mask |= ButtonPressMask;
#endif

  attrs.background_pixel = colortable[W_Black].pixelValue;
  attrs.do_not_propagate_mask = ExposureMask;
  wparent = W_Void2Window(parent)->window;
  newwin = newWindow(
		      XCreateWindow(W_Display, wparent, x, y,
   width * W_Textwidth + WIN_EDGE * 2, MENU_PAD * 2 + height * W_Textheight,
			border, CopyFromParent, InputOutput, CopyFromParent,
				    CWBackPixel | CWEventMask |
				    CWBorderPixel,
				    &attrs),
		      WIN_TEXT);
  class_hint.res_name = name;
  sz_hints = XAllocSizeHints();
  sz_hints->min_width = WIN_EDGE * 2 + width * W_Textwidth;
  sz_hints->max_width = WIN_EDGE * 2 + width * W_Textwidth;
  sz_hints->base_width = WIN_EDGE * 2;
  sz_hints->width_inc = W_Textwidth;
  sz_hints->min_height = MENU_PAD * 2 + 3 * W_Textheight;
  sz_hints->max_height = MENU_PAD * 2 + height * W_Textheight;
  sz_hints->base_height = MENU_PAD * 2 + 2 * W_Textheight;
  sz_hints->height_inc = W_Textheight;
  sz_hints->flags = PResizeInc | PMinSize | PMaxSize | PBaseSize;
  if (gcheck_result & G_SET_X || gcheck_result & G_SET_Y)
    sz_hints->flags |= WMXYHintMode_default();
  XStoreName(W_Display, newwin->window, name);
  XSetWMNormalHints(W_Display, newwin->window, sz_hints);
  XSetClassHint(W_Display, newwin->window, &class_hint);
  XSetWMHints(W_Display, newwin->window, &wm_hint);
  newwin->name = strdup(name);
  newwin->width = width;
  newwin->height = height;
  if (wparent != W_Root)
    if (checkMapped(name))
      W_MapWindow(W_Window2Void(newwin));

#if DEBUG > 0
  printf("New text window %d, child of %d\n", newwin, parent);
#endif

  XSetWindowColormap(W_Display, newwin->window, W_Colormap);
  return W_Window2Void(newwin);
}

struct window *
        newWindow(Window window, int type)
{
  struct window *newwin;

  XSetWMProtocols (W_Display, window, &wm_delete_window, 1);
  newwin = (struct window *) malloc(sizeof(struct window));

  newwin->window = window;
  newwin->type = type;
  newwin->mapped = 0;
  newwin->handle_keydown = 0;
  newwin->handle_keyup = 0;
  newwin->handle_button = 0;
  newwin->handle_expose = 0;
  addToHash(newwin);

#ifdef SHORT_PACKETS
  newwin->insensitive = 0;
#endif

  newwin->cursor = (Cursor) 0;			 /* about the best I can do * 
						  * 
						  * * -jw */
  return newwin;
}

struct window *
        findWindow(Window window)
{
  struct windowlist *entry;

  entry = hashtable[hash(window)];
  while (entry != NULL)
    {
      if (entry->window->window == window)
	return entry->window;
      entry = entry->next;
    }
  return NULL;
}

void addToHash(struct window * win)
{
  struct windowlist **new;

#if DEBUG > 0
  printf("Adding to %d\n", hash(win->window));
#endif

  new = &hashtable[hash(win->window)];
  while (*new != NULL)
    {
      new = &((*new)->next);
    }
  *new = (struct windowlist *) malloc(sizeof(struct windowlist));

  (*new)->next = NULL;
  (*new)->window = win;
}

W_Window
W_MakeScrollingWindow(name, x, y, width, height, parent, border)
char   *name;
int     x, y, width, height;
W_Window parent;
int     border;
{
  struct window *newwin;
  Window  wparent;
  XSetWindowAttributes attrs;
  XSizeHints *sz_hints;
  int     gcheck_result;
  struct scrollingWindow *sw;
  int     scw = (scrollbar ? scroll_thumb_width : 0);

#if DEBUG > 0
  printf("New window...\n");
#endif

  gcheck_result = checkGeometry(name, &x, &y, &width, &height);
  checkParent(name, &parent);
  wparent = W_Void2Window(parent)->window;
  attrs.border_pixel = colortable[W_White].pixelValue;
  attrs.event_mask = StructureNotifyMask | ExposureMask | ButtonPressMask;

#ifdef AUTOKEY
  attrs.event_mask |= KeyReleaseMask;
#endif /* AUTOKEY */

  attrs.background_pixel = colortable[W_Black].pixelValue;

  /* NOTE: CWDontPropagate seems to crash in OpenWindows */
  attrs.do_not_propagate_mask = ResizeRedirectMask | ExposureMask;
  newwin = newWindow(
		      XCreateWindow(W_Display, wparent, x, y,
				    width * W_Textwidth + WIN_EDGE * 2 + scw,
				    MENU_PAD * 2 + height * W_Textheight,
			border, CopyFromParent, InputOutput, CopyFromParent,
				    CWBackPixel | CWEventMask |
				    CWBorderPixel /* | CWDontPropagate */ ,
				    &attrs), WIN_SCROLL);
  class_hint.res_name = name;
  sz_hints = XAllocSizeHints();
  sz_hints->width_inc = W_Textwidth;
  sz_hints->height_inc = W_Textheight;
  sz_hints->min_width = WIN_EDGE * 2 + W_Textwidth + scw;
  sz_hints->min_height = MENU_PAD * 2 + W_Textheight;
  sz_hints->base_width = WIN_EDGE * 2 + scw;
  sz_hints->base_height = MENU_PAD * 2;
  sz_hints->flags = PResizeInc | PMinSize | PBaseSize;
  if (gcheck_result & XValue || gcheck_result & YValue)
    sz_hints->flags |= WMXYHintMode_default();
  XStoreName(W_Display, newwin->window, name);
  XSetWMNormalHints(W_Display, newwin->window, sz_hints);
  XFree((void *) sz_hints);
  XSetClassHint(W_Display, newwin->window, &class_hint);
  XSetWMHints(W_Display, newwin->window, &wm_hint);
  newwin->name = strdup(name);
  sw = (struct scrollingWindow *) malloc(sizeof(struct scrollingWindow));

  sw->lines = 0;
  sw->updated = 0;
  sw->head = sw->tail = sw->index = NULL;
  sw->topline = 0;
  newwin->data = (char *) sw;
  newwin->width = width;
  newwin->height = height;
  if (wparent != W_Root)
    if (checkMapped(name))
      W_MapWindow(W_Window2Void(newwin));

#if DEBUG > 0
  printf("New scroll window %d, child of %d\n", newwin, parent);
#endif

  XSetWindowColormap(W_Display, newwin->window, W_Colormap);
  return W_Window2Void(newwin);
}

/*
 * Add a string to the string list of the scrolling window.
 */
static void
        AddToScrolling(win, color, font, str, len)
struct window *win;
W_Color color;
W_Font  font;
char   *str;
int     len;
{
  struct scrollingWindow *sw;
  struct stringList *new;

  /* simple, fast */

  sw = (struct scrollingWindow *) win->data;
  if (sw->lines > 0 && sw->lines > scroll_lines /* some large number */ )
    {
      /* resuse tail */
      new = sw->tail;
      sw->tail = new->prev;
      new->prev->next = NULL;
      new->prev = NULL;
      new->next = sw->head;
      sw->head->prev = new;
      sw->head = new;
    }
  else
    {
      new = (struct stringList *) malloc(sizeof(struct stringList));

      new->next = sw->head;
      new->prev = NULL;
      if (sw->head)
	sw->head->prev = new;
      sw->head = new;
      if (!sw->tail)
	sw->tail = new;
      sw->lines++;
      /* 
       * printf("adding one line \"%s\" C:%d F:%d.\n", str, color,
       * fontNum(font)); */
    }
  sw->index = sw->head;				 /* input forces to end of *
						  * list */
  sw->topline = 0;

  sw->updated++;				 /* mark for *
						  * W_FlushScrollingWindow */

  STRNCPY(new->string, str, MAX_TEXT_WIDTH - 1);
  new->color = color;
  new->font = font;

  if (len >= MAX_TEXT_WIDTH)
    {
      new->string[MAX_TEXT_WIDTH - 1] = 0;
    }
  else
    {
      /* we pad out the string with spaces so we don't have to clear the *
       * window */
      memset(&new->string[len], ' ', MAX_TEXT_WIDTH - len - 1);
      new->string[MAX_TEXT_WIDTH - 1] = 0;
    }
}

void
        W_FlushScrollingWindow(window)

W_Window window;
{
  struct window *win = W_Void2Window(window);
  struct scrollingWindow *sw;

  if (!win->mapped)
    return;
  if (win->type != WIN_SCROLL)
    {
      fprintf(stderr, "bad window type in W_FlushScrollingWindow.\n");
      return;
    }
  sw = (struct scrollingWindow *) win->data;
  if (!sw->updated)
    return;

#ifndef NO_COPYAREA
  else
    {
      struct stringList *item;
      int y;

      if (win->height > sw->updated)
	{
	  XCopyArea(W_Display, win->window, win->window,
		    colortable[W_White].contexts[0],
		    WIN_EDGE, MENU_PAD + sw->updated * W_Textheight,
		    win->width * W_Textwidth,
		    (win->height - sw->updated) * W_Textheight,
		    WIN_EDGE, MENU_PAD);
	}


      y = (win->height - 1) * W_Textheight + fonts[1].baseline;

      for (item = sw->index; item && y > 0 && sw->updated; item = item->next,
	   y -= W_Textheight,
	   sw->updated--)
	{
	  XDrawImageString(W_Display, win->window,
		      colortable[item->color].contexts[fontNum(item->font)],
			   WIN_EDGE, MENU_PAD + y, item->string,
			   win->width);
	}
      sw->updated = 0;
      if (scrollbar)
	drawThumb(win, sw);
    }
#else
  redrawScrolling(win);
#endif
}

static void
        drawThumb(win, sw)

struct window *win;
struct scrollingWindow *sw;
{
  int x, y, h;
  int savedlines, maxrow, thumbTop, thumbHeight, totalHeight, winheight;

/*
 * savedlines : Number of offscreen text lines,
 * sw->lines - win->height
 * 
 * maxrow + 1 : Number of onscreen  text lines,
 * 
 * min(sw->lines + 1, win->height+1)
 * 
 * sw->topline    : -Number of lines above the last max_row+1 lines
 * 
 * thumbTop    = screen->topline + screen->savedlines;
 * thumbHeight = screen->max_row + 1;
 * totalHeight = thumbHeight + screen->savedlines;
 * 
 * XawScrollbarSetThumb(scrollWidget,
 * ((float)thumbTop) / totalHeight,
 * ((float)thumbHeight) / totalHeight);
 */

  savedlines = sw->lines - win->height;
  if (savedlines < 0)
    savedlines = 0;
  maxrow = sw->lines < win->height ? sw->lines : win->height;
  winheight = win->height * W_Textheight + MENU_PAD * 2;

  thumbTop = sw->topline + savedlines;
  thumbHeight = maxrow + 1;
  totalHeight = thumbHeight + savedlines;

  x = win->width * W_Textwidth + WIN_EDGE * 2;
  y = winheight * thumbTop / totalHeight;
  h = winheight * thumbHeight / totalHeight;

  XClearArea(W_Display, win->window, x, 0, scroll_thumb_width, winheight,
	     False);
  XFillRectangle(W_Display, win->window, scroll_thumb_gc,
		 x, y,
		 scroll_thumb_width, h);
  XDrawLine(W_Display, win->window, colortable[W_Red].contexts[0],
	    x, 0, x, winheight);
}

static void
        redrawScrolling(win)
struct window *win;
{
  int     y;
  struct scrollingWindow *sw;
  register struct stringList *item;

  if (!win->mapped)
    return;

  /* simple, fast */

  sw = (struct scrollingWindow *) win->data;
  if (!sw->lines)
    return;
  sw->updated = 0;

  y = (win->height - 1) * W_Textheight + fonts[1].baseline;
  for (item = sw->index; item && y > 0; item = item->next, y -= W_Textheight)
    {
      XDrawImageString(W_Display, win->window,
		       colortable[item->color].contexts[fontNum(item->font)],
		       WIN_EDGE, MENU_PAD + y, item->string,
		       win->width);
    }
  if (scrollbar)
    drawThumb(win, sw);
}

#ifdef SHORT_PACKETS
void W_SetSensitive(W_Window w, int v)
{
  struct window *win = W_Void2Window(w);

  win->insensitive = !v;

  if (win->type == WIN_SCROLL)
    redrawScrolling(win);
}
#endif

W_Window
W_MakeMenu(char *name, int x, int y, int width, int height, W_Window parent, int border)
{
  struct window *newwin;
  struct menuItem *items;
  Window  wparent;
  int     i;
  XSetWindowAttributes attrs;

#if DEBUG > 0
  printf("New window...\n");
#endif

  checkGeometry(name, &x, &y, &width, &height);
  checkParent(name, &parent);
  wparent = W_Void2Window(parent)->window;
  attrs.border_pixel = colortable[W_White].pixelValue;
  attrs.event_mask = KeyPressMask | ButtonPressMask | ExposureMask;

#ifdef AUTOKEY
  attrs.event_mask |= KeyReleaseMask;
#endif /* AUTOKEY */

  attrs.background_pixel = colortable[W_Black].pixelValue;
  attrs.do_not_propagate_mask = KeyPressMask | ButtonPressMask | ExposureMask;
  newwin = newWindow(
		      XCreateWindow(W_Display, wparent, x, y,
				    width * W_Textwidth + WIN_EDGE * 2,
   height * (W_Textheight + MENU_PAD * 2) + (height - 1) * MENU_BAR, border,
				CopyFromParent, InputOutput, CopyFromParent,
				    CWBackPixel | CWEventMask |
				    CWBorderPixel,
				    &attrs),
		      WIN_MENU);
  class_hint.res_name = name;
  XSetClassHint(W_Display, newwin->window, &class_hint);
  XSetWMHints(W_Display, newwin->window, &wm_hint);
  XStoreName(W_Display, newwin->window, name);
  newwin->name = strdup(name);
  items = (struct menuItem *) malloc(height * sizeof(struct menuItem));

  for (i = 0; i < height; i++)
    {
      /* new: allocate once and reuse -tsh */
      items[i].column = 0;
      items[i].string = (char *) malloc(MAX_TEXT_WIDTH);
      items[i].color = W_White;
    }
  newwin->data = (char *) items;
  newwin->width = width;
  newwin->height = height;
  if (wparent != W_Root)
    if (checkMapped(name))
      W_MapWindow(W_Window2Void(newwin));

#if DEBUG > 0
  printf("New menu window %d, child of %d\n", newwin, parent);
#endif

  XSetWindowColormap(W_Display, newwin->window, W_Colormap);
  return W_Window2Void(newwin);
}

void redrawMenu(struct window * win)
{
  int     count;

  for (count = 1; count < win->height; count++)
    {
      XFillRectangle(W_Display, win->window,
		     colortable[W_Grey].contexts[0],
	  0, count * (W_Textheight + MENU_PAD * 2) + (count - 1) * MENU_BAR,
		     win->width * W_Textwidth + WIN_EDGE * 2, MENU_BAR);
    }
  for (count = 0; count < win->height; count++)
    {
      redrawMenuItem(win, count);
    }
}

void redrawMenuItem(struct window *win, int n)
{
  struct menuItem *items;

  items = (struct menuItem *) win->data;
  XFillRectangle(W_Display, win->window,
		 colortable[W_Black].contexts[0],
	  WIN_EDGE, n * (W_Textheight + MENU_PAD * 2 + MENU_BAR) + MENU_PAD,
		 win->width * W_Textwidth, W_Textheight);
  if (items[n].string)
    {
      XDrawImageString(W_Display, win->window,
		       colortable[items[n].color].contexts[1],
		       WIN_EDGE + W_Textwidth * items[n].column,
		       n * (W_Textheight + MENU_PAD * 2 + MENU_BAR) + MENU_PAD + fonts[1].baseline,
		       items[n].string, strlen(items[n].string));
    }
}

static void changeMenuItem(struct window *win, int col, int n, char *str, W_Color color)
{
  struct menuItem *items;

  items = (struct menuItem *) win->data;

  STRNCPY(items[n].string, str, MAX_TEXT_WIDTH - 1);
  items[n].string[MAX_TEXT_WIDTH - 1] = 0;
  items[n].color = color;
  items[n].column = col;
  redrawMenuItem(win, n);
}

void
        W_DefineMapcursor(W_Window window)
{
  Cursor  new;
  Pixmap  mapcursmaskbit;
  Pixmap  mapcursbit;
  struct window *win = W_Void2Window(window);
  char   *path;
  static
  XColor  f, b;
  int     w, h, xh, yh;

  xh = yh = 5;
  f.pixel = colortable[W_White].pixelValue;
  b.pixel = colortable[W_Black].pixelValue;

  XQueryColor(W_Display, W_Colormap, &f);
  XQueryColor(W_Display, W_Colormap, &b);

  mapcursbit = XCreateBitmapFromData(W_Display, win->window, mapcursor_bits,
				     mapcursor_width, mapcursor_height);

  if ((path = getdefault("mapCursorDef")))
    {

      if (W_LoadBitmap(window, path, &mapcursmaskbit, &w, &h, &xh, &yh) != 1)
	{
	  mapcursmaskbit = XCreateBitmapFromData(W_Display, win->window,
			       mapmask_bits, mapmask_width, mapmask_height);
	  xh = yh = 5;
	}
      else
	{
	  mapcursbit = XCreatePixmap(W_Display, win->window, w, h, 1);
	  XFillRectangle(W_Display, mapcursbit,
			 colortable[W_White].contexts[0], 0, 0, w, h);
	}
    }

  else
    mapcursmaskbit = XCreateBitmapFromData(W_Display, win->window,
			       mapmask_bits, mapmask_width, mapmask_height);

  if (win->cursor)
    XFreeCursor(W_Display, win->cursor);

  new = XCreatePixmapCursor(W_Display, mapcursbit, mapcursmaskbit,
			    &b, &f, xh, yh);
  XRecolorCursor(W_Display, new, &b, &f);
  XDefineCursor(W_Display, win->window, new);
  win->cursor = new;
}

void
        W_DefineLocalcursor(W_Window window)
{
  Cursor  new;
  Pixmap  localcursmaskbit;
  Pixmap  localcursbit;
  struct window *win = W_Void2Window(window);
  char   *path;
  static
  XColor  f, b;
  int     w, h, xh, yh;

  xh = yh = 5;
  f.pixel = colortable[W_White].pixelValue;
  b.pixel = colortable[W_Black].pixelValue;

  XQueryColor(W_Display, W_Colormap, &f);
  XQueryColor(W_Display, W_Colormap, &b);

  localcursbit = XCreateBitmapFromData(W_Display, win->window,
		   localcursor_bits, localcursor_width, localcursor_height);

  if ((path = getdefault("localCursorDef")))
    {
      if (W_LoadBitmap(window, path, &localcursmaskbit, &w, &h, &xh, &yh) != 1)
	{
	  localcursmaskbit = XCreateBitmapFromData(W_Display, win->window,
			 localmask_bits, localmask_width, localmask_height);
	  xh = yh = 5;
	}
      else
	{
	  localcursbit = XCreatePixmap(W_Display, win->window, w, h, 1);
	  XFillRectangle(W_Display, localcursbit,
			 colortable[W_White].contexts[0], 0, 0, w, h);
	}
    }
  else
    localcursmaskbit = XCreateBitmapFromData(W_Display, win->window,
			 localmask_bits, localmask_width, localmask_height);

  if (win->cursor)
    XFreeCursor(W_Display, win->cursor);

  new = XCreatePixmapCursor(W_Display, localcursbit, localcursmaskbit,
			    &b, &f, xh, yh);
  XRecolorCursor(W_Display, new, &b, &f);
  XDefineCursor(W_Display, win->window, new);
  win->cursor = new;
}

void
        W_DefineFedCursor(W_Window window)
{
  Cursor  new;
  Pixmap  fedcursmaskbit;
  Pixmap  fedcursbit;
  struct window *win = W_Void2Window(window);

  static
  XColor  f, b;

  f.pixel = colortable[W_White].pixelValue;
  b.pixel = colortable[W_Black].pixelValue;

  XQueryColor(W_Display, W_Colormap, &f);
  XQueryColor(W_Display, W_Colormap, &b);

  fedcursbit = XCreateBitmapFromData(W_Display, win->window, fed_cruiser_bits,
				     fed_cruiser_width, fed_cruiser_height);
  fedcursmaskbit = XCreateBitmapFromData(W_Display, win->window,
			    fed_mask_bits, fed_mask_width, fed_mask_height);
  if (win->cursor)
    XFreeCursor(W_Display, win->cursor);

  new = XCreatePixmapCursor(W_Display, fedcursmaskbit, fedcursbit,
			    &b, &f, 10, 10);
  XRecolorCursor(W_Display, new, &b, &f);
  XDefineCursor(W_Display, win->window, new);
  win->cursor = new;
}

void
        W_DefineRomCursor(W_Window window)
{
  Cursor  new;
  Pixmap  romcursmaskbit;
  Pixmap  romcursbit;
  struct window *win = W_Void2Window(window);

  static
  XColor  f, b;

  f.pixel = colortable[W_White].pixelValue;
  b.pixel = colortable[W_Black].pixelValue;

  XQueryColor(W_Display, W_Colormap, &f);
  XQueryColor(W_Display, W_Colormap, &b);

  romcursbit = XCreateBitmapFromData(W_Display, win->window, rom_cruiser_bits,
				     rom_cruiser_width, rom_cruiser_height);
  romcursmaskbit = XCreateBitmapFromData(W_Display, win->window,
		      rom_mask_bits, rom_cruiser_width, rom_cruiser_height);
  if (win->cursor)
    XFreeCursor(W_Display, win->cursor);

  new = XCreatePixmapCursor(W_Display, romcursmaskbit, romcursbit,
			    &b, &f, 10, 10);
  XRecolorCursor(W_Display, new, &b, &f);
  XDefineCursor(W_Display, win->window, new);
  win->cursor = new;
}

void
        W_DefineKliCursor(W_Window window)
{
  Cursor  new;
  Pixmap  klicursmaskbit;
  Pixmap  klicursbit;
  struct window *win = W_Void2Window(window);

  static
  XColor  f, b;

  f.pixel = colortable[W_White].pixelValue;
  b.pixel = colortable[W_Black].pixelValue;

  XQueryColor(W_Display, W_Colormap, &f);
  XQueryColor(W_Display, W_Colormap, &b);

  klicursbit = XCreateBitmapFromData(W_Display, win->window, kli_cruiser_bits,
				     kli_cruiser_width, kli_cruiser_height);
  klicursmaskbit = XCreateBitmapFromData(W_Display, win->window,
		      fed_mask_bits, kli_cruiser_width, kli_cruiser_height);
  if (win->cursor)
    XFreeCursor(W_Display, win->cursor);

  new = XCreatePixmapCursor(W_Display, klicursmaskbit, klicursbit,
			    &b, &f, 10, 10);
  XRecolorCursor(W_Display, new, &b, &f);
  XDefineCursor(W_Display, win->window, new);
  win->cursor = new;
}

void
        W_DefineOriCursor(W_Window window)
{
  Cursor  new;
  Pixmap  oricursmaskbit;
  Pixmap  oricursbit;
  struct window *win = W_Void2Window(window);

  static
  XColor  f, b;

  f.pixel = colortable[W_White].pixelValue;
  b.pixel = colortable[W_Black].pixelValue;

  XQueryColor(W_Display, W_Colormap, &f);
  XQueryColor(W_Display, W_Colormap, &b);

  oricursbit = XCreateBitmapFromData(W_Display, win->window, ori_cruiser_bits,
				     ori_cruiser_width, ori_cruiser_height);
  oricursmaskbit = XCreateBitmapFromData(W_Display, win->window,
		      fed_mask_bits, ori_cruiser_width, ori_cruiser_height);
  if (win->cursor)
    XFreeCursor(W_Display, win->cursor);

  new = XCreatePixmapCursor(W_Display, oricursmaskbit, oricursbit,
			    &b, &f, 10, 10);
  XRecolorCursor(W_Display, new, &b, &f);
  XDefineCursor(W_Display, win->window, new);
  win->cursor = new;
}

void
        W_DefineTrekCursor(W_Window window)
{
  Cursor  new;
  struct window *win = W_Void2Window(window);
  XColor  f, b;
  char   *path;
  int     w, h, xh, yh, mw, mh, mxh, myh;

  f.pixel = colortable[W_Yellow].pixelValue;
  b.pixel = colortable[W_Black].pixelValue;

  XQueryColor(W_Display, W_Colormap, &f);
  XQueryColor(W_Display, W_Colormap, &b);

  if (win->cursor)
    XFreeCursor(W_Display, win->cursor);

  if ((path = getdefault("infoCursorDef")))
    {
      Pixmap  pm, mpm;

      if (W_LoadBitmap(window, path, &pm, &w, &h, &xh, &yh) != 1)
	new = XCreateFontCursor(W_Display, XC_trek);
      else
	{
	  char    mask_path[512];

	  strcpy(mask_path, path);
	  strcat(mask_path, ".mask");

	  if (W_LoadBitmap(window, mask_path, &mpm, &mw, &mh, &mxh, &myh) != 1)
	    {
	      mw = w;
	      mh = h;
	      mpm = XCreatePixmap(W_Display, win->window, w, h, 1);
	      XFillRectangle(W_Display, mpm,
			     colortable[W_White].contexts[0], 0, 0, w, h);
	    }

	  if ((w != mw) || (h != mh))
	    {
	      printf("Cursor and mask are not the same size. %s\n", path);
	      new = XCreateFontCursor(W_Display, XC_trek);
	    }
	  else
	    new = XCreatePixmapCursor(W_Display, pm, mpm,
				      &b, &f, xh, yh);
	}
    }
  else
    new = XCreateFontCursor(W_Display, XC_trek);

  XRecolorCursor(W_Display, new, &f, &b);
  XDefineCursor(W_Display, win->window, new);
  win->cursor = new;
}

void
        W_DefineWarningCursor(W_Window window)
{
  Cursor  new;
  struct window *win = W_Void2Window(window);
  XColor  f, b;

  f.pixel = colortable[W_Red].pixelValue;
  b.pixel = colortable[W_Black].pixelValue;

  XQueryColor(W_Display, W_Colormap, &f);
  XQueryColor(W_Display, W_Colormap, &b);

  if (win->cursor)
    XFreeCursor(W_Display, win->cursor);

  new = XCreateFontCursor(W_Display, XC_pirate);
  XRecolorCursor(W_Display, new, &f, &b);
  XDefineCursor(W_Display, win->window, new);
  win->cursor = new;
}

void
        W_DefineArrowCursor(W_Window window)
{
  Cursor  new;
  struct window *win = W_Void2Window(window);
  XColor  f, b;
  char   *path;
  int     w, h, xh, yh, mw, mh, mxh, myh;

  f.pixel = colortable[W_Black].pixelValue;
  b.pixel = colortable[W_White].pixelValue;

  XQueryColor(W_Display, W_Colormap, &f);
  XQueryColor(W_Display, W_Colormap, &b);

  if (win->cursor)
    XFreeCursor(W_Display, win->cursor);

  if ((path = getdefault("arrowCursorDef")))
    {
      Pixmap  pm, mpm;

      if (W_LoadBitmap(window, path, &pm, &w, &h, &xh, &yh) != 1)
	new = XCreateFontCursor(W_Display, XC_left_ptr);
      else
	{
	  char    mask_path[512];

	  strcpy(mask_path, path);
	  strcat(mask_path, ".mask");

	  if (W_LoadBitmap(window, mask_path, &mpm, &mw, &mh, &mxh, &myh) != 1)
	    {
	      mw = w;
	      mh = h;
	      mpm = XCreatePixmap(W_Display, win->window, w, h, 1);
	      XFillRectangle(W_Display, mpm,
			     colortable[W_White].contexts[0], 0, 0, w, h);
	    }

	  if ((w != mw) || (h != mh))
	    {
	      printf("Cursor and mask are not the same size. %s\n", path);
	      new = XCreateFontCursor(W_Display, XC_left_ptr);
	    }
	  else
	    new = XCreatePixmapCursor(W_Display, pm, mpm,
				      &b, &f, xh, yh);
	}
    }
  else
    new = XCreateFontCursor(W_Display, XC_left_ptr);

  XRecolorCursor(W_Display, new, &f, &b);
  XDefineCursor(W_Display, win->window, new);
  win->cursor = new;
}

void
        W_DefineTextCursor(W_Window window)
{
  Cursor  new;
  struct window *win = W_Void2Window(window);
  XColor  f, b;
  char   *path;
  int     w, h, xh, yh, mw, mh, mxh, myh;

  f.pixel = colortable[W_Yellow].pixelValue;
  b.pixel = colortable[W_Black].pixelValue;

  XQueryColor(W_Display, W_Colormap, &f);
  XQueryColor(W_Display, W_Colormap, &b);

  if (win->cursor)
    XFreeCursor(W_Display, win->cursor);

  if ((path = getdefault("textCursorDef")))
    {
      Pixmap  pm, mpm;

      if (W_LoadBitmap(window, path, &pm, &w, &h, &xh, &yh) != 1)
	new = XCreateFontCursor(W_Display, XC_xterm);
      else
	{
	  char    mask_path[512];

	  strcpy(mask_path, path);
	  strcat(mask_path, ".mask");

	  if (W_LoadBitmap(window, mask_path, &mpm, &mw, &mh, &mxh, &myh) != 1)
	    {
	      mw = w;
	      mh = h;
	      mpm = XCreatePixmap(W_Display, win->window, w, h, 1);
	      XFillRectangle(W_Display, mpm,
			     colortable[W_White].contexts[0], 0, 0, w, h);
	    }

	  if ((w != mw) || (h != mh))
	    {
	      printf("Cursor and mask are not the same size. %s\n", path);
	      new = XCreateFontCursor(W_Display, XC_xterm);
	    }
	  else
	    new = XCreatePixmapCursor(W_Display, pm, mpm,
				      &b, &f, xh, yh);
	}
    }
  else
    new = XCreateFontCursor(W_Display, XC_xterm);

  XRecolorCursor(W_Display, new, &f, &b);
  XDefineCursor(W_Display, win->window, new);
  win->cursor = new;
}

void
        W_DefineCursor(W_Window window, int width, int height, char *bits, char *mask, int xhot, int yhot)
{
  static char *oldbits = NULL;
  static Cursor curs;
  Pixmap  cursbits;
  Pixmap  cursmask;
  struct window *win;
  XColor  whiteCol, blackCol;

#if DEBUG > 0
  printf("Defining cursor for %d\n", window);
#endif

  win = W_Void2Window(window);
  whiteCol.pixel = colortable[W_White].pixelValue;
  XQueryColor(W_Display, W_Colormap, &whiteCol);
  blackCol.pixel = colortable[W_Black].pixelValue;
  XQueryColor(W_Display, W_Colormap, &blackCol);
  if (!oldbits || oldbits != bits)
    {
      cursbits = XCreateBitmapFromData(W_Display, win->window,
				       bits, width, height);
      cursmask = XCreateBitmapFromData(W_Display, win->window,
				       mask, width, height);
      oldbits = bits;
      curs = XCreatePixmapCursor(W_Display, cursbits, cursmask,
				 &whiteCol, &blackCol, xhot, yhot);
      XFreePixmap(W_Display, cursbits);
      XFreePixmap(W_Display, cursmask);
    }
  XDefineCursor(W_Display, win->window, curs);
}

int
        W_LoadBitmap(W_Window window, char *path, Pixmap * pixmap, int *width, int *height, int *x_hot, int *y_hot)
{
  int     status;
  struct window *win;

  win = W_Void2Window(window);

  status = XReadBitmapFile(W_Display, win->window, path, (unsigned int *) width,
			   (unsigned int *) height, pixmap, x_hot, y_hot);

  if (status == BitmapSuccess)
    {
      if (*x_hot < 0)
	{
	  *x_hot = *width / 2;
	  *y_hot = *height / 2;
	}

      return 1;
    }
  else
    return 0;
}

void
        W_Beep(void)
{
  XBell(W_Display, 0);
}

int
        W_WindowWidth(W_Window window)
{
  return W_Void2Window(window)->width;
}

int
        W_WindowHeight(W_Window window)
{
  return W_Void2Window(window)->height;
}

int
        W_Socket(void)
{
  return ConnectionNumber(W_Display);
}

void
        W_DestroyWindow(W_Window window)
{
  struct window *win;

#if DEBUG > 0
  printf("Destroying %d\n", window);
#endif

  win = W_Void2Window(window);
  deleteWindow(win);
  XDestroyWindow(W_Display, win->window);
  free((char *) win);
}

void deleteWindow(struct window *window)
{
  struct windowlist **rm;
  struct windowlist *temp;

  rm = &hashtable[hash(window->window)];
  while (*rm != NULL && (*rm)->window != window)
    {
      rm = &((*rm)->next);
    }
  if (*rm == NULL)
    {
      printf("Attempt to delete non-existent window!\n");
      return;
    }
  temp = *rm;
  *rm = temp->next;
  free((char *) temp);
}

void
        W_SetIconWindow(W_Window main, W_Window icon)
{
  XWMHints hints;

  XSetIconName(W_Display, W_Void2Window(icon)->window, W_Void2Window(main)->name);

  hints.flags = IconWindowHint;
  hints.icon_window = W_Void2Window(icon)->window;
#ifdef WINDOWMAKER
  hints.window_group = W_Void2Window(main)->window;
  hints.flags |= WindowGroupHint;

  XSetCommand(W_Display, W_Void2Window(main)->window, wm_argv, wm_argc);
#endif

  XSetWMHints(W_Display, W_Void2Window(main)->window, &hints);
}

static void
        scrollUp(win, y)

struct window *win;
int     y;
{
  struct scrollingWindow *sw = (struct scrollingWindow *) win->data;
  int     savedlines = sw->lines - win->height;

  if (savedlines < 0)
    savedlines = 0;

  if (sw->topline + savedlines > 0)
    {
      if (!sw->index)
	{
	  fprintf(stderr, "scroll error, NULL index (scrollUp).\n");
	  return;
	}
      sw->index = sw->index->next;
      sw->topline--;
      redrawScrolling(win);
    }
}

static void
        scrollDown(win, y)

struct window *win;
int     y;
{
  struct scrollingWindow *sw = (struct scrollingWindow *) win->data;

  if (sw->topline < 0)
    {
      if (!sw->index)
	{
	  fprintf(stderr, "scroll error, NULL index (scrollDown).\n");
	  return;
	}
      sw->index = sw->index->prev;
      sw->topline++;
      redrawScrolling(win);
    }
}


static void
        scrollPosition(win, y)

struct window *win;
int     y;
{
  struct scrollingWindow *sw = (struct scrollingWindow *) win->data;
  int     savedlines, maxrow, winheight, newtop;

  savedlines = sw->lines - win->height;
  if (savedlines < 0)
    savedlines = 0;
  maxrow = sw->lines < win->height ? sw->lines : win->height;
  winheight = win->height * W_Textheight + MENU_PAD * 2;

  newtop = (y * (savedlines + maxrow + 1)) / winheight - savedlines;
  if (newtop < -savedlines)
    newtop = -savedlines;
  else if (newtop > 0)
    newtop = 0;
  scrollTo(win, sw, newtop);
}

static void
        scrollTo(win, sw, topline)

struct window *win;
struct scrollingWindow *sw;
int     topline;
{
  while (topline < sw->topline)
    {
      if (!sw->index)
	{
	  fprintf(stderr, "scroll error, NULL index (1).\n");
	  break;
	}
      sw->index = sw->index->next;
      sw->topline--;
    }
  while (topline > sw->topline)
    {
      if (!sw->index)
	{
	  fprintf(stderr, "scroll error, NULL index (2).\n");
	  break;
	}
      sw->index = sw->index->prev;
      sw->topline++;
    }
  redrawScrolling(win);
}

static void
        scrollScrolling(wevent)

W_Event *wevent;
{
  switch (wevent->key)
    {
    case W_RBUTTON:
    case W_WUBUTTON:
      scrollUp(W_Void2Window(wevent->Window), wevent->y);
      break;
    case W_LBUTTON:
    case W_WDBUTTON:
      scrollDown(W_Void2Window(wevent->Window), wevent->y);
      break;
    case W_MBUTTON:
      scrollPosition(W_Void2Window(wevent->Window), wevent->y);
      break;
    default:
      break;
    }
}

static void
        configureScrolling(win, x, y, width, height)

struct window *win;
int     x, y;					 /* TODO */
int     width, height;

{
  int     new_text_width, new_text_height;
  int     new_real_width, new_real_height;
  XWindowAttributes wa;
  int     sw = scrollbar ? scroll_thumb_width : 0;

#if 0
  /* XXX: can't shrink window */

  if (width <= win->width * W_Textwidth + WIN_EDGE * 2 &&
      height <= win->height * W_Textheight + MENU_PAD * 2)
    return;
#endif

  XGetWindowAttributes(W_Display, win->window, &wa);

  new_text_width = (wa.width - WIN_EDGE * 2 - sw) / W_Textwidth;
  new_text_height = (wa.height - MENU_PAD * 2) / W_Textheight;

  if (new_text_width <= 0)
    new_text_width = 1;
  if (new_text_height <= 0)
    new_text_height = 1;
  if (new_text_width >= MAX_TEXT_WIDTH)
    new_text_width = MAX_TEXT_WIDTH - 1;

  new_real_width = new_text_width * W_Textwidth + WIN_EDGE * 2 + sw;
  new_real_height = new_text_height * W_Textheight + MENU_PAD * 2;

  if (new_real_height != wa.height || new_real_width != wa.width)
    {
      XResizeWindow(W_Display, win->window, new_real_width, new_real_height);
    }

  win->width = new_text_width;
  win->height = new_text_height;

  /* an expose event will follow a resize request, triggering  *
   * redrawScrolling */
}

/*****************************************************************************/
/* Looks up any default geometry specified in the defaults file and        */
/* returns the values found there.  Geometry should be of the form         */
/* [=][<width>x<height>][{+-}<xoffset>{+-}<yoffset>]                  */
/* */
/* The result returned indicates which values were set.                    */
/* XValue, YValue, WidthValue, HeightValue                                */
/* */
/*****************************************************************************/

static int
        checkGeometry(char *name, int *x, int *y, int *width, int *height)
{
  char    buf[80], *geom_default;

  sprintf(buf, "%s.geometry", name);
  geom_default = getdefault(buf);
  if (!geom_default)
    return 0;					 /* nothing set */

  return XParseGeometry(geom_default, x, y, (unsigned int *) width, (unsigned int *) height);
}

void checkParent(char *name, W_Window * parent)
{
  char   *adefault;
  char    buf[100];
  int     i;
  struct windowlist *windows;

  sprintf(buf, "%s.parent", name);
  adefault = getdefault(buf);
  if (adefault == NULL)
    return;
  /* parent must be name of other window or "root" */
  if (strcmpi(adefault, "root") == 0)
    {
      *parent = W_Window2Void(&myroot);
      return;
    }
  for (i = 0; i < HASHSIZE; i++)
    {
      windows = hashtable[i];
      while (windows != NULL)
	{
	  if (strcmpi(adefault, windows->window->name) == 0)
	    {
	      *parent = W_Window2Void(windows->window);
	      return;
	    }
	  windows = windows->next;
	}
    }
}

int checkMapped(char *name)
{
  char    buf[100];

  sprintf(buf, "%s.mapped", name);
  return booleanDefault(buf, 0);
}

int checkMappedPref(char *name, int preferred)
{
  char    buf[100];

  sprintf(buf, "%s.mapped", name);
  return booleanDefault(buf, preferred);
}

void
        W_WarpPointer(W_Window window, int x, int y)
{
  static int warped_from_x = 0, warped_from_y = 0;

  if (window == NULL)
    {
      if (W_in_message)
	{
	  XWarpPointer(W_Display, None, W_Root, 0, 0, 0, 0, warped_from_x, warped_from_y);
	  W_in_message = 0;
	}
    }
  else
    {
      findMouse(&warped_from_x, &warped_from_y);
      XWarpPointer(W_Display, None, W_Void2Window(window)->window, 0, 0, 0, 0, 0, 0);
      W_in_message = 1;
    }
}

void findMouse(int *x, int *y)
{
  Window  theRoot, theChild;
  int     wX, wY, rootX, rootY, status;
  unsigned int wButtons;

  status = XQueryPointer(W_Display, W_Root, &theRoot, &theChild, &rootX, &rootY, &wX, &wY, &wButtons);
  if (status == True)
    {
      *x = wX;
      *y = wY;
    }
  else
    {
      *x = 0;
      *y = 0;
    }
}

int
        findMouseInWin(int *x, int *y, W_Window w)
{
  Window  theRoot, theChild;
  int     wX, wY, rootX, rootY, status;
  unsigned int wButtons;
  struct window *win = W_Void2Window(w);
  Window  thisWin = win->window;

  status = XQueryPointer(W_Display, thisWin, &theRoot, &theChild,
			 &rootX, &rootY, &wX, &wY, &wButtons);
  if (status == True)
    {
      /* if it's in the window we specified then the values returned should * 
       * 
       * * be within the with and height of the window */
      if (wX <= win->width && wY <= win->height)
	{
	  *x = wX;
	  *y = wY;
	  return 1;
	}
    }
  *x = 0;
  *y = 0;
  return 0;
}

void W_Flush(void)
{
  XFlush(W_Display);
}

#define MAKE_WINDOW_GETTER(name, part) \
  W_Callback name(W_Window w) \
  { \
    return W_Void2Window(w)->part; \
  }

#define MAKE_WINDOW_SETTER(name, part) \
  void name(W_Window w, W_Callback c) \
  { \
    W_Void2Window(w)->part = c; \
  }

MAKE_WINDOW_GETTER(W_GetWindowKeyDownHandler, handle_keydown)
MAKE_WINDOW_SETTER(W_SetWindowKeyDownHandler, handle_keydown)

MAKE_WINDOW_GETTER(W_GetWindowKeyUpHandler, handle_keyup)
MAKE_WINDOW_SETTER(W_SetWindowKeyUpHandler, handle_keyup)

MAKE_WINDOW_GETTER(W_GetWindowButtonHandler, handle_button)
MAKE_WINDOW_SETTER(W_SetWindowButtonHandler, handle_button)

MAKE_WINDOW_GETTER(W_GetWindowExposeHandler, handle_expose)
MAKE_WINDOW_SETTER(W_SetWindowExposeHandler, handle_expose)

void
        W_ResizeWindow(W_Window window, int neww, int newh)	/* TSH 2/93 */



{
  struct window *w = W_Void2Window(window);
  XSizeHints *sz_hints;

  sz_hints = XAllocSizeHints();
  sz_hints->min_width = neww;
  sz_hints->max_width = neww;
  sz_hints->min_height = newh;
  sz_hints->max_height = newh;
  sz_hints->flags = PMinSize | PMaxSize;
  XSetWMNormalHints(W_Display, w->window, sz_hints);
  XResizeWindow(W_Display, w->window, neww, newh);
}

void
W_ReinitMenu(W_Window window, int neww, int newh)
{
   struct window	*win = W_Void2Window(window);
   struct menuItem	*items;
   int			i;

   items = (struct menuItem *) win->data;
   for(i=0; i< win->height; i++){
      free((char *) items[i].string);
   }
   free ((char *)items);
   items = (struct menuItem *) malloc(newh * sizeof(struct menuItem));
   for(i=0; i< newh; i++){
      items[i].column = 0;
      items[i].string = (char *) malloc(MAX_TEXT_WIDTH);
      items[i].color = W_White;
   }
   win->data = (char *) items;
}

/* this procedure should only be used if the menu is initially defined
   by W_MakeMenu as large as it will get.  If menu may grow, call 
   W_ReinitMenu first */

void
W_ResizeMenu(W_Window window, int neww, int newh) /* TSH 2/93 */
{
   struct window	*w = W_Void2Window(window);

   w->width = neww;
   w->height = newh;

   W_ResizeWindow(window, neww*W_Textwidth+WIN_EDGE*2,
            newh*(W_Textheight+MENU_PAD*2)+(newh-1)*MENU_BAR);
}

void
        W_ResizeTextWindow(W_Window window, int neww, int newh) /* TSH 2/93 */
{
  W_ResizeWindow(window, neww * W_Textwidth + WIN_EDGE * 2,
		 newh * W_Textheight + WIN_EDGE * 2);
}

int
        W_Mono(void)
{
  return (DisplayCells(W_Display, W_Screen) <= 2) || forceMono;
}

int
        W_EventsQueued(void)
{
  return XEventsQueued(W_Display, QueuedAlready);
}

int
        W_EventsQueuedCk(void)
{
  return XEventsQueued(W_Display, QueuedAfterReading);
}

int W_ReadEvents(void)
{
  XEvent  event;
  struct timeval timeout = {0, 0};
  fd_set  readfds;

  FD_ZERO(&readfds);
  FD_SET(ConnectionNumber(W_Display), &readfds);
  if (SELECT(max_fd, &readfds, 0, 0, &timeout) == 1)
    {
      XPeekEvent(W_Display, &event);
      return 1;
    }
  return 0;
}

void W_OverlayBitmap(int x, int y, W_Icon bit, W_Color color)
{
  struct icon *icon = W_Void2Icon(bit);

#if DEBUG > 4
  printf("Overlaying bitmap to %d\n", icon->window);
#endif

  XCopyPlane(W_Display, icon->bitmap, icon->window,
	     colortable[color].contexts[0], 0, 0, icon->width, icon->height,
	     x, y, 1);
}

void    W_SetWindowName(W_Window w, char *name)
{
  struct window *win = W_Void2Window(w);

  XStoreName(W_Display, win->window, name);

  return;
}

#ifdef BEEPLITE
static GC _tts_gc;
static XFontStruct *_tts_fontinfo;
static int _tts_th, _tts_tw;

int
        W_TTSTextHeight(void)
{
  return _tts_th;
}

int
        W_TTSTextWidth(char *s, int l)
{
  return XTextWidth(_tts_fontinfo, s, l);
}


void
        init_tts(void)
{
  char   *fontname;
  XGCValues values;
  char   *color;
  XColor  xc;

  fontname = getdefault("tts_font");
  if (!fontname)
    fontname = TTS_FONT;

#ifdef SHOW_DEFAULTS
  show_defaults("TTS", "tts_font", fontname, "TTS font.");
#endif

#ifdef nodef
  if (forceMono || DisplayCells(W_Display, W_Screen) <= 2)
    {
      /* this is not going to work at all for b/w */
      tts_time = 0;
      F_beeplite_flags &= ~LITE_TTS;
      return;
    }
#endif

  _tts_fontinfo = XLoadQueryFont(W_Display, fontname);
  if (!_tts_fontinfo)
    {
      fprintf(stderr, "netrek: Can't find font \"%s\".\n", fontname);
      _tts_fontinfo = XLoadQueryFont(W_Display, "fixed");
    }
  if (!_tts_fontinfo)
    {
      fprintf(stderr, "netrek: Can't find any fonts.\n");
      terminate(1);
    }
  _tts_th = _tts_fontinfo->max_bounds.descent +
      _tts_fontinfo->max_bounds.ascent;
  _tts_tw = _tts_fontinfo->max_bounds.width;

  values.font = _tts_fontinfo->fid;

  if (forceMono || DisplayCells(W_Display, W_Screen) <= 2)
    {
      values.foreground = colortable[W_White].pixelValue;
      values.function = GXor;
    }

  else
    {

      color = getdefault("tts_color");
      if (!color)
	color = "grey";

#ifdef SHOW_DEFAULTS
      show_defaults("TTS", "tts_color", color, "TTS msg color.");
#endif

      if (!XParseColor(W_Display, W_Colormap, color, &xc))
	{
	  fprintf(stderr, "netrek: Unknown tts_color \"%s\", using #777\n", color);
	  (void) XParseColor(W_Display, W_Colormap, "#777", &xc);
	}
      /* using the 8th color allocated in GetColors() */
      xc.pixel = colortable[W_Black].pixelValue | planes[0] | planes[2];
      if ((takeNearest) || (W_Visual->class != PseudoColor))
	XAllocColor(W_Display, W_Colormap, &xc);
      else
	XStoreColor(W_Display, W_Colormap, &xc);
      values.foreground = xc.pixel;
      values.function = GXor;
    }

  _tts_gc = XCreateGC(W_Display, W_Root, GCFont | GCForeground | GCFunction,
		      &values);

  XSetGraphicsExposures(W_Display, _tts_gc, False);
}

void
        W_EraseTTSText(W_Window window, int max_width, int y, int width)
{
  //  struct window *win = W_Void2Window(window);
  int x = (max_width - width) / 2;

  if (x < 0)
    x = 4;
  y -= W_TTSTextHeight();

  W_ClearArea(window, x, y, width, W_TTSTextHeight());
}

void
        W_WriteTTSText(W_Window window, int max_width, int y, int width, char *str, int len)


/* max_width of window */
/* y coordinate */
/* actual width */
/* string */
/* length of string */
{
  struct window *win = W_Void2Window(window);
  int x = (max_width - width) / 2;

  if (x < 0)
    x = 4;

  y -= _tts_fontinfo->max_bounds.descent;

  /* y -= W_TTSTextHeight(); y += _tts_fontinfo->max_bounds.ascent; */

  XDrawString(W_Display, win->window, _tts_gc, x, y, str, len);
}
#endif

void    W_Halo(int x, int y, W_Color color)
{
  struct window *win = W_Void2Window(mapw);

  if ((color != W_Ind) && (color != W_Grey))
    {
      XSetForeground(W_Display, colortable[color].contexts[0],
		     colortable[color].pixelValue);
      XDrawArc(W_Display, win->window, colortable[color].contexts[0],
	       x - (mplanet_width / 2), y - (mplanet_width / 2),
	       mplanet_width, mplanet_height, 0, 23040);
    }
}

void W_CameraSnap(W_Window window)
{
#ifdef CAMERA
  struct window *win = W_Void2Window(window);
  camera_snap(W_Display, win->window);
#else
  fprintf(stderr, "W_CameraSnap: function not implemented in this build.");
#endif
}

#ifdef FULLSCREEN

/* XFree86 VidMode X extension handling */

#include <X11/extensions/xf86vmode.h>

XF86VidModeModeInfo **video_mode_list;
XF86VidModeModeInfo *video_mode_current, *video_mode_original;
int video_mode_dotclock, video_mode_list_size;

/* restore video mode to known previous mode */
static void video_mode_off()
{
  if (video_mode_current != video_mode_original) {
    int x;
    x = XF86VidModeSwitchToMode(W_Display, W_Screen, video_mode_original);
#if DEBUG > 0
    fprintf(stderr, "video_mode_off: XF86VidModeSwitchToMode: %d\n", x);
#endif
    video_mode_current = video_mode_original;
  }
}

/* check if X server has support for changing modes */
static int video_mode_initialise() {
  int major, minor;
  if (!XF86VidModeQueryVersion(W_Display, &major, &minor)) {
    fprintf(stderr, "video_mode_initialise: XFree86-VidMode X extension absent\n");
    return 0;
  }

  static int done = 0;
  if (done) return 1;
  done++;

  int line;
  XF86VidModeModeLine current;

  /* obtain the current mode line and list of known mode lines */
  XF86VidModeGetModeLine(W_Display, W_Screen, &video_mode_dotclock, &current);
  XF86VidModeGetAllModeLines(W_Display, W_Screen,
                             &video_mode_list_size, &video_mode_list);

  /* find the current mode within the list of known mode lines */
  video_mode_current = NULL;
  for (line=0; line < video_mode_list_size; line++) {
    XF86VidModeModeInfo *mode = video_mode_list[line];
    if (mode->hdisplay == current.hdisplay &&
        mode->vdisplay == current.vdisplay &&
        mode->dotclock == video_mode_dotclock &&
        mode->htotal == current.htotal &&
        mode->vtotal == current.vtotal &&
        mode->flags == current.flags) {
      video_mode_original = video_mode_current = mode;
    }
  }

  /* do not change if the current mode was not found */
  if (video_mode_current == NULL) {
    fprintf(stderr, "video_mode_begin: this mode not found, "
            "cannot switch back, so not switching\n");
    return 0;
  }

  return 1;
}

static void video_mode_on()
{
  int line;

  /* if there is a mode line for 1024x768 then use it */
  for (line=0; line < video_mode_list_size; line++) {
    XF86VidModeModeInfo *mode = video_mode_list[line];
    if (mode->hdisplay == 1024 && mode->vdisplay == 768) {
      int x;
      x = XF86VidModeSwitchToMode(W_Display, W_Screen, mode);
#if DEBUG > 0
      fprintf(stderr, "video_mode_on: XF86VidModeSwitchToMode: %d\n", x);
#endif
      /*! @bug: if this is done on a non-local display, the X error
      XF86VidModeClientNotLocal occurs. */
      video_mode_current = mode;
      return;
    }
  }
}

static void view_port_warp(W_Window window)
{
  struct window *win = W_Void2Window(window);

  /* force the video view port to cover the window */
  int tx = 0, ty = 0;
  Window child;
  XTranslateCoordinates(W_Display, win->window, W_Root, 0, 0, &tx, &ty, &child);
  XF86VidModeSetViewPort(W_Display, W_Screen, tx, ty);
  XMapRaised(W_Display, win->window);
  XRaiseWindow(W_Display, win->window);
}

/* force the cursor to stay within the window */
static void pointer_grab_on(W_Window window)
{
  struct window *win = W_Void2Window(window);

  XGrabPointer(W_Display, win->window, True, ButtonPressMask |
                 ButtonReleaseMask | EnterWindowMask | LeaveWindowMask |
                 PointerMotionMask | PointerMotionHintMask |
                 Button1MotionMask | Button2MotionMask |
                 Button3MotionMask | Button4MotionMask |
                 Button5MotionMask | ButtonMotionMask |
                 KeymapStateMask, GrabModeAsync, GrabModeAsync,
                 win->window, None, CurrentTime);
  XGrabKeyboard(W_Display, win->window, True, GrabModeAsync,
                GrabModeAsync, CurrentTime);
  XFlush(W_Display);
}

static void pointer_grab_off(W_Window window)
{
  XUngrabPointer(W_Display, CurrentTime);
  XUngrabKeyboard(W_Display, CurrentTime);
}

static void kde_fullscreen_on(W_Window window) {
  struct window *win = W_Void2Window(window);
  Atom WM_HINTS;
  WM_HINTS = XInternAtom(W_Display, "_NET_WM_STATE", True);
  if (WM_HINTS != None) {
    Atom p[1];
    p[0] = XInternAtom(W_Display, "_NET_WM_STATE_FULLSCREEN", True);
    XChangeProperty(W_Display, win->window, WM_HINTS, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)p, 1);
  }
}

static void kde_fullscreen_off(W_Window window) {
  struct window *win = W_Void2Window(window);
  Atom WM_HINTS;
  WM_HINTS = XInternAtom(W_Display, "_NET_WM_STATE", True);
  if (WM_HINTS != None) {
    XDeleteProperty(W_Display, win->window, WM_HINTS);
  }
}

#endif /* FULLSCREEN */

void W_FullScreenOn(W_Window window)
{
  struct window *win = W_Void2Window(window);
#ifdef FULLSCREEN
#if DEBUG > 0
  fprintf(stderr, "W_FullScreenOn\n");
#endif
  XResizeWindow(W_Display, win->window, 1024, 768);
  pointer_grab_on(window);
  video_mode_on();
  view_port_warp(window);
  kde_fullscreen_on(window);
#endif
}

void W_FullScreenOff(W_Window window)
{
#ifdef FULLSCREEN
#if DEBUG > 0
  fprintf(stderr, "W_FullScreenOff\n");
#endif
  pointer_grab_off(window);
  kde_fullscreen_off(window);
  video_mode_off();
#endif
}

void W_FullScreenInitialise() {
#ifdef FULLSCREEN
#if DEBUG > 0
  fprintf(stderr, "W_FullScreenInitialise\n");
#endif
  full_screen_enabled = 0;
  full_screen_default = 0;
  if (booleanDefault("FullScreen", 0)) {
    full_screen_default++;
    if (video_mode_initialise())
      full_screen_enabled++;
  }
#endif
}

int W_FullScreenToggle(W_Window window) {
#ifdef FULLSCREEN
#if DEBUG > 0
  fprintf(stderr, "W_FullScreenToggle\n");
#endif
  if (full_screen_enabled) {
    full_screen_enabled = 0;
    W_FullScreenOff(window);
  } else {
    if (!full_screen_default) {
      if (!video_mode_initialise()) {
        return FULLSCREEN_FAILED;
      }
    }
    full_screen_enabled++;
    W_FullScreenOn(window);
  }
  return FULLSCREEN_OK;
#else
  return FULLSCREEN_NOT_COMPILED;
#endif
}

void W_FullScreenBegin(W_Window window) {
#ifdef FULLSCREEN
#if DEBUG > 0
  fprintf(stderr, "W_FullScreenBegin\n");
#endif
  if (full_screen_enabled) {
    W_FullScreenOn(window);
  }
#endif
}

/* regularly enforce */
void W_FullScreen(W_Window window) {
#ifdef FULLSCREEN
  if (full_screen_enabled) {
    view_port_warp(window);
    pointer_grab_on(window);
  }
#endif
}
