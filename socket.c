/* Socket.c
 * 
 * Kevin P. Smith 1/29/89 UDP stuff v1.0 by Andy McFadden  Feb-Apr 1992
 * 
 * UDP protocol v1.0
 * 
 * Routines to allow connection to the xtrek server.
 */
#include "config.h"
#include "copyright2.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include INC_SYS_SELECT
#include INC_MACHINE_ENDIAN
#include INC_NETINET_IN
#include INC_NETINET_TCP
#include <netdb.h>
#include <arpa/inet.h>
#include <math.h>
#include <errno.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"
#include "ltd_stats.h"
#include "wtext.h"
#include "playerlist.h"

#include "badversion.h"
#include "defaults.h"
#include "dmessage.h"
#include "getship.h"
#include "local.h"
#include "map.h"
#include "netstat.h"
#include "newwin.h"
#include "reserved.h"
#include "playback.h"
#include "rotate.h"
#include "redraw.h"
#include "short.h"
#include "stats.h"
#include "udpopt.h"
#include "warning.h"

#include "socket.h"

#define statsFile stderr        /* use stderr for stats output for now */
int gather_stats = 0;

#ifdef WIN32					 /* socket garbage in case *
						  * the client is not running 
						  * 
						  * * on NT */
#define read(f,b,l) recv(f,b,l,0)
#define write(f,b,l) send(f,b,l,0)
#define close(s) closesocket(s)
#endif

#if 0
#define INCLUDE_SCAN            /* include Amdahl scanning beams */
#endif
#define INCLUDE_VISTRACT        /* include visible tractor beams */

#define NETSTAT

#ifdef GATEWAY
/* (these values are now defined in "main.c":) char *gw_mach        =
 * "charon";     |client gateway; strcmp(serverName) int   gw_serv_port   =
 * 5000;         |what to tell the server to use int   gw_port        = 5001;
 * |where we will contact gw int   gw_local_port  = 5100;         |where we
 * expect gw to contact us
 * 
 * The client binds to "5100" and sends "5000" to the server (TCP).  The server
 * sees that and sends a UDP packet to gw on port udp5000, which passes it
 * through to port udp5100 on the client.  The client-gw gets the server's
 * host and port from recvfrom.  (The client can't use the same method since
 * these sockets are one-way only, so it connect()s to gw_port (udp5001) on
 * the gateway machine regardless of what the server sends.)
 * 
 * So all we need in .gwrc is: udp 5000 5001 tde.uts 5100
 * 
 * assuming the client is on tde.uts.  Note that a UDP declaration will work for
 * ANY server, but you need one per player, and the client has to have the
 * port numbers in advance.
 * 
 * If we're using a standard server, we're set.  If we're running through a
 * gatewayed server, we have to do some unpleasant work on the server side... */
#endif

void    handleMessage(struct mesg_spacket *packet);
void    handlePlyrInfo(struct plyr_info_spacket *packet);
void    handleKills(struct kills_spacket *packet);
void    handlePlayer(struct player_spacket *packet);
void    handleTorpInfo(struct torp_info_spacket *packet);
void    handleTorp(struct torp_spacket *packet);
void    handlePhaser(struct phaser_spacket *packet);
void    handlePlasmaInfo(struct plasma_info_spacket *packet);
void    handlePlasma(struct plasma_spacket *packet);
void    handleWarning(struct warning_spacket *packet);
void    handleMotd(struct motd_spacket *packet);
void    handleSelf(struct you_spacket *packet);
void    handleQueue(struct queue_spacket *packet);
void    handleStatus(struct status_spacket *packet);
void    handlePlanet(struct planet_spacket *packet);
void    handlePickok(struct pickok_spacket *packet);
void    handleLogin(struct login_spacket *packet);
void    handleFlags(struct flags_spacket *packet);
void    handleMask(struct mask_spacket *packet);
void    handlePStatus(struct pstatus_spacket *packet);
void    handleBadVersion(struct badversion_spacket *packet);
void    handleHostile(struct hostile_spacket *packet);
void    handleStats(struct stats_spacket *packet);
void    handlePlyrLogin(struct plyr_login_spacket *packet, int sock);
void    handleReserved(struct reserved_spacket *packet, int sock);
void    handlePlanetLoc(struct planet_loc_spacket *packet);

#ifdef HANDLE_SCAN
void    handleScan(struct scan_spacket *packet);

#endif

void    handleUdpReply(struct udp_reply_spacket *packet);
void    handleSequence(struct sequence_spacket *packet);

#ifdef RSA
void    handleRSAKey(struct rsa_key_spacket *packet);
extern void rsa_black_box(unsigned char *, unsigned char *, unsigned char *, unsigned char *);
#else
void    handleRSAKey(void *packet);
#endif

void    handleShipCap(struct ship_cap_spacket *packet);
static void handleGeneric32(struct generic_32_spacket *packet);
extern void handlePing(struct ping_spacket *packet);	/* ping.c */
extern void terminate(int error);

#ifdef SHORT_PACKETS
extern void handleShortReply(struct shortreply_spacket *packet);
extern void handleSMessage(struct mesg_s_spacket *packet);
extern void handleSWarning(struct warning_s_spacket *packet);
extern void handleSelfShort(struct youshort_spacket *packet);
extern void handleSelfShip(struct youss_spacket *packet);
extern void handleVPlayer(unsigned char *sbuf);
extern void handleVTorp(unsigned char *sbuf);
extern void handleVTorpInfo(unsigned char *sbuf);
extern void handleVPlanet(unsigned char *sbuf);

/* S_P2 */
extern void handleVPhaser(unsigned char *sbuf);
extern void handleVKills(unsigned char *sbuf);
extern void handle_s_Stats(struct stats_s_spacket *packet);

#endif /* SHORT_PACKETS */

#ifdef FEATURE_PACKETS
extern void handleFeature(struct feature_cpacket *packet);

#endif
void handleRank (struct rank_spacket *packet);
void handleLtd (struct ltd_spacket *packet);

static void pickSocket(int old);
static int connUdpConn(void);
static int openUdpConn(void);
static int recvUdpConn(void);
static int doRead(int asock);

char   *strcpyp_return(register char *s1, register char *s2, register int length);

void
        dummy(void)
{
}

#ifdef SHORT_PACKETS
unsigned char numofbits[256] =
{0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1,
 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1,
 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2,
 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1,
 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2,
 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2,
 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3,
 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1,
 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2,
 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2,
 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3,
 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2,
 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3,
 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3,
 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4,
 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

static int vtsize[9] =
{4, 8, 8, 12, 12, 16, 20, 20, 24};		 /* How big is the torppacket 

						  * 
						  * 
						  */
int     vtisize[9] =
{4, 7, 9, 11, 13, 16, 18, 20, 22};		 /* 4 byte Header + torpdata */

/* S_P2 */
int     shortversion = SHORTVERSION;		 /* Which version do we use? */

#endif /* SHORT_PACKETS */

#ifdef PACKET_LOG
void Log_Packet(char type, int act_size);
void Log_OPacket(int tpe, int size);
void print_opacket(char *packet, int size);
void print_packet(char *packet, int size);
#endif

struct packet_handler handlers[] =
{
  {0, NULL},					 /* record 0 */
  {sizeof(struct mesg_spacket), handleMessage},	 /* SP_MESSAGE */
  {sizeof(struct plyr_info_spacket), handlePlyrInfo},	/* SP_PLAYER_INFO */
  {sizeof(struct kills_spacket), handleKills},	 /* SP_KILLS */
  {sizeof(struct player_spacket), handlePlayer}, /* SP_PLAYER */
  {sizeof(struct torp_info_spacket), handleTorpInfo},	/* SP_TORP_INFO */
  {sizeof(struct torp_spacket), handleTorp},	 /* SP_TORP */
  {sizeof(struct phaser_spacket), handlePhaser}, /* SP_PHASER */
  {sizeof(struct plasma_info_spacket), handlePlasmaInfo},	/* SP_PLASMA_INFO 
								 * 
								 */
  {sizeof(struct plasma_spacket), handlePlasma}, /* SP_PLASMA */
  {sizeof(struct warning_spacket), handleWarning},	/* SP_WARNING */
  {sizeof(struct motd_spacket), handleMotd},	 /* SP_MOTD */
  {sizeof(struct you_spacket), handleSelf},	 /* SP_YOU */
  {sizeof(struct queue_spacket), handleQueue},	 /* SP_QUEUE */
  {sizeof(struct status_spacket), handleStatus}, /* SP_STATUS */
  {sizeof(struct planet_spacket), handlePlanet}, /* SP_PLANET */
  {sizeof(struct pickok_spacket), handlePickok}, /* SP_PICKOK */
  {sizeof(struct login_spacket), handleLogin},	 /* SP_LOGIN */
  {sizeof(struct flags_spacket), handleFlags},	 /* SP_FLAGS */
  {sizeof(struct mask_spacket), handleMask},	 /* SP_MASK */
  {sizeof(struct pstatus_spacket), handlePStatus},	/* SP_PSTATUS */
  {sizeof(struct badversion_spacket), handleBadVersion},	/* SP_BADVERSION 
								 * 
								 */
  {sizeof(struct hostile_spacket), handleHostile},	/* SP_HOSTILE */
  {sizeof(struct stats_spacket), handleStats},	 /* SP_STATS */
  {sizeof(struct plyr_login_spacket), handlePlyrLogin},		/* SP_PL_LOGIN 
								 * 
								 */
  {sizeof(struct reserved_spacket), handleReserved},	/* SP_RESERVED */
  {sizeof(struct planet_loc_spacket), handlePlanetLoc},		/* SP_PLANET_LOC 
								 * 
								 */

#ifdef HANDLE_SCAN
  {sizeof(struct scan_spacket), handleScan}	 /* SP_SCAN (ATM) */
#else
  {0, dummy},					 /* won't be called */
#endif

  {sizeof(struct udp_reply_spacket), handleUdpReply},	/* SP_UDP_STAT */
  {sizeof(struct sequence_spacket), handleSequence},	/* SP_SEQUENCE */
  {sizeof(struct sc_sequence_spacket), handleSequence},		/* SP_SC_SEQUENCE 
								 * 
								 */

#ifdef RSA
  {sizeof(struct rsa_key_spacket), handleRSAKey},	/* SP_RSA_KEY */
#else
  {0, dummy},					 /* #31, and dummy won't */
#endif

  {sizeof(struct generic_32_spacket), handleGeneric32}, /* SP_GENERIC_32 */
  {0, dummy},					 /* 33 */
  {0, dummy},					 /* 34 */
  {0, dummy},					 /* 35 */
  {0, dummy},					 /* 36 */
  {0, dummy},					 /* 37 */
  {0, dummy},					 /* 38 */
  {sizeof(struct ship_cap_spacket), handleShipCap},	/* SP_SHIP_CAP */

#ifdef SHORT_PACKETS
  {sizeof(struct shortreply_spacket), handleShortReply},	/* SP_S_REPLY 
								 * 
								 */
  {-1, handleSMessage},				 /* SP_S_MESSAGE */
  {-1						 /* sizeof(struct *
						  * warning_s_spacket) */ , handleSWarning},
						 /* SP_S_WARNING */
  {sizeof(struct youshort_spacket), handleSelfShort},	/* SP_S_YOU */
  {sizeof(struct youss_spacket), handleSelfShip},	/* SP_S_YOU_SS */
  {-1, /* variable */ handleVPlayer},		 /* SP_S_PLAYER */
#else
  {0, dummy},					 /* 40 */
  {0, dummy},					 /* 41 */
  {0, dummy},					 /* 42 */
  {0, dummy},					 /* 43 */
  {0, dummy},					 /* 44 */
  {0, dummy},					 /* 45 */
#endif
  {sizeof(struct ping_spacket), handlePing},	 /* SP_PING */

#ifdef SHORT_PACKETS
  {-1, /* variable */ handleVTorp},		 /* SP_S_TORP */
  {-1, handleVTorpInfo},			 /* SP_S_TORP_INFO */
  {20, handleVTorp},				 /* SP_S_8_TORP */
  {-1, handleVPlanet},				 /* SP_S_PLANET */
#else
  {0, dummy},					 /* 47 */
  {0, dummy},					 /* 48 */
  {0, dummy},					 /* 49 */
  {0, dummy},					 /* 50 */
#endif

  {0, dummy},					 /* 51 */
  {0, dummy},					 /* 52 */
  {0, dummy},					 /* 53 */
  {0, dummy},					 /* 54 */
  {0, dummy},					 /* 55 */

#ifdef SHORT_PACKETS				 /* S_P2 */
  {0, dummy},					 /* SP_S_SEQUENCE not yet * * 
						  * implemented */
  {-1, handleVPhaser},				 /* SP_S_PHASER */
  {-1, handleVKills},				 /* SP_S_KILLS */
  {sizeof(struct stats_s_spacket), handle_s_Stats},	/* SP_S_STATS */
#else
  {0, dummy},					 /* 56 */
  {0, dummy},					 /* 57 */
  {0, dummy},					 /* 58 */
  {0, dummy},					 /* 59 */
#endif

#ifdef FEATURE_PACKETS
  {sizeof(struct feature_cpacket), handleFeature},	/* SP_FEATURE; 60 */
#endif
  {sizeof(struct rank_spacket), handleRank},	/* SP_RANK; 61 */
  {sizeof(struct ltd_spacket), handleLtd},	/* SP_LTD; 62 */

};

int     sizes[] =
{
  0,						 /* record 0 */
  sizeof(struct mesg_cpacket),			 /* CP_MESSAGE */
  sizeof(struct speed_cpacket),			 /* CP_SPEED */
  sizeof(struct dir_cpacket),			 /* CP_DIRECTION */
  sizeof(struct phaser_cpacket),		 /* CP_PHASER */
  sizeof(struct plasma_cpacket),		 /* CP_PLASMA */
  sizeof(struct torp_cpacket),			 /* CP_TORP */
  sizeof(struct quit_cpacket),			 /* CP_QUIT */
  sizeof(struct login_cpacket),			 /* CP_LOGIN */
  sizeof(struct outfit_cpacket),		 /* CP_OUTFIT */
  sizeof(struct war_cpacket),			 /* CP_WAR */
  sizeof(struct practr_cpacket),		 /* CP_PRACTR */
  sizeof(struct shield_cpacket),		 /* CP_SHIELD */
  sizeof(struct repair_cpacket),		 /* CP_REPAIR */
  sizeof(struct orbit_cpacket),			 /* CP_ORBIT */
  sizeof(struct planlock_cpacket),		 /* CP_PLANLOCK */
  sizeof(struct playlock_cpacket),		 /* CP_PLAYLOCK */
  sizeof(struct bomb_cpacket),			 /* CP_BOMB */
  sizeof(struct beam_cpacket),			 /* CP_BEAM */
  sizeof(struct cloak_cpacket),			 /* CP_CLOAK */
  sizeof(struct det_torps_cpacket),		 /* CP_DET_TORPS */
  sizeof(struct det_mytorp_cpacket),		 /* CP_DET_MYTORP */
  sizeof(struct copilot_cpacket),		 /* CP_COPILOT */
  sizeof(struct refit_cpacket),			 /* CP_REFIT */
  sizeof(struct tractor_cpacket),		 /* CP_TRACTOR */
  sizeof(struct repress_cpacket),		 /* CP_REPRESS */
  sizeof(struct coup_cpacket),			 /* CP_COUP */
  sizeof(struct socket_cpacket),		 /* CP_SOCKET */
  sizeof(struct options_cpacket),		 /* CP_OPTIONS */
  sizeof(struct bye_cpacket),			 /* CP_BYE */
  sizeof(struct dockperm_cpacket),		 /* CP_DOCKPERM */
  sizeof(struct updates_cpacket),		 /* CP_UPDATES */
  sizeof(struct resetstats_cpacket),		 /* CP_RESETSTATS */
  sizeof(struct reserved_cpacket),		 /* CP_RESERVED */

#ifdef INCLUDE_SCAN
  sizeof(struct scan_cpacket),			 /* CP_SCAN (ATM) */
#else
  0,
#endif
  sizeof(struct udp_req_cpacket),		 /* CP_UDP_REQ */
  sizeof(struct sequence_cpacket),		 /* CP_SEQUENCE */

#ifdef RSA
  sizeof(struct rsa_key_cpacket),		 /* CP_RSA_KEY */
#else
  0,						 /* 37 */
#endif

  0,						 /* 38 */
  0,						 /* 39 */
  0,						 /* 40 */
  0,						 /* 41 */

#ifdef PING
  sizeof(struct ping_cpacket),			 /* CP_PING_RESPONSE */
#else
  0,
#endif

#ifdef SHORT_PACKETS
  sizeof(struct shortreq_cpacket),		 /* CP_S_REQ */
  sizeof(struct threshold_cpacket),		 /* CP_S_THRS */
  -1,						 /* CP_S_MESSAGE */
#else
  0,						 /* 43 */
  0,						 /* 44 */
  0,						 /* 45 */
#endif

  0,						 /* 46 */
  0,						 /* 47 */
  0,						 /* 48 */
  0,						 /* 49 */
  0,						 /* 50 */
  0,						 /* 51 */
  0,						 /* 52 */
  0,						 /* 53 */
  0,						 /* 54 */
  0,						 /* 55 */
  0,						 /* 56 */
  0,						 /* 57 */
  0,						 /* 58 */
  0,						 /* 59 */

#ifdef FEATURE_PACKETS
  sizeof(struct feature_cpacket),		 /* CP_FEATURE; 60 */
#endif

};

#define NUM_PACKETS (sizeof(handlers) / sizeof(handlers[0]) - 1)
#define NUM_SIZES (sizeof(sizes) / sizeof(sizes[0]) - 1)


#ifdef PACKET_LOG
/* stuff useful for logging server packets to see how much bandwidth netrek
 * is really using */
int     log_packets = 0;			 /* whether or not to be
						  * logging packets */
int     packet_log[NUM_PACKETS];		 /* number of packets logged */
int     outpacket_log[NUM_SIZES];
int     ALL_BYTES = 0;				 /* To log all bytes */
#endif /* PACKET_LOG */

int     serverDead = 0;

#define BUFSIZE 1024
char    buf[BUFSIZE];

static int udpLocalPort = 0;
static int udpServerPort = 0;
static LONG serveraddr = 0;
static LONG sequence = 0;
static int drop_flag = 0;
static int chan = -1;				 /* tells sequence checker *

						  * 
						  * * where packet is from */
static short fSpeed, fDirection, fShield, fOrbit, fRepair, fBeamup, fBeamdown,
        fCloak, fBomb, fDockperm, fPhaser, fPlasma, fPlayLock, fPlanLock,
        fTractor, fRepress;

/* internal prototypes */
static void dotimers(void);

/* print the SP_S_TORP* packets.  */
/* sbuf = pointer to packet buff  */
/* type=1 print SP_S_TORP         */
/* type=2 print SP_S_8_TORP       */
/* type=3 print SP_S_TORP_INFO    */
void print_sp_s_torp(char *sbuf, int type)
{
  unsigned char which, *data, infobitset, *infodata;
  unsigned char bitset;         
  int     dx, dy;
  int     shiftvar;
  int     i;
  char    status, war;
  register int shift = 0;                      /* How many torps are 
                                                * * extracted (for shifting) */

  if ( (type == 1) || (type == 2) )
    {
      /* now we must find the data ... :-) */
      if (sbuf[0] == SP_S_8_TORP)
	{						 /* MAX packet */
	  bitset = 0xff;
	  which = sbuf[1];
	  data = (unsigned char *) &sbuf[2];
	}
      else
	{						 /* Normal Packet */
	  bitset = sbuf[1];
	  which = sbuf[2];
	  data = (unsigned char *) &sbuf[3];
	}
      fprintf(stderr, "  bitset=0x%0X, which=%d, ", bitset, which);
#ifdef CORRUPTED_PACKETS
      /* we probably should do something clever here - jmn */
#endif
      
      for (shift = 0, i = 0; i < 8; i++, bitset >>= 1)
	{
	  if (bitset & 01)
	    {
	      dx = (*data >> shift);
	      data++;
	      shiftvar = (unsigned char) *data;	 /* to silence gcc */
	      shiftvar <<= (8 - shift);
	      dx |= (shiftvar & 511);
	      shift++;
	      dy = (*data >> shift);
	      data++;
	      shiftvar = (unsigned char) *data;	 /* to silence gcc */
	      shiftvar <<= (8 - shift);
	      dy |= (shiftvar & 511);
	      shift++;
	      if (shift == 8)
		{
		  shift = 0;
		  data++;
		}
	      fprintf(stderr, "dx=%d, dy=%d, ",dx, dy);
	    }
	}
    }
  else if (type == 3)
    {
      /* now we must find the data ... :-) */
      bitset = sbuf[1];
      which = sbuf[2];
      infobitset = sbuf[3];
      /* Where is the data ? */
      data = (unsigned char *) &sbuf[4];

      fprintf(stderr, "  bitset=0x%0X, which=%d, infobitset=0x%0X, ",
	      bitset, which, infobitset);

      infodata = (unsigned char *) &sbuf[vtisize[numofbits[(unsigned char) sbuf[1]]]];
      
      for (shift = 0, i = 0; i < 8; bitset >>= 1, infobitset >>= 1, i++)
	{
	  if (bitset & 01)
	    {
	      dx = (*data >> shift);
	      data++;
	      shiftvar = (unsigned char) *data;	 /* to silence gcc */
	      shiftvar <<= (8 - shift);
	      dx |= (shiftvar & 511);
	      shift++;
	      dy = (*data >> shift);
	      data++;
	      shiftvar = (unsigned char) *data;	 /* to silence gcc */
	      shiftvar <<= (8 - shift);
	      dy |= (shiftvar & 511);
	      shift++;
	      if (shift == 8)
		{
		  shift = 0;
		  data++;
		}
	      fprintf(stderr, "dx=%d, dy=%d, ",dx, dy);
	    } 

	  /* Now the TorpInfo */
	  if (infobitset & 01)
	    {
	      war = (unsigned char) *infodata & 15 /* 0x0f */ ;
	      status = ((unsigned char) *infodata & 0xf0) >> 4;
	      infodata++;
	      fprintf(stderr, "war=0x%0X, status=0x%0X, ", war, status);
	    }					 /* if */
	  
	}						 /* for */
      
    }
}

/* reset all the "force command" variables */
static void resetForce(void)
{
  fSpeed = fDirection = fShield = fOrbit = fRepair = fBeamup = fBeamdown =
      fCloak = fBomb = fDockperm = fPhaser = fPlasma = fPlayLock = fPlanLock =
      fTractor = fRepress = -1;
}

/* If something we want to happen hasn't yet, send it again.
 * 
 * The low byte is the request, the high byte is a max count.  When the max
 * count reaches zero, the client stops trying.  Checking is done with a
 * macro for speed & clarity. */
#define FCHECK_FLAGS(flag, force, const) {                      \
        if (force > 0) {                                        \
            if (((me->p_flags & flag) && 1) ^ ((force & 0xff) && 1)) {  \
                speedReq.type = const;                          \
                speedReq.speed = (force & 0xff);                \
                sendServerPacket((struct player_spacket *) &speedReq);  \
                V_UDPDIAG(("Forced %d:%d\n", const, force & 0xff));     \
                force -= 0x100;                                 \
                if (force < 0x100) force = -1;  /* give up */   \
            } else                                              \
                force = -1;                                     \
        }                                                       \
}
#define FCHECK_VAL(value, force, const) {                       \
        if (force > 0) {                                        \
            if ((value) != (force & 0xff)) {                    \
                speedReq.type = const;                          \
                speedReq.speed = (force & 0xff);                \
                sendServerPacket((struct player_spacket *) &speedReq);  \
                V_UDPDIAG(("Forced %d:%d\n", const, force & 0xff));     \
                force -= 0x100;                                 \
                if (force < 0x100) force = -1;  /* give up */   \
            } else                                              \
                force = -1;                                     \
        }                                                       \
}
#define FCHECK_TRACT(flag, force, const) {                      \
        if (force > 0) {                                        \
            if (((me->p_flags & flag) && 1) ^ ((force & 0xff) && 1)) {  \
                tractorReq.type = const;                        \
                tractorReq.state = ((force & 0xff) >= 0x40);    \
                tractorReq.pnum = (force & 0xff) & (~0x40);     \
                sendServerPacket((struct player_spacket *)&tractorReq); \
                V_UDPDIAG(("Forced %d:%d/%d\n", const,          \
                        tractorReq.state, tractorReq.pnum));    \
                force -= 0x100;                                 \
                if (force < 0x100) force = -1;  /* give up */   \
            } else                                              \
                force = -1;                                     \
        }                                                       \
}

