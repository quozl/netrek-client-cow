/* mswindow.c
 * Windowing code for Win32 API (aka Windows NT)
 * Jonathan Shekter Aug 94
 * Updates for COW version, Aug 1995
 * Shawn Collenburg
 * Lots of stuff, Aug 1996
 * COW 3.0 support, based on gnu win32 development system
 * Kurt Siegl May 1998
 *
 * $Log: gnu_win32.c,v $
 * Revision 1.3  2001/09/08 13:27:13  siegl
 * Support for state of the art cygwin
 *
 * Revision 1.2  2001/08/26 10:02:16  siegl
 * Playback fixes
 *
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 * */
#include "copyright2.h"

// #undef DEBUG

#define NO_BOOLEAN   //So defs.h doesn't screw us over

#include "config.h"
#undef min
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "xclrs.h"
#include "teams.bitmap"
#include "mapcursor.bitmap"
#include "localcursor.bitmap"
#include <limits.h>
#include INC_STRINGS

#undef WHITE
#undef BLACK
#undef RED
#undef GREEN
#undef YELLOW
#undef CYAN

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
#define COLORS  12
#else
#define COLORS  7
#endif

#define RaceDefaultOffset (C_ROM - RED)


#define WIN_EDGE 1   //Width or window border edge
#define MENU_PAD 4
#define MENU_BAR 4

#define WIN_GRAPH       1
#define WIN_TEXT        2
#define WIN_MENU        3
#define WIN_SCROLL      4

#define MoveTo(dc, x, y) MoveToEx(dc, x, y, NULL);

// Custom window messages, used for communicating between threads
#define WM_TERMINATE_WAIT WM_USER
#define WM_CROSS_THREAD_DESTROY (WM_USER+1)

#ifndef DEBUG
#define MAX_SCROLLWINDOW_LINES 100
#else
#define MAX_SCROLLWINDOW_LINES 300
#endif
//The max # lines a scrollwindow will have

#define EVENT_Q_SIZE 15
//The number of events our custom queue will hold

struct menuItem
   {
   char *string;
   W_Color color;
   short len;
   };

struct stringList
  {
  char *string;
  W_Color color;
  struct stringList *next;
  };

typedef struct tagWindow
   {
   HWND hwnd;
   short type;
   short border;
   W_Color BorderColor;
   short UsingCreatedCursor;
   HCURSOR cursor;
   short tiled;
   struct Icon *TileIcon;
   short NumItems;
   struct menuItem *items;
   struct stringList *strings;
   short AddedStrings;
   short TextHeight;
   short TextWidth;
   RECT ClipRect;
   W_Callback HandleKeydown;
   W_Callback HandleKeyup;
   W_Callback HandleButton;
   W_Callback HandleExpose;
   }
Window;

static Window myroot;

#define FNHEADER\
   register Window *win;\
   if (!window)\
      return(0);\
   win = (Window *)window

#define FNHEADER_VOID\
   register Window *win;\
   if (!window)\
      return;\
   win = (Window *)window

/******************************* Function prototypes *********************/
void W_Cleanup(void );
void GetColors();
LRESULT PASCAL NetrekWndProc(HWND,UINT,WPARAM,LPARAM);
unsigned char *X11toCursor(unsigned char *bits, int width, int height);
void RedrawMenu(Window *win, HDC hdc);
void RedrawScrolling(Window *win, HDC hdc);
void ChangeMenuItem(Window *win, int n, char *str, int len, W_Color color);
void AddToScrolling(Window *win, W_Color color, char *str, int len);
void DrawBorder(Window *win, HDC hdc);
unsigned char *X11toDIB(unsigned char *bits, int width, int height);
unsigned char *X11toDIBAndMirror(unsigned char *bits, int width, int height,
                                       int outwidth, int outheight);
int checkGeometry(char *name, int *x, int *y, int *width, int *height);
void checkParent(char *name, W_Window *parent);
int checkMapped(char *name);
char *GetExeDir();

inline void ResetSysColors(void);
inline void SetTrekSysColors(void);

/******************************* Globals ****************************/
HINSTANCE MyInstance;
char ExeDir[100];
char HomeDir[100];
char ClassName[] = "NetrekWindow";
char FontFileName[] = "\\netrek.fon";
extern int DefaultsLoaded;
HWND AnyWindow;
DWORD MainThreadID;
#ifdef SUPPORT_WIN32S
extern W_Window console;
#endif

int W_FastClear=0;
W_Font W_BigFont, W_RegularFont;
W_Font W_HighlightFont, W_UnderlineFont;
W_Color W_White=WHITE, W_Black=BLACK, W_Red=RED, W_Green=GREEN;
W_Color W_Yellow=YELLOW, W_Cyan=CYAN, W_Grey=GREY;
#ifdef RACE_COLORS
W_Color W_Ind = C_IND, W_Fed = C_FED, W_Rom = C_ROM, W_Kli = C_KLI, W_Ori = C_ORI;
#endif

int W_Textwidth=6, W_Textheight=10;
int forceMono = 0;

const int SysColorNames[] = { COLOR_BTNFACE, COLOR_BTNTEXT, COLOR_3DFACE,
                              COLOR_3DDKSHADOW, COLOR_3DHILIGHT };
DWORD TrekSysColors[] = { 0, 0xffffff, 0, 0xc0c0c0, 0x808080 };
DWORD SysColors[sizeof(TrekSysColors)];

HDC GlobalMemDC, GlobalMemDC2;
HBITMAP GlobalOldMemDCBitmap, GlobalOldMemDC2Bitmap;
HCURSOR TrekCursor, WarnCursor;

struct
   {
   COLORREF rgb;
   HBRUSH brush;
   HPEN pen;
   HPEN dashedpen;
   }
colortable[COLORS] = {
   { RGB(0xff, 0xff, 0xff), 0, 0, 0}, //White
   { RGB(0x00, 0x00, 0x00), 0, 0, 0}, //Black
   { RGB(0xff, 0x5f, 0x5f), 0, 0, 0}, //Red
   { RGB(0x5f, 0xff, 0x5f), 0, 0, 0}, //Green
   { RGB(0xff, 0xff, 0x5f), 0, 0, 0}, //Yellow
   { RGB(0x5f, 0xff, 0xff), 0, 0, 0}, //Cyan
   { RGB(0xa0, 0xa0, 0xa0), 0, 0, 0}  //Light grey
#ifdef RACE_COLORS
   ,
   { RGB(0xff, 0x5f, 0x5f), 0, 0, 0}, //Rom
   { RGB(0x5f, 0xff, 0x5f), 0, 0, 0}, //Kli
   { RGB(0xff, 0xff, 0x5f), 0, 0, 0}, //Fed
   { RGB(0x5f, 0xff, 0xff), 0, 0, 0}, //Ori
   { RGB(0xa0, 0xa0, 0xa0), 0, 0, 0}  //Ind
#endif   
},
// Used when we're in fixed-16 color mode - color values altered so that the color
// mapper picks the proper color. These are "intense" versions of the standard colors
altcolortable[COLORS] = {
   { RGB(0xff, 0xff, 0xff), 0, 0, 0}, //White
   { RGB(0x00, 0x00, 0x00), 0, 0, 0}, //Black
   { RGB(0xff, 0x00, 0x00), 0, 0, 0}, //Red
   { RGB(0x00, 0xff, 0x00), 0, 0, 0}, //Green
   { RGB(0xff, 0xff, 0x00), 0, 0, 0}, //Yellow
   { RGB(0x00, 0xff, 0xff), 0, 0, 0}, //Cyan
   { RGB(0xa0, 0xa0, 0xa0), 0, 0, 0}  //Light grey
#ifdef RACE_COLORS
   ,
   { RGB(0xff, 0x00, 0x00), 0, 0, 0}, //Rom
   { RGB(0x00, 0xff, 0x00), 0, 0, 0}, //Kli
   { RGB(0xff, 0xff, 0x00), 0, 0, 0}, //Fed
   { RGB(0x00, 0xff, 0xff), 0, 0, 0}, //Ori
   { RGB(0xa0, 0xa0, 0xa0), 0, 0, 0}  //Ind
#endif
};

char *colornames[COLORS] = {
   {"white"},
   {"black"},
   {"red"},
   {"green"},
   {"yellow"},
   {"cyan"},
   {"light grey"}
#ifdef RACE_COLORS
  ,
  {"Rom"},
  {"Kli"},
  {"Fed"},
  {"Ori"},
  {"Ind"}
#endif
};

HPALETTE NetrekPalette = 0;

//A structure that the W_Icon type (really a bitmap, not an icon) points to
struct Icon
   {
   HWND hwnd;              //The window this is going into
   HBITMAP bm;             //The Windows bitmap it has been stored in
   int x,y;                //The source position in this bitmap
   int width, height;      //guess...
   RECT *ClipRectAddr;     //Address of the window clip region
   };

#ifndef SCALE_BITMAPS
#define BIGBITMAP_WIDTH 240
#define BIGBITMAP_HEIGHT 240
#else
static int BIGBITMAP_WIDTH = 240;
static int BIGBITMAP_HEIGHT = 240;
#endif

struct BitmapList
   {
   HBITMAP bm;
   struct BitmapList *next;   //Put these in a LL for later de-allocation
   };

struct BitmapList *CurrentBitmap = NULL;
int bmlCount = 0;


//Table that we will build use for virtual key code conversion. We can't
//just let Windows do it as we want to proces  strange things like
// ^@ or ^# which the standard windows TranslateMessage() / ToAscii()
//will not do. So, we process VK codes instead, which means we have
//to manually map the SHIFT key. We create this mapping by asking for
// the VK + shift state for each ascii char during init.

//Mapping tables for VK codes to ASCII. We can't just let windows do
//it as we want strange combinations like ^@, so we build these maps
//ourselves.
char VKMap[256];
char VKShiftMap[256];

char *GetExeDir()
   {
   return ExeDir;
   }

void W_Initialize(char *display)
   {
   int i;
   TEXTMETRIC tm;
   HBITMAP junk;
   LOGFONT lf;
   char FileName[100];
   WNDCLASS wc;
   static int InitDone = 0;
   char * c;
   HDC hdc;
   char *fontname;
   char defaultFontname[100];

   GetColors();

   if (InitDone)
      return;
   InitDone = -1;

#ifdef DEBUG
   printf("Initializing windowing system\n");
#endif

   MyInstance = GetModuleHandle(NULL);
//Get the executable file directory
   c = ExeDir + GetModuleFileName(MyInstance, ExeDir, sizeof(ExeDir)) -1;
   while (*c != '\\')   //remove filename
      *(c--) = 0;
   *c = 0;  //remove last backslash

   //Register our class
   wc.style         = CS_NOCLOSE | CS_HREDRAW | CS_VREDRAW;  //Don't allow them to close windows 
   wc.lpfnWndProc   = NetrekWndProc;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = sizeof(Window *); //Store Window * in extra space
   wc.hInstance     = MyInstance;
   wc.hIcon         = LoadIcon(MyInstance, "main");
   wc.hCursor       = NULL;                        //We're handling our own cursors
   wc.hbrBackground = NULL;
   wc.lpszMenuName  = NULL;
   wc.lpszClassName = ClassName;

   RegisterClass(&wc);

   //Load our cursors - the ones we use from a resource file, anyway
   TrekCursor = LoadCursor(MyInstance, "trek");
   WarnCursor = LoadCursor(MyInstance, "warn");

   /* Clear the font structure. */
   memset(&lf, 0, sizeof(LOGFONT));
   
   /*
    * In order to try and match the old windows client try and load
    * up the netrek font.
    */
   strcpy(FileName, GetExeDir());
   strcat(FileName, FontFileName);
   if (AddFontResource(FileName) == 0)
      {
      strcpy (defaultFontname, "Netrek");
      }
   else
      {
      strcpy (defaultFontname, "\0");
      }

   /* Negative font height means match character units not cell units */
   lf.lfHeight = -intDefault("fontsize", 10); 
   lf.lfCharSet = ANSI_CHARSET;
   lf.lfOutPrecision = OUT_TT_PRECIS;   
   lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
   lf.lfQuality = DEFAULT_QUALITY;
   lf.lfPitchAndFamily = FF_MODERN | FIXED_PITCH;
   
   /*
    *  If the user has set a font use that
    */
   fontname = getdefault("font");
   if (fontname == NULL)
      strcpy (lf.lfFaceName, defaultFontname);
   else
      strcpy (lf.lfFaceName, fontname);
   lf.lfWeight = FW_REGULAR;
   W_RegularFont = (W_Font) CreateFontIndirect(&lf);
   if (W_RegularFont == NULL)
      {
      printf ("Unable to create regular font.\n");
      }

   /*
    *  If the user has set a font use that
    */
   fontname = getdefault("boldfont");
   if (fontname == NULL)
      strcpy (lf.lfFaceName, defaultFontname);
   else
      strcpy (lf.lfFaceName, fontname);
   lf.lfWeight = FW_BOLD;
   W_HighlightFont = (W_Font) CreateFontIndirect(&lf);
   if (W_HighlightFont == NULL)
      {
      printf ("Unable to create highlight font.\n");
      }

   /*
    *  If the user has set a font use that
    */
   fontname = getdefault("italicfont");
   if (fontname == NULL)
      strcpy (lf.lfFaceName, defaultFontname);
   else
      strcpy (lf.lfFaceName, fontname);
   lf.lfWeight = FW_REGULAR;
   lf.lfUnderline = TRUE;
   W_UnderlineFont = (W_Font) CreateFontIndirect(&lf);
   if (W_UnderlineFont == NULL)
      {
      printf ("Unable to create Underline font.\n");
      }

   /*
    *  If the user has set a font use that
    */
   fontname = getdefault("bigfont");
   if (fontname == NULL)
      strcpy (lf.lfFaceName, "Arial");
   else
      strcpy (lf.lfFaceName, fontname);
   lf.lfUnderline = FALSE;
   lf.lfWeight = FW_MEDIUM;
   lf.lfHeight = 52; // 52
   lf.lfWidth  = 32;
   W_BigFont = (W_Font) CreateFontIndirect(&lf);
   if (W_BigFont == NULL)
      {
      printf ("Unable to create Big font.\n");
      }

   //Set up or memory DCs. We need to get the default bitmap and store it,
   //for later use. The only way we can do this is to select another bitmap
   //into it... So we'll create a junk one, then delete it.
   GlobalMemDC = CreateCompatibleDC(NULL);
   GlobalMemDC2 = CreateCompatibleDC(NULL);
   junk = CreateBitmap(1,1,1,1,NULL);
   GlobalOldMemDCBitmap = (HBITMAP) SelectObject(GlobalMemDC, junk);
   SelectObject(GlobalMemDC, GlobalOldMemDCBitmap);
   GlobalOldMemDC2Bitmap = (HBITMAP) SelectObject(GlobalMemDC2, junk);
   SelectObject(GlobalMemDC2, GlobalOldMemDC2Bitmap);
   DeleteObject(junk);

   /*
    *  Determine font size.
    */
   {
   TEXTMETRIC ftm;
   SelectObject(GlobalMemDC,W_HighlightFont);
   if (GetTextMetrics(GlobalMemDC,&ftm))
      {
      W_Textwidth = ftm.tmAveCharWidth;
      W_Textheight = ftm.tmHeight;
      }

   SelectObject(GlobalMemDC,W_RegularFont);
   if (GetTextMetrics(GlobalMemDC,&ftm))
      {
      if (W_Textwidth < ftm.tmAveCharWidth)
         W_Textwidth = ftm.tmAveCharWidth;
      if (W_Textheight < ftm.tmHeight)
         W_Textheight = ftm.tmHeight;
      }

   SelectObject(GlobalMemDC,W_UnderlineFont);
   if (GetTextMetrics(GlobalMemDC,&ftm))
      {
      if (W_Textwidth < ftm.tmAveCharWidth)
         W_Textwidth = ftm.tmAveCharWidth;
      if (W_Textheight < ftm.tmHeight)
         W_Textheight = ftm.tmHeight;
      }
   }

   //Create Virtual Key mapping table
   memset(VKMap, 0, sizeof(VKMap));
   memset(VKShiftMap, 0, sizeof(VKShiftMap));
   for (i=0; i<128; i++)
      {
      SHORT res = VkKeyScan((TCHAR) i);

	  /* Converted SHR's to TESTs, faster on x86 -SAC */
      if (!(res & 0xff00))            //!highbyte == no shift,ctrl,alt
        VKMap[res] = i;
      else if (res & 0x100)   //Bit 1 of high byte = shift key
        VKShiftMap[res & 0xff] = i;
      }
    VKMap[VK_ESCAPE] = 27; // 27 is mapped as Ctrl-[ by Windows
    VKMap[VK_TAB] = 'i'+96;     // Make it look like '^i' so macroKey: TAB will work
    VKShiftMap[VK_SPACE] =' ';  // Map shift+space -> space
    VKShiftMap[VK_RETURN] ='\r'; // Map shift+return -> return
    VKShiftMap[VK_ESCAPE] =27; // Map shift+escape-> escape
   
   MainThreadID = GetCurrentThreadId(); // Save main thread ID so we can tell
                                        // which thread we're in later

   // Get the current system colors
   for (i=0; i<sizeof(SysColorNames); i++)
   {
       SysColors[i] = GetSysColor(SysColorNames[i]);
   }


#ifdef HAVE_XPM
  GetPixmaps(&GlobalMemDC, &myroot);
#endif

   atexit(W_Cleanup);
   }


