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
 */

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
	int x, y;
#ifdef MOUSE_AS_SHIFT
	int modifier;
#endif
 }
W_Event;

/* mapping for some X11 keys that are not represented by ASCII or Latin-1 */
#define W_Key_Up   1
#define W_Key_Down 2

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
extern int W_BigTextwidth, W_BigTextheight, W_Textwidth, W_Textheight;
extern int W_FastClear;
extern W_Font W_MyPlanetFont, W_FriendlyPlanetFont, W_EnemyPlanetFont;

extern void W_Initialize (char *str);
extern void W_GetPixmaps(W_Window t, W_Window g);
extern void W_RenameWindow(W_Window window, char *str);
extern W_Window W_MakeWindow (char *name, int x, int y, int width, int height, W_Window parent, int border, W_Color color);
extern W_Icon W_StoreBitmap (int width, int height, char *data, W_Window window);
extern W_Window W_MakeTextWindow (char *name, int x, int y, int width, int height, W_Window parent, int border);
extern W_Window W_MakeScrollingWindow (char *name, int x, int y, int width, int height, W_Window parent, int border);
extern void W_FlushScrollingWindow(W_Window window);
extern void W_SetSensitive(W_Window w, int v);
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
void W_DefineMapcursor(W_Window window);
void W_DefineLocalcursor(W_Window window);
void W_DefineFedCursor(W_Window window);
void W_DefineRomCursor(W_Window window);
void W_DefineKliCursor(W_Window window);
void W_DefineOriCursor(W_Window window);
void W_DefineTrekCursor(W_Window window);
void W_DefineWarningCursor(W_Window window);
void W_DefineTextCursor(W_Window window);
extern void W_DefineCursor (W_Window window, int width, int height, char *bits, char *mask, int xhot, int yhot);
extern int W_IsMapped (W_Window window);
void W_ReinitMenu(W_Window window, int neww, int newh);
void W_ResizeMenu(W_Window window, int neww, int newh);
void W_ResizeWindow(W_Window window, int neww, int newh);
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
extern void W_WriteCircle(W_Window, int, int, int, W_Color);
extern void W_WriteTriangle(W_Window, int, int, int, int, W_Color);
extern void W_CacheClearArea(W_Window, int, int, int, int);
extern void W_FlushClearAreaCache(W_Window);
extern void W_FlushLineCaches(W_Window);
extern void W_ResizeTextWindow(W_Window, int, int);
extern int W_Mono(void);
extern int W_EventsQueuedCk(void);
extern void W_OverlayBitmap(int, int, W_Icon, W_Color);
extern void W_WriteTriangle(W_Window, int, int, int, int, W_Color);
extern void W_Flush(void);

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

#define W_EV_CLOSED	6	/* window was closed by user action */

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

void W_FillArea(W_Window window, int x, int y, int width, int height, W_Color color);
void W_DefineArrowCursor(W_Window window);
void W_GalacticBgd(int which);
void W_LocalBgd(int which);

int W_TTSTextHeight(void);
int W_TTSTextWidth(char *s, int l);
void W_EraseTTSText(W_Window window, int max_width, int y, int width);
void W_WriteTTSText(W_Window window, int max_width, int y, int width, char *str, int len);
void W_Halo(int x, int y, W_Color color);

void W_FullScreenOn(W_Window window);
void W_FullScreenOff(W_Window window);
void W_FullScreenInitialise(void);
int W_FullScreenToggle(W_Window window);
#define FULLSCREEN_NOT_COMPILED -1
#define FULLSCREEN_OK 0
#define FULLSCREEN_FAILED 1
void W_FullScreenBegin(W_Window window);
void W_FullScreen(W_Window window);
void W_CameraSnap(W_Window window);
int W_SpNextEvent(W_Event *wevent);
int findMouseInWin(int *x, int *y, W_Window w);
int W_EventsQueued(void);
int W_EventsQueuedCk(void);

void W_SetBackground(W_Window w, int which);
void *W_SetBackgroundImage(W_Window w, char *name);

void W_NextScreenShot(W_Window w, int x, int y);
void W_DrawScreenShot(W_Window w, int x, int y);

void *W_ReadImage(W_Window w, char *name);
void W_DrawImage(int x, int y, void *sprite_v);
void W_DropImage(void *sprite_v);

#endif /* _h_Wlib */