void checkForce(void)
{
  struct speed_cpacket speedReq;
  struct tractor_cpacket tractorReq;

  FCHECK_VAL(me->p_speed, fSpeed, CP_SPEED);	 /* almost always repeats */

#ifdef nodef					 /* not needed */
  FCHECK_VAL(me->p_dir, fDirection, CP_DIRECTION);	/* (ditto) */
#endif

  FCHECK_FLAGS(PFSHIELD, fShield, CP_SHIELD);
  FCHECK_FLAGS(PFORBIT, fOrbit, CP_ORBIT);
  FCHECK_FLAGS(PFREPAIR, fRepair, CP_REPAIR);
  FCHECK_FLAGS(PFBEAMUP, fBeamup, CP_BEAM);
  FCHECK_FLAGS(PFBEAMDOWN, fBeamdown, CP_BEAM);
  FCHECK_FLAGS(PFCLOAK, fCloak, CP_CLOAK);
  FCHECK_FLAGS(PFBOMB, fBomb, CP_BOMB);
  FCHECK_FLAGS(PFDOCKOK, fDockperm, CP_DOCKPERM);
  FCHECK_VAL(phasers[me->p_no].ph_status, fPhaser, CP_PHASER);	/* bug: dir 0 
								 * 
								 */
  FCHECK_VAL(plasmatorps[me->p_no].pt_status, fPlasma, CP_PLASMA);	/* (ditto) 
									 * 
									 */
  FCHECK_FLAGS(PFPLOCK, fPlayLock, CP_PLAYLOCK);
  FCHECK_FLAGS(PFPLLOCK, fPlanLock, CP_PLANLOCK);

  FCHECK_TRACT(PFTRACT, fTractor, CP_TRACTOR);
  FCHECK_TRACT(PFPRESS, fRepress, CP_REPRESS);
}

void connectToServer(int port)
{
  int     s;
  struct sockaddr_in addr;
  struct sockaddr_in naddr;
  socklen_t len;
  fd_set  readfds;
  struct timeval timeout;
  struct hostent *hp;
  int     optval;

  serverDead = 0;
  if (sock != -1)
    {
      shutdown(sock, 2);
      close(sock);
      sock = -1;
    }

#ifdef nodef
  sleep(3);					 /* I think this is necessary
						  * * * for some unknown
						  * reason */
#endif

  printf("Waiting for connection (port %d). \n", port);

  if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
      printf("I can't create a socket\n");
      terminate(2);
    }

  /* allow local address resuse */
  optval = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &optval, sizeof(int))
      < 0)
    {
      perror("setsockopt");
    }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if (bind(s, (struct sockaddr *) &addr, sizeof(addr)) < 0)
    {

#ifdef nodef
      sleep(10);
      if (bind(s, &addr, sizeof(addr)) < 0)
	{
	  sleep(10);
	  if (bind(s, &addr, sizeof(addr)) < 0)
	    {
	      printf("I can't bind to port!\n");
	      terminate(3);
	    }
	}
#endif

      perror("bind");				 /* NEW */
      terminate(1);
    }
  if (listen(s, 1) < 0)
    perror("listen");

  len = sizeof(naddr);

tryagain:
  timeout.tv_sec = 240;				 /* four minutes */
  timeout.tv_usec = 0;
  FD_ZERO(&readfds);
  FD_SET(s, &readfds);

  if (s >= max_fd)
    max_fd = s + 1;

  if (SELECT(max_fd, &readfds, NULL, NULL, &timeout) == 0)
    {
      printf("Well, I think the server died!\n");
      terminate(0);
    }

  sock = accept(s, (struct sockaddr *) &naddr, &len);

  if (sock == -1)
    {
      goto tryagain;
    }

  if (sock >= max_fd)
    max_fd = sock + 1;

  printf("Got connection.\n");

  close(s);
  pickSocket(port);				 /* new socket != port */


  /* This is necessary; it tries to determine who the caller is, and set * *
   * "serverName" and "serveraddr" appropriately. */
  len = sizeof(struct sockaddr_in);

  if (getpeername(sock, (struct sockaddr *) &addr, &len) < 0)
    {
      perror("unable to get peername");
      serverName = "nowhere";
    }
  else
    {
      serveraddr = addr.sin_addr.s_addr;
      hp = gethostbyaddr((char *) &addr.sin_addr.s_addr, sizeof(LONG), AF_INET);
      if (hp != NULL)
	{
	  serverName = (char *) malloc(strlen(hp->h_name) + 1);
	  strcpy(serverName, hp->h_name);
	}
      else
	{
	  serverName = (char *) malloc(strlen(inet_ntoa(addr.sin_addr)) + 1);
	  strcpy(serverName, inet_ntoa(addr.sin_addr));
	}
    }
  printf("Connection from server %s (0x%lx)\n", serverName, serveraddr);

}


static void set_tcp_opts(int s)
{
  int     optval = 1;
  struct protoent *ent;

  ent = getprotobyname("TCP");
  if (!ent)
    {
      fprintf(stderr, "TCP protocol not found.\n");
      return;
    }
  if (setsockopt(s, ent->p_proto, TCP_NODELAY, &optval, sizeof(int)) < 0)
            perror("setsockopt");
}

#ifdef nodef
static void set_udp_opts(int s)
{
  int     optval = BUFSIZ;
  struct protoent *ent;

  ent = getprotobyname("UDP");
  if (!ent)
    {
      fprintf(stderr, "UDP protocol not found.\n");
      return;
    }
  if (setsockopt(s, ent->p_proto, SO_RCVBUF, &optval, sizeof(int)) < 0)
            perror("setsockopt");
}
#endif

void callServer(int port, char *server)
{
  int     s;
  struct sockaddr_in addr;
  struct hostent *hp;

  serverDead = 0;

  fprintf(stderr, "connecting to %s on port %d\n", server, port);

  if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror("failed in socket");
      terminate(1);
    }
  set_tcp_opts(s);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if ((addr.sin_addr.s_addr = inet_addr(server)) == -1)
    {
      if ((hp = gethostbyname(server)) == NULL)
	{
	  printf("unable to resolve hostname %s\n", server);
	  terminate(1);
	}
      else
	{
	  addr.sin_addr.s_addr = *(LONG *) hp->h_addr;
	}
    }
  serveraddr = addr.sin_addr.s_addr;

  if (connect(s, (struct sockaddr *) &addr, sizeof(addr)) < 0)
    {
      perror("failed in connect");
      fprintf(stderr, "connection failed\n");
      terminate(1);
    }
  fprintf(stderr, "connected\n");

  sock = s;

  if (sock >= max_fd)
    max_fd = sock + 1;

#ifdef TREKHOPD
  /* We use a different scheme from gw: we tell the server who we want to * * 
   * connect to and what our local UDP port is; it picks its own UDP ports *
   * * and tells us what they are.  This is MUCH more flexible and avoids a *
   * * number of problems, and may come in handy if the blessed scheme
   * changes. * * It's also slightly more work. */
  {
    extern int port_req, use_trekhopd, serv_port;
    extern char *host_req;
    struct mesg_cpacket msg;
    struct mesg_spacket reply;
    int     n, count, *ip;
    char   *buf;

    if (use_trekhopd)
      {
	msg.type = SP_MESSAGE;
	msg.group = msg.indiv = msg.pad1 = 0;
	ip = (int *) msg.mesg;
	*(ip++) = htons(port_req);
	*(ip++) = htons(gw_local_port);
	STRNCPY(msg.mesg + 8, login, 8);
	strcpy(msg.mesg + 16, host_req);
	if (gwrite(s, &msg, sizeof(struct mesg_cpacket)) < 0)
	  {
	    fprintf(stderr, "trekhopd init failure\n");
	    terminate(1);
	  }
	printf("--- trekhopd request sent, awaiting reply\n");
	/* now block waiting for reply */
	count = sizeof(struct mesg_spacket);

	for (buf = (char *) &reply; count; buf += n, count -= n)
	  {
	    if ((n = read(s, buf, count)) <= 0)
	      {
		perror("trekhopd read");
		terminate(1);
	      }
	  }

	if (reply.type != SP_MESSAGE)
	  {
	    fprintf(stderr, "Got bogus reply from trekhopd (%d)\n",
		    reply.type);
	    terminate(1);
	  }
	ip = (int *) reply.mesg;
	gw_serv_port = ntohl(*ip++);
	gw_port = ntohl(*ip++);
	serv_port = ntohl(*ip++);
	printf("--- trekhopd reply received\n");

	/* printf("ports = %d/%d, %d\n", gw_serv_port, gw_port, serv_port); */
      }
  }
#endif /* TREKHOPD */

  pickSocket(port);				 /* new socket != port */
}

int isServerDead(void)
{
  return serverDead;
}

void socketPauseCommon(int wake_on_user_input)
{
  struct timeval timeout;
  fd_set  readfds;
  int retval;

  timeout.tv_sec = 1;
  timeout.tv_usec = 0;

  FD_ZERO(&readfds);
  FD_SET(sock, &readfds);
  if (udpSock >= 0)				 /* new */
    FD_SET(udpSock, &readfds);
#ifndef HAVE_WIN32
  if (wake_on_user_input)
    FD_SET(W_Socket(), &readfds);
#endif

  retval = SELECT(max_fd, &readfds, 0, 0, &timeout);
  if (retval < 0) perror("select"); /* FIXME: EBADF on kill of ntserv during outfit */
}

void socketPauseNoUser(void)
{
  socketPauseCommon(0);
}

void socketPause(void)
{
  socketPauseCommon(1);
}

int readFromServer(fd_set *readfds)
{
  int     retval = 0;

  if (serverDead)
    return 0;

  if (!readfds)
    {
      struct timeval timeout;
      int     rs;
      fd_set  mask;

      readfds = &mask;

      FD_ZERO(readfds);
      FD_SET(sock, readfds);
      if (udpSock >= 0)
	FD_SET(udpSock, readfds);
      timeout.tv_sec = 0;
      timeout.tv_usec = 0;
      if ((rs = SELECT(max_fd, readfds, 0, 0, &timeout)) == 0)
	{
	  dotimers();
	  return 0;
	}
      if (rs < 0) {
	if (errno == EINTR) return 0;
	perror("select");
	sleep(1);
      }
    }
  if (udpSock >= 0 && FD_ISSET(udpSock, readfds))
    {
      /* WAS V_ */
      UDPDIAG(("Activity on UDP socket\n"));
      chan = udpSock;
      if (commStatus == STAT_VERIFY_UDP)
	{
	  sequence = 0;				 /* reset sequence #s */
	  resetForce();

	  printUdpInfo();
	  UDPDIAG(("UDP connection established.\n"));

	  commMode = COMM_UDP;
	  commStatus = STAT_CONNECTED;
	  commSwitchTimeout = 0;
	  if (udpClientRecv != MODE_SIMPLE)
	    sendUdpReq(COMM_MODE + udpClientRecv);
	  if (udpWin)
	    {
	      udprefresh(UDP_CURRENT);
	      udprefresh(UDP_STATUS);
	    }
	}
      retval += doRead(udpSock);
    }

  /* Read info from the xtrek server */
  if (FD_ISSET(sock, readfds))
    {
      chan = sock;
      if (commMode == COMM_TCP)
	drop_flag = 0;				 /* just in case */
      /* Bug fix for unnecessary redraws with UDP on - reported by TP */
      //      if (commMode == COMM_UDP)
      //	doRead (sock);
      //      else
	retval += doRead (sock);
    }

  dotimers();
  return retval != 0;				 /* convert to 1/0 */
}

static void dotimers(void)
{
  /* if switching comm mode, decrement timeout counter */
  if (commSwitchTimeout > 0)
    {
      if (!(--commSwitchTimeout))
	{
	  /* timed out; could be initial request to non-UDP server (which * * 
	   * won't be answered), or the verify packet got lost en route to *
	   * * the server.  Could also be a request for TCP which timed out *
	   * * (weird), in which case we just reset anyway. */
	  commModeReq = commMode = COMM_TCP;
	  commStatus = STAT_CONNECTED;
	  if (udpSock >= 0)
	    closeUdpConn();
	  if (udpWin)
	    {
	      udprefresh(UDP_CURRENT);
	      udprefresh(UDP_STATUS);
	    }
	  warning("Timed out waiting for UDP response from server");
	  UDPDIAG(("Timed out waiting for UDP response from server\n"));
	}
    }
  /* if we're in a UDP "force" mode, check to see if we need to do something */
  if (udpClientSend > 1 && commMode == COMM_UDP)
    checkForce();
}

int
        getvpsize(char *bufptr)
{
  int     size;

  switch (*bufptr)
    {
    case SP_S_MESSAGE:
      size = ((unsigned char) bufptr[4]);	 /* IMPORTANT  Changed */
      break;
    case SP_S_WARNING:
      if ((unsigned char) bufptr[1] == STEXTE_STRING ||
	  (unsigned char) bufptr[1] == SHORT_WARNING)
	{
	  size = ((unsigned char) bufptr[3]);
	}
      else
	size = 4;				 /* Normal Packet */
      break;
    case SP_S_PLAYER:
      if ((unsigned char) bufptr[1] & (unsigned char) 128)
	{					 /* Small +extended Header */
	  size = ((unsigned char) (bufptr[1] & 0x3f) * 4) + 4;
	}
      else if ((unsigned char) bufptr[1] & 64)
	{					 /* Small Header */
	  if (shortversion >= SHORTVERSION)	 /* S_P2 */
	    size = ((unsigned char) (bufptr[1] & 0x3f) * 4) + 4 + (bufptr[2] * 4);
	  else
	    size = ((unsigned char) (bufptr[1] & 0x3f) * 4) + 4;
	}
      else
	{					 /* Big Header */
	  size = ((unsigned char) bufptr[1] * 4 + 12);
	}
      break;
    case SP_S_TORP:
      size = vtsize[numofbits[(unsigned char) bufptr[1]]];
      break;
    case SP_S_TORP_INFO:
      size = (vtisize[numofbits[(unsigned char) bufptr[1]]]
	      + numofbits[(unsigned char) bufptr[3]]);
      break;
    case SP_S_PLANET:
      size = ((unsigned char) bufptr[1] * VPLANET_SIZE) + 2;
      break;
    case SP_S_PHASER:				 /* S_P2 */
      switch ((unsigned char) bufptr[1] & 0x0f)
	{
	case PHFREE:
	case PHHIT:
	case PHMISS:
	  size = 4;
	  break;
	case PHHIT2:
	  size = 8;
	  break;
	default:
	  size = sizeof(struct phaser_s_spacket);

	  break;
	}
      break;
    case SP_S_KILLS:				 /* S_P2 */
      size = ((unsigned char) bufptr[1] * 2) + 2;
      break;
    default:
      fprintf(stderr, "Unknown variable packet\n");
      /* terminate(1); */
      return -1;
      break;
    }
  if ((size % 4) != 0)
    {
      size += (4 - (size % 4));
    }

  return size;
}

static int doRead(int asock)
{
  struct timeval timeout;
  fd_set  readfds;
  char   *bufptr;
  int     size;
  int     count;
  int     temp;

  timeout.tv_sec = 0;
  timeout.tv_usec = 0;

  count = read(asock, buf, BUFSIZE);

#ifdef NETSTAT
  if (netstat &&
      (asock == udpSock ||
       commMode != COMM_UDP ||
       udpClientRecv == MODE_TCP))
    {
      ns_record_update(count);
    }
#endif

  if (debug)
    printf("read %d bytes from %s socket\n",
	   count, asock == udpSock ? "UDP" : "TCP");

  if (count <= 0)
    {
      if (asock == udpSock)
	{

#ifndef WIN32
	  if (errno == ECONNREFUSED)
#else
	  if (WSAGetLastError() == WSAECONNREFUSED)
#endif

	    {
	      struct sockaddr_in addr;

	      UDPDIAG(("asock=%d, sock=%d, udpSock=%d, errno=%d\n",
		       asock, sock, udpSock, errno));
	      UDPDIAG(("count=%d\n", count));
	      UDPDIAG(("Hiccup(%d)!  Reconnecting\n", errno));
	      addr.sin_addr.s_addr = serveraddr;
	      addr.sin_port = htons(udpServerPort);
	      addr.sin_family = AF_INET;
	      if (connect(udpSock, (struct sockaddr *) &addr, sizeof(addr)) < 0)
		{
		  perror("connect");
		  UDPDIAG(("Unable to reconnect\n"));
		  /* and fall through to disconnect */
		}
	      else
		{
		  UDPDIAG(("Reconnect successful\n"));
		  return 0;
		}
	    }
	  UDPDIAG(("*** UDP disconnected (res=%d, err=%d)\n",
		   count, errno));
	  warning("UDP link severed");
	  printUdpInfo();
	  closeUdpConn();
	  commMode = commModeReq = COMM_TCP;
	  commStatus = STAT_CONNECTED;
	  if (udpWin)
	    {
	      udprefresh(UDP_STATUS);
	      udprefresh(UDP_CURRENT);
	    }
	  return 0;
	}
      printf("server disconnected\n");
      close(asock);
      serverDead = 1;
      return 0;
    }
  bufptr = buf;
  while (bufptr < buf + count)
    {
      /* this goto label for a bug w/ short packets */
    computesize:
      if (*bufptr < 1 ||
          *bufptr > NUM_PACKETS ||
          handlers[(unsigned char) *bufptr].size == 0) {
        int i;

        fprintf(stderr, "netrek protocol stream alignment failure, "
                "next byte %d (0x%02x)\n", *bufptr, *bufptr);

        fprintf(stderr, "protocol buffer dump, bytes %d, [bufptr] at %d :\n",
                count, bufptr - buf);
        for (i = 0; i < count; i++) {
          if (i == (bufptr - buf)) {
            fprintf(stderr, "[%02x]", (unsigned int) buf[i] & 0xff);
          } else {
            fprintf(stderr, " %02x ", (unsigned int) buf[i] & 0xff);
          }
        }
        fprintf(stderr, "\n");
        return 0;
      }
      size = handlers[(unsigned char) *bufptr].size;

#ifdef SHORT_PACKETS
      if (size == -1)
	{					 /* variable packet */
	  size = getvpsize(bufptr);
	  if (size <= 0)
	    {
	      fprintf(stderr, "Bad short-packet size value (%d)\n", size);
	      return 0;
	    }

	  if (debug)
	    printf("read variable packet size %d, type %d\n",
		   size, *bufptr);
	}
#endif /* SHORT_PACKETS */

      if (size == 0)
	fprintf(stderr, "Variable packet has 0 length! type=%d Trying to read more!\n", *bufptr);
      /* read broke in the middle of a packet, wait until we get the rest */
      while (size > count + (buf - bufptr) || size == 0)
	{
	  /* We wait for up to ten seconds for rest of packet. If we don't *
	   * * get it, we assume the server died. */
	  timeout.tv_sec = 20;
	  timeout.tv_usec = 0;
	  FD_ZERO(&readfds);
	  FD_SET(asock, &readfds);
	  /* readfds=1<<asock; */
	  if ((temp = SELECT(max_fd, &readfds, 0, 0, &timeout)) == 0)
	    {
	      printf("Packet fragment.  Server must be dead\n");
	      serverDead = 1;
	      close(sock);
	      return 0;
	    }

	  /* 88=largest short packet, messages */
	  if (size == 0)
	    {
	      temp = read(asock, buf + count, 88);
	    }
	  else
	    temp = read(asock, buf + count, size - (count + (buf - bufptr)));
	  count += temp;
	  if (temp <= 0)
	    {
	      printf("2) Got read() of %d.  Server is dead\n", temp);
	      serverDead = 1;
	      close(sock);
	      return 0;
	    }
	  /* go back to the size computation, hopefully with the rest of the */
	  /* aborted packet in the buffer. */
	  if (size == 0)
	    goto computesize;
	}
      if (handlers[(unsigned char) *bufptr].handler != NULL)
	{
	  if (asock != udpSock ||
	      (!drop_flag || *bufptr == SP_SEQUENCE || *bufptr == SP_SC_SEQUENCE))
	    {
	      if (debug)
		printf("read packet %d\n", *bufptr);

#ifdef RECORDGAME
	      if (recordFile != NULL && ckRecordPacket(*bufptr))
		{
		  if (fwrite(bufptr, size, 1, recordFile) != 1)
		    {
		      perror("write: (recordFile)");
		      fclose(recordFile);
		      recordFile = NULL;
		    }
		}
#endif

#ifdef PACKET_LOG
	      if (log_packets)
		{
		  Log_Packet((char) (*bufptr), size);
		  print_packet(bufptr,size);
		}
#endif

	      if (asock == udpSock)
		packets_received++;

	      (*(handlers[(unsigned char) *bufptr].handler)) (bufptr

#ifdef CORRUPTED_PACKETS
					      ,asock
#endif

		  );
	    }
	  else
	    {
	      if (debug)
		{
		  if (drop_flag)
		    printf("%d bytes dropped.\n", size);
		}
	      UDPDIAG(("Ignored type %d\n", *bufptr));
	    }
	}
      else
	{
	  printf("Handler for packet %d not installed...\n", *bufptr);
	}

      bufptr += size;

#ifdef nodef
      if (bufptr > buf + BUFSIZ)
	{
	  MCOPY(buf + BUFSIZ, buf, BUFSIZ);
	  if (count == BUFSIZ * 2)
	    {
	      FD_ZERO(&readfds);
	      FD_SET(asock, &readfds);
	      /* readfds = 1<<asock; */
	      if ((temp = SELECT(max_fd, &readfds, 0, 0, &timeout)) > 0)
		{
		  temp = read(asock, buf + BUFSIZ, BUFSIZ);
		  count = BUFSIZ + temp;
		  if (temp <= 0)
		    {
		      printf("3) Got read() of %d.  Server is dead\n", temp);
		      serverDead = 1;
		      close(sock);
		      return 0;
		    }
		}
	      else
		{
		  count = BUFSIZ;
		}
	    }
	  else
	    {
	      count -= BUFSIZ;
	    }
	  bufptr -= BUFSIZ;
	}
#endif
    }
  return 1;
}

