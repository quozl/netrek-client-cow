/*
 * winmain.c
 * Main file for the MS-Windows port of Netrek. Contains the "main" function,
 * as well as Windows implemenations of major Unix functions. Handles differences
 * between NT and Win32s, specifically the lack of consoles on Win32s.
 *
 * Copyright (C) 1993-1995 Jonathan Shekter
 */

#define NO_BOOLEAN

#include <windows.h>
#include <windowsx.h>
#include <winsock.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "win32/string.h"
#include <pwd.h>

#include "config.h"
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"

#ifdef __BORLANDC__
   #pragma warn -par  //so we don't get a zillion parameter never used messages
#endif
#ifdef _MSC_VER
#define putenv _putenv
#endif

//Global data
HINSTANCE MyInstance;
char ExeDir[100];
char HomeDir[100];
#ifdef SUPPORT_WIN32S
BOOL fWin32s;
W_Window console;
int PrintfSafe = 0;     //Turns off printf during critical operations

void Wprintf(char *format,...);
void Wfprintfstderr(char *format, ...);
#endif

void WinMainCleanup();

char *GetExeDir()
   {
   return ExeDir;
   }

#ifdef _MSC_VER_ACK
/* Micro$oft, in their infinite wisdom, no longer support main(int, char**)
 * in Win32! (or at least in the version I got). So write a WinMain that is
 * compatable with the rest of the code. -SAC */
/* Well, not quite - you have to remember to set the linker to console :/
 * At any rate, I'm leaving this here, 'cause I bet it's gonna be need
 * before to oft long -SAC 01 Aug 1996 */
/* int
WINAPI
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nShowCmd
    ); */

