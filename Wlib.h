/* Wlib.h

 * Include file for the Windowing interface.
 *
 * Kevin P. Smith  6/11/89
 *
 * The deal is this:
 *   Call W_Initialize(), and then you may call any of the listed fuinctions.
 *   Also, externals you are allowed to pass or use include W_BigFont,
 *     W_RegularFont, W_UnderlineFont, W_HighlightFont, W_White, W_Black,
 *     W_Red, W_Green, W_Yellow, W_Cyan, W_Grey, W_Textwidth, and W_Textheight.
 *
 * $Log: Wlib.h,v $
 * Revision 1.3  2006/05/22 13:12:19  quozl
 * fix compilation warnings
 *
 * Revision 1.2  2001/08/21 20:52:15  siegl
 *
 * mouse wheel support
 *
 * Revision 1.1.1.1  1998/11/01 17:24:08  siegl
 * COW 3.0 initial revision
 * */

#ifndef _h_Wlib
#define _h_Wlib


#include "copyright2.h"
#include "defs.h"

typedef void (*W_Callback) ();

typedef char *W_Window;

typedef struct event
 {
	int type;
	W_Window Window;
	unsigned char key;
	int x,y;
#ifdef MOUSE_AS_SHIFT
	int modifier;
#endif
 }
W_Event;

extern W_Callback W_GetWindowKeyDownHandler (W_Window w);
extern W_Callback W_GetWindowKeyUpHandler (W_Window w);
extern W_Callback W_GetWindowButtonHandler (W_Window w);
extern W_Callback W_GetWindowExposeHandler (W_Window w);
extern void W_SetWindowKeyDownHandler (W_Window w, W_Callback c);
extern void W_SetWindowKeyUpHandler (W_Window w, W_Callback c);
extern void W_SetWindowButtonHandler (W_Window w, W_Callback c);
extern void W_SetWindowExposeHandler (W_Window w, W_Callback c);

typedef char *W_Icon;
typedef char *W_Font;
typedef int W_Color;

extern W_Font W_BigFont, W_RegularFont, W_UnderlineFont, W_HighlightFont,
  W_IndyFont;
extern W_Color W_White, W_Black, W_Red, W_Green, W_Yellow, W_Cyan, W_Grey;
#ifdef RACE_COLORS
extern W_Color W_Ind, W_Fed, W_Rom, W_Kli, W_Ori;
#endif
extern int W_Textwidth, W_Textheight;
extern int W_FastClear;
extern W_Font W_MyPlanetFont, W_FriendlyPlanetFont, W_EnemyPlanetFont;

extern void W_Initialize (char *str);
extern W_Window W_MakeWindow (char *name, int x, int y, int width, int height, W_Window parent, int border, W_Color color);
extern W_Icon W_StoreBitmap (int width, int height, char *data, W_Window window);
extern W_Window W_MakeTextWindow (char *name, int x, int y, int width, int height, W_Window parent, int border);
extern W_Window W_MakeScrollingWindow (char *name, int x, int y, int width, int height, W_Window parent, int border);
extern W_Window W_MakeMenu (char *name, int x, int y, int width, int height, W_Window parent, int border);
extern void W_WriteText (W_Window window, int x, int y, W_Color color, char *str, int len, W_Font font);
extern void W_MaskText (W_Window window, int x, int y, W_Color color, char *str, int len, W_Font font);
extern void W_WriteBitmap (int x, int y, W_Icon bit, W_Color color);
extern void W_ClearArea (W_Window window, int x, int y, int width, int height);
extern void W_MakeLine (W_Window window, int x0, int y0, int x1, int y1, W_Color color);
extern void W_MapWindow (W_Window window);
extern void W_UnmapWindow (W_Window window);
extern int W_EventsPending (void);
extern void W_NextEvent (W_Event *wevent);
extern void W_SetWindowName(W_Window window, char *name);
extern void W_TileWindow (W_Window window, W_Icon bit);
extern void W_UnTileWindow (W_Window window);
extern void W_ChangeBorder (W_Window window, int color);
extern void W_DefineCursor (W_Window window, int width, int height, char *bits, char *mask, int xhot, int yhot);
extern int W_IsMapped (W_Window window);
extern void W_Beep (void);
extern void W_DestroyWindow (W_Window window);
extern int W_WindowWidth (W_Window window);
extern int W_WindowHeight (W_Window window);
extern int W_Socket (void);
extern void W_ClearWindow (W_Window window);
extern void W_SetIconWindow (W_Window main, W_Window icon);
extern void W_CacheLine(W_Window, int, int, int, int, int);
extern void W_MakeTractLine(W_Window, int, int, int, int, W_Color);
extern void W_MakePhaserLine(W_Window, int, int, int, int, W_Color);
extern void W_WriteTriangle(W_Window, int, int, int, int, W_Color);
extern void W_CacheClearArea(W_Window, int, int, int, int);
extern void W_FlushClearAreaCache(W_Window);
extern void W_FlushLineCaches(W_Window);
extern void W_OverlayBitmap(int, int, W_Icon, W_Color);
extern void W_WriteTriangle(W_Window, int, int, int, int, W_Color);

#define W_EV_EXPOSE	1
#define W_EV_KEY	2
#define W_EV_BUTTON	3

#ifdef MOUSE_AS_SHIFT
#define W_EV_MKEY	4
#endif

#ifdef AUTOKEY
#define W_EV_KEY_OFF	4
#endif /*AUTOKEY */

#ifdef MOTION_MOUSE
#define W_EV_CM_BUTTON	5
#endif

#define W_LBUTTON       1
#define W_MBUTTON       2
#define W_RBUTTON       3
#define W_WUBUTTON      4    // Mouse wheel up
#define W_WDBUTTON      5    // wheel down
#define W_X1BUTTON      6    // future extra keys
#define W_X2BUTTON      7


#ifdef SHIFTED_MOUSE
#define W_SHIFT_BUTTON	0x08
#define W_CTRL_BUTTON	0x10
#define W_BUTTON_RANGE  0x20
#else
#define W_BUTTON_RANGE  8
#endif

#define W_BoldFont W_HighlightFont



#endif /* _h_Wlib */