//Does all the resource de-allocation needed
//The rest of the silly code doesn't play nice and call DestroyIcon(), etc,
//so we have to track resource usage ourself.
void W_Cleanup()
   {
   int i;
   struct BitmapList *tmp, *p = CurrentBitmap;
   char FileName[100];

   // Reset the system colors to something recognizable
   ResetSysColors();

   //Get rid of the color pens, etc.
   for (i=0; i<COLORS; i++)
      {
      DeleteObject(colortable[i].brush);
      DeleteObject(colortable[i].pen);
      DeleteObject(colortable[i].dashedpen);
      }

   //Delete our two loaded (and shared, and hence not deleted when the window is
   //destroyed) cursors
   DestroyCursor(TrekCursor);
   DestroyCursor(WarnCursor);

   //Delete our fonts
   DeleteObject((HFONT)W_RegularFont);
   DeleteObject((HFONT)W_HighlightFont);
   DeleteObject((HFONT)W_UnderlineFont);
   DeleteObject((HFONT)W_BigFont);

   strcpy(FileName, GetExeDir());
   strcat(FileName, FontFileName);
   RemoveFontResource(FileName);

   //Select the original bitmaps back and delete our memory DCs
   SelectObject(GlobalMemDC, GlobalOldMemDCBitmap);
   DeleteDC(GlobalMemDC);
   SelectObject(GlobalMemDC2, GlobalOldMemDC2Bitmap);
   DeleteDC(GlobalMemDC2);

   //Destory our custom palette
   if (NetrekPalette)
      DeleteObject(NetrekPalette);

   //Remove the bitmap structures we've created, by traversing the linked list
   while (p)
      {
      tmp = p->next;
      DeleteObject(p->bm);
      free(p);
      p = tmp;
      }
   }


   /*
    * Figure out what kind of display we have, for color mapping
    * There are basically three possible situations:
    * 1) A fixed 16 color display (use alternate colors so they map well)
    * 2) A paletted 8-bit display (create a custom palette and use it)
    * 3) A 16 (15?) or 24 bit true-color display (no problem...)
    * We also let the use force a selection on the above.
    */

#define hexdig(a) ( ((a)>='a' && (a)<='f') ? ((a)-'a'+10) : \
                    ( ((a)>='A' && (a)<='F') ? ((a)-'A'+10)  : ((a)-'0') ) )
#define hexbyte(a) ( hexdig(*(a))*16 + hexdig(*((a)+1)) )

void GetColors()
   {
   int i,j;
   char *def;
   int dtype=0;
   HDC hdc;

   if (DefaultsLoaded)   
      {
      dtype=intDefault("forceDisplay", 0);
      if (dtype==0)
          {
          if (booleanDefault("forceMono", 0))
            dtype = 1;
          else
            {
            //Look at display hardware and make a choice

           hdc = GetDC(HWND_DESKTOP);
           i=GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
#ifdef DEBUG
            printf("Bits per pixel detected: %d\n", i);
#endif           
            
            if ( (i<=4) || ((i<15) && !(GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)) )
               dtype = 1;
            else if ( (i<15) && (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) )
               dtype=2;
            else
               dtype=3;

            ReleaseDC(HWND_DESKTOP, hdc);
            }
         }

       if (dtype == 1)                                  // need extra bright clrs on 4 plane display
          memcpy(colortable, altcolortable, sizeof(colortable));
          
       //Modify colors from netrekrc file directives
       for (i=0; i<COLORS; i++)
          {
          char buf[30];
          sprintf(buf, "color.%s", colornames[i]);      //Check netrekrc file
          def = getdefault(buf);
#ifdef RACE_COLORS
          if (!def && i>=COLORS-RaceDefaultOffset)      // For race colors we use default color
             {                                         // color for that race if not set, i.e.
             sprintf(buf, "color.%s", colornames[i-RaceDefaultOffset]);      
             def = getdefault(buf);                    // if "color.rom" is not set, "color.red"
             }
#endif
          if (def)                 
             if (def[0] == '#')                         //Explicit RGB color
                {
                colortable[i].rgb = RGB(hexbyte(def+1), hexbyte(def+3), hexbyte(def+5));
                }
             else
                {
                for (j=0; j<XCLRS; j++)                //Else assume color name
                   if (!stricmp(def, xclrs[j].name))
                      break;
                if (j!=XCLRS)
                   colortable[i].rgb = RGB(xclrs[j].r, xclrs[j].g, xclrs[j].b);
                else
                   fprintf(stderr, "Color '%s' unknown\n", def);
               }
          }
          
       switch (dtype)
         {
         case 1:
   #ifdef DEBUG
            printf("16 color display detected.\n");
   #endif
            break;

         case 2:
            //Paletted display. Create a custom palette, and set the RGB indices
            //in the colortable to palette indices instead of explicit colors
            {
            char space[sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * (COLORS)];
            LOGPALETTE *pal = (LOGPALETTE *) space;

            pal->palVersion = 0x300;      //Windows version >=3.0 palette
            pal->palNumEntries = COLORS;
            for (i=0; i<COLORS; i++)
               {
               pal->palPalEntry[i].peRed = GetRValue(colortable[i].rgb);
               pal->palPalEntry[i].peGreen = GetGValue(colortable[i].rgb);
               pal->palPalEntry[i].peBlue = GetBValue(colortable[i].rgb);
               pal->palPalEntry[i].peFlags = 0;
               colortable[i].rgb = PALETTEINDEX(i);
               }
            NetrekPalette = CreatePalette(pal);

   #ifdef DEBUG
            printf("Paletted color display detected.\n");
   #endif
            }
            break;

         //else, everything's okey dokey, we have a true-color display
         }

      //Create the various pens and brushes for each color
      for (i=0; i<COLORS; i++)
         {
          DWORD dashbits[] = { 1, 2 };
         colortable[i].brush = CreateSolidBrush(colortable[i].rgb);
         colortable[i].pen = CreatePen(PS_SOLID | PS_INSIDEFRAME, 1, colortable[i].rgb);
         colortable[i].dashedpen = CreatePen(PS_DOT, 1, colortable[i].rgb);
         }
      }
   }

W_Window W_RenameWindow(W_Window window, char *str)
   {
   FNHEADER;
   SetWindowText(win->hwnd, str);
   return window;
   }


//Internal function - used as the base for all window creation
Window *newWindow(char *name, int x, int y, int width, int height,
                        W_Window parent, int border, W_Color color, int type)
   {
   Window *window,*parentwin=0;
   HDC hdc;
   char title_buff[100];
   char *s;
   DWORD SpecialStyle = 0;

   if ( !(window = (Window *)malloc(sizeof(Window)) ) )
      {
      printf("Not enough memory to create a new window.");
      return 0;
      }
   memset(window, 0, sizeof(Window));

   //Stuff ripped from x11window.c - provides nicer titles when the window
   // is a wait window or the main window.
   if (strcmp (name, "netrek") == 0)
      {
      if (title)
        s = title;
      else
        {
        s = title_buff;
        sprintf (title_buff, "Netrek  @  %s", serverName);
        }
      SpecialStyle = WS_OVERLAPPEDWINDOW;    //Make main window sizeable
      //SpecialStyle |=WS_BORDER;
      }
   else if (strcmp (name, "wait") == 0)
      {
      if (title)
        s = title;
      else
        s = serverName;
      width += GetSystemMetrics( SM_CXFRAME);
      //Make the wait window show up in the task list
      SpecialStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
      parentwin = &myroot;
      }
   else if (strncmp (name, "Meta", 4) == 0)
      {
      s = name;
      height += GetSystemMetrics( SM_CYFRAME ) + border;
      //Make the Metaserver window show up in the task list
      SpecialStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
      parentwin = &myroot;
      }
#ifdef SUPPORT_WIN32S      
   else if (strcmp (name, "Console") == 0)
      {
      SpecialStyle = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
      s = name;
      }
#endif      
   else
      s = name;

   //Mask border - 15 pixels max - and make sure it's at least 1, and that it's not black
   if (border > 15) border = 15;
   if (!border) border=1;
   if (color == BLACK) color = WHITE;

   //Set the various attributes to default states
   window->type = type;
   window->border = border;
   window->BorderColor=color;
   window->cursor = LoadCursor(NULL, IDC_ARROW);

   //Check the parent - parent to root (==desktop) if null
   if (!parentwin)
   {
   if (!parent)
        {
        parentwin =  (baseWin ? (Window *)baseWin : &myroot);
            SpecialStyle |= baseWin ? WS_POPUP | WS_CAPTION : WS_POPUP;
        }
   else
        {
        parentwin =  (Window *)parent;
        SpecialStyle |= WS_CHILD;
        }
   }

   //Actually create the window
   //Hacked to allow negative create locations -SAC
   window->hwnd = CreateWindow(  ClassName,
                                 s,
                                 WS_CLIPSIBLINGS | WS_CLIPCHILDREN | SpecialStyle,
                                 x + parentwin->border,
                                 y + parentwin->border,
                                 width + border*2,
                                 height + border*2 +
                                    ( (SpecialStyle & WS_CAPTION) ? GetSystemMetrics( SM_CYCAPTION ) : 0),
                                 parentwin->hwnd,
                                 NULL,
                                 MyInstance,
                                 (void *)window);  //Pass Window struct * as user param

   if (!window->hwnd)
      {
      printf("CreateWindow() for %s failed...",name);
      return(0);
      }

   hdc = GetDC(window->hwnd);
   //Select the custom palette if we're in a paletted mode
   if (NetrekPalette)
      {
      SelectPalette(hdc, NetrekPalette, FALSE);
      RealizePalette(hdc);
      }
   ReleaseDC(window->hwnd, hdc);

   if (!AnyWindow)
      AnyWindow = window->hwnd;
   
   return window;
   }

W_Window W_MakeWindow(char *name, int x, int y, int width, int height,
                        W_Window parent, int border, W_Color color)
   {
   Window *newwin;

   //Get the default position, etc.
   checkGeometry(name, &x, &y, &width, &height);

   //Get the default parent..
   checkParent(name, &parent);

   if ( !(newwin = newWindow(name, x, y, width, height, parent, border, color, WIN_GRAPH)))
      return(0);

   //Map (show) the window if the user spec'd it
   if (checkMapped(name))
      W_MapWindow((W_Window)newwin);

   return (W_Window) newwin;
   }


