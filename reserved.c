
/* reserved.c (adapted for trekhopd)
 * 
 * Kevin P. Smith   7/3/89
 *
 * $Log: reserved.c,v $
 * Revision 1.3  2006/05/16 06:25:25  quozl
 * some compilation fixes
 *
 * Revision 1.2  1999/03/25 20:56:26  siegl
 * CygWin32 autoconfig fixes
 *
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 * */
#include "config.h"
#include "copyright2.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include INC_NETINET_IN
#include <netdb.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"

#ifdef GATEWAY
extern unsigned LONG netaddr;

#endif
extern void terminate(int error);

makeReservedPacket(struct reserved_spacket *packet)
{
  int     i;

  for (i = 0; i < 16; i++)
    {
      packet->data[i] = RANDOM() % 256;
    }
  packet->type = SP_RESERVED;
}

/* A "random" table we use to hose our data. */

static unsigned char swap[16][16] =
{
  {4, 13, 9, 7, 5, 8, 14, 2, 10, 11, 6, 3, 12, 15, 0, 1},
  {0, 4, 12, 5, 15, 2, 7, 3, 14, 6, 1, 10, 8, 13, 11, 9},
  {9, 5, 4, 2, 3, 6, 10, 12, 1, 7, 15, 14, 11, 0, 13, 8},
  {5, 8, 15, 4, 6, 9, 1, 12, 10, 3, 13, 11, 2, 7, 0, 14},
  {15, 13, 1, 14, 4, 0, 8, 12, 11, 9, 2, 10, 3, 6, 7, 5},
  {6, 7, 9, 11, 14, 1, 4, 13, 2, 12, 8, 15, 10, 0, 5, 3},
  {11, 0, 10, 14, 7, 12, 2, 4, 15, 13, 3, 9, 1, 5, 8, 6},
  {9, 8, 2, 10, 12, 14, 3, 7, 4, 0, 11, 1, 5, 15, 6, 13},
  {15, 12, 3, 0, 10, 8, 13, 9, 14, 2, 4, 5, 7, 6, 1, 11},
  {14, 3, 0, 11, 2, 10, 15, 12, 13, 1, 5, 4, 6, 8, 7, 9},
  {1, 2, 7, 3, 8, 0, 10, 14, 6, 5, 13, 12, 4, 9, 11, 15},
  {2, 1, 11, 9, 7, 3, 12, 6, 5, 8, 0, 10, 14, 4, 15, 13},
  {0, 12, 15, 1, 13, 7, 6, 5, 3, 14, 11, 8, 9, 10, 2, 4},
  {10, 9, 0, 6, 1, 14, 5, 12, 7, 4, 15, 13, 8, 2, 3, 11},
  {3, 11, 6, 12, 10, 5, 14, 1, 9, 15, 7, 2, 13, 8, 4, 0},
  {12, 6, 5, 8, 14, 4, 13, 15, 10, 0, 11, 9, 3, 1, 7, 2}
};

encryptReservedPacket(struct reserved_spacket *spacket, struct reserved_cpacket *cpacket, int server, int pno)
{
  struct sockaddr_in saddr;
  struct hostent *hp;
  struct in_addr address;
  unsigned char mixin1, mixin2, mixin3, mixin4, mixin5;
  int     i, j, k;
  char    buf[16];
  unsigned char *s;
  int     len;

#ifndef GATEWAY
  unsigned LONG netaddr = 0;

#endif

  MCOPY(spacket->data, cpacket->data, 16);
  MCOPY(spacket->data, cpacket->resp, 16);
  cpacket->type = CP_RESERVED;

  /* if we didn't get it from -H, go ahead and query the socket */
  if (netaddr == 0)
    {
      len = sizeof(saddr);
      if (getpeername(sock, (struct sockaddr *) &saddr, (socklen_t *) &len) < 0)
	{
	  perror("getpeername(sock)");
	  terminate(1);
	}
      netaddr = saddr.sin_addr.s_addr;
    }

  /* printf("Verifying with netaddr %x\n", netaddr); */
  mixin1 = (netaddr >> 24) & 0xff;
  mixin2 = pno;
  mixin3 = (netaddr >> 16) & 0xff;
  mixin4 = (netaddr >> 8) & 0xff;
  mixin5 = netaddr & 0xff;

  j = 0;
  for (i = 0; i < 23; i++)
    {
      j = (j + cpacket->resp[j]) & 15;
      s = swap[j];
      cpacket->resp[s[i % 7]] ^= mixin1;
      cpacket->resp[s[i % 5]] ^= mixin2;
      cpacket->resp[s[i % 13]] ^= mixin3;
      cpacket->resp[s[i % 3]] ^= mixin4;
      cpacket->resp[s[i % 11]] ^= mixin5;
      for (k = 0; k < 16; k++)
	{
	  buf[k] = cpacket->resp[s[k]];
	}
      MCOPY(buf, cpacket->resp, 16);
    }
}
