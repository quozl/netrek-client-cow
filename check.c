/****************************************************************************/
/***  File:  check.c                                                      ***/
/***  Function:                                                           ***/
/***  Revisions:                                                          ***/
/*
 * $Log: check.c,v $
 * Revision 1.2  1999/03/25 20:56:26  siegl
 * CygWin32 autoconfig fixes
 *
 * Revision 1.1.1.1  1998/11/01 17:24:08  siegl
 * COW 3.0 initial revision
 * */
/***   9/20/93  VEG changed last while loop of file to improve clarity.   ***/
/***   Also re-indented and spaced to make source easier to read.         ***/
/***   Lastly, changed size of declared character buffer from 4096 + 1    ***/
/***   to just 4096, since read only reads up to the number of characters ***/
/***   requested.  Decided to make this a local define, BUF_SIZE.         ***/
/***                                                                      ***/
/****************************************************************************/

/****************************************************************************/
/***                           Include Files                              ***/
/****************************************************************************/
#include "config.h"
#include "copyright2.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <errno.h>
#include INC_NETINET_IN
#include <arpa/inet.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"

/* For close() and read(), at least in MSVC -SAC */
#include INC_IO

extern void terminate(int error);

/******************************************************************************/
/***     Size lof character data buffer                                     ***/
/******************************************************************************/
#define  BUF_SIZE    ( 4096 )

/******************************************************************************/
/***                          Static Character Buffer                       ***/
/******************************************************************************/
static char buf[BUF_SIZE];

/******************************************************************************/
/***  check()  will obtain the connection to the server and will forever    ***/
/***  read server data into a 4096 byte buffer area, unless an error is     ***/
/***  detected.                                                             ***/
/******************************************************************************/
check(void)
{
  struct sockaddr_in addr;
  struct hostent *hp;
  int     cc, sock;

  /* get numeric form */
  if ((addr.sin_addr.s_addr = inet_addr(serverName)) == -1)
    {
      if ((hp = gethostbyname(serverName)) == NULL)
	{
	  fprintf(stderr, "unknown host '%s'\n", serverName);
	  terminate(0);
	}
      else
	{
	  addr.sin_addr.s_addr = *(LONG *) hp->h_addr;
	}
    }
  addr.sin_family = AF_INET;
  addr.sin_port = htons(xtrekPort);
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror("socket");
      terminate(0);
    }
  printf("checking %s on port %d\n", serverName, xtrekPort);
  if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0)
    {
      perror("connect");
      close(sock);
      terminate(0);
    }

  while ((cc = read(sock, buf, BUF_SIZE)) > 0)
    {
      fwrite(buf, cc, 1, stdout);
    }
  if (cc < 0)
    {
      perror("read");
    }
  close(sock);
  terminate(0);
}