//Make a window that contains fixed-spaced text, where columns must be
//aligned, etc. The x and y passed are in character cells, not pixels.
W_Window W_MakeTextWindow(char *name, int x, int y, int width, int height,
                           W_Window parent, int border)
   {
   Window *newwin;

   //Get the default position, etc.
   checkGeometry(name, &x, &y, &width, &height);

   //Get the default parent..
   checkParent(name, &parent);

   if (!(newwin = newWindow(  name, x, y,
                              width * W_Textwidth + WIN_EDGE * 2,
                              MENU_PAD * 2 + height * W_Textheight,
                              parent, border, W_Black, WIN_TEXT)) )
      return(0);

   //Store the original textheight, width
   newwin->TextHeight = height;
   newwin->TextWidth = width;

   //Map (show) the window if the user spec'd it
   if (checkMapped(name))
      W_MapWindow((W_Window)newwin);

   return (W_Window) newwin;
   }

// Make a WIN_SCROLL type window.
// We use a scrollbar so we can look through the text, something the X version
// didn't have. Nyah, nyah.
W_Window W_MakeScrollingWindow(char *name, int x, int y, int width, int height,
                                 W_Window parent, int border)
   {
   HWND hsb;
   RECT ra;
   Window *newwin;
   int i;

   //Get the default position, etc.
   checkGeometry(name, &x, &y, &width, &height);

   //Get the default parent..
   checkParent(name, &parent);

   if (! (newwin = newWindow( name, x, y,
                              width * W_Textwidth + WIN_EDGE * 2 + GetSystemMetrics(SM_CXVSCROLL),
                              height * W_Textheight + MENU_PAD * 2,
                              parent, border, W_White, WIN_SCROLL)) )
      return(0);

   //Store the original textheight, width
   newwin->TextHeight = height;
   newwin->TextWidth = width;

   //Give it a scroll bar, and set the range (to zero, initially)
   SetWindowLong(newwin->hwnd, GWL_STYLE, GetWindowLong(newwin->hwnd, GWL_STYLE) | WS_VSCROLL );
   SetScrollRange(newwin->hwnd, SB_VERT, 0, 0, FALSE);

   //Map (show) the window if the user spec'd it
   if (checkMapped(name))
      W_MapWindow((W_Window)newwin);

   return (W_Window) newwin;
   }


//Make a menu window. This is similar to a text window in that it has a 1x1 mappiing
//between coordinates and character cells. Note that the height parameter
//specified here indicates the number of items
W_Window W_MakeMenu(char *name, int x, int y,int width,int height,W_Window parent,int border)
   {
   struct menuItem *items;
   Window *newwin;
   HWND ParentHWND;

   int i;

   //Get the default position, etc.
   checkGeometry(name, &x, &y, &width, &height);

   //Get the default parent..
   checkParent(name, &parent);

   if (! (newwin = newWindow( name, x, y,
                              width * W_Textwidth + WIN_EDGE * 2,
                              height * (W_Textheight + MENU_PAD * 2) + (height - 1) * MENU_BAR,
                              parent, border, W_Black, WIN_MENU)) )
      return(0);

   if ( !(items=(struct menuItem *) malloc(height*sizeof(struct menuItem))) )
      fprintf(stderr,"Could not allocate storage for menu window");
   else
      for (i=0; i<height; i++)
         {
         items[i].string=NULL;
         items[i].color=W_White;
         }
   newwin->items = items;
   newwin->NumItems = height;
   newwin->TextHeight = height;

   //Map (show) the window if the user spec'd it
   if (checkMapped(name))
      W_MapWindow((W_Window)newwin);

   return (W_Window) newwin;
   }

void W_ChangeBorder(W_Window window, W_Color color)
   {
   HDC hdc;
   FNHEADER_VOID;

   win->BorderColor = color;

   hdc = GetDC(win->hwnd);                         //Turn off boder clipping
   if (NetrekPalette)
      {
      SelectPalette(hdc, NetrekPalette, FALSE);
      RealizePalette(hdc);
      }
   DrawBorder(win, hdc);
   ReleaseDC(win->hwnd, hdc);
   }


//Displays the window.
void W_MapWindow(W_Window window)
   {
   FNHEADER_VOID;

   ShowWindow(win->hwnd, SW_SHOWNORMAL);
   BringWindowToTop(win->hwnd);
   if (window==baseWin &&
       !booleanDefault("netrek.w32caption",1))
       PostMessage(win->hwnd, WM_SYSKEYDOWN, VK_RETURN, 0);
   }

//Hides the window.
void W_UnmapWindow(W_Window window)
   {
   FNHEADER_VOID;

   ShowWindow(win->hwnd, SW_HIDE);
   }

//Returns TRUE if the window is visible, and also not minimized
int W_IsMapped(W_Window window)
   {
   FNHEADER;

   return (IsWindowVisible(win->hwnd) && !IsIconic(win->hwnd));
   }

void W_FillArea(W_Window window, int x, int y, int width, int height, int color)
   {
   HDC hdc;
   RECT r;
   register int border;
   FNHEADER_VOID;
   border = win->border;

   //do a rectangle intersection with the clipping rect, inlined for speed
   x+=border; y+=border;
   r.left = max(x, border);
   r.right= min(x+width, win->ClipRect.right);
   if (r.right < r.left)
      return;                                      //Horizantal extents do not overlap
   r.top = max(y, border);
   r.bottom = min(y+height, win->ClipRect.bottom);
   if (r.bottom < r.top)
      return;                                      //Vertical extents do not overlap

   hdc = GetDC(win->hwnd);
   if (NetrekPalette)
      {
      SelectPalette(hdc, NetrekPalette, FALSE);
      RealizePalette(hdc);
      }
   FillRect(hdc, &r, colortable[color].brush);
   ReleaseDC(win->hwnd, hdc);
   }

void W_ClearArea(W_Window window, int x, int y, int width, int height)
   {
   HDC hdc;
   RECT r;
   register int border;
   FNHEADER_VOID;
   border = win->border;

   if (win->type == WIN_TEXT)
      {
      x = x * W_Textwidth + WIN_EDGE;
      y = y * W_Textheight + MENU_PAD;
      width *= W_Textwidth;
      height *= W_Textheight;
      }

   //do a rectangle intersection with the clipping rect, inlined for speed
   x+=border; y+=border;
   r.left = max(x, border);
   r.right= min(x+width, win->ClipRect.right);
   if (r.right < r.left)
      return;                                      //Horizantal extents do not overlap
   r.top = max(y, border);
   r.bottom = min(y+height, win->ClipRect.bottom);
   if (r.bottom < r.top)
      return;                                      //Vertical extents do not overlap

   hdc = GetDC(win->hwnd);
   if (NetrekPalette)
      {
      SelectPalette(hdc, NetrekPalette, FALSE);
      RealizePalette(hdc);
      }
   // FillRect doesn't do the edges (right and bottom) -SAC 
   r.right++; r.bottom++;
   FillRect(hdc, &r, colortable[W_Black].brush);
   ReleaseDC(win->hwnd, hdc);
   }

//We don't need to cache...
void W_CacheClearArea(W_Window window, int x, int y, int width, int height)
   {
   W_ClearArea(window, x, y, width, height);
   }

void W_FlushClearAreaCache(W_Window window)
   {
   //Hmmm.... Do I really prefer lima beans, or coconuts... I wonder...
   }


// Clear multiple areas
void W_ClearAreas(W_Window window, int *xs, int *ys, int *widths, int *heights, int num)
{
   HDC hdc;
   RECT r;
   register int border,x,y,width,height;
   FNHEADER_VOID;
   border = win->border;

   hdc = GetDC(win->hwnd);
   if (NetrekPalette)
      {
      SelectPalette(hdc, NetrekPalette, FALSE);
      RealizePalette(hdc);
      }

   while (num)
   {
      num--;

      x = xs[num];
      y = ys[num];
      width = widths[num];
      height = heights[num];
      
      if (win->type == WIN_TEXT)
         {
         x = x * W_Textwidth + WIN_EDGE;
         y = y * W_Textheight + MENU_PAD;
         width *= W_Textwidth;
         height *= W_Textheight;
         }

      //do a rectangle intersection with the clipping rect, inlined for speed
      x+=border; y+=border;
      r.left = max(x, border);
      r.right= min(x+width, win->ClipRect.right);
      if (r.right < r.left)
         continue;                                 //Horizantal extents do not overlap
      r.top = max(y, border);
      r.bottom = min(y+height, win->ClipRect.bottom);
      if (r.bottom < r.top)
         continue;                                  //Vertical extents do not overlap

      FillRect(hdc, &r, colortable[W_Black].brush);
   }
   
   ReleaseDC(win->hwnd, hdc);

   return;
}

void W_ClearWindow(W_Window window)
   {
   HDC hdc;
   FNHEADER_VOID;

   hdc = GetDC(win->hwnd);
   if (NetrekPalette)
      {
      SelectPalette(hdc, NetrekPalette, FALSE);
      RealizePalette(hdc);
      }

   if (!win->tiled)
      FillRect(hdc, &win->ClipRect, colortable[BLACK].brush);
   else
      {
      struct Icon *icon = win->TileIcon;
      RECT r;
      int i,j;

      GetClientRect(win->hwnd, &r);

      //Bitmap should be white on window bk color
      SetTextColor(hdc, colortable[BLACK].rgb);
      SetBkColor(hdc, colortable[WHITE].rgb);

      //Select the bitmap
      SelectObject(GlobalMemDC, icon->bm);

      //Paste the bitmap into the window
      for (i=0; i<r.bottom; i+=icon->height)
         for (j=0; j<r.right; j+=icon->width)
            BitBlt(hdc, j, i,
                   icon->width, icon->height, GlobalMemDC, 0, 0, SRCCOPY );
      DrawBorder(win, hdc);
      }

   ReleaseDC(win->hwnd, hdc);
   }


//These globals used for communication between the following three fns
//We implement a custom queue of only those message we are interested in.
//The event (message) processing would really be quite simple, except
//for the fact that many messages in Windows are _sent_, rather than
//retrieved via GetMessage(), so the window proc must be able to post
//events to the queue on its own without calling W_NextEvent().
W_Event EventQueue[EVENT_Q_SIZE];
int EventHead=0, EventTail=0;

//Are there events (messages) in our queue?
//Here is the main message loop of our program. We first a)check
//to see if there is an event in the queue, and then if there isn't b) do
//the PeekMessage() thing, then c) check the queue again
int W_EventsPending()
   {
   MSG msg;

   if ( (GetAsyncKeyState(VK_SHIFT) & ~0x1) && (GetAsyncKeyState(27) & ~0x1))
      exit(-1);   //Safety device - Shft+ESC = clean termination


   if (EventHead != EventTail)      //Event already in queue
      return(1);

   //Essentially, while there is a message in the application queue
   //and we haven't got and "event", continue processing messages
   while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
      {
      //TranslateMessage(&msg);          //Yes, translate keyboard messages,
      DispatchMessage(&msg);           //dispatch the message to the window proc
      if  (EventHead != EventTail)
         return (1);
      }

   return (0);                         //Nothing waiting
   }

// Wait for an event to become available; it returns zero if it recieves a
// WM_TERMAINTE_WAIT (defined as WM_USER) message,
// which we use to reset the game (it's issued by another thread)
int W_WaitForEvent()
{
   MSG msg;

   while (EventHead == EventTail)      // Get an event
      {
   
      GetMessage(&msg, NULL, 0, 0);
      if (msg.message == WM_TERMINATE_WAIT)          // Quit message recieved
         return 0;

      //TranslateMessage(&msg);          //translate keyboard messages,
      DispatchMessage(&msg);           //dispatch the message to the window proc
      }
   return 1;
}

// Force W_WaitForEvent to terminate. Notice we send the message to 
// any window, the point is that it will be processed in the main thread.
void W_TerminateWait()
{
   PostMessage(AnyWindow, WM_TERMINATE_WAIT, 0, 0);
}

//Get the next event (message). Simply copies the event *, and advances the pointer
void W_NextEvent(W_Event *event)
   {
   if (EventHead == EventTail)   //Make sure there actually is a message!
      W_WaitForEvent();

   EventHead = (EventHead + 1) % EVENT_Q_SIZE;  //Advance queue pointer
   memcpy(event, &EventQueue[EventHead], sizeof(W_Event));
   }

//Return any already queued events
int W_SpNextEvent(W_Event *event)
 {
 if (W_EventsQueued())
    {
    W_NextEvent(event);
    return 1;
    }
 return 0;
 }


//The main window proc for all of the game's windows. Strange design, this netrek code...
//But I guess this is really just a dispatcher function - the events are actually
//"processed" in the rest of the code. Also, all these #ifdef's clutter it needlessly...

//Two very useful defines. These capture an event, after it is determined to be
//valid. Doing this AFTER the event (message) is determined valid increases
//the code size, and makes for the macros, but it also greatly speeds up
//processing of the zillion other messages that go through the loop.

#define GET_STRUCT_PTR\
   win=(Window *)GetWindowLong(hwnd, 0)

#define STORE_EVENT\
   i = (EventTail + 1) % EVENT_Q_SIZE;\
   if (i != EventHead)\
      EventTail = i;\
   if (InSendMessage())\
      GetCursorPos(&pos);\
   else\
      {\
      *((DWORD *)&spos) = GetMessagePos();\
      pos.x = spos.x; pos.y = spos.y;\
      }\
   ScreenToClient(hwnd, &pos);\
   EventQueue[EventTail].x = pos.x - win->border;\
   EventQueue[EventTail].y = pos.y - win->border;\
   EventQueue[EventTail].Window=(W_Window) win;\
   if ( win->type == WIN_MENU )\
      {\
      if (EventQueue[EventTail].y % (W_Textheight + MENU_PAD * 2 + MENU_BAR) >=\
            W_Textheight + MENU_PAD * 2)\
       return (0);\
      EventQueue[EventTail].y /= (W_Textheight + MENU_PAD * 2 + MENU_BAR) ;\
      }