void    handleTorp(struct torp_spacket *packet)
{
  struct torp *thetorp;

#ifdef CORRUPTED_PACKETS
  if (ntohs(packet->tnum) >= MAXPLAYER * MAXTORP)
    {
      fprintf(stderr, "handleTorp: bad index %d\n", ntohs(packet->tnum));
      return;
    }
#endif

  weaponUpdate = 1;
  thetorp = &torps[ntohs(packet->tnum)];

  thetorp->t_x = ntohl(packet->x);
  thetorp->t_y = ntohl(packet->y);
  thetorp->t_dir = packet->dir;
  thetorp->t_updateFuse = TORP_UPDATE_FUSE * server_ups / 10;


#ifdef ROTATERACE
  if (rotate)
    {
      rotate_coord(&thetorp->t_x, &thetorp->t_y, rotate_deg,
		   GWIDTH / 2, GWIDTH / 2);
      rotate_dir(&thetorp->t_dir, rotate_deg);
    }
#endif

  if (gather_stats) {
    /*STATS_SP_TORP:DIR,TNUM,X,Y*/
    fprintf(statsFile, "\nSTATS_SP_TORP:\t");
    fprintf(statsFile, "%d\t%u\t%u\t%u",
            ((struct torp_spacket *) packet)->dir,
            ntohs(((struct torp_spacket *) packet)->tnum),
            ntohl(((struct torp_spacket *) packet)->x),
            ntohl(((struct torp_spacket *) packet)->y));
  }
}

void    handleTorpInfo(struct torp_info_spacket *packet)
{
  struct torp *thetorp;

#ifdef CORRUPTED_PACKETS
  if (ntohs(packet->tnum) >= MAXPLAYER * MAXTORP)
    {
      fprintf(stderr, "handleTorpInfo: bad index %d\n", ntohs(packet->tnum));
      return;
    }
#endif

  weaponUpdate = 1;
  thetorp = &torps[ntohs(packet->tnum)];
  thetorp->t_updateFuse = TORP_UPDATE_FUSE * server_ups / 10;

  if (packet->status == TEXPLODE && thetorp->t_status == TFREE)
    {
      /* FAT: redundant explosion; don't update p_ntorp */
      /* printf("texplode ignored\n"); */
      return;
    }

  if (packet->status && thetorp->t_status == TFREE)
    {
      players[thetorp->t_owner].p_ntorp++;

#ifdef nodef
      if (players[thetorp->t_owner].p_status == PEXPLODE)
	fprintf(stderr, "TORP STARTED WHEN PLAYER EXPLODED\n");
#endif

      /* BORG TEST */

#ifdef BD
      if (bd)
	bd_new_torp(ntohs(packet->tnum), thetorp);
#endif
    }
  if (packet->status == TFREE && thetorp->t_status)
    {
      players[thetorp->t_owner].p_ntorp--;
    }
  thetorp->t_war = packet->war;

  if (packet->status != thetorp->t_status)
    {
      /* FAT: prevent explosion reset */
      thetorp->t_status = packet->status;
      if (thetorp->t_status == TEXPLODE)
	{
	  thetorp->t_fuse = NUMDETFRAMES * server_ups / 10;
	}
    }
  if (gather_stats) {
    /*STATS_SP_TORP_INFO:WAR\tSTATUS\tTNUM*/
    fprintf(statsFile, "\nSTATS_SP_TORP_INFO:\t");
    fprintf(statsFile, "%d\t%d\t%u",
            ((struct torp_info_spacket *) packet)->war,
            ((struct torp_info_spacket *) packet)->status,
            ntohs(((struct torp_info_spacket *) packet)->tnum));
  }
}

void    handleStatus(struct status_spacket *packet)
{
  status->tourn = packet->tourn;
  status->armsbomb = ntohl(packet->armsbomb);
  status->planets = ntohl(packet->planets);
  status->kills = ntohl(packet->kills);
  status->losses = ntohl(packet->losses);
  status->time = ntohl(packet->time);
  status->timeprod = ntohl(packet->timeprod);

  if (debug)
    {
      printf("SERVER STATS:\n");
      printf("time: %d\n", status->time / (60 * 60 * 10));
      printf("kills: %d\n", status->kills);
      printf("losses: %d\n", status->losses);
      printf("planets: %d\n", status->planets);
      printf("armsbomb: %d\n", status->armsbomb);
    }

  if (gather_stats) {
    /*STATS_SP_STATUS:\tTOURN\tARMSBOMB\tPLANETS\tKILLS\tLOSSES\tTIME\tTIMEPROD*/
    fprintf(statsFile, "\nSTATS_SP_STATUS:\t");
    fprintf(statsFile, "\t%d\t%u\t%u\t%u\t%u\t%u\t%u",
            ((struct status_spacket *) packet)->tourn,
            ntohl(((struct status_spacket *) packet)->armsbomb),
            ntohl(((struct status_spacket *) packet)->planets),
            ntohl(((struct status_spacket *) packet)->kills),
            ntohl(((struct status_spacket *) packet)->losses),
            ntohl(((struct status_spacket *) packet)->time),
            ntohl(((struct status_spacket *) packet)->timeprod));
  }
}

void become(struct player *pl)
{
  int p_no;

  p_no = pl->p_no;
  memcpy(&pl->p_ship, &me->p_ship, sizeof(struct ship));
  memcpy(&pl->p_stats, &me->p_stats, sizeof(struct stats));
  memcpy(pl, me, sizeof(struct player));
  pl->p_no = p_no;
  me->p_status = PFREE;
}

void    handleSelf(struct you_spacket *packet) /* SP_YOU */
{
  struct player* pl;
  static int seen = 0;

  pl = &players[(unsigned char) packet->pnum];

#ifdef CORRUPTED_PACKETS
  if (packet->pnum < 0 || packet->pnum >= MAXPLAYER)
    {
      fprintf(stderr, "handleSelf: bad index %d\n", packet->pnum);
      return;
    }
#endif
  if (seen && packet->pnum != me->p_no) become(pl);
  seen++;

  if (!F_many_self)
    {
      me = (ghoststart ? &players[ghost_pno] : pl);
      myship = &(me->p_ship);
      mystats = &(me->p_stats);
    }

#ifdef PLIST2
  if (pl->p_hostile != packet->hostile)
    {
      pl->p_hostile = packet->hostile;
      PlistNoteHostile(packet->pnum);
    }
#else
  pl->p_hostile = packet->hostile;
#endif

  pl->p_swar = packet->swar;
  pl->p_armies = packet->armies;

#ifdef nodef
  if (((pl->p_flags & PFGREEN) && (ntohl(packet->flags) & PFRED)) ||
      ((pl->p_flags & PFRED) && (ntohl(packet->flags) & PFGREEN)))
    printf("green/red transition\n");
#endif

  pl->p_flags = ntohl(packet->flags);
  if (pl->p_flags & PFPLOCK)
    {
      redrawPlayer[pl->p_playerl] = 1;
    }
  pl->p_damage = ntohl(packet->damage);
  pl->p_shield = ntohl(packet->shield);
  pl->p_fuel = ntohl(packet->fuel);
  pl->p_etemp = ntohs(packet->etemp);
  pl->p_wtemp = ntohs(packet->wtemp);
  pl->p_whydead = ntohs(packet->whydead);
  pl->p_whodead = ntohs(packet->whodead);

#ifdef INCLUDE_VISTRACT
  if (packet->tractor & 0x40) {
    pl->p_tractor = (short) packet->tractor & (~0x40);	/* ATM - visible 
							 * tractors */
  }

#ifdef nodef					 /* tmp */
  else
    pl->p_tractor = -1;
#endif /* nodef */

#endif
}


void    handlePlayer(struct player_spacket *packet)
{
  register struct player *pl;
  int     x, y;

#ifdef CORRUPTED_PACKETS
  if (packet->pnum >= MAXPLAYER)
    {
      fprintf(stderr, "handlePlayer: bad index %d\n", packet->pnum);
      return;
    }
#endif


  pl = &players[(unsigned char) packet->pnum];

  pl->p_dir = packet->dir;
  pl->p_speed = packet->speed;
  PlistNoteSpeed(packet->pnum);

  if (F_cloak_maxwarp && pl != me)
    {
      if (pl->p_speed == 0xf)
	pl->p_flags |= PFCLOAK;
      else if (pl->p_flags & PFCLOAK)
	pl->p_flags &= ~PFCLOAK;
    }
  x = ntohl(packet->x);
  y = ntohl(packet->y);

#ifdef WARP_DEAD
  if (F_dead_warp && pl->p_speed == 14 && x == -10000 && y == -10000 && (pl->p_status != PEXPLODE))
    {
      pl->p_status = PEXPLODE;
      x = pl->p_x;
      y = pl->p_y;
      if (pl->p_dir > DEADPACKETS)
	pl->p_explode = EX_FRAMES;
      else
	pl->p_explode = 0;
      redrawPlayer[packet->pnum] = 1;
      PlistNoteUpdate(packet->pnum);
    }
#endif

  pl->p_x = x;
  pl->p_y = y;

  redrawPlayer[(unsigned char) packet->pnum] = 1;

  if (me == pl)
    {
      extern int my_x, my_y;			 /* From short.c */

      my_x = x;
      my_y = y;
    }


#ifdef ROTATERACE
  if (rotate)
    {
      rotate_coord(&pl->p_x, &pl->p_y, rotate_deg, GWIDTH / 2, GWIDTH / 2);
      rotate_dir(&pl->p_dir, rotate_deg);
    }
#endif

  if (gather_stats) {
    /* SP_PLAYER:PNUN,DIR,SPEED,X,Y */
    fprintf(statsFile, "\nSTATS_SP_PLAYER:\t");
    fprintf(statsFile, "%d\t%u\t%d\t%d\t%d",
            ((struct player_spacket *) packet)->pnum,
            ((struct player_spacket *) packet)->dir,
            ((struct player_spacket *) packet)->speed,
            ntohl(((struct player_spacket *) packet)->x),
            ntohl(((struct player_spacket *) packet)->y));
  }
}


void    handleWarning(struct warning_spacket *packet)
{
  packet->mesg[sizeof(packet->mesg) - 1] = '\0'; /* guarantee null * *
						  * termination */
  warning(packet->mesg);
}

void sendShortPacket(char type, char state)
{
  struct speed_cpacket speedReq;

  bzero(&speedReq, sizeof(speedReq));
  speedReq.type = type;
  speedReq.speed = state;
  sendServerPacket((struct player_spacket *) &speedReq);

  /* if we're sending in UDP mode, be prepared to force it */
  if (commMode == COMM_UDP && udpClientSend >= 2)
    {
      switch (type)
	{
	case CP_SPEED:
	  fSpeed = state | 0x100;
	  break;
	case CP_DIRECTION:
	  fDirection = state | 0x100;
	  break;
	case CP_SHIELD:
	  fShield = state | 0xa00;
	  break;
	case CP_ORBIT:
	  fOrbit = state | 0xa00;
	  break;
	case CP_REPAIR:
	  fRepair = state | 0xa00;
	  break;
	case CP_CLOAK:
	  fCloak = state | 0xa00;
	  break;
	case CP_BOMB:
	  fBomb = state | 0xa00;
	  break;
	case CP_DOCKPERM:
	  fDockperm = state | 0xa00;
	  break;
	case CP_PLAYLOCK:
	  fPlayLock = state | 0xa00;
	  break;
	case CP_PLANLOCK:
	  fPlanLock = state | 0xa00;
	  break;
	case CP_BEAM:
	  if (state == 1)
	    fBeamup = 1 | 0x500;
	  else
	    fBeamdown = 2 | 0x500;
	  break;
	}

      /* force weapons too? */
      if (udpClientSend >= 3)
	{
	  switch (type)
	    {
	    case CP_PHASER:
	      fPhaser = state | 0x100;
	      break;
	    case CP_PLASMA:
	      fPlasma = state | 0x100;
	      break;
	    }
	}
    }
}

void sendServerPacket(void *buffer)
{
  int     size;
  struct player_spacket *packet = (struct player_spacket *) buffer;

  if (serverDead)
    return;
  if (packet->type < 1 ||
      packet->type > NUM_SIZES ||
      sizes[(unsigned char) packet->type] == 0)
    {
      printf("Attempt to send strange packet %d!\n", packet->type);
      return;
    }
  size = sizes[(unsigned char) packet->type];

#ifdef PACKET_LOG
  if (log_packets)
    {
      Log_OPacket(packet->type, size);
      print_opacket((char *) packet,size);
    }
#endif

  if (commMode == COMM_UDP)
    {
      /* for now, just sent everything via TCP */
    }
  if (commMode == COMM_TCP || !udpClientSend)
    {
      /* special case for verify packet */
      if (packet->type == CP_UDP_REQ)
	{
	  if (((struct udp_req_cpacket *) packet)->request == COMM_VERIFY)
	    goto send_udp;
	}
      /* business as usual (or player has turned off UDP transmission) */
      if (gwrite(sock, (char *) packet, size) != size)
	{
	  printf("gwrite failed.  Server must be dead\n");
	  serverDead = 1;
	}
    }
  else
    {
      /* UDP stuff */
      switch (packet->type)
	{
	case CP_SPEED:
	case CP_DIRECTION:
	case CP_PHASER:
	case CP_PLASMA:
	case CP_TORP:
	case CP_QUIT:
	case CP_PRACTR:
	case CP_SHIELD:
	case CP_REPAIR:
	case CP_ORBIT:
	case CP_PLANLOCK:
	case CP_PLAYLOCK:
	case CP_BOMB:
	case CP_BEAM:
	case CP_CLOAK:
	case CP_DET_TORPS:
	case CP_DET_MYTORP:
	case CP_REFIT:
	case CP_TRACTOR:
	case CP_REPRESS:
	case CP_COUP:
	case CP_DOCKPERM:

#ifdef INCLUDE_SCAN
	case CP_SCAN:
#endif

	case CP_PING_RESPONSE:
	  /* non-critical stuff, use UDP */
	send_udp:
	  packets_sent++;

	  V_UDPDIAG(("Sent %d on UDP port\n", packet->type));
	  if (gwrite(udpSock, (char *) packet, size) != size)
	    {
	      UDPDIAG(("gwrite on UDP failed.  Closing UDP connection\n"));
	      warning("UDP link severed");
	      /* serverDead=1; */
	      commModeReq = commMode = COMM_TCP;
	      commStatus = STAT_CONNECTED;
	      commSwitchTimeout = 0;
	      if (udpWin)
		{
		  udprefresh(UDP_STATUS);
		  udprefresh(UDP_CURRENT);
		}
	      if (udpSock >= 0)
		closeUdpConn();
	    }
	  break;

	default:
	  /* critical stuff, use TCP */
	  if (gwrite(sock, (char *) packet, size) != size)
	    {
	      printf("gwrite failed.  Server must be dead\n");
	      serverDead = 1;
	    }
	}
    }
}

void    handlePlanet(struct planet_spacket *packet)
{
  struct planet *plan;

  /* FAT: prevent excessive redraw */
  int     redraw = 0;

#ifdef CORRUPTED_PACKETS
  if (packet->pnum >= MAXPLANETS)
    {
      fprintf(stderr, "handlePlanet: bad index %d\n", packet->pnum);
      return;
    }
#endif

  plan = &planets[(unsigned char) packet->pnum];

#ifdef nodef
  monpoprate(plan, packet);
#endif

  if (plan->pl_owner != packet->owner)
    redraw = 1;
  plan->pl_owner = packet->owner;
  if (plan->pl_owner < FED || plan->pl_owner > ORI)
    plan->pl_owner = NOBODY;
  if (plan->pl_info != packet->info)
    redraw = 1;
  plan->pl_info = packet->info;
  /* Redraw the planet because it was updated by server */

  if (plan->pl_flags != (int) ntohs(packet->flags))
    redraw = 1;
  plan->pl_flags = (int) ntohs(packet->flags);

  if (plan->pl_armies != ntohl(packet->armies))
    {
      /* don't redraw when armies change unless it crosses the '4' army 
       * limit. Keeps people from watching for planet 'flicker' when 
       * players are beaming */
      int     planetarmies = ntohl(packet->armies);

      if ((plan->pl_armies < 5 && planetarmies > 4) ||
	  (plan->pl_armies > 4 && planetarmies < 5))
	redraw = 1;
    }
  plan->pl_armies = ntohl(packet->armies);

#ifndef RECORDGAME
  if (plan->pl_info == 0)
    {
      plan->pl_owner = NOBODY;
    }
#endif

  if (redraw)
    {
      plan->pl_flags |= PLREDRAW;
    }
}

void    handlePhaser(struct phaser_spacket *packet)
{
  struct phaser *phas;

#ifdef CORRUPTED_PACKETS
  if (packet->pnum >= MAXPLAYER)
    {
      fprintf(stderr, "handlePhaser: bad index %d\n", packet->pnum);
      return;
    }
  if (packet->status == PHHIT &&
      (ntohl(packet->target) < 0 || ntohl(packet->target) >= MAXPLAYER))
    {
      fprintf(stderr, "handlePhaser: bad target %d\n", ntohl(packet->target));
      return;
    }
#endif

  weaponUpdate = 1;
  phas = &phasers[(unsigned char) packet->pnum];
  phas->ph_status = packet->status;
  phas->ph_dir = packet->dir;
  phas->ph_x = ntohl(packet->x);
  phas->ph_y = ntohl(packet->y);
  phas->ph_target = ntohl(packet->target);
  phas->ph_fuse = 0;
#ifdef SOUND
  phas->sound_phaser = 1;
#endif
  phas->ph_updateFuse = PHASER_UPDATE_FUSE * server_ups / 10;

#ifdef ROTATERACE
  if (rotate)
    {
      rotate_coord(&phas->ph_x, &phas->ph_y, rotate_deg, GWIDTH / 2, GWIDTH / 2);
      rotate_dir(&phas->ph_dir, rotate_deg);
    }
#endif
  
  if (gather_stats) {
    /* not getting any data?*/
    /*STATS_SP_PHASER:\tpnum\tstats\tdir\tx\ty\ttarget*/
    fprintf(statsFile, "\nSTATS_SP_PHASER\t");
    fprintf(statsFile, "%d\t%d\t%u\t%d\t%d\t%d",
            ((struct phaser_spacket *) packet)->pnum,
            ((struct phaser_spacket *) packet)->status,
            ((struct phaser_spacket *) packet)->dir,
            ntohl(((struct phaser_spacket *) packet)->x),
            ntohl(((struct phaser_spacket *) packet)->y),
            ntohl(((struct phaser_spacket *) packet)->target));
  }
}

void    handleMessage(struct mesg_spacket *packet)
{
  if (packet->m_from >= MAXPLAYER)
    packet->m_from = 255;

#ifdef CORRUPTED_PACKETS
  packet->mesg[sizeof(packet->mesg) - 1] = '\0';
#endif

  /* printf("flags: 0x%x\n", packet->m_flags); 
   * printf("from: %d\n", packet->m_from); 
   * printf("recpt: %d\n", packet->m_recpt); 
   * printf("mesg: %s\n", packet->mesg); 
   */

  dmessage(packet->mesg, packet->m_flags, packet->m_from, packet->m_recpt);
  
  if (gather_stats) {
    /*STATS_SP_MESSAGE:\tFLAGS\tRECPT\tFROM\tMESG*/
    fprintf(statsFile, "\nSTATS_SP_MESSAGE:\t");
    fprintf(statsFile, "0x%0X\t%d\t%d\t%s",
            ((struct mesg_spacket *) packet)->m_flags,
            ((struct mesg_spacket *) packet)->m_recpt,
            ((struct mesg_spacket *) packet)->m_from,
            ((struct mesg_spacket *) packet)->mesg);
  }
}

void    handleQueue(struct queue_spacket *packet)
{
  queuePos = ntohs(packet->pos);
}

void sendTeamReq(int team, int ship)
{
  struct outfit_cpacket outfitReq;

  bzero(&outfitReq, sizeof(outfitReq));
  outfitReq.type = CP_OUTFIT;
  outfitReq.team = team;
  outfitReq.ship = ship;
  sendServerPacket(&outfitReq);
}

void    handlePickok(struct pickok_spacket *packet)
{
  pickOk = packet->state;
}

void sendLoginReq(char *name, char *pass, char *login, char query)
{
  struct login_cpacket packet;

  bzero(&packet, sizeof(packet));
  strcpy(packet.name, name);
  strcpy(packet.password, pass);
  if (strlen(login) > 15)
    login[15] = 0;
  strcpy(packet.login, login);
  packet.type = CP_LOGIN;
  packet.query = query;
  sendServerPacket((struct player_spacket *) &packet);
}

void    handleLogin(struct login_spacket *packet)
{
  loginAccept = packet->accept;
  if (packet->accept)
    {
      /* no longer needed .. we have it in xtrekrc bcopy(packet->keymap, 
       * mystats->st_keymap, 96); */
      mystats->st_flags = ntohl(packet->flags);

#ifdef nodef
      namemode = (me->p_stats.st_flags / ST_NAMEMODE) & 1;
      keeppeace = (me->p_stats.st_flags / ST_KEEPPEACE) & 1;
      showlocal = (me->p_stats.st_flags / ST_SHOWLOCAL) & 3;
      showgalactic = (me->p_stats.st_flags / ST_SHOWGLOBAL) & 3;
#endif
    }
}