int WINAPI wWinMain(HINSTANCE MyInstance, 
				    HINSTANCE hPrevInstance, 
					LPSTR lpCmdLine, 
					int showCmd)
{
	char *c;
	WSADATA data;
	char *lpCmdLineCopy; // Gotta free it later :/
	int argc;
	char **argv;

	lpCmdLineCopy = calloc(1,strlen(lpCmdLine)+10);

	strcpy(lpCmdLineCopy, "netrek.exe");

	if (*lpCmdLine)
	{
		lpCmdLineCopy[10] = ' ';
		strcpy(lpCmdLineCopy+11,lpCmdLine);
	}

	for(argc=0, c=lpCmdLineCopy; c != NULL; c++)
		if (*c==' ')
		  argc++;

	argv = calloc(argc+1, sizeof(c));
	argv[0] = lpCmdLineCopy;

	if (argc)
	{
		int t; //MSCV can do it :)

		c = strtok(lpCmdLineCopy," ");
		for(t=1; c; c = strtok(" ",lpCmdLineCopy),t++)
			argv[t] = c;

	}
#else /* Non-Visual C */

//Our main. Does some init before calling the actual Netrek code, which
//now has main2() instead of main().
int main(int argc, char **argv)
   {
   char *c;
   WSADATA data;                                   //Using sockets - perform WinSock startup

   MyInstance = GetModuleHandle(NULL);

#ifdef SUPPORT_WIN32S
   fWin32s = (getenv("FORCECONSOLE") ? 1 : GetVersion() & 0x80000000);
#endif

#endif /* Visual C */

   //Get home path (screwny NT environ vars), and set HOME var
   if (c=getenv("HOMEDRIVE"))
      {
      strcpy(HomeDir, "HOME=");
      strcat(HomeDir, c);
      if (c=getenv("HOMEPATH"))
         strcat(HomeDir, c);
      if (HomeDir[strlen(HomeDir)-1] != '\\')
        strcat(HomeDir, "\\");
      putenv(HomeDir);
      }

   //Get the executable file directory
   c = ExeDir + GetModuleFileName(MyInstance, ExeDir, sizeof(ExeDir)) -1;
   while (*c != '\\')   //remove filename
      *(c--) = 0;
   *c = 0;  //remove last backslash


   //Kick winsock into gear
   if (WSAStartup(MAKEWORD(1,1), &data) != 0)
      {
      fprintf(stderr,"Could not find WINSOCK.DLL / WINSOCK initializtion failed\n");
      return -1;
      }
   if ( data.wVersion<MAKEWORD(1,1) )
      {
      fprintf(stderr,"Bad WINSOCK.DLL version (need at least v1.1).\n");
      WSACleanup();
      return -1;
      }

#ifdef SUPPORT_WIN32S
   //Create the "Console" window if running on Win32s
   W_Initialize(NULL);   /* NULL == don't need a display name as not on X */
   if (fWin32s)
      {
      console = W_MakeScrollingWindow("Console", 0, 0, 80, 10, NULL, 2);
      PrintfSafe = 1;                                 //Now OK to printf
      }
#endif

   atexit(WinMainCleanup);

   main2(argc, argv);                                 //Call BRM main!

   return 0;
   }


void WinMainCleanup()
   {
#ifdef SUPPORT_WIN32S      
   if (fWin32s)
      {
      //If we're using our own special console window,
      //pause before closing so the user can read
      Wprintf("Netrek/Win32 terminating...");
      W_DestroyWindow(console);
      PrintfSafe=0;        //Console is dead
      }
#endif
   WSACleanup();        //Shut down our sockets
   }


//***************************************************************************//
// Various UNIX functions that will enable the code to compile as-is without //
// changes - only this file added.                                           //
//***************************************************************************//

//Get process ID
int getpid()
   {
   return (int) GetCurrentProcessId();
   }


//Get user ID - fugde!
char *getuid()
   {
   return (char *)1;
   }

//getpwuid() function - does it for real under NT. Just copies the given string...
struct passwd *getpwuid(char *getuidstuff)
   {
   static struct passwd pw;
   static char name[100];
   char *p;
   int i= sizeof(name);

   memset(name,0,sizeof(name));

   /* Try to get user w/ WNetGetUser instead of GetUserName in the
      hope that it will retrieve the user name on WFWG */
   if ((WNetGetUser(NULL, name, &i)!=NO_ERROR ) ||
       !stricmp(name,"Administrator") ||
       !stricmp(name,"admin")         ||
       !stricmp(name,"root"))
   {
       /* Pretty lame ids - use the one given in the rc file */
       p = NULL;
       p = getdefault("login");
       if (!p || !name[0])
        return NULL;
       pw.pw_name = p;
       return &pw;
   }

   pw.pw_name = name;
   return &pw;
   }

//Sleep...
void sleep(int seconds)
   {
   Sleep(seconds * 1000);
   }


// rint
double rint(double r)
{
   return floor(r+0.5);
}


//A more useful perror(), reports last winsock error as well
void perror(const char *str)
   {
   printf("%s: errno = %d, WSALast = %d\n", str, errno, WSAGetLastError());
   }


#ifdef NEW_SELECT
// ********************************* select ********************************
// This is our implementation of select, using WaitForMultipleObjects()
// We support W_Socket() here too. Note that we only handle readfds
#define W_SOCKET 0x0CABBABE // can be anything but must match mswindow.c
int PASCAL select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
   HANDLE handles[FD_SETSIZE];
   DWORD got;
   int i,j,ms;
   int wsocket_present = 0;

   // Set timeout
   if (timeout)
      {
      ms = (timeout->tv_sec<<10) + (timeout->tv_usec>>10);
      printf("sec = %d, usec = %d\n", timeout->tv_sec, timeout->tv_usec);
      }
   else
      ms = INFINITE;

   printf("ms=%d, ", ms);
   
   /* Scan through the array, copying and looking for W_Socket */
   for (i=j=0; i<readfds->fd_count; i++)
      {
      if (readfds->fd_array[i] != W_SOCKET)
      {
         printf("handle %d, sock=%d\n", readfds->fd_array[i], sock);
         handles[j++] = readfds->fd_array[i];
      }
      else
         wsocket_present = 1;
      }

   printf("handles=%d, handle[0]=%d, wsocket=%d\n", j, handles[0], wsocket_present);

   readfds->fd_count=1;
   
/*   if (wsocket_present)
      {
      if (W_EventsPending())   //MsgWait... does not return if there is input ready, only
         {                     //if it _arrives_ 
         readfds->fd_count=2;
         readfds->fd_array[1] = W_SOCKET;
         } 
      else 
         got = MsgWaitForMultipleObjects(j, handles, FALSE, // false == wait for only one object
                                         ms,  QS_ALLINPUT);
      if (got == WAIT_OBJECT_0+j) // == windowing event arrived
         {
         readfds->fd_array[0] = W_SOCKET;
         return 1;
         }
      }
   else*/
//      got = WaitForMultipleObjects(j, handles, FALSE /* wait for one only */,ms)  ;
        got = WaitForSingleObject(handles[0], ms);
   
   if (got == WAIT_TIMEOUT)
      {
      printf("Timeout\n");
      readfds->fd_count = 0;  // clear
      return 0;
      }

   readfds->fd_array[0] = handles[got-WAIT_OBJECT_0];
   return 1;

   printf("Success! Returned %d\n", got);
}
#endif