//A similar version of the above that gets the mouse pos from LPARAM -
//used in processing mouse messages
#define STORE_EVENT_MOUSE\
   i = (EventTail + 1) % EVENT_Q_SIZE;\
   if (i != EventHead)\
      EventTail = i;\
   if (InSendMessage())\
      GetCursorPos(&pos);\
   else\
      {\
      *((DWORD *)&spos) = GetMessagePos();\
      pos.x = spos.x; pos.y = spos.y;\
      }\
   ScreenToClient(hwnd, &pos);\
   EventQueue[EventTail].x = pos.x - win->border;\
   EventQueue[EventTail].y = pos.y - win->border;\
   EventQueue[EventTail].Window=(W_Window) win;\
   if ( win->type == WIN_MENU )\
      {\
      if (EventQueue[EventTail].y % (W_Textheight + MENU_PAD * 2 + MENU_BAR) >=\
            W_Textheight + MENU_PAD * 2)\
       return (0);\
      EventQueue[EventTail].y /= (W_Textheight + MENU_PAD * 2 + MENU_BAR) ;\
      }

LRESULT CALLBACK NetrekWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
   {
   register Window *win;
   POINT pos;
   POINTS spos;
   HDC hdc;
   PAINTSTRUCT ps;
   int i,j, thresh;
   struct Icon *icon;
   RECT r;
#if defined(MOTION_MOUSE) || defined(XTRA_MESSAGE_UI)
   static POINTS prev_pos;
   static HWND LastPressHwnd;
#endif

   switch (msg)
      {
      case WM_CREATE:
         //Store our structure pointer - passed from newWindow() as final CreateWindow() param
         SetWindowLong(hwnd, 0, (LONG)(Window *)((CREATESTRUCT *)lParam)->lpCreateParams );
         return(0);

      case WM_SIZE:
         //Create/adjust our clipping rect, which is used to prevent drawing into
         //the border area.
         GET_STRUCT_PTR;

         win->ClipRect.left = win->ClipRect.top = win->border;
         win->ClipRect.right = LOWORD(lParam) - win->border;
         win->ClipRect.bottom =  HIWORD(lParam) - win->border;
         break;

      case  WM_PAINT:
         GET_STRUCT_PTR;

         hdc = BeginPaint(hwnd, &ps);           //Do the beginpaint/endpaint thing
                                                //so that the invalid rectangle is updated
         if (NetrekPalette)
            {
            SelectPalette(hdc, NetrekPalette, FALSE);
            RealizePalette(hdc);
            }
                                                
         i = TRUE;                              //Store this event
         if (win->type == WIN_MENU)
            {
            RedrawMenu(win, hdc);
            i = FALSE;                          //Don't store this event
            }
         else if (win->type == WIN_SCROLL)
            {
            RedrawScrolling(win, hdc);
            i = FALSE;                          //Don't store this event
            }

         DrawBorder(win, hdc);

         EndPaint(hwnd, &ps);                   //Update the invalid region, etc.

         if (i)
            {
            i = (EventTail + 1) % EVENT_Q_SIZE;
            if (i != EventHead)
               EventTail = i;
            EventQueue[EventTail].type=W_EV_EXPOSE;
            EventQueue[EventTail].Window = (W_Window) GetWindowLong(hwnd, 0);
            EventQueue[EventTail].x = ps.rcPaint.left ;//- win->border;
            EventQueue[EventTail].y = ps.rcPaint.top ;//- win->border;
/*            EventQueue[EventTail].width = ps.rcPaint.right - ps.rcPaint.left;
            EventQueue[EventTail].height = ps.rcPaint.bottom - ps.rcPaint.top;*/
            }
         return(0);

      case WM_LBUTTONDOWN:
         BringWindowToTop(hwnd);
         GET_STRUCT_PTR;

         STORE_EVENT_MOUSE;
         LastPressHwnd = hwnd;
         
         EventQueue[EventTail].key = W_LBUTTON;
#ifdef SHIFTED_MOUSE
         if (wParam & MK_SHIFT)
            EventQueue[EventTail].key |= W_SHIFT_BUTTON;
         if (wParam & MK_CONTROL)
            EventQueue[EventTail].key |= W_CTRL_BUTTON;
#endif /* SHIFTED_MOUSE */

#ifdef MOTION_MOUSE
         prev_pos = MAKEPOINTS(lParam);
#endif
         EventQueue[EventTail].type=W_EV_BUTTON;
         return(0);


      case WM_MBUTTONDOWN:                
         BringWindowToTop(hwnd);
         GET_STRUCT_PTR;

         STORE_EVENT_MOUSE;
         LastPressHwnd = hwnd;
         
         EventQueue[EventTail].key = W_MBUTTON;
#ifdef SHIFTED_MOUSE
         if (wParam & MK_SHIFT)
            EventQueue[EventTail].key |= W_SHIFT_BUTTON;
         if (wParam & MK_CONTROL)
            EventQueue[EventTail].key |= W_CTRL_BUTTON;
#endif /* SHIFTED_MOUSE */

#ifdef MOTION_MOUSE
         prev_pos = MAKEPOINTS(lParam);
#endif
         EventQueue[EventTail].type=W_EV_BUTTON;
         return(0);

      case WM_RBUTTONDOWN:
         BringWindowToTop(hwnd);
         GET_STRUCT_PTR;

         STORE_EVENT_MOUSE;
         LastPressHwnd = hwnd;
         
         EventQueue[EventTail].key = W_RBUTTON;
#ifdef SHIFTED_MOUSE
         if (wParam & MK_SHIFT)
            EventQueue[EventTail].key |= W_SHIFT_BUTTON;
         if (wParam & MK_CONTROL)
            EventQueue[EventTail].key |= W_CTRL_BUTTON;
#endif /* SHIFTED_MOUSE */

#ifdef MOTION_MOUSE
         prev_pos = MAKEPOINTS(lParam);
#endif
         EventQueue[EventTail].type=W_EV_BUTTON;
         return(0);

      // Watch for stuff like ALT-ENTER
      case WM_SYSKEYDOWN:
         if (wParam == VK_RETURN)
         {
             LONG wl;
             RECT r;
             int cx,cy;

             cx = GetSystemMetrics(SM_CXEDGE);
             if (!cx)
                 cx = GetSystemMetrics(SM_CXBORDER);
             cy = GetSystemMetrics(SM_CYEDGE);
             if (!cy)
                 GetSystemMetrics(SM_CYBORDER);

             wl = GetWindowLong(((Window *)baseWin)->hwnd, GWL_STYLE);
             GetWindowRect(((Window *)baseWin)->hwnd, &r);
             if (wl & WS_CAPTION)
             {
                 wl &= ~WS_CAPTION;
                 r.left += GetSystemMetrics(SM_CXBORDER);
                 r.top  += GetSystemMetrics(SM_CYBORDER);
             } else {
                 wl |= WS_CAPTION;
                 r.left -= GetSystemMetrics(SM_CXBORDER);
                 r.top  -= GetSystemMetrics(SM_CYBORDER);
             }
             SetWindowLong(((Window *)baseWin)->hwnd, GWL_STYLE, wl);
             // Update the window since the height has changed
             MoveWindow(((Window *)baseWin)->hwnd, r.left, r.top,
                         r.right-r.left, r.bottom-r.top, TRUE);
             return(0);
         }          
         break;

      //Keyboard event - also catch the mousebuttons, #ifdef
      case WM_KEYDOWN:
         //Manually map key to ASCII based on shift state
         j = ( (GetKeyState(VK_SHIFT) & ~0x1 ) ? VKShiftMap : VKMap)[wParam];

         if (!j)    //No event if we can't map the key to ASCII
            break;
            
         //If we're not supposed to ignore caps lock, xlate keys on Caps Lock
         if (GetKeyState(VK_CAPITAL) & 0x1)
            if (ignoreCaps)
               printf("Got a capslock!\n");
            else
               j = ( islower((char)j) ? toupper((char)j) : tolower((char)j) );
         
         //Modified version of STORE_EVENT... we flip window pointers
         //in middle of processing after we get the mouse pos.  This
         //lets us emulate X-style keyboard focus

         i = (EventTail + 1) % EVENT_Q_SIZE;
         if (i != EventHead)
            EventTail = i;
         if (InSendMessage())
            GetCursorPos(&pos);
         else
           {
           *((DWORD *)&spos) = GetMessagePos();
           pos.x = spos.x; pos.y = spos.y;
           }

         //Change the source window in the middle of all this to the one
         //the mouse is in.

         hwnd = WindowFromPoint( pos);
         if (GetWindowLong(hwnd, GWL_WNDPROC) != (LONG) NetrekWndProc)
            return 0; //Mouse is not in one of our windows

         if (!hwnd || hwnd == ((Window *)baseWin))
            hwnd = ((Window *)w)->hwnd;

         LastPressHwnd = hwnd;

         ScreenToClient(hwnd, &pos);

         GET_STRUCT_PTR;

         EventQueue[EventTail].x = pos.x - win->border;
         EventQueue[EventTail].y = pos.y - win->border;
         EventQueue[EventTail].Window=(W_Window) win;
         if ( win->type == WIN_MENU )
           {
           if (EventQueue[EventTail].y % (W_Textheight + MENU_PAD * 2 + MENU_BAR) >=
                 W_Textheight + MENU_PAD * 2)
               return (0);
           EventQueue[EventTail].y /= (W_Textheight + MENU_PAD * 2 + MENU_BAR) ;
           }

         EventQueue[EventTail].type=W_EV_KEY;

#ifdef CONTROL_KEY
         if (use_control_key && (GetKeyState(VK_CONTROL) & ~0x1) )
            EventQueue[EventTail].key = (char) j + 96;
         else
            EventQueue[EventTail].key=(char) j;
#else
         EventQueue[EventTail].key=(char)j;
#endif
         return(0);

      case WM_MOUSEMOVE:
         GET_STRUCT_PTR;
         SetCursor(win->cursor);

#if defined(MOTION_MOUSE) || defined(XTRA_MESSAGE_UI)
         if  ((win->type != WIN_GRAPH) || (hwnd != LastPressHwnd))  
            return(0);  // only allow in graphics windows and must be the window last clicked in
                        // (the latter avoids a bug where Windows sends a mousemove message
                        //  to a window exposed when the one on top is hidden/destroyed)

		 //Check to see if we are entering a message and should be looking
		 //to place the message on hold
		 if (messageon && messHoldThresh)
		 {
			 if (messMouseDelta)
             {
                 /* a^2 + b^2 = c^2 - messHoldThresh is c^2.
                  * Keep a running total of (a^2 + b^2) */
                 int x;

                 x = prev_pos.x - LOWORD(lParam);
                 messMouseDelta += x*x;
                 x = prev_pos.y - HIWORD(lParam);
                 messMouseDelta += x*x;
             } else {
                 messMouseDelta++;
                 prev_pos = MAKEPOINTS(lParam);
             }
             if ((messMouseDelta>=messHoldThresh) || 
                 (wParam & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON)))
                 message_hold();
             return(0);
         }

         if (! (wParam & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON)) )
            return(0);  //We don't want to know about it if no mouse buttons are down

         //Check to see if the mouse has moved >= user_motion_thresh
         thresh = abs (prev_pos.x - LOWORD(lParam)) + abs (prev_pos.y - HIWORD(lParam));
         if (thresh < user_motion_thresh)
            return (0);

         prev_pos = MAKEPOINTS(lParam);

         //Okay, we have an event - store and process further
         STORE_EVENT_MOUSE;
         EventQueue[EventTail].type = W_EV_BUTTON;

          if ((wParam & MK_LBUTTON) || (wParam & MK_MBUTTON) || (wParam & MK_RBUTTON))
          {
            if (wParam & MK_LBUTTON)
                EventQueue[EventTail].key = W_LBUTTON;
            else if (wParam & MK_MBUTTON)
               EventQueue[EventTail].key = W_MBUTTON;
            else if (wParam & MK_RBUTTON)
                EventQueue[EventTail].key = W_RBUTTON;

#ifdef SHIFTED_MOUSE
         //Turn shift+button into a differnt code, and that sort of thing...
         if (extended_mouse)
            {
	      if (wParam & MK_SHIFT)
	            EventQueue[EventTail].key |= W_SHIFT_BUTTON;
         	      if (wParam & MK_CONTROL)
            		EventQueue[EventTail].key |= W_CTRL_BUTTON;
               }
#endif /* SHIFTED_MOUSE */
            return(0);
            }