void sendTractorReq(char state, char pnum)
{
  struct tractor_cpacket tractorReq;

  memset(&tractorReq, 0, sizeof(struct tractor_cpacket));
  tractorReq.type = CP_TRACTOR;
  tractorReq.state = state;
  tractorReq.pnum = pnum;
  sendServerPacket((struct player_spacket *) &tractorReq);

  if (state)
    fTractor = pnum | 0x40;
  else
    fTractor = 0;
}

void sendRepressReq(char state, char pnum)
{
  struct repress_cpacket repressReq;

  memset(&repressReq, 0, sizeof(struct repress_cpacket));
  repressReq.type = CP_REPRESS;
  repressReq.state = state;
  repressReq.pnum = pnum;
  sendServerPacket((struct player_spacket *) &repressReq);

  if (state)
    fRepress = pnum | 0x40;
  else
    fRepress = 0;
}

void sendDetMineReq(short int torp)
{
  struct det_mytorp_cpacket detReq;

  memset(&detReq, 0, sizeof(struct det_mytorp_cpacket));
  detReq.type = CP_DET_MYTORP;
  detReq.tnum = htons(torp);
  sendServerPacket((struct player_spacket *) &detReq);
}

void    handlePlasmaInfo(struct plasma_info_spacket *packet)
{
  struct plasmatorp *thetorp;

#ifdef CORRUPTED_PACKETS
  if (ntohs(packet->pnum) >= MAXPLAYER * MAXPLASMA)
    {
      fprintf(stderr, "handlePlasmaInfo: bad index %d\n", packet->pnum);
      return;
    }
#endif

  weaponUpdate = 1;
  thetorp = &plasmatorps[ntohs(packet->pnum)];
  thetorp->pt_updateFuse = PLASMA_UPDATE_FUSE * server_ups / 10;
  if (packet->status == PTEXPLODE && thetorp->pt_status == PTFREE)
    {
      /* FAT: redundant explosion; don't update p_nplasmatorp */
      return;
    }
  if (!thetorp->pt_status && packet->status)
    {
      players[thetorp->pt_owner].p_nplasmatorp++;
    }
  if (thetorp->pt_status && !packet->status)
    {
      players[thetorp->pt_owner].p_nplasmatorp--;
    }
  thetorp->pt_war = packet->war;
  if (thetorp->pt_status != packet->status)
    {
      /* FAT: prevent explosion timer from being reset */
      thetorp->pt_status = packet->status;
      if (thetorp->pt_status == PTEXPLODE)
	{
	  thetorp->pt_fuse = NUMDETFRAMES * server_ups / 10;
	}
    }
    
  if (gather_stats) {
    /*STATS_SP_PLASMA_INFO:\tWAR\tSTATUS\tPNUM*/
    fprintf(statsFile, "\nSTATS_SP_PLASMA_INFO:\t");
    fprintf(statsFile, "%d\t%d\t%u",
            ((struct plasma_info_spacket *) packet)->war,
            ((struct plasma_info_spacket *) packet)->status,
            ntohs(((struct plasma_info_spacket *) packet)->pnum));
  }
}

void    handlePlasma(struct plasma_spacket *packet)
{
  struct plasmatorp *thetorp;

#ifdef CORRUPTED_PACKETS
  if (ntohs(packet->pnum) >= MAXPLAYER * MAXPLASMA)
    {
      fprintf(stderr, "handlePlasma: bad index %d\n", packet->pnum);
      return;
    }
#endif

  weaponUpdate = 1;
  thetorp = &plasmatorps[ntohs(packet->pnum)];
  thetorp->pt_x = ntohl(packet->x);
  thetorp->pt_y = ntohl(packet->y);
  thetorp->pt_updateFuse = PLASMA_UPDATE_FUSE * server_ups / 10;

#ifdef ROTATERACE
  if (rotate)
    {
      rotate_coord(&thetorp->pt_x, &thetorp->pt_y, rotate_deg, GWIDTH / 2, GWIDTH / 2);
    }
#endif

  if (gather_stats) {
    /*STATS_SP_PLASMA:\tPNUM\tX\tY*/
    fprintf(statsFile, "\nSTATS_SP_PLASMA:\t%u\t%d\t%d",
            ntohs(((struct plasma_spacket *) packet)->pnum),
            ntohl(((struct plasma_spacket *) packet)->x),
            ntohl(((struct plasma_spacket *) packet)->y));
  }
}

void    handleFlags(struct flags_spacket *packet)
{
  struct player* pl;

  pl = &players[(unsigned char) packet->pnum];

#ifdef CORRUPTED_PACKETS
  if (packet->pnum >= MAXPLAYER)
    {
      fprintf(stderr, "handleFlags: bad index %d\n", packet->pnum);
      return;
    }
#endif

  if (players[(unsigned char) packet->pnum].p_flags != ntohl(packet->flags)

#ifdef INCLUDE_VISTRACT
      || players[(unsigned char) packet->pnum].p_tractor !=
      ((short) packet->tractor & (~0x40))
#endif

      )
    {
      /* FAT: prevent redundant player update */
      redrawPlayer[(unsigned char) packet->pnum] = 1;
    }
  else
    return;

  players[(unsigned char) packet->pnum].p_flags = ntohl(packet->flags);

#ifdef INCLUDE_VISTRACT
  if (packet->tractor & 0x40) {
    players[(unsigned char) packet->pnum].p_tractor =
      (short) packet->tractor & (~0x40); /* ATM visible tractors */
  } else
#endif /* INCLUDE_VISTRACT */

    players[(unsigned char) packet->pnum].p_tractor = -1;
}

void    handleKills(struct kills_spacket *packet)
{

#ifdef CORRUPTED_PACKETS
  if (packet->pnum >= MAXPLAYER)
    {
      fprintf(stderr, "handleKills: bad index %d\n", packet->pnum);
      return;
    }
#endif

  if (players[(unsigned char) packet->pnum].p_kills !=
      ntohl(packet->kills) / 100.0)
    {
      players[(unsigned char) packet->pnum].p_kills =
        ntohl(packet->kills) / 100.0;
      /* FAT: prevent redundant player update */
      PlistNoteUpdate((unsigned char) packet->pnum);

#ifdef ARMY_SLIDER
      if (me == &players[(unsigned char) packet->pnum])
	{
	  calibrate_stats();
	  redrawStats();
	}
#endif /* ARMY_SLIDER */
    }
    
  if (gather_stats) {
    /*STATS_SP_KILLS:\tPNUM\tKILLS*/
    fprintf(statsFile, "\nSTATS_SP_KILLS:\t");
    fprintf(statsFile, "%d\t%u",
            ((struct kills_spacket *) packet)->pnum,
            ntohl(((struct kills_spacket *) packet)->kills));
  }
}

void    handlePStatus(struct pstatus_spacket *packet)
{
  register struct player *j;

#ifdef CORRUPTED_PACKETS
  if (packet->pnum >= MAXPLAYER)
    {
      fprintf(stderr, "handlePStatus: bad index %d\n", packet->pnum);
      return;
    }
#endif

  j = &players[(unsigned char) packet->pnum];

  if (packet->status == j->p_status)
    return;

  if (packet->status == PEXPLODE)
    {
      j->p_explode = 0;
    }

  /* Ignore DEAD status. Instead, we treat it as PEXPLODE. This gives us time
   * * * to animate all the frames necessary for the explosions at our own
   * pace.  */
  else if (packet->status == PDEAD)
    {
      packet->status = PEXPLODE;

      if (j->p_status == PEXPLODE)		 /* Prevent redundant updates 
						  * 
						  */
	return;
    }

  /* guarantees we won't miss the kill message since this is TCP */
  else if (packet->status == PALIVE && j->p_status != PALIVE)
    j->p_kills = 0.;

  /* update the player list, especially if this signals a new arrival */
  PlistNoteUpdate((unsigned char) packet->pnum);
  if (j->p_status == PFREE || packet->status == PFREE)
    PlistNoteArrive(packet->pnum);

  j->p_status = packet->status;
  redrawPlayer[(unsigned char) packet->pnum] = 1;
}

void    handleMotd(struct motd_spacket *packet)
{
  packet->line[sizeof(packet->line) - 1] = '\0';
  newMotdLine(packet->line);
}

void sendMessage(char *mes, int group, int indiv)
{
  struct mesg_cpacket mesPacket;

  bzero(&mesPacket, sizeof(mesPacket));
#ifdef SHORT_PACKETS
  if (recv_short)
    {
      int     size;

      size = strlen(mes);
      size += 5;				 /* 1 for '\0', 4 * *
						  * packetheader */
      if ((size % 4) != 0)
	size += (4 - (size % 4));
      mesPacket.pad1 = (char) size;
      sizes[CP_S_MESSAGE] = size;
      mesPacket.type = CP_S_MESSAGE;
    }
  else
#endif

    mesPacket.type = CP_MESSAGE;
  mesPacket.group = group;
  mesPacket.indiv = indiv;
  STRNCPY(mesPacket.mesg, mes, 80);
  sendServerPacket((struct player_spacket *) &mesPacket);
}


void    handleMask(struct mask_spacket *packet)
{
  motd_refresh();
  tournMask = packet->mask;
}

void sendOptionsPacket(void)
{
  struct options_cpacket optPacket;

  bzero(&optPacket, sizeof(optPacket));
  optPacket.type = CP_OPTIONS;
  optPacket.flags =
      htonl(ST_MAPMODE +			 /* always on */
	    ST_NAMEMODE * namemode +
	    ST_SHOWSHIELDS +			 /* always on */
	    ST_KEEPPEACE * keeppeace +
	    ST_SHOWLOCAL * showlocal +
	    ST_SHOWGLOBAL * showgalactic);
  MCOPY(mystats->st_keymap, optPacket.keymap, 96);
  sendServerPacket((struct player_spacket *) &optPacket);
}

static void pickSocket(int old)
{
  int     newsocket;
  struct socket_cpacket sockPack;

  /* If baseLocalPort is defined we want to start from that */
  if(baseLocalPort)
      newsocket = baseLocalPort;
  else
      newsocket = (getpid() & 32767);
  while (newsocket < 2048 || newsocket == old)
    {
      if(baseLocalPort)
          newsocket++;
      else
          newsocket = (newsocket + 10687) & 32767;
    }
  bzero(&sockPack,sizeof(sockPack));
  sockPack.type = CP_SOCKET;
  sockPack.socket = htonl(newsocket);
  sockPack.version = (char) SOCKVERSION;
  sockPack.udp_version = (char) UDPVERSION;
  sendServerPacket((struct player_spacket *) &sockPack);
  /* Did we get new socket # sent? */
  if (serverDead)
    return;
  nextSocket = newsocket;
}

static void
handleBadVersionSorry (char *reason)
{
    printf("%s\nTry again later.\n", reason);
}

void    handleBadVersion(struct badversion_spacket *packet)
{
  int badversion = packet->why;

  if (badversion >= 0 && badversion <= MAXBADVERSION) {
    handleBadVersionSorry(badversion_long_strings[badversion]);
  }
  terminate(EXIT_BADVERSION_BASE+badversion);
}

int gwrite(int fd, char *buf, register int bytes)
{
  LONG    orig = bytes;
  register LONG n;

  while (bytes)
    {
      n = write(fd, buf, bytes);
      if (n < 0)
	{
	  if (fd == udpSock)
	    {
	      fprintf(stderr, "Tried to write %d, %d\n", fd, bytes);
	      perror("write");
	      printUdpInfo();
	    }
	  return -1;
	}
      bytes -= n;
      buf += n;
    }
  return orig;
}

void    handleHostile(struct hostile_spacket *packet)
{
  int p_no = packet->pnum;
  struct player *pl;

#ifdef CORRUPTED_PACKETS
  if (p_no < 0 || p_no >= MAXPLAYER)
    {
      fprintf(stderr, "handleHostile: bad index %d\n", p_no);
      return;
    }
#endif

  pl = &players[p_no];
  if (pl->p_swar != packet->war ||
      pl->p_hostile != packet->hostile)
    {
      /* FAT: prevent redundant player update & redraw */
      pl->p_swar = packet->war;
      pl->p_hostile = packet->hostile;
      PlistNoteHostile(p_no);
      redrawPlayer[p_no] = 1;
    }
}

void    handlePlyrLogin(struct plyr_login_spacket *packet, int sock)
{
  int p_no = packet->pnum;
  struct player *pl;

#ifdef CORRUPTED_PACKETS
  if (sock == udpSock)
    {
      fprintf(stderr, "garbage packet discarded.\n");
      return;
    }
  if (packet->pnum < 0 || packet->pnum >= MAXPLAYER)
    {
      fprintf(stderr, "handlePlyrLogin: bad index %d\n", p_no);
      return;
    }
  packet->name[sizeof(packet->name) - 1] = '\0';
  packet->monitor[sizeof(packet->monitor) - 1] = '\0';
  packet->login[sizeof(packet->login) - 1] = '\0';
#endif

  pl = &players[p_no];

  if (identityBlind) {
    strcpy(pl->p_name, "");
    strcpy(pl->p_monitor, "");
    strcpy(pl->p_login, "");
    pl->p_stats.st_rank = 0;
  } else {
    strcpy(pl->p_name, packet->name);
    strcpyp_return(pl->p_monitor, packet->monitor, sizeof(pl->p_monitor));
    strcpy(pl->p_login, packet->login);
    pl->p_stats.st_rank = packet->rank;
  }

  if (p_no == me->p_no)
    {
      /* This is me.  Set some stats */
      if (lastRank == -1)
	{
	  if (loggedIn)
	    {
	      lastRank = packet->rank;
	    }
	}
      else
	{
	  if (lastRank != packet->rank)
	    {
	      lastRank = packet->rank;
	      promoted = 1;
	    }
	}
    }

  PlistNoteUpdate(p_no);

  if (gather_stats) {
    /* STATS_SP_PLAYER:PNUN,RANK,NAME,MONITOR,LOGIN */
    fprintf(statsFile, "\nSTATS_SP_PL_LOGIN:\t");
    fprintf(statsFile, "%d\t%d\t%s\t%s\t%s",
            ((struct plyr_login_spacket *) packet)->pnum,
            ((struct plyr_login_spacket *) packet)->rank,
            ((struct plyr_login_spacket *) packet)->name,
            ((struct plyr_login_spacket *) packet)->monitor,
            ((struct plyr_login_spacket *) packet)->login);
  }
}

void    handleStats(struct stats_spacket *packet)
{
  int p_no = packet->pnum;
  struct player *pl;

#ifdef CORRUPTED_PACKETS
  if (p_no < 0 || p_no >= MAXPLAYER)
    {
      fprintf(stderr, "handleStats: bad index %d\n", p_no);
      return;
    }
#endif

  pl = &players[p_no];

  pl->p_stats.st_tkills = ntohl(packet->tkills);
  pl->p_stats.st_tlosses = ntohl(packet->tlosses);
  pl->p_stats.st_kills = ntohl(packet->kills);
  pl->p_stats.st_losses = ntohl(packet->losses);
  pl->p_stats.st_tticks = ntohl(packet->tticks);
  pl->p_stats.st_tplanets = ntohl(packet->tplanets);
  pl->p_stats.st_tarmsbomb = ntohl(packet->tarmies);
  pl->p_stats.st_sbkills = ntohl(packet->sbkills);
  pl->p_stats.st_sblosses = ntohl(packet->sblosses);
  pl->p_stats.st_armsbomb = ntohl(packet->armies);
  pl->p_stats.st_planets = ntohl(packet->planets);

  if ((pl->p_ship.s_type == STARBASE) && (SBhours))
    {
      pl->p_stats.st_sbticks = ntohl(packet->maxkills);
    }
  else
    {
      pl->p_stats.st_maxkills = ntohl(packet->maxkills) / 100.0;
    }
  pl->p_stats.st_sbmaxkills = ntohl(packet->sbmaxkills) / 100.0;

  /* For some reason, we get updates for freed players.  When this
  happens, don't bother to update the player list. */
  if (pl->p_status != PFREE)
    PlistNoteUpdate(p_no);
}

void    handlePlyrInfo(struct plyr_info_spacket *packet)
{
  int p_no = packet->pnum;
  struct player *pl;
  static int lastship = -1;

#ifdef CORRUPTED_PACKETS
  if (p_no < 0 || p_no >= MAXPLAYER)
    {
      fprintf(stderr, "handlePlyrInfo: bad index %d\n", p_no);
      return;
    }
  if (packet->team > ALLTEAM)
    {
      fprintf(stderr, "handlePlyrInfo: bad team %d\n", packet->team);
      return;
    }
#endif

  pl = &players[p_no];

  PlistNoteUpdate(p_no);
  if ((pl->p_team != packet->team) ||		 /* Check 0 system default */
      ((pl->p_team == 0) && (pl->p_mapchars[0] != teamlet[0])))
    {
      pl->p_team = packet->team;
      pl->p_mapchars[0] = teamlet[pl->p_team];
      PlistNoteArrive(p_no);

      if (pl == me)
	redrawall = 1;				 /* Update the map if I * *
						  * change teams */
    }

  getship(&pl->p_ship, packet->shiptype);
  pl->p_mapchars[1] = shipnos[pl->p_no];


  if (me == pl && lastship != me->p_ship.s_type)
    {
      redrawTstats();
      calibrate_stats();
      redrawStats();				 /* TSH */
    }
  redrawPlayer[p_no] = 1;

  if (gather_stats) {
    /*STATS_SP_PLAYER_INFO:\tPNUM\tSHIPTYPE\tTEAM*/
    fprintf(statsFile, "\nSTATS_SP_PLAYER_INFO:\t");
    fprintf(statsFile, "%d\t%d\t%d",
            ((struct plyr_info_spacket *) packet)->pnum,
            ((struct plyr_info_spacket *) packet)->shiptype,
            ((struct plyr_info_spacket *) packet)->team);
  }
}

void sendUpdatePacket(LONG speed)
{
  struct updates_cpacket packet;

  packet.type = CP_UPDATES;
  timerDelay = speed;
  packet.usecs = htonl(speed);
  sendServerPacket((struct player_spacket *) &packet);
}

void    handlePlanetLoc(struct planet_loc_spacket *packet)
{
  int pl_no = packet->pnum;
  struct planet *pl;

#ifdef CORRUPTED_PACKETS
  if (pl_no < 0 || pl_no >= MAXPLANETS)
    {
      fprintf(stderr, "handlePlanetLoc: bad index\n");
      return;
    }
#endif

  pl = &planets[pl_no];
  pl_update[pl_no].plu_x = pl->pl_x;
  pl_update[pl_no].plu_y = pl->pl_y;

  if (pl_update[pl_no].plu_update != -1)
    {
      pl_update[pl_no].plu_update = 1;
    }
  else
    pl_update[pl_no].plu_update = 0;

  pl->pl_x = ntohl(packet->x);
  pl->pl_y = ntohl(packet->y);
  strcpy(pl->pl_name, packet->name);
  pl->pl_namelen = strlen(packet->name);
  pl->pl_flags |= (PLREDRAW | PLCLEAR);
  reinitPlanets = 1;

#ifdef ROTATERACE
  if (rotate)
    {
      rotate_coord(&pl->pl_x, &pl->pl_y, rotate_deg, GWIDTH / 2, GWIDTH / 2);
    }
#endif

  if (gather_stats) {
    /*STATS_SP_PLANET_LOC:\tPNUM\tX\tY\tNAME*/
    fprintf(statsFile, "\nSTATS_SP_PLANET_LOC:\t");
    fprintf(statsFile, "%d\t%d\t%d\t%s",
            ((struct planet_loc_spacket *) packet)->pnum,
            ntohl(((struct planet_loc_spacket *) packet)->x),
            ntohl(((struct planet_loc_spacket *) packet)->y),
            ((struct planet_loc_spacket *) packet)->name);
  }
}


void    handleReserved(struct reserved_spacket *packet, int sock)
{
  struct reserved_cpacket response;

  bzero(&response, sizeof(response));
#ifdef CORRUPTED_PACKETS
  if (sock == udpSock)
    {
      fprintf(stderr, "garbage Reserved packet discarded.\n");
      return;
    }
#endif

#if !defined(BORG)

#ifndef RSA
  encryptReservedPacket(packet, &response, serverName, me->p_no);
  sendServerPacket((struct player_spacket *) &response);
#else

  if (RSA_Client)
    {						 /* can use -o option for old
						  * * * blessing */
      /* client sends back a 'reserved' packet just saying RSA_VERSION info */
      /* theoretically, the server then sends off a rsa_key_spacket * for the
       * * * client to then respond to */
      warning(RSA_VERSION);
      STRNCPY(response.resp, RSA_VERSION, RESERVED_SIZE);
      MCOPY(packet->data, response.data, RESERVED_SIZE);
      response.type = CP_RESERVED;

#ifdef DEBUG
      printf("Sending RSA reserved response\n");
#endif
    }
  else
    {
      /* If server gods don't like NEWMACRO/SMARTMACRO they better install *
       * * RSA... */
      UseNewMacro = 1;
      UseSmartMacro = 1;
      encryptReservedPacket(packet, &response, serverName, me->p_no);
    }

  sendServerPacket((struct player_spacket *) &response);
#endif /* RSA */

#endif /* defined(BORG) */
}

void    handleShipCap(struct ship_cap_spacket *packet)
{
  unsigned short stype;

  stype = ntohs(packet->s_type);
  shipvals[stype].s_torpspeed = ntohs(packet->s_torpspeed);
  shipvals[stype].s_maxshield = ntohl(packet->s_maxshield);
  shipvals[stype].s_maxdamage = ntohl(packet->s_maxdamage);
  shipvals[stype].s_maxegntemp = ntohl(packet->s_maxegntemp);
  shipvals[stype].s_maxwpntemp = ntohl(packet->s_maxwpntemp);
  shipvals[stype].s_maxarmies = ntohs(packet->s_maxarmies);
  shipvals[stype].s_maxfuel = ntohl(packet->s_maxfuel);
  shipvals[stype].s_maxspeed = ntohl(packet->s_maxspeed);
  shipvals[stype].s_width = ntohs(packet->s_width);
  shipvals[stype].s_height = ntohs(packet->s_height);
  shipvals[stype].s_phaserdamage = ntohs(packet->s_phaserrange);
  getship(myship, myship->s_type);
}

static void
handleGeneric32_a (struct generic_32_spacket_a *packet)
{
    me->p_repair_time = packet->repair_time;
    me->pl_orbit = packet->pl_orbit;
}

