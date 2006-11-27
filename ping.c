
/* ping.c
 *
 * $Log: ping.c,v $
 * Revision 1.2  2006/05/22 13:13:24  quozl
 * initialise packet buffers
 *
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 * */

#include "config.h"
#include "copyright2.h"
#include <stdio.h>
#include <sys/types.h>
#include INC_NETINET_IN
#include <sys/time.h>
#include <math.h>
#include <errno.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"



/* These are used only in pingstats.c */

int     ping_iloss_sc = 0;			 /* inc % loss 0--100, server

						  * 
						  * * * to client */
int     ping_iloss_cs = 0;			 /* inc % loss 0--100, client

						  * 
						  * * * to server */
int     ping_tloss_sc = 0;			 /* total % loss 0--100, *

						  * 
						  * * server to client */
int     ping_tloss_cs = 0;			 /* total % loss 0--100, *

						  * 
						  * * client to server */
int     ping_lag = 0;				 /* delay in ms of last ping */
int     ping_av = 0;				 /* rt time */
int     ping_sd = 0;				 /* std deviation */

static int sum, n;
static int M, var;
static double s2;

void    handlePing(struct ping_spacket *packet)	 /* SP_PING */
{
  ping = 1;					 /* we got a ping */

  /* printf("ping received at %d (lag: %d)\n", msetime(), (int)packet->lag); */
  sendServerPingResponse((int) packet->number);
  ping_lag = ntohs(packet->lag);
  ping_iloss_sc = (int) packet->iloss_sc;
  ping_iloss_cs = (int) packet->iloss_cs;
  ping_tloss_sc = (int) packet->tloss_sc;
  ping_tloss_cs = (int) packet->tloss_cs;

  calc_lag();

  if (W_IsMapped(pStats))			 /* pstat window */
    updatePStats();
}

startPing(void)
{
  static
  struct ping_cpacket packet;
  extern int serverDead;

  packet.type = CP_PING_RESPONSE;
  packet.pingme = 1;

  if (gwrite(sock, (char *) &packet, sizeof(struct ping_cpacket)) !=
      sizeof  (struct ping_cpacket))
    {
      printf("gwrite failed.\n");
      serverDead = 1;
    }
}

stopPing(void)
{
  static
  struct ping_cpacket packet;
  extern int serverDead;

  packet.type = CP_PING_RESPONSE;
  packet.pingme = 0;

  if (gwrite(sock, (char *) &packet, sizeof(struct ping_cpacket)) !=
      sizeof  (struct ping_cpacket))
    {
      printf("gwrite failed.\n");
      serverDead = 1;
    }
}


sendServerPingResponse(int number)		 /* CP_PING_RESPONSE */

{
  struct ping_cpacket packet;
  int     s;
  extern int serverDead;
  int     ps;

  if (udpSock >= 0)
    {
      s = udpSock;
      packets_sent++;
    }
  else
    s = sock;

  bzero(&packet, sizeof(packet));
  packet.type = CP_PING_RESPONSE;
  packet.pingme = (char) ping;
  packet.number = (unsigned char) number;
  /* count this one */
  packet.cp_sent = htonl(packets_sent);
  packet.cp_recv = htonl(packets_received);

  /* printf("ping response sent at %d\n", msetime()); */

  if (gwrite(s, (char *) &packet, sizeof(struct ping_cpacket)) !=
      sizeof  (struct ping_cpacket))
    {
      printf("gwrite failed.\n");
      serverDead = 1;
    }
}

calc_lag(void)
{

#ifdef nodef
  /* probably ghostbusted */
  if (ping_lag > 2000 || ping_lag == 0)
    return;
#endif

  n++;
  sum += ping_lag;
  s2 += (ping_lag * ping_lag);
  if (n == 1)
    return;

  M = sum / n;
  var = (s2 - M * sum) / (n - 1);

  ping_av = M;
  ping_sd = (int) sqrt((double) var);
}