#endif /* MOTION_MOUSE */

         return (0);


      //And now, for your amusement and edification, some of the more exotic
      //mesages handled here for things like our scroll windows,
      //background bitmap tiling, and dragging windows by their borders

      //Scroll bar moved message - from a scrolling window
      case WM_VSCROLL:
         GET_STRUCT_PTR;
         i = GetScrollPos(hwnd, SB_VERT);
         switch (LOWORD(wParam))
            {
            case SB_LINEDOWN:
               i += 1;
               break;
            case SB_LINEUP:
               i -= 1;
               break;
            case SB_PAGEDOWN:
               i += win->TextHeight;
               break;
            case SB_PAGEUP:
               i -= win->TextHeight;
               break;
            case SB_THUMBTRACK:
            case SB_THUMBPOSITION:
               i = HIWORD(wParam);
               break;
            default:
               return(0);
            }
         SetScrollPos(hwnd, SB_VERT, i, TRUE);        //Move scroll
         InvalidateRect(hwnd, &win->ClipRect, TRUE);  //Redraw text in window
         UpdateWindow(hwnd);
         return(0);

      //Trap WM_ERASEBKGRND, to handle windows with tiled backgrounds
      case WM_ERASEBKGND:
         GET_STRUCT_PTR;

         GetClientRect(hwnd, &r);
         if (NetrekPalette)
            {
            SelectPalette((HDC)wParam, NetrekPalette, FALSE);
            RealizePalette((HDC)wParam);
            }

         if (!win->tiled)
            {
            FillRect((HDC)wParam, &r, colortable[BLACK].brush);
            }
         else
            {
            //As far as I can tell, the bitmap should be white on window bk color...
            SetTextColor((HDC)wParam, colortable[BLACK].rgb);
            SetBkColor((HDC)wParam, colortable[WHITE].rgb);
   
            //Select the bitmap
            icon = win->TileIcon;
            SelectObject(GlobalMemDC, icon->bm);
   
             //Paste the bitmap into the window
             for (i=0; i<r.bottom; i+=icon->height)
               for (j=0; j<r.right; j+=icon->width)
                  BitBlt(  (HDC)wParam, j, i,
                           icon->width, icon->height, GlobalMemDC, 0, 0, SRCCOPY );
            }
   
          return TRUE;

      //Return "in caption" whenever the mouse is on the border, or in client otherwise
      case WM_NCHITTEST:
         i = DefWindowProc(hwnd, WM_NCHITTEST, wParam, lParam);
         if (i == HTCLIENT)
            {
            GET_STRUCT_PTR;
            pos.x = LOWORD(lParam); pos.y = HIWORD(lParam);
            ScreenToClient(hwnd, &pos);
            return ( ( (pos.x < win->border) || (pos.x >= win->ClipRect.right) ||
                     (pos.y < win->border) || (pos.y >= win->ClipRect.bottom) ) ?
                  HTCAPTION :
                  HTCLIENT );
            }
/*
         else if (i == HTBORDER)
            return HTCAPTION;
*/
         else
            return i;

      //This message sent from another thread to indicate a window should be destroyed,
      // which only the main thread can do
      case WM_CROSS_THREAD_DESTROY:
         DestroyWindow((HWND)wParam);
         return 0;
      
      //Delete all of our data, etc, when the window is killed
      //DON'T PRINTF() HERE!!! Unless of course you feel like crashing...
      case WM_DESTROY:
         GET_STRUCT_PTR;

         if (win->UsingCreatedCursor)
            DeleteObject( win->cursor );

         //If it is a menu or scroll window, free the menu item data
         if ( win->type == WIN_MENU )
            {
            for (i=0; i<win->NumItems; i++)
               if (win->items[i].string)
                  free((void *)win->items[i].string);
            free((void *)win->items);
            }
         else if ( win->type == WIN_SCROLL)
            {
            struct stringList *p = win->strings;
            struct stringList *tmp;
            while (p)
               {
               tmp = p->next;
               free((void *)p->string);
               free((void *)p);
               p = tmp;
               }
            }

         //Free the Window structure
         free((void *)win);

         return(0);

      case WM_SETFOCUS:
          if (hwnd==((Window *)baseWin)->hwnd)
          {
              SetTrekSysColors();
              return (0);
          }
          break;

      case WM_KILLFOCUS:
          //if (baseWin && (((void *)wParam)==((Window *)baseWin)->hwnd))
          if (hwnd==((Window *)baseWin)->hwnd)
          {
              ResetSysColors();
              return (0);
          }
          break;

      }

   return DefWindowProc(hwnd, msg, wParam, lParam);
   }

//Slightly diffent from W_EventsPending -- does not get any new events, merely
//reads from out queue.
int W_EventsQueued()
   {
   return  (EventHead != EventTail);
   }

// Used for playback - QueuedAfterReading - should be fixed later
int W_EventsQueuedCk(void)
{
  return (W_EventsPending());
}


//Draw a line.. yay, what a challenge
//Well, considering XDrawLine goes from pt to pt, and
//LineTo doesn't draw the last point, a bigger one -SAC 07 Aug 1996
//Changed to SACLineTo 25 Aug 96
void W_MakeLine(W_Window window, int x0, int y0, int x1, int y1, W_Color color)
   {
   register HDC hdc;
   register HWND hwnd;
   register int border;
   FNHEADER_VOID;
   border=win->border;

   hdc = GetDC(win->hwnd);
   if (NetrekPalette)
      {
      SelectPalette(hdc, NetrekPalette, FALSE);
      RealizePalette(hdc);
      }
   SelectObject(hdc, colortable[color].pen);
   MoveTo(hdc, x0+border, y0+border);
   LineTo(hdc, x1+border, y1+border);
   /* Get that last point in there ... -SAC */
   SetPixel(hdc, x1+border, y1+border, colortable[color].rgb);
   ReleaseDC(win->hwnd, hdc);

   }

//We don't need to cache...
void W_CacheLine(W_Window window, int x0, int y0, int x1, int y1, W_Color color)
{
   W_MakeLine(window, x0, y0, x1, y1, color);
}

void W_FlushLineCaches(W_Window window)
{
}

// Make *many* lines :)
void W_MakeLines(W_Window window, int *x0, int *y0, int *x1, int *y1, int num, W_Color color)
{
   register HDC hdc;
   register int border;
   FNHEADER_VOID;
   border=win->border;

   hdc = GetDC(win->hwnd);
   if (NetrekPalette)
      {
      SelectPalette(hdc, NetrekPalette, FALSE);
      RealizePalette(hdc);
      }
   SelectObject(hdc, colortable[color].pen);

   while (num)
   {
      num--;
      MoveTo(hdc, x0[num]+border, y0[num]+border);
      LineTo(hdc, x1[num]+border, y1[num]+border);
	  SetPixel(hdc, x1[num]+border, y1[num]+border, colortable[color].rgb);
   }
   
   ReleaseDC(win->hwnd, hdc);

   return;
}


//Draw a dashed line, inidicating a tractor/pressor beam
//Do it manually though, as Win32 sucks at creating arbitrary pens
//Apologies if this isn't perfect, it's been a while since I had to
//write an integer linedraw routine.
//Whoops! Can't swap the points - 26 Aug 96
#define CINIT 1<<31
void W_MakeTractLine (W_Window window, int x0, int y0, int x1, int y1, W_Color color)
   {
HDC hdc;
HWND hwnd;
int border;
unsigned long d1,d2,d3;
int dp /* Dash pointer */, dc /* Dash counter */;
register int Md /* Major direction */, md; /* minor direction */
/* 3 blank, 1 solid... etc. -SAC */
int dashdesc[] = { 10, 1 };

FNHEADER_VOID;

    dashdesc[0] = intDefault("tpdotdist",dashdesc[0]);

    md = x0;
    x0 = x1;
    x1 = md;
    md = y0;
    y0 = y1;
    y1 = md;

    hdc = GetDC(win->hwnd);
    if (NetrekPalette)
      {
      SelectPalette(hdc, NetrekPalette, FALSE);
      RealizePalette(hdc);
      }

    border=win->border;
    x0 += border;
    y0 += border;
    x1 += border;
    y1 += border;
    d2 = abs(x0-x1);
    d3 = abs(y0-y1);
    if (d2 > d3)
    {
        /* Major axis is x */

        if (d3) d1 = (((long long) d3)<<32)/d2;
           else d1 = 1;
        d2 = CINIT;
        Md = x0 < x1 ? 1 : -1;
        md = y0 < y1 ? 1 : -1;
        dp = 0;
        dc = dashdesc[dp];

        while ((Md==1 ? x0 <= x1 : x0 >= x1))
        {
            if (dp & 1) // An odd number
                SetPixel(hdc, x0, y0, colortable[color].rgb);
            dc--;
            if (dc<1)
            {
                if (++dp >= 2)
                {
                    dp = 0;
                    dashdesc[dp]+=2;
                }
                dc = dashdesc[dp];
            }

            x0+=Md;
            d3 = d2;
            d2+=d1;
            if (!d1 || (d3 > d2))
            {
                y0 += md;
                dc--;
            }
        }
    } else {
        /* Major axis is y */

        if (d2) d1 = (((long long) d2)<<32)/d3;
           else d1 = 1;
        d2 = CINIT;

        Md = y0 < y1 ? 1 : -1;
        md = x0 < x1 ? 1 : -1;
        dp = 0;
        dc = dashdesc[dp];

        while ((Md==1 ? y0 <= y1 : y0 >= y1))
        {
            if (dp & 1) // An odd number
                SetPixel(hdc, x0, y0, colortable[color].rgb);
            dc--;
            if (dc<1)
            {
                if (++dp >= 2)
                {
                    dp = 0;
                    dashdesc[dp]+=2;
                }
                dc = dashdesc[dp];
            }

            y0+=Md;
            d3 = d2;
            d2+=d1;
            if (!d1 || (d3 > d2))
            {
                x0 += md;
                dc--;
            }
        }
    }

    ReleaseDC(win->hwnd, hdc);
    return;
}

//Draw a dashed line, inidicating a tractor/pressor beam
void Old_W_MakeTractLine (W_Window window, int x0, int y0, int x1, int y1, W_Color color)
   {
   register HDC hdc;
   register HWND hwnd;
   register int border;
   FNHEADER_VOID;
   border=win->border;
   
   hdc = GetDC(win->hwnd);
   if (NetrekPalette)
      {
      SelectPalette(hdc, NetrekPalette, FALSE);
      RealizePalette(hdc);
      }
   SetBkMode(hdc, TRANSPARENT);
   SelectObject(hdc, colortable[color].dashedpen);
   MoveTo(hdc, x0+border, y0+border);
   LineTo(hdc, x1+border, y1+border);
   ReleaseDC(win->hwnd, hdc);

   }

//The same as MakeLine, for now... at least, for the X code it was.
void W_MakePhaserLine (W_Window window, int x0, int y0, int x1, int y1, W_Color color)
   {
   register HDC hdc;
   register HWND hwnd;
   register int border;
   FNHEADER_VOID;
   border=win->border;

   hdc = GetDC(win->hwnd);
   if (NetrekPalette)
      {
      SelectPalette(hdc, NetrekPalette, FALSE);
      RealizePalette(hdc);
      }
   SelectObject(hdc, colortable[color].pen);
   MoveTo(hdc, x0+border, y0+border);
   LineTo(hdc, x1+border, y1+border);
   SetPixel(hdc, x1+border, y1+border, colortable[color].rgb);
   ReleaseDC(win->hwnd, hdc);

   }

//Draw a triangle. Yay.
void W_WriteTriangle (W_Window window, int x, int y, int s, int t, W_Color color)
   {
   register HDC hdc;
   register HWND hwnd;
   POINT points[3];
   FNHEADER_VOID;

   x+= win->border;
   y+= win->border;
   
   if (t == 0)
      {
      points[0].x = x;
      points[0].y = y;
      points[1].x = x + s;
      points[1].y = y - s;
      points[2].x = x - s;
      points[2].y = y - s;
      }
   else
      {
      points[0].x = x;
      points[0].y = y;
      points[1].x = x + s;
      points[1].y = y + s;
      points[2].x = x - s;
      points[2].y = y + s;
      }


   hdc = GetDC(win->hwnd);
   if (NetrekPalette)
      {
      SelectPalette(hdc, NetrekPalette, FALSE);
      RealizePalette(hdc);
      }
   SelectObject(hdc, colortable[color].pen);
   SelectObject(hdc, GetStockObject(NULL_BRUSH));

   Polygon(hdc, points, 3);

   ReleaseDC(win->hwnd, hdc);
   }


//Put some text in a window. This is a straight ExtTextOut call if it's a
//graphics window, a ExtTextOut call with mapped coordinates if it's a
//text window, an addition to a listbox if it's a scrolling window
//(in which case the position is ignored: it always goes at the end
//of the list) and a call to set a menu item if it's a menu window.
void W_WriteText(W_Window window, int x,int y,W_Color color,char *str,int len, W_Font font)
   {
   HDC hdc;
   RECT r;
   SIZE ext;
   register int border;
   FNHEADER_VOID;
   border = win->border;

   switch (win->type)
      {
      case WIN_TEXT:
         x = x * W_Textwidth + WIN_EDGE;
         y = y * W_Textheight+MENU_PAD;
         /* fall through */

      case WIN_GRAPH:
         hdc = GetDC(win->hwnd);
         if (NetrekPalette)
            {
            SelectPalette(hdc, NetrekPalette, FALSE);
            RealizePalette(hdc);
            }

         SelectObject(hdc, (HFONT)font);
         SetTextColor(hdc, colortable[color].rgb);
         SetBkColor(hdc, colortable[BLACK].rgb);

         GetTextExtentPoint32(hdc, str, len, &ext);

         //do a rectangle intersection with the clipping rect, inlined for speed
         x+=border; y+=border;
         r.left = max(x, border);
         r.right= min(x+ext.cx, win->ClipRect.right);
         if (r.right < r.left)
            return;                                      //Horizantal extents do not overlap
         r.top = max(y, border);
         r.bottom = min(y+ext.cy, win->ClipRect.bottom);
         if (r.bottom < r.top)
            return;                                      //Vertical extents do not overlap

         ExtTextOut( hdc, x, y, ETO_CLIPPED|ETO_OPAQUE, &r, str, len, NULL);

         ReleaseDC(win->hwnd, hdc);
         break;

      case WIN_SCROLL:
         str[len]=0;
         AddToScrolling(win, color, str, len);
         break;

      case WIN_MENU:
        { 
           char buf[100];
           STRNCPY(buf,str,len);
           buf[len]=0;    /* Core dump for consts fix */
           ChangeMenuItem(win, y, buf, len, color);
           break;
         }

      default:
         printf("Unknown window type in W_WriteText");
      }
   }