static void
handleGeneric32_b (struct generic_32_spacket_b *packet)
{
    int rate;
    static int unsafe = GU_UNSAFE;
    static int idling = 0;
    static int saved_ups = 0;

    me->p_repair_time = ntohs(packet->repair_time);
    me->pl_orbit = packet->pl_orbit;
    context->gameup = ntohs(packet->gameup);
    context->tournament_teams = packet->tournament_teams;
    context->tournament_age = packet->tournament_age;
    context->tournament_age_units = packet->tournament_age_units;
    context->tournament_remain = packet->tournament_remain;
    context->tournament_remain_units = packet->tournament_remain_units;
    context->starbase_remain = packet->starbase_remain;
    context->team_remain = packet->team_remain;

    /* to maintain update rate when safe-idle, set updatespersec.idle: -1 */

    rate = intDefault("updatespersec.idle", 1);
    if ((unsafe ^ context->gameup) & GU_UNSAFE) {
      unsafe = context->gameup;
      if (context->gameup & GU_UNSAFE) {
        if (idling) {
	  if (rate < 0) {
	    warning("Safe idle stop.");
	  } else {
	    sendUpdatePacket(1000000 / saved_ups);
	    warning("Safe idle stop, normal update rate resumed.");
	  }
	  if (context->tournament_age_units == 's' &&
	      context->tournament_age < 5 &&
	      booleanDefault("game-begin-while-idle-beep", 1)) W_Beep();
	  idling = 0;
        }
      } else {
        if (!context->gameup & GU_UNSAFE) {
          if (!idling) {
	    if (rate < 0) {
	      warning("Safe idle start.");
	    } else {
	      saved_ups = client_ups;
	      sendUpdatePacket(1000000 / rate);
	      idling++;
	      warning("Safe idle start, update rate reduced.");
	    }
          }
        }
      }
    }
}

static void
handleGeneric32 (struct generic_32_spacket *packet)
{
  switch (packet->version) {
  case 0x60+GENERIC_32_VERSION_A:
    handleGeneric32_a((struct generic_32_spacket_a *) packet);
    break;
  case 0x60+GENERIC_32_VERSION_B:
    handleGeneric32_b((struct generic_32_spacket_b *) packet);
    break;
  }
}

void
handleRank (struct rank_spacket *packet)
{
  int i = packet->rnum;
  int j = packet->rmax + 1;

  if (i < 0 || j < 0 || i > j) return;
  if (j > nranks) {
    ranks = (struct rank *) realloc(ranks, j * sizeof(struct rank));
    memset(&ranks[nranks], 0, (j - nranks) * sizeof(struct rank));
  }
  nranks = j;
  W_ChangeBorder(rankw, gColor);
  packet->name[15] = 0;
  ranks[i].name = strdup(packet->name);
  packet->cname[7] = 0;
  ranks[i].cname = strdup(packet->cname);
  ranks[i].hours = (float) (ntohl (packet->hours) / 100.0);
  ranks[i].ratings = (float) (ntohl (packet->ratings) / 100.0);
  ranks[i].offense = (float) (ntohl (packet->offense) / 100.0);
}

static void dump_prefix(const char *abbr, const char *name) {
  fprintf(stderr, "%-10s %-30s ", abbr, name);
}

#define dump_stat(STAT) { \
    fprintf(stderr, " %8u\n", (unsigned int) ltd.STAT); \
}

#define dump_max(STAT) { \
    fprintf(stderr, " %8u\n", (unsigned int) ltd.STAT); \
}

void
handleLtd (struct ltd_spacket *packet)
{
  if (packet->version != LTD_VERSION) return;

  struct ltd_stats ltd;

  ltd.kills.total                     = ntohl(packet->kt);
  ltd.kills.max                       = (double)(ntohl(packet->kmax)) / 100.0;
  ltd.kills.first                     = ntohl(packet->k1);
  ltd.kills.first_potential           = ntohl(packet->k1p);
  ltd.kills.first_converted           = ntohl(packet->k1c);
  ltd.kills.second                    = ntohl(packet->k2);
  ltd.kills.second_potential          = ntohl(packet->k2p);
  ltd.kills.second_converted          = ntohl(packet->k2c);
  ltd.kills.phasered                  = ntohl(packet->kbp);
  ltd.kills.torped                    = ntohl(packet->kbt);
  ltd.kills.plasmaed                  = ntohl(packet->kbs);
  ltd.deaths.total                    = ntohl(packet->dt);
  ltd.deaths.potential                = ntohl(packet->dpc);
  ltd.deaths.converted                = ntohl(packet->dcc);
  ltd.deaths.dooshed                  = ntohl(packet->ddc);
  ltd.deaths.phasered                 = ntohl(packet->dbp);
  ltd.deaths.torped                   = ntohl(packet->dbt);
  ltd.deaths.plasmaed                 = ntohl(packet->dbs);
  ltd.deaths.acc                      = ntohl(packet->acc);
  ltd.planets.taken                   = ntohl(packet->ptt);
  ltd.planets.destroyed               = ntohl(packet->pdt);
  ltd.bomb.planets                    = ntohl(packet->bpt);
  ltd.bomb.planets_8                  = ntohl(packet->bp8);
  ltd.bomb.planets_core               = ntohl(packet->bpc);
  ltd.bomb.armies                     = ntohl(packet->bat);
  ltd.bomb.armies_8                   = ntohl(packet->ba8);
  ltd.bomb.armies_core                = ntohl(packet->bac);
  ltd.ogged.armies                    = ntohl(packet->oat);
  ltd.ogged.dooshed                   = ntohl(packet->odc);
  ltd.ogged.converted                 = ntohl(packet->occ);
  ltd.ogged.potential                 = ntohl(packet->opc);
  ltd.ogged.bigger_ship               = ntohl(packet->ogc);
  ltd.ogged.same_ship                 = ntohl(packet->oec);
  ltd.ogged.smaller_ship              = ntohl(packet->olc);
  ltd.ogged.sb_armies                 = ntohl(packet->osba);
  ltd.ogged.friendly                  = ntohl(packet->ofc);
  ltd.ogged.friendly_armies           = ntohl(packet->ofa);
  ltd.armies.total                    = ntohl(packet->at);
  ltd.armies.attack                   = ntohl(packet->aa);
  ltd.armies.reinforce                = ntohl(packet->ar);
  ltd.armies.ferries                  = ntohl(packet->af);
  ltd.armies.killed                   = ntohl(packet->ak);
  ltd.carries.total                   = ntohl(packet->ct);
  ltd.carries.partial                 = ntohl(packet->cp);
  ltd.carries.completed               = ntohl(packet->cc);
  ltd.carries.attack                  = ntohl(packet->ca);
  ltd.carries.reinforce               = ntohl(packet->cr);
  ltd.carries.ferries                 = ntohl(packet->cf);
  ltd.ticks.total                     = ntohl(packet->tt);
  ltd.ticks.yellow                    = ntohl(packet->tyel);
  ltd.ticks.red                       = ntohl(packet->tred);
  ltd.ticks.zone[0]                   = ntohl(packet->tz0);
  ltd.ticks.zone[1]                   = ntohl(packet->tz1);
  ltd.ticks.zone[2]                   = ntohl(packet->tz2);
  ltd.ticks.zone[3]                   = ntohl(packet->tz3);
  ltd.ticks.zone[4]                   = ntohl(packet->tz4);
  ltd.ticks.zone[5]                   = ntohl(packet->tz5);
  ltd.ticks.zone[6]                   = ntohl(packet->tz6);
  ltd.ticks.zone[7]                   = ntohl(packet->tz7);
  ltd.ticks.potential                 = ntohl(packet->tpc);
  ltd.ticks.carrier                   = ntohl(packet->tcc);
  ltd.ticks.repair                    = ntohl(packet->tr);
  ltd.damage_repaired                 = ntohl(packet->dr);
  ltd.weapons.phaser.fired            = ntohl(packet->wpf);
  ltd.weapons.phaser.hit              = ntohl(packet->wph);
  ltd.weapons.phaser.damage.inflicted = ntohl(packet->wpdi);
  ltd.weapons.phaser.damage.taken     = ntohl(packet->wpdt);
  ltd.weapons.torps.fired             = ntohl(packet->wtf);
  ltd.weapons.torps.hit               = ntohl(packet->wth);
  ltd.weapons.torps.detted            = ntohl(packet->wtd);
  ltd.weapons.torps.selfdetted        = ntohl(packet->wts);
  ltd.weapons.torps.wall              = ntohl(packet->wtw);
  ltd.weapons.torps.damage.inflicted  = ntohl(packet->wtdi);
  ltd.weapons.torps.damage.taken      = ntohl(packet->wtdt);
  ltd.weapons.plasma.fired            = ntohl(packet->wsf);
  ltd.weapons.plasma.hit              = ntohl(packet->wsh);
  ltd.weapons.plasma.phasered         = ntohl(packet->wsp);
  ltd.weapons.plasma.wall             = ntohl(packet->wsw);
  ltd.weapons.plasma.damage.inflicted = ntohl(packet->wsdi);
  ltd.weapons.plasma.damage.taken     = ntohl(packet->wsdt);

  return;

  fprintf(stderr, "SP_LTD test output begins\n");
  dump_prefix("kt",   "kills total");			dump_stat(kills.total);
  dump_prefix("kmax", "kills max");			dump_max(kills.max);
  dump_prefix("k1",   "kills first");			dump_stat(kills.first);
  dump_prefix("k1p",  "kills first potential");		dump_stat(kills.first_potential);
  dump_prefix("k1c",  "kills first converted");		dump_stat(kills.first_converted);
  dump_prefix("k2",   "kills second");			dump_stat(kills.second);
  dump_prefix("k2p",  "kills second potential");	dump_stat(kills.second_potential);
  dump_prefix("k2c",  "kills second converted");	dump_stat(kills.second_converted);
  dump_prefix("kbp",  "kills by phaser");		dump_stat(kills.phasered);
  dump_prefix("kbt",  "kills by torp");			dump_stat(kills.torped);
  dump_prefix("kbs",  "kills by smack");		dump_stat(kills.plasmaed);
  dump_prefix("dt",   "deaths total");			dump_stat(deaths.total);
  dump_prefix("dpc",  "deaths as potential carrier");	dump_stat(deaths.potential);
  dump_prefix("dcc",  "deaths as converted carrier");	dump_stat(deaths.converted);
  dump_prefix("ddc",  "deaths as dooshed carrier");	dump_stat(deaths.dooshed);
  dump_prefix("dbp",  "deaths by phaser");		dump_stat(deaths.phasered);
  dump_prefix("dbt",  "deaths by torp");		dump_stat(deaths.torped);
  dump_prefix("dbs",  "deaths by smack");		dump_stat(deaths.plasmaed);
  dump_prefix("acc",  "actual carriers created");	dump_stat(deaths.acc);
  dump_prefix("ptt",  "planets taken total");		dump_stat(planets.taken);
  dump_prefix("pdt",  "planets destroyed total");	dump_stat(planets.destroyed);
  dump_prefix("bpt",  "bombed planets total");		dump_stat(bomb.planets);
  dump_prefix("bp8",  "bombed planets <=8");		dump_stat(bomb.planets_8);
  dump_prefix("bpc",  "bombed planets core");		dump_stat(bomb.planets_core);
  dump_prefix("bat",  "bombed armies total");		dump_stat(bomb.armies);
  dump_prefix("ba8",  "bombed_armies <= 8");		dump_stat(bomb.armies_8);
  dump_prefix("bac",  "bombed armies core");		dump_stat(bomb.armies_core);
  dump_prefix("oat",  "ogged armies total");		dump_stat(ogged.armies);
  dump_prefix("odc",  "ogged dooshed carrier");		dump_stat(ogged.dooshed);
  dump_prefix("occ",  "ogged converted carrier");	dump_stat(ogged.converted);
  dump_prefix("opc",  "ogged potential carrier");	dump_stat(ogged.potential);
  dump_prefix("o>c",  "ogged bigger carrier");		dump_stat(ogged.bigger_ship);
  dump_prefix("o=c",  "ogged same carrier");		dump_stat(ogged.same_ship);
  dump_prefix("o<c",  "ogger smaller carrier");		dump_stat(ogged.smaller_ship);
  dump_prefix("osba", "ogged sb armies");		dump_stat(ogged.sb_armies);
  dump_prefix("ofc",  "ogged friendly carrier");	dump_stat(ogged.friendly);
  dump_prefix("ofa",  "ogged friendly armies");		dump_stat(ogged.friendly_armies);
  dump_prefix("at",   "armies carried total");		dump_stat(armies.total);
  dump_prefix("aa",   "armies used to attack");		dump_stat(armies.attack);
  dump_prefix("ar",   "armies used to reinforce");	dump_stat(armies.reinforce);
  dump_prefix("af",   "armies ferried");		dump_stat(armies.ferries);
  dump_prefix("ak",   "armies killed");			dump_stat(armies.killed);
  dump_prefix("ct",   "carries total");			dump_stat(carries.total);
  dump_prefix("cp",   "carries partial");		dump_stat(carries.partial);
  dump_prefix("cc",   "carries completed");		dump_stat(carries.completed);
  dump_prefix("ca",   "carries to attack");		dump_stat(carries.attack);
  dump_prefix("cr",   "carries to reinforce");		dump_stat(carries.reinforce);
  dump_prefix("cf",   "carries to ferry");		dump_stat(carries.ferries);
  dump_prefix("tt",   "ticks total");			dump_stat(ticks.total);
  dump_prefix("tyel", "ticks in yellow");		dump_stat(ticks.yellow);
  dump_prefix("tred", "ticks in red");			dump_stat(ticks.red);
  dump_prefix("tz0",  "ticks in zone 0");		dump_stat(ticks.zone[0]);
  dump_prefix("tz1",  "ticks in zone 1");		dump_stat(ticks.zone[1]);
  dump_prefix("tz2",  "ticks in zone 2");		dump_stat(ticks.zone[2]);
  dump_prefix("tz3",  "ticks in zone 3");		dump_stat(ticks.zone[3]);
  dump_prefix("tz4",  "ticks in zone 4");		dump_stat(ticks.zone[4]);
  dump_prefix("tz5",  "ticks in zone 5");		dump_stat(ticks.zone[5]);
  dump_prefix("tz6",  "ticks in zone 6");		dump_stat(ticks.zone[6]);
  dump_prefix("tz7",  "ticks in zone 7");		dump_stat(ticks.zone[7]);
  dump_prefix("tpc",  "ticks as potential carrier");	dump_stat(ticks.potential);
  dump_prefix("tcc",  "ticks as carrier++");		dump_stat(ticks.carrier);
  dump_prefix("tr",   "ticks in repair");		dump_stat(ticks.repair);
  dump_prefix("dr",   "damage repaired");		dump_stat(damage_repaired);
  dump_prefix("wpf",  "weap phaser fired");		dump_stat(weapons.phaser.fired);
  dump_prefix("wph",  "weap phaser hit");		dump_stat(weapons.phaser.hit);
  dump_prefix("wpdi", "weap phaser damage inflicted");	dump_stat(weapons.phaser.damage.inflicted);
  dump_prefix("wpdt", "weap phaser damage taken");	dump_stat(weapons.phaser.damage.taken);
  dump_prefix("wtf",  "weap torp fired");		dump_stat(weapons.torps.fired);
  dump_prefix("wth",  "weap torp hit");			dump_stat(weapons.torps.hit);
  dump_prefix("wtd",  "weap torp detted");		dump_stat(weapons.torps.detted);
  dump_prefix("wts",  "weap torp self detted");		dump_stat(weapons.torps.selfdetted);
  dump_prefix("wtw",  "weap torp hit wall");		dump_stat(weapons.torps.wall);
  dump_prefix("wtdi", "weap torp damage inflicted");	dump_stat(weapons.torps.damage.inflicted);
  dump_prefix("wtdt", "weap torp damage taken");	dump_stat(weapons.torps.damage.taken);
  dump_prefix("wsf",  "weap smack fired");		dump_stat(weapons.plasma.fired);
  dump_prefix("wsh",  "weap smack hit");		dump_stat(weapons.plasma.hit);
  dump_prefix("wsp",  "weap smack phasered");		dump_stat(weapons.plasma.phasered);
  dump_prefix("wsw",  "weap smack hit wall");		dump_stat(weapons.plasma.wall);
  dump_prefix("wsdi", "weap smack damage inflicted");	dump_stat(weapons.plasma.damage.inflicted);
  dump_prefix("wsdt", "weap smack damage taken");	dump_stat(weapons.plasma.damage.taken);
  fprintf(stderr, "SP_LTD test output ends\n");
}

#ifdef RSA
void    handleRSAKey(struct rsa_key_spacket *packet)
{
  struct rsa_key_cpacket response;
  struct sockaddr_in saddr;
  socklen_t len;
  unsigned char *data;

#ifdef GATEWAY
  extern unsigned LONG netaddr;
  extern int serv_port;
#endif

  bzero(&response, sizeof(response));
  response.type = CP_RSA_KEY;
  /* encryptRSAPacket (packet, &response);      old style rsa-client  */

#ifdef GATEWAY
  /* if we didn't get it from -H, go ahead and query the socket */
  if (netaddr == 0)
    {
      len = sizeof(saddr);
      if (getpeername(sock, (struct sockaddr *) &saddr, &len) < 0)
	{
	  perror("getpeername(sock)");
	  terminate(1);
	}
    }
  else
    {
      saddr.sin_addr.s_addr = htonl(netaddr);
      saddr.sin_port = htons(serv_port);
    }
#else
  /* query the socket to determine the remote host (ATM) */
  len = sizeof(saddr);
  if (getpeername(sock, (struct sockaddr *) &saddr, &len) < 0)
    {
      perror("getpeername(sock)");
      terminate(1);
    }
#endif

  /* replace the first few bytes of the message */
  /* will be the low order bytes of the number */
  data = packet->data;
  MCOPY(&saddr.sin_addr.s_addr, data, sizeof(saddr.sin_addr.s_addr));
  data += sizeof(saddr.sin_addr.s_addr);
  MCOPY(&saddr.sin_port, data, sizeof(saddr.sin_port));

  rsa_black_box(response.resp, packet->data, response.public, response.global);

  sendServerPacket((struct player_spacket *) &response);
}
#endif

#ifdef INCLUDE_SCAN
void    handleScan(packet)
struct scan_spacket *packet;
{
  struct player *pp;

#ifdef CORRUPTED_PACKETS
  if (packet->pnum >= MAXPLAYER)
    {
      fprintf(stderr, "handleScan: bad index\n");
      return;
    }
#endif

  if (packet->success)
    {
      pp = &players[packet->pnum];
      pp->p_fuel = ntohl(packet->p_fuel);
      pp->p_armies = ntohl(packet->p_armies);
      pp->p_shield = ntohl(packet->p_shield);
      pp->p_damage = ntohl(packet->p_damage);
      pp->p_etemp = ntohl(packet->p_etemp);
      pp->p_wtemp = ntohl(packet->p_wtemp);
      informScan(packet->pnum);
    }
}

informScan(p)
int     p;
{
}
#endif /* INCLUDE_SCAN */

/* UDP stuff */
void sendUdpReq(int req)
{
  struct udp_req_cpacket packet;

  memset(&packet, 0, sizeof(struct udp_req_cpacket));
  packet.type = CP_UDP_REQ;
  packet.request = req;

  if (req >= COMM_MODE)
    {
      packet.request = COMM_MODE;
      packet.connmode = req - COMM_MODE;
      sendServerPacket((struct player_spacket *) &packet);
      return;
    }
  if (req == COMM_UPDATE)
    {

#ifdef SHORT_PACKETS
      if (recv_short)
	{					 /* not necessary */
	  /* Let the client do the work, and not the network :-) */

	  resetWeaponInfo();
	}
#endif

      sendServerPacket((struct player_spacket *) &packet);
      warning("Sent request for full update");
      return;
    }
  if (req == commModeReq)
    {
      warning("Request is in progress, do not disturb");
      return;
    }
  if (req == COMM_UDP)
    {
      /* open UDP port */
      if (openUdpConn() >= 0)
	{
	  UDPDIAG(("Bound to local port %d on fd %d\n", udpLocalPort, udpSock));
	}
      else
	{
	  UDPDIAG(("Bind to local port %d failed\n", udpLocalPort));
	  commModeReq = COMM_TCP;
	  commStatus = STAT_CONNECTED;
	  commSwitchTimeout = 0;
	  if (udpWin)
	    udprefresh(UDP_STATUS);
	  warning("Unable to establish UDP connection");

	  return;
	}
    }
  /* send the request */
  packet.type = CP_UDP_REQ;
  packet.request = req;
  packet.port = htonl(udpLocalPort);

#ifdef GATEWAY
  if (!strcmp(serverName, gw_mach))
    {
      packet.port = htons(gw_serv_port);	 /* gw port that server * *
						  * should call */
      UDPDIAG(("+ Telling server to contact us on %d\n", gw_serv_port));
    }
#endif

#ifdef UDP_PORTSWAP
  if (portSwap)
    packet.connmode = CONNMODE_PORT;		 /* have him send his port */
  else
#endif
    packet.connmode = CONNMODE_PACKET;		 /* we get addr from packet */

  sendServerPacket((struct player_spacket *) &packet);

  /* update internal state stuff */
  commModeReq = req;
  if (req == COMM_TCP)
    commStatus = STAT_SWITCH_TCP;
  else
    commStatus = STAT_SWITCH_UDP;
  commSwitchTimeout = 25;			 /* wait 25 updates (about *
						  * * five seconds) */

  UDPDIAG(("Sent request for %s mode\n", (req == COMM_TCP) ?
	   "TCP" : "UDP"));

#ifdef UDP_PORTSWAP
  if (!portSwap)
#endif
  if ((req == COMM_UDP) && recvUdpConn() < 0)
    {
      UDPDIAG(("Sending TCP reset message\n"));
      packet.request = COMM_TCP;
      packet.port = 0;
      commModeReq = COMM_TCP;
      sendServerPacket((struct player_spacket *) &packet);
      /* we will likely get a SWITCH_UDP_OK later; better ignore it */
      commModeReq = COMM_TCP;
      commStatus = STAT_CONNECTED;
      commSwitchTimeout = 0;
    }

  if (udpWin)
    udprefresh(UDP_STATUS);
}