#ifdef SUPPORT_WIN32S      

//******************************** STDIO HANDLING *****************************//
// Under Win32s we have to do our own stdio. Yeesh!

//A useful function to replace printf().
void Wprintf(char *format,...)
   {
   char buf[300];
   va_list args;
   va_start(args, format);
   vsprintf(buf,format,args);
   va_end(args);
   vsprintf(buf,format,args);
   MessageBox(NULL,buf,"Netrek/Win32",MB_OK);
   }

//Another function, this one to replace fprintf(stderr,...)
void Wfprintfstderr(char *format, ...)
   {
   char buf[300];
   va_list args;
   va_start(args, format);
   vsprintf(buf,format,args);
   va_end(args);
   MessageBeep(MB_ICONEXCLAMATION);
   MessageBox(NULL,buf,"Netrek/Win32: Error",MB_OK|MB_ICONEXCLAMATION);
   }


/* - The following fns not needed in Win32, as we actually have a real, live console of our own
   - But, under Win32s we don't... so... arrgh */


//Yet another function, this one actually IS printf() - overrides the default
//definition, sends it to the console window -
int printf(const char *format,...)
   {
   char buf[300];
   int len, junk;
   va_list args;

   va_start(args, format);
   len = vsprintf(buf,format,args);
   va_end(args);

   if (fWin32s)
     {
     if (!PrintfSafe)  //Flag that helps us avoid infinite recursions and other nasties
        {
        MessageBox(NULL, buf, "This is a printf", MB_OK);
        return 0; 
        }
      PrintfSafe = 0;
      if (!W_IsMapped(console))
         W_MapWindow(console);
      W_WriteText(console, 0, 0, W_White, buf, len, W_RegularFont);
      W_FlushScrollingWindow(console);
      PrintfSafe = 1;
      }
   else
     WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buf, len, &junk, NULL);

   return len;
   }

//Arrghh our version of fwrite...
int myfwrite(char *buf, int count1, FILE *file)
   {
   register int count = count1;
   while (count--)
      {
      if (fputc(*buf, file) == EOF)
         return ((count1 - count)-1);
      buf++;
      }
   return count1;
   }


//Redriect fprintf() to stdout and stderr to our console window under Win32s
int fprintf(FILE *handle, const char *format,...)
   {
   char buf[300];
   int len, junk;
   va_list args;

   va_start(args, format);
   len = vsprintf(buf,format,args);
   va_end(args);

   if (handle == stdout || handle == stderr)
         {
         if (fWin32s)
            {
            if (!PrintfSafe)
                {
                MessageBox(NULL, buf, "This is an fprintf to stderr/stdout", MB_OK);
                return 0;
                }
            PrintfSafe = 0;
            if (!W_IsMapped(console))
               W_MapWindow(console);
            W_WriteText(console, 0, 0, W_White, buf, len, W_RegularFont);
            W_FlushScrollingWindow(console);
            PrintfSafe = 1;
            }
         else
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buf, len, &junk, NULL);
         }
   else
      return myfwrite(buf, len, handle);

   return len;
   }


//More nastyness: this time for fwrite. I hate Win32s. No consoles!
size_t fwrite(char *buf, int blksz, int count, FILE *handle)
   {
   int junk;

   if (handle == stdout || handle == stderr)
         {
         if (fWin32s)
            {
            if (!PrintfSafe)
                {
                MessageBox(NULL, buf, "This is an fwrite to stderr/stdout", MB_OK);
                return 0;
                }
            PrintfSafe = 0;
            if (!W_IsMapped(console))
               W_MapWindow(console);
            W_WriteText(console, 0, 0, W_White, buf, blksz*count, W_RegularFont);
            W_FlushScrollingWindow(console);
            PrintfSafe = 1;
            }
         else
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buf, blksz*count, &junk, NULL);
         }
   else
      return myfwrite(buf, count*blksz, handle);

   return blksz*count;
   }

#endif  /* WIN32S */

   