void W_MaskText(W_Window window, int x,int y,W_Color color,char *str,int len, W_Font font)
   {
   HDC hdc;
   FNHEADER_VOID;

   switch (win->type)
      {
      case WIN_TEXT:
         x = x * W_Textwidth + WIN_EDGE;
         y = y * W_Textheight+MENU_PAD;
         /* fall through */

      case WIN_GRAPH:
         hdc = GetDC(win->hwnd);
         if (NetrekPalette)
            {
            SelectPalette(hdc, NetrekPalette, FALSE);
            RealizePalette(hdc);
            }
         SelectObject(hdc, (HFONT)font);
         SetTextColor(hdc, colortable[color].rgb);
         SetBkMode(hdc, TRANSPARENT);

         ExtTextOut( hdc, x+win->border, y+win->border, ETO_CLIPPED, &(win->ClipRect), str, len, NULL);

         ReleaseDC(win->hwnd, hdc);
         break;

      case WIN_SCROLL:
         str[len]=0;
         AddToScrolling(win, color, str, len);
         break;

      case WIN_MENU:
         str[len]=0;
         ChangeMenuItem(win, y, str, len, color);
         break;

      default:
         printf("Unknown window type in W_WriteText");
      }
   }


// Succesively dump each BitmapList
void W_ShowBitmaps()
{
    static struct BitmapList *pBM=0;
    static W_Window tmp = 0;
    HDC hdc;
    int t;

    printf("pBM: %p\nbmlCount: %d",pBM,bmlCount);

    if (!pBM)
    {
        pBM = CurrentBitmap;
        if (!tmp)
            tmp = W_MakeWindow("Bitmap",10,10,BIGBITMAP_WIDTH+5,BIGBITMAP_HEIGHT+5,
                               NULL,0,W_Black);
        if (tmp)
            W_MapWindow(tmp);
    } else
    {
        pBM = pBM->next;
    }

    if (pBM && tmp)
    {
        printf("tmp: %p\n",tmp);
        if (!SelectObject(GlobalMemDC, pBM->bm))
            goto fail;
        if (!(hdc = GetDC( ((Window *)tmp)->hwnd)))
            goto fail;
        BitBlt(hdc,0,0,BIGBITMAP_WIDTH, BIGBITMAP_HEIGHT, GlobalMemDC, 0, 0, SRCCOPY);
        SelectObject(GlobalMemDC, GlobalOldMemDCBitmap);
        ReleaseDC(((Window *)tmp)->hwnd, hdc);
    fail: ;

    } else
    {
        // We've reached the end, kill the window.
        if (tmp)
            W_DestroyWindow(tmp);
        pBM = NULL;
    }

    return;
}

//Creates a bitmap object out of X11 bitmap data...
//What we actually do is allocate a number of 240x240 bitmaps, and copy
//each image to a new section of the bitmap. Part of the W_Icon structure
//that we return has an x,y index into this bitmap. In this fashion,
//we avoid allocating every bitmap handle in the system for the 1,000 or so
//bitmaps that we need.
//Fixed to actually work as intended - 19 Aug 96 SAC
W_Icon W_StoreBitmap(int width,int height,char *bits,W_Window window)
   {
   static int CurX=0, CurY=-1, MaxY=0;
   struct Icon *bitmap=0;
   struct BitmapList *NewBitmap=0;
   unsigned char *bits2=0;
   HBITMAP temp=0,temp2=0;
   int newwidth,re;

   FNHEADER;

   if (CurY==-1)
       CurY = BIGBITMAP_HEIGHT+1; // Force the first bitmap to be created

   //Allocate memory for the bitmap structure and convert the
   //X11 bitmap into a useful form
   if (!(bitmap = (struct Icon *) malloc(sizeof(struct Icon))) ||
         !(bits2 = X11toDIB(bits, width, height)) )
      goto memfail;

   newwidth = ((width + 0xf) & ~0xf);
   //Create a temporary Windows bitmap object
   if (! (temp = CreateBitmap(newwidth, height, 1, 1, bits2)) )
      goto memfail;

   free(bits2);

   //Save the width, height, window...
   bitmap->hwnd = win->hwnd;
   bitmap->width = width;
   bitmap->height = height;
   bitmap->ClipRectAddr = &win->ClipRect;

   //Is the bitmap large enough to justify it's own HBitmap, or
   //should we make it part of a "Big Bitmap"?
   if ( (width>(BIGBITMAP_WIDTH/2)) && (height>(BIGBITMAP_HEIGHT/2)) ) //if it's big....
      {
      if ( !(NewBitmap = (struct BitmapList *) malloc(sizeof(struct BitmapList))) )
         goto memfail;

      //Copy the bitmap to new location so we can mirror it
      if ( !(temp2 = CreateBitmap(width, height, 1, 1, NULL)) )
         goto memfail;

      printf(".");
      bmlCount++;

      //Mirror bitmap
      SelectObject(GlobalMemDC, temp);
      SelectObject(GlobalMemDC2, temp2);
      StretchBlt(GlobalMemDC2, 0, 0, width, height, GlobalMemDC, newwidth-1, 0, -width, height, SRCCOPY);

      NewBitmap->bm = temp2;
      bitmap->bm = temp2;
      bitmap->x = 0;
      bitmap->y = 0;

      if (CurrentBitmap)
         {
         NewBitmap->next = CurrentBitmap->next; //Insert the entry in the linked list
         CurrentBitmap->next = NewBitmap;       //as the item after CurrentBitmap
         }
      else
         {
         printf("Got impossible internal error in W_StoreBitmap(). Please report to author.\n");
         exit(-1);
         }

      }
   else  // Small(ish)
      {
      //Figure out where to store the image in the current big bitmap
      width = (width + 0x7) & ~0x7; //Round up to nearest eight bits
      //height = (height + 0x7) & ~0x7;
      if ( (CurX + width) > BIGBITMAP_WIDTH || (CurY + height) > BIGBITMAP_HEIGHT )
         {
         CurX = 0;               //Advance to next line
         CurY += MaxY;
         MaxY = 0;
         if ( (CurY + height) > BIGBITMAP_HEIGHT)
            {
            HBRUSH SpeckleBr;
            HBITMAP hb ;
            WORD mask[] = { 0xaaaa, 0x5555 }; 
            RECT bmrect;

            if ( !(NewBitmap = (struct BitmapList *) malloc(sizeof(struct BitmapList))) ||
                 !(NewBitmap->bm = CreateBitmap(BIGBITMAP_WIDTH, BIGBITMAP_HEIGHT, 1,1, NULL)) )
               goto memfail;

            bmlCount++;

            NewBitmap->next = CurrentBitmap;    //Advance to next bitmap
            CurrentBitmap = NewBitmap;          //Store in LL

            CurX = 0;
            CurY = 0;
            MaxY = 0;
            bmrect.top = 0;
            bmrect.left = 0;
            bmrect.right = BIGBITMAP_WIDTH-1;
            bmrect.bottom = BIGBITMAP_HEIGHT-1;

            hb = CreateBitmap(2,2,1,1,(const void *) mask   );
            SpeckleBr = CreatePatternBrush(hb);
            DeleteObject(hb);
            SelectObject(GlobalMemDC2, CurrentBitmap->bm);
            FillRect(GlobalMemDC2, &bmrect, SpeckleBr);
            DeleteObject(SpeckleBr);
            SelectObject(GlobalMemDC2, GlobalOldMemDC2Bitmap);

            }
         }

      //Copy the bits to their new location, mirroring as we go
      if (!SelectObject(GlobalMemDC, temp))
          printf("SelectObject(DC, temp) bombed");
      if (!SelectObject(GlobalMemDC2, CurrentBitmap->bm))
          printf("SelectObject(DC2,CurrentBitmap->tm) bombed");

#ifdef FOOBAR
      // Don't ask -SAC
      re = 100;
      while (!StretchBlt(GlobalMemDC2, CurX, CurY, width, height, GlobalMemDC, newwidth-1, 0, -width, height, SRCCOPY) &&
             re--);
      if (re<100)
      {
          DWORD x;
          x = GetLastError();
          printf("   CurX=%3d CurY=%3d width=%3d height=%3d newwidth-1=%3d -width=%3d re=%3d %d\n",
                 CurX, CurY, width, height, newwidth-1, -width, re, x);
      } else {
          printf("++ CurX=%3d CurY=%3d width=%3d height=%3d newwidth-1=%3d -width=%3d\n",
                 CurX, CurY, width, height, newwidth-1, -width);
      }
#else
      StretchBlt(GlobalMemDC2, CurX, CurY, width, height, GlobalMemDC, newwidth-1, 0, -width, height, SRCCOPY);
   #endif

      MaxY = max(MaxY, height);    //The current row (of bitmaps) max height

      //Store the position in the big bitmap, and increment the current X pointer
      bitmap->bm = CurrentBitmap->bm;
      bitmap->x = CurX;
      bitmap->y = CurY;
      CurX += width;
      }

   SelectObject(GlobalMemDC, GlobalOldMemDCBitmap);   //So we don't crunch on the next line...
   DeleteObject(temp);
   SelectObject(GlobalMemDC2, GlobalOldMemDC2Bitmap); //Bitmaps can only be selected into one
                                                      //DC at a time, so deselect it from here.
   return (W_Icon) bitmap;

memfail:
   printf("Memory allocation or CreateBitmap() failure in StoreBitmap\n");
   if (bits2) free(bits2);
   if (bitmap) free(bitmap);
   if (NewBitmap) free(NewBitmap);
   if (temp) DeleteObject(temp);
   if (temp2) DeleteObject(temp2);
   return NULL;
   }

//Put an image on the screen (yay!). Because of all the #$%#$ work that
//has gone into storing the bitmaps (see above), this is thankfully
//a low-overhead call - almost directly translates into the GDI BitBlt
void W_WriteBitmap(int x, int y, W_Icon icon, W_Color color)
   {
   register struct Icon *bitmap = (struct Icon *)icon;
   register int border,width,height;
   register int srcx,srcy;
   HDC hdc;

   //Fast (I hope) rectangle intersection, don't overwrite our borders
   srcx=bitmap->x;
   srcy=bitmap->y;
   border=bitmap->ClipRectAddr->top;
   x+=border; y+=border;
   if (x < border)
     {
     width = bitmap->width - (border-x);
     srcx += border-x;
     x=border;
     }
   else if ( (width = bitmap->ClipRectAddr->right-x) > bitmap->width)
      width = bitmap->width;
   if (y<border)
     {
     height = bitmap->height - (border-y);
     srcy += (border-y);
     y=border;
     }
   else if ( (height = bitmap->ClipRectAddr->bottom-y) > bitmap->height)
      height = bitmap->height;

   hdc = GetDC( bitmap->hwnd );
   if (NetrekPalette)
      {
      SelectPalette(hdc, NetrekPalette, FALSE);
      RealizePalette(hdc);
      }
   SelectObject(GlobalMemDC, bitmap->bm );

   //Set the color of the bitmap
   //(oddly enough, 1-bit = bk color, 0-bit = text (fg) color)
   SetBkColor(hdc, colortable[color].rgb);
   SetTextColor(hdc, colortable[BLACK].rgb);

   BitBlt(hdc, x, y,                                //Copy the bitmap
            width,
            height,
            GlobalMemDC,
            srcx,
            srcy,
            SRCPAINT);                                   // <-- using OR mode

   ReleaseDC( bitmap->hwnd, hdc);
   }

void W_TileWindow(W_Window window, W_Icon icon)
   {
   FNHEADER_VOID;

   win->tiled = TRUE;
   win->TileIcon = (struct Icon *)icon;

   InvalidateRect(win->hwnd, NULL, TRUE);
   UpdateWindow(win->hwnd);
   }

void W_UnTileWindow(W_Window window)
   {
   FNHEADER_VOID;

   if (!win->tiled)
      return;

   win->tiled = FALSE;

   InvalidateRect(win->hwnd, NULL, TRUE);
   UpdateWindow(win->hwnd);
   }


int W_WindowWidth(W_Window window)
   {
   FNHEADER;

   return (win->ClipRect.right - win->border);
   }

int W_WindowHeight(W_Window window)
   {
   FNHEADER;

   return (win->ClipRect.bottom - win->border);
   }

//We fake window events as a socket, by using this magic number
//and a fudge in our implementation of select()... 
int W_Socket()
   {
   return(0);   // No Socket for events return stdin
   }

void W_Beep()
   {
   MessageBeep(MB_ICONEXCLAMATION);
   }

void W_SetIconWindow(W_Window main, W_Window icon)
   {
   /*
    * MS-Windows requires an icon, i.e. a pixmap, not a window, as the iconized image.
    * So, ignore this. We use our own special blend of 11 herbs and spices, stored
    * in the resource file, and coated in crispy batter, as the icon for all of our windows.
    */
   }

//Easy enough... except for the fact that only the thread which created the window
// can destroy it. So if we are not the main thread we post a message
void W_DestroyWindow(W_Window window)
   {
   FNHEADER_VOID;
#ifdef DEBUG
   printf("Destroying %X\n", win->hwnd);
#endif
   if (GetCurrentThreadId() == MainThreadID)
      DestroyWindow(win->hwnd);
   else
      PostMessage(AnyWindow, WM_CROSS_THREAD_DESTROY, (WPARAM)win->hwnd, 0);
   }

//Redraw a menu window.
void RedrawMenu(Window *win, HDC hdc)
   {
   int count;
   RECT r;

   SelectObject(hdc, (HFONT)W_RegularFont);

   r.left = win->border;
   r.right = win->ClipRect.right;

   for (count = 1; count < win->NumItems; count++)
      {
      r.top =  count * (W_Textheight + MENU_PAD * 2) + (count - 1) * MENU_BAR + win->border;
      r.bottom = r.top + MENU_BAR;
      FillRect(hdc, &r, colortable[W_White].brush);
      }

   SetBkMode(hdc, TRANSPARENT);

   for (count = 0; count < win->NumItems; count++)
      if ((win->items[count]).string)
         {
         SetTextColor(hdc, colortable[(win->items[count]).color].rgb);
         TextOut( hdc, WIN_EDGE + win->border,
                  count * (W_Textheight + MENU_PAD * 2 + MENU_BAR) + MENU_PAD + win->border,
                  win->items[count].string, win->items[count].len);
         }
   }