void    handleUdpReply(struct udp_reply_spacket *packet)
{
  struct udp_req_cpacket response;

  UDPDIAG(("Received UDP reply %d\n", packet->reply));
  commSwitchTimeout = 0;

  memset(&response, 0, sizeof(struct udp_req_cpacket));
  response.type = CP_UDP_REQ;

  switch (packet->reply)
    {
    case SWITCH_TCP_OK:
      if (commMode == COMM_TCP)
	{
	  UDPDIAG(("Got SWITCH_TCP_OK while in TCP mode; ignoring\n"));
	}
      else
	{
	  commMode = COMM_TCP;
	  commStatus = STAT_CONNECTED;
	  warning("Switched to TCP-only connection");
	  closeUdpConn();
	  UDPDIAG(("UDP port closed\n"));
	  if (udpWin)
	    {
	      udprefresh(UDP_STATUS);
	      udprefresh(UDP_CURRENT);
	    }
	}
      break;
    case SWITCH_UDP_OK:
      if (commMode == COMM_UDP)
	{
	  UDPDIAG(("Got SWITCH_UDP_OK while in UDP mode; ignoring\n"));
	}
      else
	{
	  /* the server is forcing UDP down our throat? */
	  if (commModeReq != COMM_UDP)
	    {
	      UDPDIAG(("Got unsolicited SWITCH_UDP_OK; ignoring\n"));
	    }
	  else
	    {

#ifdef UDP_PORTSWAP
	      if (portSwap)
		{
		  udpServerPort = ntohl(packet->port);
		  if (connUdpConn() < 0)
		    {
		      UDPDIAG(("Unable to connect, resetting\n"));
		      warning("Connection attempt failed");
		      commModeReq = COMM_TCP;
		      commStatus = STAT_CONNECTED;
		      if (udpSock >= 0)
			closeUdpConn();
		      if (udpWin)
			{
			  udprefresh(UDP_STATUS);
			  udprefresh(UDP_CURRENT);
			}
		      response.request = COMM_TCP;
		      response.port = 0;
		      goto send;
		    }
		}
#else
	      /* this came down UDP, so we MUST be connected */
	      /* (do the verify thing anyway just for kicks) */
#endif

	      UDPDIAG(("Connected to server's UDP port\n"));
	      commStatus = STAT_VERIFY_UDP;
	      if (udpWin)
		udprefresh(UDP_STATUS);
	      response.request = COMM_VERIFY;	 /* send verify request on *
						  * * UDP */
	      response.port = 0;
	      commSwitchTimeout = 25;		 /* wait 25 updates */
	    send:
	      sendServerPacket((struct player_spacket *) &response);
	    }
	}
      break;
    case SWITCH_DENIED:
      if (ntohs(packet->port))
	{
	  UDPDIAG(("Switch to UDP failed (different version)\n"));
	  warning("UDP protocol request failed (bad version)");
	}
      else
	{
	  UDPDIAG(("Switch to UDP denied\n"));
	  warning("UDP protocol request denied");
	}
      commModeReq = commMode;
      commStatus = STAT_CONNECTED;
      commSwitchTimeout = 0;
      if (udpWin)
	udprefresh(UDP_STATUS);
      if (udpSock >= 0)
	closeUdpConn();
      break;
    case SWITCH_VERIFY:
      UDPDIAG(("Received UDP verification\n"));
      break;
    default:
      fprintf(stderr, "netrek: Got funny reply (%d) in UDP_REPLY packet\n",
	      packet->reply);
      break;
    }
}

#define MAX_PORT_RETRY  10
static int openUdpConn(void)
{
  struct sockaddr_in addr;
  struct hostent *hp;
  int     attempts;

  if (udpSock >= 0)
    {
      fprintf(stderr, "netrek: tried to open udpSock twice\n");
      return 0;				 /* pretend we succeeded * *
						  * (this could be bad) */
    }
  if ((udpSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
      perror("netrek: unable to create DGRAM socket");
      return -1;
    }

#ifdef nodef
  set_udp_opts(udpSock);
#endif /* nodef */

  if (udpSock >= max_fd)
    max_fd = udpSock + 1;

  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_family = AF_INET;

  errno = 0;
  udpLocalPort = (getpid() & 32767) + (RANDOM() % 256);

  /* if baseLocalPort is defined, we want to start from that */
  if (baseLocalPort)
    {
      udpLocalPort = baseLocalPort;
      UDPDIAG(("using base port %d\n", baseLocalPort));
    }

  for (attempts = 0; attempts < MAX_PORT_RETRY; attempts++)
    {
      while (udpLocalPort < 2048)
	{
	  udpLocalPort = (udpLocalPort + 10687) & 32767;
	}

#ifdef GATEWAY
      /* we need the gateway to know where to find us */
      if (!strcmp(serverName, gw_mach))
	{
	  UDPDIAG(("+ gateway test: binding to %d\n", gw_local_port));
	  udpLocalPort = gw_local_port;
	}
#endif

      addr.sin_port = htons(udpLocalPort);
      if (bind(udpSock, (struct sockaddr *) &addr, sizeof(addr)) >= 0)
	break;

      /* bind() failed, so find another port.  If we're tunneling through a * 
       * 
       * * router-based firewall, we just increment; otherwise we try to mix
       * it * * up a little.  The check for ports < 2048 is done above. */
      if (baseLocalPort)
	udpLocalPort++;
      else
	udpLocalPort = (udpLocalPort + 10687) & 32767;
    }
  if (attempts == MAX_PORT_RETRY)
    {
      perror("netrek: bind");
      UDPDIAG(("Unable to find a local port to bind to\n"));
      close(udpSock);
      udpSock = -1;
      return -1;
    }
  UDPDIAG(("Local port is %d\n", udpLocalPort));

  /* determine the address of the server */
  if (!serveraddr)
    {
      if ((addr.sin_addr.s_addr = inet_addr(serverName)) == -1)
	{
	  if ((hp = gethostbyname(serverName)) == NULL)
	    {
	      printf("Who is %s?\n", serverName);
	      terminate(0);
	    }
	  else
	    {
	      addr.sin_addr.s_addr = *(LONG *) hp->h_addr;
	    }
	}
      serveraddr = addr.sin_addr.s_addr;
      UDPDIAG(("Found serveraddr == 0x%lx\n", serveraddr));
    }
  return 0;
}

#ifdef UDP_PORTSWAP
int connUdpConn()
{
  struct sockaddr_in addr;

  addr.sin_addr.s_addr = serveraddr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(udpServerPort);

  UDPDIAG(("Connecting to host 0x%lx on port %d\n", serveraddr, udpServerPort));
  if (connect(udpSock, (struct sockaddr *) &addr, sizeof(addr)) < 0)
    {
      perror("netrek: unable to connect UDP socket");
      printUdpInfo();				 /* debug */
      return -1;
    }

#ifdef nodef
  len = sizeof(addr);
  if (getsockname(udpSock, &addr, &len) < 0)
    {
      perror("netrek: unable to getsockname(UDP)");
      UDPDIAG(("Can't get our own socket; connection failed\n"));
      close(udpSock);
      udpSock = -1;
      return -1;
    }
  printf("udpLocalPort %d, getsockname port %d\n",
	 udpLocalPort, addr.sin_port);
#endif

  return 0;
}
#endif

static int recvUdpConn(void)
{
  fd_set  readfds;
  struct timeval to;
  struct sockaddr_in from;
  int     res;
  socklen_t fromlen;

  MZERO(&from, sizeof(from));			 /* don't get garbage if * *
						  * really broken */
  ns_init(3);

  /* we patiently wait until the server sends a packet to us */
  /* (note that we silently eat the first one) */
  UDPDIAG(("Issuing recvfrom() call\n"));
  printUdpInfo();
  fromlen = sizeof(from);
  FD_ZERO(&readfds);
  FD_SET(udpSock, &readfds);
  to.tv_sec = 6;				 /* wait 3 seconds, then * *
						  * abort */
  to.tv_usec = 0;
  if ((res = SELECT(max_fd, &readfds, 0, 0, &to)) <= 0)
    {
      if (!res)
	{
	  UDPDIAG(("timed out waiting for response\n"));
	  warning("UDP connection request timed out");
	  return -1;
	}
      else
	{
	  perror("select() before recvfrom()");
	  return -1;
	}
    }
  if (recvfrom(udpSock, buf, BUFSIZE, 0, (struct sockaddr *) &from, &fromlen) < 0)
    {
      perror("recvfrom");
      UDPDIAG(("recvfrom failed, aborting UDP attempt\n"));
      return -1;
    }
  if (from.sin_addr.s_addr != serveraddr)
    {
      /* safe? */
      serveraddr = from.sin_addr.s_addr;
      UDPDIAG(("Warning: from 0x%x, but server is 0x%lx\n",
	       from.sin_addr.s_addr, serveraddr));
    }
  if (from.sin_family != AF_INET)
    {
      UDPDIAG(("Warning: not AF_INET (%d)\n", from.sin_family));
    }
  udpServerPort = ntohs(from.sin_port);
  UDPDIAG(("recvfrom() succeeded; will use server port %d\n", udpServerPort));

#ifdef GATEWAY
  if (!strcmp(serverName, gw_mach))
    {
      UDPDIAG(("+ actually, I'm going to use %d\n", gw_port));
      udpServerPort = gw_port;
      from.sin_port = htons(udpServerPort);
    }
#endif

  if (connect(udpSock, (struct sockaddr *) &from, sizeof(from)) < 0)
    {
      perror("netrek: unable to connect UDP socket after recvfrom()");
      close(udpSock);
      udpSock = -1;
      return -1;
    }
  return 0;
}

int closeUdpConn(void)
{
  V_UDPDIAG(("Closing UDP socket\n"));
  if (udpSock < 0)
    {
      fprintf(stderr, "netrek: tried to close a closed UDP socket\n");
      return -1;
    }
  shutdown(udpSock, 2);
  close(udpSock);
  udpSock = -1;
  return 0;
}

void printUdpInfo(void)
/* doesn't use UDPDIAG macro because that is for level 2 of udpDebug;
 * printUdpInfo applies to level 1 as well */
#define PUDPDIAG(x)      { if (udpDebug) { printf("UDP: "); printf x; }}
{
  struct sockaddr_in addr;
  socklen_t len;

  len = sizeof(addr);
  if (getsockname(udpSock, (struct sockaddr *) &addr, &len) < 0)
    {
      /* perror("printUdpInfo: getsockname"); */
      return;
    }

  PUDPDIAG(("LOCAL: addr=0x%x, family=%d, port=%d\n", addr.sin_addr.s_addr,
	    addr.sin_family, ntohs(addr.sin_port)));

  if (getpeername(udpSock, (struct sockaddr *) &addr, &len) < 0)
    {
      /* perror("printUdpInfo: getpeername"); */
      return;
    }
  PUDPDIAG(("PEER : addr=0x%x, family=%d, port=%d\n", addr.sin_addr.s_addr,
	    addr.sin_family, ntohs(addr.sin_port)));
}
#undef PUDPDIAG

void    handleSequence(struct sequence_spacket *packet)
{
  static int recent_count = 0, recent_dropped = 0;
  LONG    newseq;

  drop_flag = 0;
  if (chan != udpSock)
    return;					 /* don't pay attention to *
						  * * TCP sequence #s */
  udpTotal++;
  recent_count++;

  /* update percent display every 256 updates (~50 seconds usually) */
  if (!(udpTotal & 0xff))
    if (udpWin)
      udprefresh(UDP_DROPPED);

  newseq = (LONG) ntohs(packet->sequence);
  /* printf("read %d - ", newseq); */

  if (((unsigned short) sequence) > 65000 &&
      ((unsigned short) newseq) < 1000)
    {
      /* we rolled, set newseq = 65536+sequence and accept it */
      sequence = ((sequence + 65536) & 0xffff0000) | newseq;
    }
  else
    {
      /* adjust newseq and do compare */
      newseq |= (sequence & 0xffff0000);

      if (!udpSequenceChk)
	{					 /* put this here so that * * 
						  * turning seq check */
	  sequence = newseq;			 /* on and off doesn't make * 
						  * 
						  * * us think we lost */
	  return;				 /* a whole bunch of packets. 
						  * 
						  */
	}
      if (newseq > sequence)
	{
	  /* accept */
	  if (newseq != sequence + 1)
	    {
	      udpDropped += (newseq - sequence) - 1;
	      udpTotal += (newseq - sequence) - 1;	/* want TOTAL packets 
							 * 
							 */
	      recent_dropped += (newseq - sequence) - 1;
	      recent_count += (newseq - sequence) - 1;
	      if (udpWin)
		udprefresh(UDP_DROPPED);
	      UDPDIAG(("sequence=%ld, newseq=%ld, we lost some\n",
		       sequence, newseq));
	    }
	  sequence = newseq;
	  /* S_P2 */
	  if (shortversion == SHORTVERSION && recv_short)
	    {
	      me->p_flags = (me->p_flags & 0xffff00ff) | (unsigned int) packet->flag16 << 8;
	    }
	}
      else
	{
	  /* reject */
	  if (packet->type == SP_SC_SEQUENCE)
	    {
	      V_UDPDIAG(("(ignoring repeat %ld)\n", newseq));
	    }
	  else
	    {
	      UDPDIAG(("sequence=%ld, newseq=%ld, ignoring transmission\n",
		       sequence, newseq));
	    }
	  /* the remaining packets will be dropped and we shouldn't count the
	   * * * SP_SEQUENCE packet either */
	  packets_received--;
	  drop_flag = 1;
	}
    }
  /* printf("newseq %d, sequence %d\n", newseq, sequence); */
  if (recent_count > UDP_RECENT_INTR)
    {
      /* once a minute (at 5 upd/sec), report on how many were dropped */
      /* during the last UDP_RECENT_INTR updates                       */
      udpRecentDropped = recent_dropped;
      recent_count = recent_dropped = 0;
      if (udpWin)
	udprefresh(UDP_DROPPED);
    }
}


#ifdef PACKET_LOG
static int Max_CPS = 0;
static int Max_CPSout = 0;
static time_t Start_Time = 0;
static double s2 = 0;
static int sumpl = 0;
static int numpl = 0;
static int outdata_this_sec = 0;
static double sout2 = 0;
static int sumout = 0;

/* HW clumsy but who cares ... :-) */
static int vari_sizes[NUM_PACKETS];
static int cp_msg_size;				 /* For CP_S_MESSAGE */

void Log_Packet(char type, int act_size)
{
  static time_t lasttime;
  static int data_this_sec;
  time_t  this_sec;

  if (log_packets == 0)
    return;

  if (type <= 0 && type > NUM_PACKETS)
    {
      fprintf(stderr, "Attempted to log a bad packet? \n");
      return;
    }
  packet_log[(unsigned) type]++;
  /* data_this_sec += handlers[type].size; */
  data_this_sec += act_size;			 /* HW */
  ALL_BYTES += act_size;			 /* To get all bytes */
  if (handlers[(unsigned) type].size == -1)
    {						 /* vari packet */
      vari_sizes[(unsigned) type] += act_size;
    }
  this_sec = time(NULL);
  if (this_sec != lasttime)
    {
      lasttime = this_sec;
      if (log_packets > 1)
	{
	  fprintf(stdout, "%d %d %d\n", (int) (this_sec - Start_Time), data_this_sec, outdata_this_sec);
	}
      if (Start_Time == 0)
	{
	  Start_Time = this_sec;
	}
      /* ignore baudage on the first few seconds of reception -- * that's * * 
       * when we get crushed by the motd being sent */
      if (lasttime > Start_Time + 10)
	{
	  if (data_this_sec > Max_CPS)
	    Max_CPS = data_this_sec;
	  if (outdata_this_sec > Max_CPSout)
	    Max_CPSout = outdata_this_sec;
	  sumpl += data_this_sec;
	  s2 += (data_this_sec * data_this_sec);
	  sout2 += outdata_this_sec * outdata_this_sec;
	  sumout += outdata_this_sec;
	  numpl++;
	}
      data_this_sec = 0;
      outdata_this_sec = 0;
    }
}

void Log_OPacket(int type, int size)
{
  /* Log Packet will handle the per second resets of this */
  if (log_packets == 0)
    return;
  outpacket_log[type]++;
  outdata_this_sec += size;

#ifdef SHORT_PACKETS
  if (type == CP_S_MESSAGE)
    cp_msg_size += size;			 /* HW */
#endif
}

/* print out out the cool information on packet logging */
void Dump_Packet_Log_Info(void)
{
  int     i;
  time_t  Now;
  int     total_bytes = 0;
  int     outtotal_bytes = 0;
  int     calc_temp;

  Now = time(NULL);

  printf("Packet Logging Summary:\n");
  printf("Start time: %s ", ctime(&Start_Time));
  printf(" End time: %s Elapsed play time: %3.2f min\n",
	 ctime(&Now), (float) ((Now - Start_Time) / 60));
  printf("Maximum CPS in during normal play: %d bytes per sec\n", Max_CPS);
  printf("Standard deviation in: %d\n",
	 (int) sqrt((numpl * s2 - sumpl * sumpl) / (numpl * (numpl - 1))));
  printf("Maximum CPS out during normal play: %d bytes per sec\n", Max_CPSout);
  printf("Standard deviation out: %d\n",
     (int) sqrt((numpl * sout2 - sumout * sumout) / (numpl * (numpl - 1))));

#ifdef SHORT_PACKETS
  /* total_bytes = ALL_BYTES; *//* Hope this works  HW */
  for (i = 0; i <= NUM_PACKETS; i++)
    {						 /* I think it must be <= */
      if (handlers[i].size != -1)
	total_bytes += handlers[i].size * packet_log[i];
      else
	total_bytes += vari_sizes[i];
    }						 /* The result should be == * 
						  * 
						  * * ALL_BYTES HW */
#else
  for (i = 0; i <= NUM_PACKETS; i++)
    {
      total_bytes += handlers[i].size * packet_log[i];
    }
#endif

  for (i = 0; i <= NUM_SIZES; i++)
    {

#ifdef SHORT_PACKETS
      if (handlers[i].size != -1)
	outtotal_bytes += outpacket_log[i] * sizes[i];
      else
	outtotal_bytes += cp_msg_size;		 /* HW */
#else
      outtotal_bytes += outpacket_log[i] * sizes[i];
#endif
    }

  printf("Total bytes received %d, average CPS: %4.1f\n",
	 total_bytes, (float) (total_bytes / (Now - Start_Time)));
  printf("Server Packets Summary:\n");
  printf("Num #Rcvd    Size   TotlBytes   %%Total\n");
  for (i = 0; i <= NUM_PACKETS; i++)
    {

#ifdef SHORT_PACKETS
      if (handlers[i].size != -1)
	calc_temp = handlers[i].size * packet_log[i];
      else
	calc_temp = vari_sizes[i];

      printf("%3d %5d    %4d   %9d   %3.2f\n",
	     i, packet_log[i], handlers[i].size, calc_temp,
	     (float) (calc_temp * 100 / total_bytes));
#else
      calc_temp = handlers[i].size * packet_log[i];
      printf("%3d %5d    %4d   %9d   %3.2f\n",
	     i, packet_log[i], handlers[i].size, calc_temp,
	     (float) (calc_temp * 100 / total_bytes));
#endif
    }
  printf("Total bytes sent %d, average CPS: %4.1f\n",
	 outtotal_bytes, (float) (outtotal_bytes / (Now - Start_Time)));
  printf("Client Packets Summary:\n");
  printf("Num #Sent    Size   TotlBytes   %%Total\n");
  for (i = 0; i <= NUM_SIZES; i++)
#ifdef SHORT_PACKETS
    {
      if (sizes[i] == -1)
	calc_temp = cp_msg_size;
      else
	calc_temp = sizes[i] * outpacket_log[i];
      printf("%3d %5d    %4d   %9d   %3.2f\n",
	     i, outpacket_log[i], sizes[i], calc_temp,
	     (float) (calc_temp * 100 / outtotal_bytes));
    }
#else
    {
      calc_temp = sizes[i] * outpacket_log[i];
      printf("%3d %5d    %4d   %9d   %3.2f\n",
	     i, outpacket_log[i], sizes[i], calc_temp,
	     (float) (calc_temp * 100 / outtotal_bytes));
    }
#endif
}

void print_packet(char *packet, int size)
{
   int i;                        /* lcv */
   unsigned char *data;
   int kills, pnum, nplanets;
   struct planet_s_spacket *plpacket;

   if(log_packets == 0) return;

   switch ( packet[0] )
     {
       case SP_MESSAGE:
         fprintf(stderr, "\nS->C SP_MESSAGE\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  m_flags=0x%0X, m_recpt=%d, m_from=%d, mesg=\"%s\",", 
		   ((struct mesg_spacket *) packet)->m_flags, 
		   ((struct mesg_spacket *) packet)->m_recpt, 
		   ((struct mesg_spacket *) packet)->m_from, 
		   ((struct mesg_spacket *) packet)->mesg );
	 break;
       case SP_PLAYER_INFO  :                   /* general player info not */ 
						/* * elsewhere */
	 fprintf(stderr, "\nS->C SP_PLAYER_INFO\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  pnum=%d, shiptype=%d, team=%d,",
		   ((struct plyr_info_spacket *) packet)->pnum,
		   ((struct plyr_info_spacket *) packet)->shiptype,
		   ((struct plyr_info_spacket *) packet)->team );
         break;
       case SP_KILLS        :                   /* # kills a player has */
	 fprintf(stderr, "\nS->C SP_KILLS\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  pnum=%d, kills=%u,",
		   ((struct kills_spacket *) packet)->pnum,
		   ntohl(((struct kills_spacket *) packet)->kills) );
	 break;
       case SP_PLAYER       :                   /* x,y for player */
	 fprintf(stderr, "\nS->C SP_PLAYER\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  pnum=%d, dir=%u, speed=%d, x=%d, y=%d,",
		   ((struct player_spacket *) packet)->pnum,
		   ((struct player_spacket *) packet)->dir,
		   ((struct player_spacket *) packet)->speed,
		   ntohl(((struct player_spacket *) packet)->x),
		   ntohl(((struct player_spacket *) packet)->y) );

	 break;
       case SP_TORP_INFO    :                   /* torp status */
	 fprintf(stderr, "\nS->C SP_TORP_INFO\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  war=%d, status=%d, tnum=%u,",
		   ((struct torp_info_spacket *) packet)->war,
		   ((struct torp_info_spacket *) packet)->status,
		   ntohs(((struct torp_info_spacket *) packet)->tnum) );
	 break;
       case SP_TORP         :                   /* torp location */
	 fprintf(stderr, "\nS->C SP_TORP\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  dir=%d, tnum=%u, x=%u, y=%u,",
		   ((struct torp_spacket *) packet)->dir,
		   ntohs(((struct torp_spacket *) packet)->tnum),
		   ntohl(((struct torp_spacket *) packet)->x),
		   ntohl(((struct torp_spacket *) packet)->y) );
	 break;
       case SP_PHASER       :                   /* phaser status and * *
						 * direction */
	 fprintf(stderr, "\nS->C SP_PHASER\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  pnum=%d, status=%d, dir=%u, x=%d, y=%d, target=%d,",
		   ((struct phaser_spacket *) packet)->pnum,
		   ((struct phaser_spacket *) packet)->status,
		   ((struct phaser_spacket *) packet)->dir,
		   ntohl(((struct phaser_spacket *) packet)->x),
		   ntohl(((struct phaser_spacket *) packet)->y),
		   ntohl(((struct phaser_spacket *) packet)->target) );
	 break;
       case SP_PLASMA_INFO  :                  /* player login information */
	 fprintf(stderr, "\nS->C SP_PLASMA_INFO\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  war=%d, status=%d  pnum=%u,",
		   ((struct plasma_info_spacket *) packet)->war,
		   ((struct plasma_info_spacket *) packet)->status,
		   ntohs(((struct plasma_info_spacket *) packet)->pnum) );
	 break;
       case SP_PLASMA       :                  /* like SP_TORP */
	 fprintf(stderr, "\nS->C SP_PLASMA\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  pnum=%u, x=%d, y=%d,",
		   ntohs(((struct plasma_spacket *) packet)->pnum),
		   ntohl(((struct plasma_spacket *) packet)->x),
		   ntohl(((struct plasma_spacket *) packet)->y) );
	 break;
       case SP_WARNING      :                  /* like SP_MESG */
	 fprintf(stderr,"\nS->C SP_WARNING\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  mesg=\"%s\",",
		   ((struct warning_spacket *) packet)->mesg);
	 break;
       case SP_MOTD         :                  /* line from .motd screen */
	 fprintf(stderr,"\nS->C SP_MOTD\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  line=\"%s\",",
		   ((struct motd_spacket *) packet)->line);
	 break;
       case SP_YOU          :                  /* info on you? */
	 fprintf(stderr, "\nS->C SP_YOU\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  pnum=%d, hostile=%d, swar=%d, armies=%d, flags=0x%0X, damage=%d, shield=%d, fuel=%d, etemp=%u, wtemp=%u, whydead=%u, whodead=%u,",
		   ((struct you_spacket *) packet)->pnum,
		   ((struct you_spacket *) packet)->hostile,
		   ((struct you_spacket *) packet)->swar,
		   ((struct you_spacket *) packet)->armies,
		   ntohs(((struct you_spacket *) packet)->flags),
		   ntohl(((struct you_spacket *) packet)->damage),
		   ntohl(((struct you_spacket *) packet)->shield),
		   ntohl(((struct you_spacket *) packet)->fuel),
		   ntohs(((struct you_spacket *) packet)->etemp),
		   ntohs(((struct you_spacket *) packet)->wtemp),
		   ntohs(((struct you_spacket *) packet)->whydead),
		   ntohs(((struct you_spacket *) packet)->whodead) );
	 break;
       case SP_QUEUE        :                  /* estimated loc in queue? */
	 fprintf(stderr, "\nS->C SP_QUEUE\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  pos=%u,",
		   ntohs(((struct queue_spacket *) packet)->pos) );
	 break;
       case SP_STATUS       :                  /* galaxy status numbers */
	 fprintf(stderr, "\nS->C SP_STATUS\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  tourn=%d, armsbomb=%u, planets=%u, kills=%u, losses=%u, time=%u, timeprod=%u,",
		   ((struct status_spacket *) packet)->tourn,
		   ntohl(((struct status_spacket *) packet)->armsbomb),
		   ntohl(((struct status_spacket *) packet)->planets),
		   ntohl(((struct status_spacket *) packet)->kills),
		   ntohl(((struct status_spacket *) packet)->losses),
		   ntohl(((struct status_spacket *) packet)->time),
		   ntohl(((struct status_spacket *) packet)->timeprod) );
	 break;
       case SP_PLANET       :                  /* planet armies & * *
						* facilities */
	 fprintf(stderr, "\nS->C SP_PLANET\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  pnum=%d, owner=%d, info=%d, flags=0x%0X, armies=%d,",
		   ((struct planet_spacket *) packet)->pnum,
		   ((struct planet_spacket *) packet)->owner,
		   ((struct planet_spacket *) packet)->info,
		   ntohs(((struct planet_spacket *) packet)->flags),
		   ntohl(((struct planet_spacket *) packet)->armies) );
	 break;
       case SP_PICKOK       :                  /* your team & ship was * *
						* accepted */
	 fprintf(stderr, "\nS->C SP_PICKOK\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  state=%d,",
		   ((struct pickok_spacket *) packet)-> state );
	 break;
       case SP_LOGIN        :                  /* login response */
	 fprintf(stderr, "\nS->C SP_LOGIN\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  accept=%d, flags=0x%0X, keymap=\"%s\",",
		   ((struct login_spacket *) packet)->accept,
		   ntohl(((struct login_spacket *) packet)->flags),
		   ((struct login_spacket *) packet)->keymap );
	 break;
       case SP_FLAGS        :                  /* give flags for a player */
	 fprintf(stderr, "\nS->C SP_FLAGS\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  pnum=%d, flags=0x%0X,",
		   ((struct flags_spacket *) packet)->pnum,
		   ntohl(((struct flags_spacket *) packet)->flags) );
	 break;
       case SP_MASK         :                  /* tournament mode mask */
	 fprintf(stderr, "\nS->C SP_MASK\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  mask=%d,",
		   ((struct mask_spacket *) packet)->mask );
	 break;
       case SP_PSTATUS      :                  /* give status for a player */
	 fprintf(stderr, "\nS->C SP_PSTATUS\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  pnum=%d, status=%d,",
		   ((struct pstatus_spacket *) packet)->pnum,
		   ((struct pstatus_spacket *) packet)->status );
	 break;
       case SP_BADVERSION   :                  /* invalid version number */
	 fprintf(stderr, "\nS->C SP_BADVERSION\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  why=%d,",
		   ((struct badversion_spacket *) packet)->why );
	 break;
       case SP_HOSTILE      :                  /* hostility settings for a
						* * * player */
	 fprintf(stderr, "\nS->C SP_HOSTILE\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  pnum=%d, war=%d, hostile=%d,",
		   ((struct hostile_spacket *) packet)->pnum,
		   ((struct hostile_spacket *) packet)->war,
		   ((struct hostile_spacket *) packet)->hostile );
	 break;
       case SP_STATS        :                  /* a player's statistics */
	 fprintf(stderr, "\nS->C SP_STATS\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  pnum=%d, tkills=%d, tlosses=%d, kills=%d, losses=%d, tticks=%d, tplanets=%d, tarmies=%d, sbkills=%d, sblosses=%d, armies=%d, planets=%d, maxkills=%d, sbmaxkills=%d,",
		   ((struct stats_spacket *) packet)->pnum,
		   ntohl(((struct stats_spacket *) packet)->tkills),
		   ntohl(((struct stats_spacket *) packet)->tlosses),
		   ntohl(((struct stats_spacket *) packet)->kills),
		   ntohl(((struct stats_spacket *) packet)->losses),
		   ntohl(((struct stats_spacket *) packet)->tticks),
		   ntohl(((struct stats_spacket *) packet)->tplanets),
		   ntohl(((struct stats_spacket *) packet)->tarmies),
		   ntohl(((struct stats_spacket *) packet)->sbkills),
		   ntohl(((struct stats_spacket *) packet)->sblosses),
		   ntohl(((struct stats_spacket *) packet)->armies),
		   ntohl(((struct stats_spacket *) packet)->planets),
		   ntohl(((struct stats_spacket *) packet)->maxkills),
		   ntohl(((struct stats_spacket *) packet)->sbmaxkills) );
	 break;
       case SP_PL_LOGIN     :                  /* new player logs in */
	 fprintf(stderr, "\nS->C SP_PL_LOGIN\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  pnum=%d, rank=%d, name=\"%s\", monitor=\"%s\", login=\"%s\",",
		   ((struct plyr_login_spacket *) packet)->pnum,
		   ((struct plyr_login_spacket *) packet)->rank,
		   ((struct plyr_login_spacket *) packet)->name,
		   ((struct plyr_login_spacket *) packet)->monitor,
		   ((struct plyr_login_spacket *) packet)->login );
	 break;
       case SP_RESERVED     :                  /* for future use */
	 fprintf(stderr, "\nS->C SP_RESERVED\t");
         if (log_packets > 1)
           {
             fprintf(stderr, "  data=");
             for( i = 0; i < 16; i++)
               fprintf(stderr, "0x%0X ", (unsigned char)((struct reserved_spacket *) packet)->data[i]);
             fprintf(stderr, ",");
           }
	 break;
       case SP_PLANET_LOC   :                  /* planet name, x, y */
	 fprintf(stderr, "\nS->C SP_PLANET_LOC\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  pnum=%d, x=%d, y=%d, name=\"%s\",",
		   ((struct planet_loc_spacket *) packet)->pnum,
		   ntohl(((struct planet_loc_spacket *) packet)->x),
		   ntohl(((struct planet_loc_spacket *) packet)->y),
		   ((struct planet_loc_spacket *) packet)->name );
	 break;
#ifdef INCLUDE_SCAN
       /* NOTE: not implemented */
       case SP_SCAN         :                  /* ATM: results of player *
						* * scan */
	 fprintf(stderr, "\nS->C SP_SCAN\t");
	 if(log_packets > 1)
	   fprintf(stderr, "not implemented,");
	 break;
#endif
       case SP_UDP_REPLY    :                  /* notify client of UDP * *
						* status */
	 fprintf(stderr, "\nS->C SP_UDP_REPLY\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  reply=%d, port=%d,",
		   ((struct udp_reply_spacket *) packet)->reply,
		   ntohl(((struct udp_reply_spacket *) packet)->port) );
	 break;
       case SP_SEQUENCE     :                  /* sequence # packet */
	 fprintf(stderr, "\nS->C SP_SEQUENCE\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  flag16=0x%0X, sequence=%u,",
		   ((struct sequence_spacket *) packet)->flag16,
		   ntohs(((struct sequence_spacket *) packet)->sequence) );
	 break;
       case SP_SC_SEQUENCE  :                  /* this trans is * *
						* semi-critical info */
	 fprintf(stderr, "\nS->C SP_SC_SEQUENCE\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  sequence=%u,",
		   ntohs(((struct sc_sequence_spacket *) packet)->sequence) );
	 break;
       
#ifdef RSA
       case SP_RSA_KEY      :                  /* handles binary * *
						* verification */
	 fprintf(stderr, "\nS->C SP_RSA_KEY\t");
	 if(log_packets > 1)
	   {
	     fprintf(stderr, "  data=");
	     for(i = 0; i < KEY_SIZE; i++)
	       fprintf(stderr, "0x%0X ",((struct rsa_key_spacket *) packet)->data[i]);
	     fprintf(stderr, ",");
	   }
	 break;
#endif
       case SP_GENERIC_32   :
	 fprintf(stderr, "\nS->C SP_GENERIC_32\t");
	 if (log_packets > 1)
	   switch (((struct generic_32_spacket *)packet)->version) {
	   case 0x60+GENERIC_32_VERSION_A:
	     fprintf(stderr, "  version=%c, repair_time=%d, pl_orbit=%d,",
		     ((struct generic_32_spacket_a *) packet)->version,
		     ((struct generic_32_spacket_a *) packet)->repair_time,
		     ((struct generic_32_spacket_a *) packet)->pl_orbit);
	     break;
	   case 0x60+GENERIC_32_VERSION_B:
	     fprintf(stderr, "  version=%c, repair_time=%d, pl_orbit=%d, gameup=0x%x, tourn_teams=%d, tourn_age=%d, tourn_age_units=%c, tourn_remain=%d, tourn_remain_units=%c, starbase_remain=%d, team_remain=%d,",
		     ((struct generic_32_spacket_b *) packet)->version,
		     ntohs(((struct generic_32_spacket_b *) packet)->repair_time),
		     ((struct generic_32_spacket_b *) packet)->pl_orbit,
		     ntohs(((struct generic_32_spacket_b *) packet)->gameup),
		     ((struct generic_32_spacket_b *) packet)->tournament_teams,
		     ((struct generic_32_spacket_b *) packet)->tournament_age,
		     ((struct generic_32_spacket_b *) packet)->tournament_age_units,
		     ((struct generic_32_spacket_b *) packet)->tournament_remain,
		     ((struct generic_32_spacket_b *) packet)->tournament_remain_units,
		     ((struct generic_32_spacket_b *) packet)->starbase_remain,
		     ((struct generic_32_spacket_b *) packet)->team_remain );      
	     break;
	   default:
	     fprintf(stderr, "  version=0x%x (unknown)", ((struct generic_32_spacket *)packet)->version);
	   }
	 break;
       
       case SP_SHIP_CAP     :                   /* Handles server ship mods */
	 fprintf(stderr, "\nS->C SP_SHIP_CAP\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  operation=%d, s_type=%u, s_torpspeed=%u, s_phaserrange=%u, s_maxspeed=%d, s_maxfuel=%d, s_maxshield=%d, s_maxdamage=%d, s_maxwpntemp=%d, s_maxegntemp=%d, s_width=%u, s_height=%d, s_maxarmies=%d, s_letter=%d, s_name=\"%s\", s_desig1=%c, s_desig2=%c, s_bitmap=%u,",
		   ((struct ship_cap_spacket *) packet)->operation,
		   ntohs(((struct ship_cap_spacket *) packet)->s_type),
		   ntohs(((struct ship_cap_spacket *) packet)->s_torpspeed),
		   ntohs(((struct ship_cap_spacket *) packet)->s_phaserrange),
		   ((struct ship_cap_spacket *) packet)->s_maxspeed,
		   ((struct ship_cap_spacket *) packet)->s_maxfuel,
		   ((struct ship_cap_spacket *) packet)->s_maxshield,
		   ((struct ship_cap_spacket *) packet)->s_maxdamage,
		   ((struct ship_cap_spacket *) packet)->s_maxwpntemp,
		   ((struct ship_cap_spacket *) packet)->s_maxegntemp,
		   ntohs(((struct ship_cap_spacket *) packet)->s_width),
		   ntohs(((struct ship_cap_spacket *) packet)->s_height),
		   ntohs(((struct ship_cap_spacket *) packet)->s_maxarmies),
		   ((struct ship_cap_spacket *) packet)->s_letter,
		   ((struct ship_cap_spacket *) packet)->s_name,
		   ((struct ship_cap_spacket *) packet)->s_desig1,
		   ((struct ship_cap_spacket *) packet)->s_desig2,
		   ntohs(((struct ship_cap_spacket *) packet)->s_bitmap) );

	 break;
#ifdef SHORT_PACKETS
       case SP_S_REPLY      :                  /* reply to send-short * *
						* request */
	 fprintf(stderr, "\nS->C SP_S_REPLY\t");
	 if (log_packets > 1)
	   fprintf(stderr,"  repl=%d, windside=%u, gwidth=%d,",
		   ((struct shortreply_spacket *) packet)->repl,
		   ntohs(((struct shortreply_spacket *) packet)->winside),
		   ntohl(((struct shortreply_spacket *) packet)->gwidth) );
	 break;
       case SP_S_MESSAGE    :                  /* var. Message Packet */
	 fprintf(stderr, "\nS->C SP_S_MESSAGE\t");
	 if (log_packets > 1)
           
	   fprintf(stderr, "  m_flags=0x%0X, m_recpt=%u, m_from=%u, length=%u, mesg=\"%s\",",
	    	   ((struct mesg_s_spacket *) packet)->m_flags,
		   ((struct mesg_s_spacket *) packet)->m_recpt,
		   ((struct mesg_s_spacket *) packet)->m_from,
		   ((struct mesg_s_spacket *) packet)->length,
		   &( ((struct mesg_s_spacket *) packet)->mesg ) );
	 break;
       case SP_S_WARNING    :                  /* Warnings with 4  Bytes */
	 fprintf(stderr, "\nS->C SP_S_WARNING\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  whichmessage=%u, argument=%d, argument2=%d,",
		   ((struct warning_s_spacket *) packet)->whichmessage,
		   ((struct warning_s_spacket *) packet)->argument,
		   ((struct warning_s_spacket *) packet)->argument2 );
	 break;
       case SP_S_YOU        :                  /* hostile,armies,whydead,etc
						* * * .. */
	 fprintf(stderr, "\nS->C SP_S_YOU\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  pnum=%d, hostile=%d, swar=%d, armies=%d, whydead=%d, whodead=%d, flags=0x%0X,",
		   ((struct youshort_spacket *) packet)->pnum,
		   ((struct youshort_spacket *) packet)->hostile,
		   ((struct youshort_spacket *) packet)->swar,
		   ((struct youshort_spacket *) packet)->armies,
		   ((struct youshort_spacket *) packet)->whydead,
		   ((struct youshort_spacket *) packet)->whodead,
		   ntohl(((struct youshort_spacket *) packet)->flags) );
	 break;
       case SP_S_YOU_SS     :                  /* your ship status */
	 fprintf(stderr, "\nS->C SP_S_YOU_SS\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  ddamage=%u, shield=%u, fuel=%u, etemp=%u, wtemp=%u,",
		   ntohs(((struct youss_spacket *) packet)->damage),
		   ntohs(((struct youss_spacket *) packet)->shield),
		   ntohs(((struct youss_spacket *) packet)->fuel),
		   ntohs(((struct youss_spacket *) packet)->etemp),
		   ntohs(((struct youss_spacket *) packet)->wtemp) );
	 break;
       case SP_S_PLAYER     :                  /* variable length player *
						* * packet */
	 fprintf(stderr, "\nS->C SP_S_PLAYER\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  packets=%d, dir=%u, speed=%d, x=%d, y=%d,",
		   ((struct player_s_spacket *) packet)->packets,
		   ntohl(((struct player_s_spacket *) packet)->dir),
		   ((struct player_s_spacket *) packet)->speed,
		   ntohl(((struct player_s_spacket *) packet)->x),
		   ntohl(((struct player_s_spacket *) packet)->y) );
	 break;
#endif
       
#ifdef PING
       case SP_PING         :                  /* ping packet */
	 fprintf(stderr, "\nS->C SP_PING\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  number=%u, lag=%u, tloss_sc=%u, tloss_cs=%u, iloss_sc=%u, iloss_cs=%u,",
		   ((struct ping_spacket *) packet)->number,
		   ((struct ping_spacket *) packet)->lag,
		   ((struct ping_spacket *) packet)->tloss_sc,
		   ((struct ping_spacket *) packet)->tloss_cs,
		   ((struct ping_spacket *) packet)->iloss_sc,
		   ((struct ping_spacket *) packet)->iloss_cs );
	 break;
#endif
#ifdef FEATURE_PACKETS
       case SP_FEATURE      :
	 fprintf(stderr, "\nS->C SP_FEATURE\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  feature_type=%c, arg1=%d, arg2=%d, value=%d, name=\"%s\",",
		   ((struct feature_cpacket *) packet)->feature_type,
		   ((struct feature_cpacket *) packet)->arg1,
		   ((struct feature_cpacket *) packet)->arg2,
		   ntohl(((struct feature_cpacket *) packet)->value),
		   ((struct feature_cpacket *) packet)->name );
	 break;
#endif
       case SP_RANK         :
	 fprintf(stderr, "\nS->C SP_RANK\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  rnum=%d, rmax=%d, name=\"%s\", hours=%d, ratings=%d, offense=%d, cname=\"%s\"",
		   ((struct rank_spacket *) packet)->rnum,
		   ((struct rank_spacket *) packet)->rmax,
		   ((struct rank_spacket *) packet)->name,
		   ntohl(((struct rank_spacket *) packet)->hours),
		   ntohl(((struct rank_spacket *) packet)->ratings),
		   ntohl(((struct rank_spacket *) packet)->offense),
		   ((struct rank_spacket *) packet)->cname );
	 break;
       case SP_LTD          :
         fprintf(stderr, "\nS->C SP_LTD\t");
         if (log_packets > 2) {
           struct ltd_spacket *lp = (struct ltd_spacket *) packet;
           fprintf(stderr,
                   "  version='%c', "
                   "kt=%d, "
                   "kmax=%d/100, "
                   "k1=%d, "
                   "k1p=%d, "
                   "k1c=%d, "
                   "k2=%d, "
                   "k2p=%d, "
                   "k2c=%d, "
                   "kbp=%d, "
                   "kbt=%d, "
                   "kbs=%d, "
                   "dt=%d, "
                   "dpc=%d, "
                   "dcc=%d, "
                   "ddc=%d, "
                   "dbp=%d, "
                   "dbt=%d, "
                   "dbs=%d, "
                   "acc=%d, "
                   "ptt=%d, "
                   "pdt=%d, "
                   "bpt=%d, "
                   "bp8=%d, "
                   "bpc=%d, "
                   "bat=%d, "
                   "ba8=%d, "
                   "bac=%d, "
                   "oat=%d, "
                   "odc=%d, "
                   "occ=%d, "
                   "opc=%d, "
                   "ogc=%d, "
                   "oec=%d, "
                   "olc=%d, "
                   "osba=%d, "
                   "ofc=%d, "
                   "ofa=%d, "
                   "at=%d, "
                   "aa=%d, "
                   "ar=%d, "
                   "af=%d, "
                   "ak=%d, "
                   "ct=%d, "
                   "cp=%d, "
                   "cc=%d, "
                   "ca=%d, "
                   "cr=%d, "
                   "cf=%d, "
                   "tt=%d, "
                   "tyel=%d, "
                   "tred=%d, "
                   "tz0=%d, "
                   "tz1=%d, "
                   "tz2=%d, "
                   "tz3=%d, "
                   "tz4=%d, "
                   "tz5=%d, "
                   "tz6=%d, "
                   "tz7=%d, "
                   "tpc=%d, "
                   "tcc=%d, "
                   "tr=%d, "
                   "dr=%d, "
                   "wpf=%d, "
                   "wph=%d, "
                   "wpdi=%d, "
                   "wpdt=%d, "
                   "wtf=%d, "
                   "wth=%d, "
                   "wtd=%d, "
                   "wts=%d, "
                   "wtw=%d, "
                   "wtdi=%d, "
                   "wtdt=%d, "
                   "wsf=%d, "
                   "wsh=%d, "
                   "wsp=%d, "
                   "wsw=%d, "
                   "wsdi=%d, "
                   "wsdt=%d, \n",
                   lp->version,
                   ntohl(lp->kt),
                   ntohl(lp->kmax),
                   ntohl(lp->k1),
                   ntohl(lp->k1p),
                   ntohl(lp->k1c),
                   ntohl(lp->k2),
                   ntohl(lp->k2p),
                   ntohl(lp->k2c),
                   ntohl(lp->kbp),
                   ntohl(lp->kbt),
                   ntohl(lp->kbs),
                   ntohl(lp->dt),
                   ntohl(lp->dpc),
                   ntohl(lp->dcc),
                   ntohl(lp->ddc),
                   ntohl(lp->dbp),
                   ntohl(lp->dbt),
                   ntohl(lp->dbs),
                   ntohl(lp->acc),
                   ntohl(lp->ptt),
                   ntohl(lp->pdt),
                   ntohl(lp->bpt),
                   ntohl(lp->bp8),
                   ntohl(lp->bpc),
                   ntohl(lp->bat),
                   ntohl(lp->ba8),
                   ntohl(lp->bac),
                   ntohl(lp->oat),
                   ntohl(lp->odc),
                   ntohl(lp->occ),
                   ntohl(lp->opc),
                   ntohl(lp->ogc),
                   ntohl(lp->oec),
                   ntohl(lp->olc),
                   ntohl(lp->osba),
                   ntohl(lp->ofc),
                   ntohl(lp->ofa),
                   ntohl(lp->at),
                   ntohl(lp->aa),
                   ntohl(lp->ar),
                   ntohl(lp->af),
                   ntohl(lp->ak),
                   ntohl(lp->ct),
                   ntohl(lp->cp),
                   ntohl(lp->cc),
                   ntohl(lp->ca),
                   ntohl(lp->cr),
                   ntohl(lp->cf),
                   ntohl(lp->tt),
                   ntohl(lp->tyel),
                   ntohl(lp->tred),
                   ntohl(lp->tz0),
                   ntohl(lp->tz1),
                   ntohl(lp->tz2),
                   ntohl(lp->tz3),
                   ntohl(lp->tz4),
                   ntohl(lp->tz5),
                   ntohl(lp->tz6),
                   ntohl(lp->tz7),
                   ntohl(lp->tpc),
                   ntohl(lp->tcc),
                   ntohl(lp->tr),
                   ntohl(lp->dr),
                   ntohl(lp->wpf),
                   ntohl(lp->wph),
                   ntohl(lp->wpdi),
                   ntohl(lp->wpdt),
                   ntohl(lp->wtf),
                   ntohl(lp->wth),
                   ntohl(lp->wtd),
                   ntohl(lp->wts),
                   ntohl(lp->wtw),
                   ntohl(lp->wtdi),
                   ntohl(lp->wtdt),
                   ntohl(lp->wsf),
                   ntohl(lp->wsh),
                   ntohl(lp->wsp),
                   ntohl(lp->wsw),
                   ntohl(lp->wsdi),
                   ntohl(lp->wsdt)
                   );
         }
         break;
#ifdef SHORT_PACKETS
       case SP_S_TORP       :                  /* variable length torp * *
						* packet */
	 fprintf(stderr, "\nS->C SP_S_TORP\t");
	 if (log_packets > 1)
	   print_sp_s_torp(packet, 1);
	 break;
       case SP_S_TORP_INFO  :                  /* SP_S_TORP with TorpInfo */
	 fprintf(stderr, "\nS->C SP_S_TORP_INFO\t");
	 if (log_packets > 1)         /* struct built by hand in handleVTorp */
	   print_sp_s_torp(packet, 3);
	 break;
       case SP_S_8_TORP     :                  /* optimized SP_S_TORP */
	 fprintf(stderr, "\nS->C SP_S_8_TORP\t");
	 if (log_packets > 1)
	   print_sp_s_torp(packet, 2);
	 break;
       case SP_S_PLANET     :                  /* see SP_PLANET */
	 fprintf(stderr, "\nS->C SP_S_PLANET\t"); 
	 if (log_packets > 1)
	   {
	     plpacket = (struct planet_s_spacket *) &packet[2];
	     nplanets = packet[1];
             fprintf(stderr, "nplanets = %d, ", nplanets);
             for(i = 0; i < nplanets; i++, plpacket++ )
	       fprintf(stderr, 
		       "pnum = %d, pl_owner = %d, info = %d, flags = %d, armies = %d ", 
		       plpacket->pnum, 
		       plpacket->owner, 
		       plpacket->info, 
		       plpacket->armies,
		       ntohs(plpacket->flags) );
	   }
	 fprintf(stderr,"\n");
	 break;
       
       /* S_P2 */
       case SP_S_SEQUENCE   :                  /* SP_SEQUENCE for * *
						* compressed packets */
	 fprintf(stderr, "\nS->C SP_S_SEQUENCE\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  No struct defined,");
	 break;
       case SP_S_PHASER     :                  /* see struct */
	 fprintf(stderr, "\nS->C SP_S_PHASER\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  status=%d, pnum=%d, target=%d, dir=%d, x=%d, y=%d",
		   ((((struct phaser_s_spacket *) packet)->status) & 0x0f),
		   ((((struct phaser_s_spacket *) packet)->pnum) & 0x3f),
		   ((struct phaser_s_spacket *) packet)->target,
		   ((struct phaser_s_spacket *) packet)->dir,
		   (SCALE * (ntohs(((struct phaser_s_spacket*) packet)->x))),
		   (SCALE * (ntohs(((struct phaser_s_spacket*) packet)->y))) );
	 break;
       case SP_S_KILLS      :                  /* # of kills player have */
	 fprintf(stderr, "\nS->C SP_S_KILLS\t");
	 if (log_packets > 1)
	   {
	     fprintf(stderr, "  pnum=%d, ",
		      (unsigned char) packet[1]);
             data = (unsigned char *) &packet[2];
             for (i = 0; i < (unsigned) packet[1]; i++) 
	       {
		 kills = (unsigned short) *data++;
                 kills |= (unsigned short) ((*data & 0x03) << 8);
                 pnum = (unsigned char) *data++ >> 2;
                 fprintf(stderr, "pnum = %d, kills = %d ",pnum, kills);
	       }
	   }
	 fprintf(stderr,"\n");
	 break;
       case SP_S_STATS      :                  /* see SP_STATS */
	 fprintf(stderr, "\nS->C SP_S_STATS\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  pnum=%d, tplanets=%d, tkills=%d, tlosses=%d, kills=%d, losses=%d, tticks=%d, tarmies=%d, sbkills=%d, sblosses=%d, armies=%d, planets=%d, maxkills=%d, sbmaxkills=%d,",
		   ((struct stats_spacket *) packet)->pnum,
		   ntohs(((struct stats_spacket *) packet)->tplanets),
		   ntohs(((struct stats_spacket *) packet)->tkills),
		   ntohs(((struct stats_spacket *) packet)->tlosses),
		   ntohs(((struct stats_spacket *) packet)->kills),
		   ntohs(((struct stats_spacket *) packet)->losses),
		   ntohl(((struct stats_spacket *) packet)->tticks),
		   ntohl(((struct stats_spacket *) packet)->tarmies),
		   ntohs(((struct stats_spacket *) packet)->sbkills),
		   ntohs(((struct stats_spacket *) packet)->sblosses),
		   ntohs(((struct stats_spacket *) packet)->armies),
		   ntohs(((struct stats_spacket *) packet)->planets),
		   ntohl(((struct stats_spacket *) packet)->maxkills),
		   ntohl(((struct stats_spacket *) packet)->sbmaxkills) );
	 break;
#endif
     default: 
       fprintf(stderr, "\nS->C UNKNOWN\t");
       if(log_packets > 1)
	 fprintf(stderr, "  type=%d,",packet[0]);
     }
   
#ifdef nodef /* #ifdef SHORT_PACKETS */
   switch( *((char *) packet) )
     {
       /* variable length packets */
       case VPLAYER_SIZE    :
	 fprintf(stderr, "\nS->C VPLAYER_SIZE\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  No struct defined, same enum value as SP_PLAYER,");
	 break;
       case SHORTVERSION    :                   /* other number blocks, like
						 * * * UDP Version */
	 fprintf(stderr, "\nS->C SHORTVERSION\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  No struct defined, same enum value as SP_MOTD,");
	 break;
       case OLDSHORTVERSION :                   /* S_P2 */
	 fprintf(stderr, "\nS->C OLDSHORTVERSION\t");
	 if (log_packets > 1)
	   fprintf(stderr, "  No struct defined, same enum value as SP_WARNING,");
	 break;
     }
#endif

}


void print_opacket(char *packet, int size)
{
  int i;  /* lcv */

  switch(packet[0])
    {
      /* packets sent from remote client to xtrek server */
    case CP_MESSAGE      :                    /* send a message */
      fprintf(stderr, "\nC->S CP_MESSAGE\t");
      if (log_packets > 1)
	fprintf(stderr, "  group=%d, indiv=%d, mesg=\"%s\",",
		((struct mesg_cpacket *) packet)->group,
		((struct mesg_cpacket *) packet)->indiv,
		((struct mesg_cpacket *) packet)->mesg );
      break;
    case CP_SPEED        :                    /* set speed */
      fprintf(stderr, "\nC->S CP_SPEED\t");
      if (log_packets > 1)
	fprintf(stderr, "  speed=%d,",
		((struct speed_cpacket *) packet)->speed );
      break;
    case CP_DIRECTION    :                    /* change direction */
      fprintf(stderr, "\nC->S CP_DIRECTION\t");
      if (log_packets > 1)
	fprintf(stderr, "  dir=%u,",
		((struct dir_cpacket *) packet)->dir );
      break;
    case CP_PHASER       :                    /* phaser in a direction */
      fprintf(stderr, "\nC->S CP_PHASER\t");
      if (log_packets > 1)
	fprintf(stderr, "  dir=%u,",
		((struct phaser_cpacket *) packet)-> dir );
      break;
    case CP_PLASMA       :                    /* plasma (in a direction) */
      fprintf(stderr, "\nC->S CP_PLAMSA\t");
      if (log_packets > 1)
	fprintf(stderr, "  dir=%u,",
		((struct plasma_cpacket *) packet)->dir );
      break;
    case CP_TORP         :                    /* fire torp in a direction */
      fprintf(stderr, "\nC->S CP_TORP\t");
      if (log_packets > 1)
	fprintf(stderr, "  dir=%u,",
		((struct torp_cpacket *) packet)->dir );
      break;
    case CP_QUIT         :                    /* self destruct */
      fprintf(stderr, "\nC->S CP_QUIT\t");
      if (log_packets > 1)
	fprintf(stderr, "  no args,");
      break;
    case CP_LOGIN        :                    /* log in (name, password) */
      fprintf(stderr, "\nC->S CP_LOGIN\t");
      if (log_packets > 1)
	fprintf(stderr, "  query=%d, name=\"%s\", password=\"%s\", login=\"%s\",",
		((struct login_cpacket *) packet)->query,
		((struct login_cpacket *) packet)->name,
		((struct login_cpacket *) packet)->password,
		((struct login_cpacket *) packet)->login );
      break;
    case CP_OUTFIT       :                    /* outfit to new ship */
      fprintf(stderr, "\nC->S CP_OUTFIT\t");
      if (log_packets > 1)
	fprintf(stderr, "  team=%d, ship=%d,",
		((struct outfit_cpacket *) packet)->team,
		((struct outfit_cpacket *) packet)->ship );
      break;
    case CP_WAR          :                    /* change war status */
      fprintf(stderr, "\nC->S CP_WAR\t");
      if (log_packets > 1)
	fprintf(stderr, "  newmask=0x%0X,",
		((struct war_cpacket *) packet)->newmask );
      break;
    case CP_PRACTR       :                    /* create practice robot? */
      fprintf(stderr, "\nC->S CP_PRACTR\t");
      if (log_packets > 1)
	fprintf(stderr, "  no args,");
      break;
    case CP_SHIELD       :                    /* raise/lower sheilds */
      fprintf(stderr, "\nC->S CP_SHIELD\t");
      if (log_packets > 1)
	fprintf(stderr, "  state=%d,",
		((struct shield_cpacket *) packet)->state );
      break;
    case CP_REPAIR       :                    /* enter repair mode */
      fprintf(stderr, "\nC->S CP_REPAIR\t");
      if (log_packets > 1)
	fprintf(stderr, "  state=%d,",
		((struct repair_cpacket *) packet)-> state );
      break;
    case CP_ORBIT        :                    /* orbit planet/starbase */
      fprintf(stderr, "\nC->S CP_ORBIT\t");
      if (log_packets > 1)
	fprintf(stderr, "  state=%d,",
		((struct orbit_cpacket *) packet)->state );
      break;
    case CP_PLANLOCK     :                    /* lock on planet */
      fprintf(stderr, "\nC->S CP_PLANLOCK\t");
      if (log_packets > 1)
	fprintf(stderr, "  pnum = %d,",
		((struct planlock_cpacket *) packet)->pnum );
      break;
    case CP_PLAYLOCK     :                    /* lock on player */
      fprintf(stderr, "\nC->S CP_PLAYLOCK\t");
      if (log_packets > 1)
	fprintf(stderr, "  pnum=%d,",
		((struct playlock_cpacket *) packet)->pnum );
      break;
    case CP_BOMB         :                    /* bomb a planet */
      fprintf(stderr, "\nC->S CP_BOMB\t");
      if (log_packets > 1)
	fprintf(stderr, "  state=%d,",
		((struct bomb_cpacket *) packet)->state );
      break;
    case CP_BEAM         :                    /* beam armies up/down */
      fprintf(stderr, "\nC->S CP_BEAM\t");
      if (log_packets > 1)
	fprintf(stderr, "  state=%d,",
		((struct beam_cpacket *) packet)->state );
      break;
    case CP_CLOAK        :                    /* cloak on/off */
      fprintf(stderr, "\nC->S CP_CLOAK\t");
      if (log_packets > 1)
	fprintf(stderr, "  state=%d,",
		((struct cloak_cpacket *) packet)->state );
      break;
    case CP_DET_TORPS    :                    /* detonate enemy torps */
      fprintf(stderr, "\nC->S CP_DET_TORPS\t");
      if (log_packets > 1)
	fprintf(stderr, "  no args,");
      break;
    case CP_DET_MYTORP   :                    /* detonate one of my torps */
      fprintf(stderr, "\nC->S CP_DET_MYTORP\t");
      if (log_packets > 1)
	fprintf(stderr, "  tnum=%u,",
		ntohs(((struct det_mytorp_cpacket *) packet)->tnum) );
      break;
    case CP_COPILOT      :                    /* toggle copilot mode */
      fprintf(stderr, "\nC->S CP_COPILOT\t");
      if (log_packets > 1)
	fprintf(stderr, "  state=%d,",
                ((struct copilot_cpacket *) packet)->state );
      break;
    case CP_REFIT        :                    /* refit to different ship * 
					       * 
					       * * type */
      fprintf(stderr, "\nC->S CP_REFIT\t");
      if (log_packets > 1)
	fprintf(stderr, "  ship=%d,",
		((struct refit_cpacket *) packet)->ship );
      break;
    case CP_TRACTOR      :                    /* tractor on/off */
      fprintf(stderr, "\nC->S CP_TRACTOR\t");
      if (log_packets > 1)
	fprintf(stderr, "  state=%d, pnum=%d,",
		((struct tractor_cpacket *) packet)->state,
		((struct tractor_cpacket *) packet)->pnum );
      break;
    case CP_REPRESS      :                    /* pressor on/off */
      fprintf(stderr, "\nC->S CP_REPRESS\t");
      if (log_packets > 1)
	fprintf(stderr, "  state=%d, pnum=%d,",
		((struct repress_cpacket *) packet)->state,
		((struct repress_cpacket *) packet)->pnum );
      break;
    case CP_COUP         :                    /* coup home planet */
      fprintf(stderr, "\nC->S CP_COUP\t");
      if (log_packets > 1)
	fprintf(stderr, "  no args,");
      break;
    case CP_SOCKET       :                    /* new socket for * *
				               * reconnection */
      fprintf(stderr, "\nC->S CP_SOCKET\t");
      if (log_packets > 1)
	fprintf(stderr, "  version=%d, udp_version=%d\n, socket=%u,",
		((struct socket_cpacket *) packet)->version, 
		((struct socket_cpacket *) packet)->udp_version,
		ntohl(((struct socket_cpacket *) packet)->socket)  );
      break;
    case CP_OPTIONS      :                    /* send my options to be * * 
					       * saved */
      fprintf(stderr, "\nC->S CP_OPTIONS\t");
      if (log_packets > 1)
	fprintf(stderr, "  flags=0x%0X, keymap=\"%s\",",
		ntohl(((struct options_cpacket *) packet)->flags),
		((struct options_cpacket *) packet)->keymap );
      break;
    case CP_BYE          :                    /* I'm done! */
      fprintf(stderr, "\nC->S CP_BYE\t");
      if (log_packets > 1)
	fprintf(stderr, "  no args,");
      break;
    case CP_DOCKPERM     :                    /* set docking permissions */
      fprintf(stderr, "\nC->S CP_DOCKPERM\t");
      if (log_packets > 1)
	fprintf(stderr, "  state=%d,",
		((struct dockperm_cpacket *) packet)->state );
      break;
    case CP_UPDATES      :                    /* set number of usecs per * 
				               * 
					       * * update */
      fprintf(stderr, "\nC->S CP_UPDATES\t");
      if (log_packets > 1)
	fprintf(stderr, "  usecs=%u,",
		ntohl(((struct updates_cpacket *) packet)->usecs) );
      break;
    case CP_RESETSTATS   :                    /* reset my stats packet */
      fprintf(stderr, "\nC->S CP_RESETSTATS\t");
      if (log_packets > 1)
	fprintf(stderr, "  verify=%c,",
		((struct resetstats_cpacket *) packet)->verify );
      break;
    case CP_RESERVED     :                    /* for future use */
      fprintf(stderr, "\nC->S CP_RESERVED\t");
      if (log_packets > 1)
	{
	  fprintf(stderr, "  data=" );
	  for( i = 0; i < 16; i++) 
            fprintf(stderr, "0x%0X ",  (unsigned char)((struct reserved_cpacket *) packet)->data[i]);
	  fprintf(stderr, ", resp=" );
	  for( i = 0; i < 16; i++) 
            fprintf(stderr, "0x%0X ",  (unsigned char)((struct reserved_cpacket *) packet)->resp[i]);
	  fprintf(stderr, ",");
	}
      break;
      
#ifdef INCLUDE_SCAN
      /* NOTE: not implemented. */
    case CP_SCAN         :                    /* ATM: request for player * 
					       * 
					       * * scan */
      fprintf(stderr, "\nC->S CP_SCAN\t");
      if (log_packets > 1)
	fprintf(stderr, "  not implemented," );
      break;
#endif

    case CP_UDP_REQ      :                    /* request UDP on/off */
      fprintf(stderr, "\nC->S CP_UDP_REQ\t");
      if (log_packets > 1)
	fprintf(stderr, "  request=%d, connmode=%d, port=%d,",
		((struct udp_req_cpacket *) packet)->request,
		((struct udp_req_cpacket *) packet)->connmode,
		ntohl(((struct udp_req_cpacket *) packet)->port) );
      break;
    case CP_SEQUENCE     :                    /* sequence # packet */
      fprintf(stderr, "\nC->S CP_SEQUENCE\t");
      if (log_packets > 1)
	fprintf(stderr, "  sequence=%u,",
		ntohs(((struct sequence_cpacket *) packet)->sequence) );
      break;

#ifdef RSA
    case CP_RSA_KEY      :                    /* handles binary * *
					       * verification */
      fprintf(stderr, "\nC->S CP_RSA_KEY\t");
      if (log_packets > 1)
	{
	fprintf(stderr, "  global=");
	for(i = 0; i < KEY_SIZE; i++)
	  fprintf(stderr, "0x%0X ",((struct rsa_key_cpacket *)packet)->global[i]);
	fprintf(stderr,",");
	fprintf(stderr, "  public=");
	for(i = 0; i < KEY_SIZE; i++)
	  fprintf(stderr, "0x%0X ",((struct rsa_key_cpacket *)packet)->public[i]);
	fprintf(stderr,",");
	fprintf(stderr, "  resp=");
	for(i = 0; i < KEY_SIZE; i++)
	  fprintf(stderr, "0x%0X ",((struct rsa_key_cpacket *)packet)->resp[i]);
	fprintf(stderr,",");
	}
      break;
#endif
    case CP_PING_RESPONSE :                   /* client response */
      /* note: ping.c calls gwrite directly, so we do not see this */
      fprintf(stderr, "\nC->S CP_PING_RESPONSE\t");
      if (log_packets > 1)
	fprintf(stderr, "  number=%u, pingme=%d, cp_sent=%d, cp_recv=%d",
		((struct ping_cpacket *) packet)->number,
		((struct ping_cpacket *) packet)->pingme,
		ntohl(((struct ping_cpacket *) packet)->cp_sent),
		ntohl(((struct ping_cpacket *) packet)->cp_recv) );
      break;
      
#ifdef SHORT_PACKETS
    case CP_S_REQ        :          
      fprintf(stderr, "\nC->S CP_S_REQ\t");
      if (log_packets > 1)
	fprintf(stderr, "  req=%d, version=%d,",
		((struct shortreq_cpacket *) packet)->req,
		((struct shortreq_cpacket *) packet)->version );
      break;
    case CP_S_THRS       :         
      fprintf(stderr, "\nC->S CP_S_THRS\t");
      if (log_packets > 1)
	fprintf(stderr, "  thresh=%u,",
		ntohs(((struct threshold_cpacket *) packet)->thresh) );
      break;
    case CP_S_MESSAGE    :                    /* vari. Message Packet */
      fprintf(stderr, "\nC->S CP_S_MESSAGE\t");
      if (log_packets > 1)
	fprintf(stderr, "  size=%d, group=%d, indiv=%d, mess=\"%s\",",
                ((struct mesg_cpacket *) packet)->pad1,
                ((struct mesg_cpacket *) packet)->group,
                ((struct mesg_cpacket *) packet)->indiv,
                ((struct mesg_cpacket *) packet)->mesg );
      break;
    case CP_S_RESERVED   :      
      fprintf(stderr, "\nC->S CP_S_RESERVED\t");
      if (log_packets > 1)
	fprintf(stderr, "  no struct defined,");
      break;
    case CP_S_DUMMY      :
      fprintf(stderr, "\nC->S CP_S_DUMMY\t");
      if (log_packets > 1)
	fprintf(stderr, "  no struct defined,");
      break;
#endif

#ifdef FEATURE_PACKETS
    case CP_FEATURE      :  
      fprintf(stderr, "\nC->S CP_FEATURE\t");
      if (log_packets > 1)
	fprintf(stderr, "  feature_type=%c, arg1=%d, arg2=%d, value=%d, name=\"%s\",",
		((struct feature_cpacket *) packet)->feature_type,
		((struct feature_cpacket *) packet)->arg1,
		((struct feature_cpacket *) packet)->arg2,
		ntohl(((struct feature_cpacket *) packet)->value),
		((struct feature_cpacket *) packet)->name );
      break;
#endif
    default             :
       fprintf(stderr, "\nC->S UNKNOWN\t");
       if(log_packets > 1)
	 fprintf(stderr, "  type=%d,",packet[0]);
    }
  

}

#endif /* PACKET_LOG */

char   *
        strcpyp_return(register char *s1, register char *s2, register int length)
{
  while (length && *s2)
    {
      *s1++ = *s2++;
      length--;
    }
  if (length > 0)
    {
      while (length--)
	*s1++ = ' ';
    }
  return s1;
}