//Change a specific menu item, and redraw.
void ChangeMenuItem(Window *win, int n, char *str, int len, W_Color color)
   {
   register struct menuItem *p = &(win->items[n]);
   HDC hdc;
   RECT r;

   if (n >= win->NumItems)
      return;

   if (p->string)
      free(p->string);

   p->string = (char *) malloc(len + 1);
   strcpy(p->string, str);
   p->color = color;
   p->len = len;

   hdc = GetDC(win->hwnd);
   if (NetrekPalette)
      {
      SelectPalette(hdc, NetrekPalette, FALSE);
      RealizePalette(hdc);
      }

   r.left = win->ClipRect.left;
   r.right = win->ClipRect.right;
   r.top = n * (W_Textheight + MENU_PAD * 2 + MENU_BAR) + MENU_PAD + win->border;
   r.bottom = r.top + W_Textheight;
   FillRect(hdc, &r, colortable[W_Black].brush);

   SetBkMode(hdc, TRANSPARENT);
   SelectObject(hdc, (HFONT)W_RegularFont);

   if (p->string)
      {
      SetTextColor(hdc, colortable[p->color].rgb);
      TextOut( hdc, WIN_EDGE + win->border,
               n * (W_Textheight + MENU_PAD * 2 + MENU_BAR) + MENU_PAD + win->border,
               p->string, p->len);
      }

   DrawBorder(win, hdc);
   ReleaseDC(win->hwnd, hdc);
   }


void AddToScrolling(Window *win, W_Color color, char *str, int len)
   {
   struct stringList *p = win->strings;
   struct stringList *end, *p2;
   int NumStrings = win->NumItems;

   //Find the end of the linked-list of strings...
   if (p)  // ...if the list has been created
      {
      end = p;
      while (end->next)
         end=end->next;
      }
   else
      {
      end = 0;
      NumStrings = 0;
      }

   if (NumStrings < MAX_SCROLLWINDOW_LINES)  //Create a new stringList item
      {
      p2 = (struct stringList *) malloc(sizeof(struct stringList));
      if (!p2)
         {
         fprintf(stderr,"Not enough memory to allocate a new listbox string.\n");
         return;
         }

      if (!p)
         win->strings = p2;                        //Store the new start of list

      p = p2;                                      //Point p to the new string

      win->NumItems = ++NumStrings;                //Inc the string number
      }
   else  //Re-use the first string, place it at the end of the list
      {
      if (p->string)
         free(p->string);
      win->strings = p->next;                      //Store the new start of list
      }

   if (str[len-1] == '\n')                         //Remove trailing newlines
      str[--len] = 0;

   p->string = (char *) malloc(win->TextWidth+ 1);
   if (!p->string)
      {
      printf("Not enough memory to allocate a new listbox string.");
      return;
      }

   p->color = color;
   strncpy(p->string, str, win->TextWidth);

      /* we pad out the string with spaces so we don't have to clear
         the window */
   if (len < win->TextWidth-1)
      memset(p->string + len, ' ', win->TextWidth-len-1);
   p->string[win->TextWidth-1] = 0;

   if (end) end->next = p;
   p->next = 0;

   //Mark as out of date...
   win->AddedStrings++; 
   InvalidateRect (win->hwnd, NULL, FALSE);
   }

//Redraw a scrolling window
void RedrawScrolling(Window *win, HDC hdc)
   {
   struct stringList *p = win->strings;
   int NumStrings = win->NumItems;
   int HiddenLines;
   int i;

   if (!win->AddedStrings && !win->NumItems)
       return;

   if (win->AddedStrings)              //If strings were added since last paint,
    {                                  //Turn off clipping == FULL redraw
      i = win->NumItems - win->TextHeight;
      i = max(0, i);
      if ((i - win->AddedStrings) > GetScrollPos(win->hwnd, SB_VERT))
        {
          // The player is looking back in the list. Don't move things on him!
          HiddenLines = GetScrollPos(win->hwnd,SB_VERT);
          win->AddedStrings = 0;
          SetScrollRange(win->hwnd, SB_VERT, 0, i, TRUE);
        } else {
          // We are at then end away, scroll away
          SelectClipRgn(hdc, NULL);
          win->AddedStrings = 0;
          SetScrollRange(win->hwnd, SB_VERT, 0, i, FALSE);
             //update the scrollbar range
          SetScrollPos(win->hwnd, SB_VERT, i, TRUE);
             //update the scrollbar pos and redraw the scroll bar
          HiddenLines = i;
        }
    } else {
       HiddenLines = GetScrollPos(win->hwnd,SB_VERT);
    }

   //Advance to the first visible line
   NumStrings -= HiddenLines;
   while (HiddenLines--)
      p = p->next;

   //Setup the HDC
   SelectObject(hdc, (HFONT)W_RegularFont);
   SetBkMode(hdc, OPAQUE);
   SetBkColor(hdc, colortable[W_Black].rgb);

   //Draw each line of text
   if (NumStrings > win->TextHeight)
      NumStrings = win->TextHeight;
   for (i = win->TextHeight-NumStrings; i<win->TextHeight && p; i++)
      {
      SetTextColor(hdc, colortable[p->color].rgb);
      TextOut( hdc, WIN_EDGE, MENU_PAD + i * W_Textheight, p->string, strlen(p->string));
      p = p->next;
      }

   }

void W_FlushScrollingWindow(W_Window window)
   {
   HDC hdc;
   struct stringList *p;
   int HiddenLines;
   int y;
   FNHEADER_VOID;

   if (!win->AddedStrings)
      return;

   y = win->NumItems - win->TextHeight;
   y = max(0, y);

   // Just update scrollbar if we are looking back
   if ((y - win->AddedStrings) > GetScrollPos(win->hwnd, SB_VERT))
     {
       SetScrollRange(win->hwnd, SB_VERT, 0, y, TRUE);
       win->AddedStrings = 0;
       return;
     }
   //Do full redraw if faster
   if (win->AddedStrings > (win->TextHeight/2))
      {
      InvalidateRect(win->hwnd, NULL, FALSE);
      UpdateWindow(win->hwnd);                  //Generates paint msg, which calls RedrawScrolling
      return;
      }

   //Setup the HDC
   hdc = GetDC(win->hwnd);
   if (NetrekPalette)
      {
      SelectPalette(hdc, NetrekPalette, FALSE);
      RealizePalette(hdc);
      }
   SelectObject(hdc, (HFONT)W_RegularFont);
   SetBkColor(hdc, colortable[W_Black].rgb);
   SetBkMode(hdc, OPAQUE);

   //update the scrollbar range and position
   SetScrollRange(win->hwnd, SB_VERT, 0, y, FALSE);
   SetScrollPos(win->hwnd, SB_VERT, y, TRUE);

      //Scroll up however many lines. Use ScrollDC se we don't invalidate the window
      y = win->TextHeight - win->AddedStrings;

      if (y < 0)                        //Pathalogical case (but it does happen):
         {                              //First Flush() call has more lines added than will fit
         win->AddedStrings += y;        //in one window height
         y=0;
         }
      else
         ScrollDC(hdc, 0, -win->AddedStrings * W_Textheight, &win->ClipRect, &win->ClipRect, NULL, NULL);

   //Advance to the first visible line
   p = win->strings;
   HiddenLines = max(0, win->NumItems - win->AddedStrings);
   for (; HiddenLines; HiddenLines--)
      p = p->next;

   //Draw each line of text
   y = y * W_Textheight + MENU_PAD;
   while (win->AddedStrings)
      {
      SetTextColor(hdc, colortable[p->color].rgb);
      TextOut( hdc, WIN_EDGE, y, p->string, win->TextWidth);
      p = p->next;
      y += W_Textheight;
      win->AddedStrings--;
      }

   DrawBorder(win, hdc);
   ReleaseDC(win->hwnd, hdc);
   }

void W_DefineMapcursor(W_Window window)
   {
   W_DefineCursor(window,
                  mapcursor_width,
                  mapcursor_height,
                  mapmask_bits,
                  NULL,
                  mapcursor_x_hot,
                  mapcursor_y_hot);
   }

void W_DefineLocalcursor(W_Window window)
   {
   W_DefineCursor(window,
                  localcursor_width,
                  localcursor_height,
                  localmask_bits,
                  NULL,
                  localcursor_x_hot,
                  localcursor_y_hot);
   }

#define MakeTeamCursor(upper, team) \
void W_Define##upper##Cursor(W_Window window) \
   { \
   W_DefineCursor(window, \
                  team##_cruiser_width, \
                  team##_cruiser_height, \
                  team##_cruiser_bits, \
                  NULL,  \
                  team##_cruiser_width/2, \
                  team##_cruiser_height/2); \
   }

MakeTeamCursor(Fed, fed)
MakeTeamCursor(Kli, kli)
MakeTeamCursor(Rom, rom)
MakeTeamCursor(Ori, ori)


//Sets the cursor to a Star-Trek badge
void W_DefineTrekCursor(W_Window window)
   {
   FNHEADER_VOID;

   if (win->UsingCreatedCursor)
      {
      DestroyCursor( win->cursor );
      win->UsingCreatedCursor = FALSE;
      }

   win->cursor = TrekCursor;
   }

//Sets the cursor to an exclamation mark
void W_DefineWarningCursor(W_Window window)
   {
   FNHEADER_VOID;

   if (win->UsingCreatedCursor)
      {
      DestroyCursor( win->cursor );
      win->UsingCreatedCursor = FALSE;
      }

   win->cursor = WarnCursor;
   }


//Sets the cursor to the ol' boring arrow...
void W_DefineArrowCursor(W_Window window)
   {
   FNHEADER_VOID;

   if (win->UsingCreatedCursor)
      {
      DestroyCursor( win->cursor );
      win->UsingCreatedCursor = FALSE;
      }

   win->cursor = LoadCursor(NULL, IDC_ARROW);
   }

//Sets the cursor to a an I-beam, indicating text insertion
void W_DefineTextCursor(W_Window window)
   {
   FNHEADER_VOID;

   if (win->UsingCreatedCursor)
      {
      DestroyCursor( win->cursor );
      win->UsingCreatedCursor = FALSE;
      }

   win->cursor = LoadCursor(NULL, IDC_IBEAM);
   }


//Sets the cursor for a window, from an arbitrary set of bits
void W_DefineCursor(W_Window window,int width,int height,
               char *bits,char *mask,int xhot,int yhot)
   {
   HCURSOR old;
   int flags;
   int i;
   unsigned char *bits2, *mask2;
   FNHEADER_VOID;

   //NB: we throw away the passed bitmap and create our own,
   //since the X bitmap passed is blank, since it uses the masks
   //to display.
   bits2 = X11toCursor(bits, width, height);
   mask2 = (unsigned char *) malloc(32 * 32/8);

   if (!bits2 || !mask2)
      {
      fprintf(stderr, "Memory allocation failed while setting a cursor");
      return;
      }

   //Create the mask as the inverse of the bitmap
   for (i=0; i<32*32/8; i++)
      mask2[i] = ~bits2[i];

   //Check to see if we are using a created (i.e., must be deleted) cursor already,
   //and also set this flag for future reference
   if (win->UsingCreatedCursor)
      DestroyCursor( win->cursor );
   else
      win->UsingCreatedCursor = TRUE;

   win->cursor =
      CreateCursor(MyInstance, xhot, yhot, 32, 32, (void *)mask2, (void *)bits2);

   free(mask2);
   free(bits2);
   }


/***************************************************************************/
/* Looks up any default geometry specified in the defaults file and        */
/* returns the values found there.  Geometry should be of the form         */
/* 502x885+1+1, 502x885, or +1+1.  This allows width or width and height   */
/* or width height and x, or width height x and y, or x or x and y. Note   */
/* that height or y can not be specified unless width and x are also       */
/* specified.                                                              */
/*                                                                         */
/*   The result returned indicates which values were set.                  */
/*                                                                         */
/* result & 0x1 ----> width set        G_SET_WIDTH                         */
/* result & 0x2 ----> height set       G_SET_HEIGHT                        */
/* result & 0x4 ----> x set            G_SET_X                             */
/* result & 0x8 ----> y set            G_SET_Y                             */
/***************************************************************************/
/* Hacked to handle negative geometries 21 Aug 96 -SAC */
#define G_SET_WIDTH 0x1
#define G_SET_HEIGHT 0x2
#define G_SET_X 0x4
#define G_SET_Y 0x8
int checkGeometry(char *name, int *x, int *y, int *width, int *height)
   {
   char           *geom_default;
   char            buf[100];
   char           *s;
   int             result = 0;

   sprintf(buf, "%s.geometry", name);
   geom_default = getdefault(buf);
   if (!geom_default)
      return 0;                  /* nothing set */
   s = geom_default;
   if (*s != '+' && *s != '-')   /* width and height at least */
      {
      while (*s != 'x' && *s != 0)
         s++;
      *width = atoi(geom_default);
      result |= G_SET_WIDTH;
      if (*s == 0)
         return result;
      s++;
         geom_default = s;
      while (*s != '+' && *s != '-' && *s != 0)
         s++;
      *height = atoi(geom_default);
         result |= G_SET_HEIGHT;
      if (*s == 0)
         return result;
      }
   s++;
   geom_default = s;
   while (*s != '+' && *s != '-' && *s != 0)
      s++;
   *x = atoi(geom_default);
   if (*(geom_default-1)=='-')
       *x = -(*x);
   result |= G_SET_X;
   if (*s == 0)
      return result;
   s++;
   geom_default = s;
   *y = atoi(geom_default);
   if (*(geom_default-1)=='-')
       *y = -(*y);
   result |= G_SET_Y;
   return result;
   }

void checkParent (char *name, W_Window *parent)
   {
   char *adefault;
   char buf[100];
   int i;
   HWND found;

   sprintf (buf, "%s.parent", name);
   adefault = getdefault (buf);
   if (adefault == NULL)
      return;

   /* parent must be name of other window or "root" */
   if (strcmpi (adefault, "root") == 0)
      {
      //*parent = (W_Window) &myroot;
      *parent = NULL;
      return;
      }
   else if (strcmpi(adefault,"netrek") == 0)
      {
      //we changed the title of the netrek window,
      //so FindWindow won't work. Do it manually
      *parent = baseWin;
      return;
      }

   /* find the window with the name specified */
   if (found = FindWindow(ClassName, adefault))
      *parent = (W_Window) GetWindowLong(found, 0);   //Return struct ptr

   return;
   }


//Should this window be initially mapped?
int checkMapped(char *name)
   {
   char *adefault;
   char buf[100];

   sprintf(buf, "%s.mapped", name);
   return(booleanDefault(buf, 0));
   }

checkMappedPref(char *name, int preferred)
{
  char   *adefault;
  char    buf[100];

  sprintf(buf, "%s.mapped", name);
  return (booleanDefault(buf, preferred));
}

//Move the mouse cursor to the new window, or back to old pos if window==NULL
void W_WarpPointer(W_Window window)
   {
   static POINT warped_from;
   static int W_in_message = 0;

   if (window == NULL)
      {
      if (W_in_message)
         {
         SetCursorPos(warped_from.x, warped_from.y);
         W_in_message = 0;
         }
      }
   else
      {
      RECT r;
      GetWindowRect(((Window *)window)->hwnd, &r);
      GetCursorPos(&warped_from);
      SetCursorPos((r.left + r.right)/2, (r.top + r.bottom)/2);
      W_in_message = 1;
      }
   }


//An un-documented function that seems to be needed anyway
//Checks if the mouse is in the given window - and if so, where?
int findMouseInWin(int *x, int *y, W_Window window)
   {
   RECT rect;
   POINT p;
   HWND hwnd;
   FNHEADER;

   GetCursorPos(&p);
   hwnd = ChildWindowFromPoint(((Window *)baseWin)->hwnd, p);
   if (!hwnd || (W_Window)GetWindowLong(hwnd, 0) != window)
        {
        *x=0;
        *y=0;
        return 0;
        }

   ScreenToClient(hwnd, &p);
   *x = p.x;
   *y = p.y;
   return 1;
   }


void DrawBorder(Window *win, HDC hdc)
   {
   RECT r;
   register int size = win->border;

   GetClientRect(win->hwnd, &r);
   SelectObject(hdc, colortable[ win->BorderColor ].pen);
   SelectObject(hdc, GetStockObject(NULL_BRUSH));

   for ( ; size; size--)                           //Draw a series of shrinking rectangles
      {
      Rectangle(hdc, r.left, r.top, r.right, r.bottom);
      r.left++;
      r.top++;
      r.bottom--;
      r.right--;
      }

   }


//Converts a NxM X11 cursor bitmap, where N<=32 && M<=32 into a 32 x 32 Windows cursor bitmap
//To expand the size, it simply places the NxM in the upper left corner
//of the 32x32
unsigned char *X11toCursor(unsigned char *bits, int width, int height)
   {
   if (width > 32 || height > 32)
      {
      fprintf(stderr,"Attempt to define cursor > 32x32. Cursor will be garbled!");
      width = min(32, width);
      height = min(32, height);
      }
   return X11toDIBAndMirror(bits, width, height, 32, 32);
   }


//Convert X11-style packed monochrome bitmap information to a Windows monochrome DIB
//Picks the smallest size that will fit the original X-bitmap (X-bitmaps are byte-padded
//while Windows bitmaps are word-padded). Does not mirror the bitmap  --  we use StretchBlt
//with negative width when we copy into the initial memory DC during StoreBitmap (faster method)
unsigned char *X11toDIB(unsigned char *bits, int width, int height)
   {
   int outbytewidth = ((width + 0xF) & ~0xF)/8;        //width of a line in bytes
   int inbytewidth =  (width + 0x7) /8;
   int i;
   register int j;
   register unsigned char *data;
   unsigned char *base =
      (unsigned char *)malloc(outbytewidth * height);    //malloc decode space
   if (!base)
      return NULL;

   memset(base, 0, outbytewidth * height);         //Clear the array

   for (i=0; i<height; i++)
     {
     data = base + ((i+1) * outbytewidth);
     for (j=0; j<inbytewidth; j++)
        *--data = *bits++;
     }

   return base;
   }


//Converts X-style monochrome bitmap to GDI usable format
//and mirrors the image (As X11 bmps are right-to-left). Used for
//defining cursors.
unsigned char *X11toDIBAndMirror(unsigned char *bits, int width, int height,
                                       int outwidth, int outheight)
   {
   register unsigned char rot,rot2;                   //Need lotsa register vars
   register unsigned char *data;                      //for this to run fast
   register int j,i;

   int bytewidth = outwidth /8;                          //width of a line in bytes
   unsigned char *base =
      (unsigned char *)malloc(bytewidth * outheight);    //malloc decode space
   if (!base)
      return NULL;

   memset(base, 0, bytewidth * outheight);         //Clear the array

   //Perform the actual conversion. X11 bitmaps are stored in
   //reverse bit-order (i.e. 0xC0 when the image is actually 0x03)
   for (i=0; i<height; i++)                        //For each line in height...
      {
      data = base + (i*bytewidth);                 //Get start of line in dest
      rot=0x01;                                    //Mask = 00000001b
      rot2=0x80;                                   //Mask = 10000000b
      for (j=0; j<width; j++)                      //For each pixel in width...
         {
         if (*bits & rot)                          //If bit is set in source,
            *data |= rot2;                         // or dest with the other mask.

         rot<<=1;                                  //Shift masks in opposite directions
         rot2>>=1;
         if (!rot2)                                //If we have done 8 shifts,
            {
            rot=0x01;                              // reset masks,
            rot2=0x80;
            bits++;                                // inc source and dest ptrs.
            data++;
            }
         }
      if (rot != 0x1)                              //If we didn't process the whole byte,
         bits++;                                   //inc source ptr
      }

   return base;
   }


void W_Flush()
   {
   /* Supposed to flush input caches... whatever */
   }

#define MAKE_WINDOW_GETTER(name, part) \
 W_Callback name(W_Window w) \
  { \
  return ( w ? ((Window *)w)->part : NULL );\
  }

#define MAKE_WINDOW_SETTER(name, part) \
 W_Callback name(W_Window w, W_Callback c) \
  { \
  if (w)\
   ((Window *)w)->part = c;\
  return c;\
  }

MAKE_WINDOW_GETTER(W_GetWindowKeyDownHandler, HandleKeydown);
MAKE_WINDOW_SETTER(W_SetWindowKeyDownHandler, HandleKeydown);

MAKE_WINDOW_GETTER(W_GetWindowKeyUpHandler, HandleKeyup);
MAKE_WINDOW_SETTER(W_SetWindowKeyUpHandler, HandleKeyup);

MAKE_WINDOW_GETTER(W_GetWindowButtonHandler, HandleButton);
MAKE_WINDOW_SETTER(W_SetWindowButtonHandler, HandleButton);

MAKE_WINDOW_GETTER(W_GetWindowExposeHandler, HandleExpose);
MAKE_WINDOW_SETTER(W_SetWindowExposeHandler, HandleExpose);


void W_ResizeWindow(W_Window window, int neww, int newh)              /* TSH 2/93 */
   {
   if (!window)
      return;
   SetWindowPos(((Window *)window)->hwnd, NULL, 0, 0, neww, newh, SWP_NOMOVE);
   }

void W_ResizeTextWindow(W_Window window, int neww, int newh)                /* TSH 2/93 */
   {
   FNHEADER_VOID;

   if (!window)
      return;
   win->TextHeight = newh;
   win->TextWidth = neww;
   W_ResizeWindow(window, neww*W_Textwidth + WIN_EDGE*2,
                  newh*W_Textheight + WIN_EDGE*2 );
   }

void W_ResizeMenu(W_Window window, int neww, int newh)                /* TSH 2/93 */
  {
  FNHEADER_VOID;

  win->NumItems = newh;
  win->TextHeight = newh;
  win->TextWidth = neww;

  W_ResizeWindow(window, neww*W_Textwidth+WIN_EDGE*2,
                 newh*(W_Textheight+MENU_PAD*2)+(newh-1)*MENU_BAR);
  }

//Is this a mono display?
int W_Mono()
   {
   HDC hdc = GetDC(HWND_DESKTOP);
   register int result =
      ( (GetDeviceCaps(hdc, PLANES) == 1) && (GetDeviceCaps(hdc, COLORRES) == 1) );
   ReleaseDC(HWND_DESKTOP, hdc);
   return result;
   }


#ifdef  SHORT_PACKETS
/* Apparently this solves some problem on X, but here it just forces a redraw...*/
/* Hrm - This was using redrawScrolling(win), updated to (win, hdc) -SAC 24 Jul 96 */
/* Was void W_SetSensitive(Window *window, int b) -SAC */
void W_SetSensitive(Window *window, int b)
{
   HDC hdc;
   
   FNHEADER_VOID;


   b; /* Makes compiler happy -SAC */
   if (win->type == WIN_SCROLL)
     {
       hdc = GetDC(win);
       if (hdc)
	 RedrawScrolling(win,hdc);
       ReleaseDC(win,hdc);
     }
}
#endif

#ifdef BEEPLITE

// We use OR mode for drawing bitmaps anyway, so this is the same call
void W_OverlayBitmap(int x, int y, W_Icon icon, W_Color color)
   {
   register struct Icon *bitmap = (struct Icon *)icon;
   register int border,width,height;
   register int srcx,srcy;
   HDC hdc;

   //Fast (I hope) rectangle intersection, don't overwrite our borders
   srcx=bitmap->x;
   srcy=bitmap->y;
   border=bitmap->ClipRectAddr->top;
   x+=border; y+=border;
   if (x < border)
     {
     width = bitmap->width - (border-x);
     srcx += border-x;
     x=border;
     }
   else if ( (width = bitmap->ClipRectAddr->right-x) > bitmap->width)
      width = bitmap->width;
   if (y<border)
     {
     height = bitmap->height - (border-y);
     srcy += (border-y);
     y=border;
     }
   else if ( (height = bitmap->ClipRectAddr->bottom-y) > bitmap->height)
      height = bitmap->height;

   hdc = GetDC( bitmap->hwnd );
   if (NetrekPalette)
      {
      SelectPalette(hdc, NetrekPalette, FALSE);
      RealizePalette(hdc);
      }
   SelectObject(GlobalMemDC, bitmap->bm );

   //Set the color of the bitmap
   //(oddly enough, 1-bit = bk color, 0-bit = text (fg) color)
   SetBkColor(hdc, colortable[color].rgb);
   SetTextColor(hdc, colortable[BLACK].rgb);

   BitBlt(hdc, x, y,                                //Copy the bitmap
            width,
            height,
            GlobalMemDC,
            srcx,
            srcy,
            SRCPAINT);                                   // <-- using OR mode

   ReleaseDC( bitmap->hwnd, hdc);
   }


void W_EraseTTSText(W_Window window, int max_width, int y, int width)
{
  register int x = (max_width - width) / 2;

  if (x < 0)
    x = 4;
  y -= W_Textheight;

  W_ClearArea(window, x, y, width, W_Textheight);
}

void W_WriteTTSText(W_Window window, int max_width, int y, int width, char *str, int len)
{
  register int x = (max_width - width) / 2;
  HDC hdc;
  FNHEADER_VOID;

  if (x < 0)
    x = 4;

  y -= W_Textheight;

  hdc = GetDC(win->hwnd);
  if (NetrekPalette)
   {
   SelectPalette(hdc, NetrekPalette, FALSE);
   RealizePalette(hdc);
   }
  
  SetTextColor(hdc, colortable[GREY].rgb);
  SetBkMode(hdc, TRANSPARENT);
  TextOut(hdc, x, y, str, len);
  ReleaseDC(win->hwnd, hdc);
}


int W_TTSTextWidth(char *s, int len)
{
   return len*W_Textwidth;
}

#endif

void W_SetWindowName(W_Window window, char *name)
{
  FNHEADER_VOID;

  SetWindowText(win->hwnd, name);
  return;
}

//////////////////////////////////////////////////////////////////////////////////
// Reset the system colors. inlines >> macros! -SAC

inline void ResetSysColors(void)
{
  if (booleanDefault("mungscrollbarcolors",0))
    SetSysColors(sizeof(SysColorNames),SysColorNames,SysColors);
  return;
}

inline void SetTrekSysColors(void)
{
  if (booleanDefault("mungscrollbarcolors",0))
    SetSysColors(sizeof(SysColorNames),SysColorNames,TrekSysColors);
  return;
}

pastebuffer(void)
{
/*
  int     nbytes, x;
  char   *buff, c;

  buff = XFetchBuffer(W_Display, &nbytes, 0);
  for (x = 0; x < nbytes; x++)
    {
      c = buff[x];
      smessage(c);
    }
*/
}

#ifdef HAVE_XPM
void W_Halo ( int x, int y, W_Color color)
{
  // struct window *win = W_Void2Window(mapw);

  if ( (color != W_Ind) && (color != W_Grey) ) 
  {
#ifdef LATER
    XSetForeground(W_Display, colortable[color].contexts[0],
                   colortable[color].pixelValue);
    XDrawArc(W_Display, win->window, colortable[color].contexts[0],
               x-(mplanet_width/2), y-(mplanet_width/2),
               mplanet_width, mplanet_height, 0, 23040);
#endif
  }
}
#endif  /* HAVE_XPM */

