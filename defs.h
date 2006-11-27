

/* defs.h
 *
 * $Log: defs.h,v $
 * Revision 1.3  2006/09/19 10:20:39  quozl
 * ut06 full screen, det circle, quit on motd, add icon, add desktop file
 *
 * Revision 1.2  1999/08/05 16:46:32  siegl
 * remove several defines (BRMH, RABBITEARS, NEWDASHBOARD2)
 *
 * Revision 1.1.1.1  1998/11/01 17:24:09  siegl
 * COW 3.0 initial revision
 * */

#ifndef _h_defs
#define _h_defs


#include "copyright.h"


/* #$!@$#% length of address field of messages */
#define ADDRLEN 10

#ifdef NBT
#define MAX_MACRO       500
#endif

#ifndef MAXPLAYER
#define MAXPLAYER 36            /* Defined in config.h as well */
#endif

#define TESTERS 4               /* Priveledged slots for
                                 * robots and game 'testers' */
#define MAXPLANETS 40
#define MAXTORP 8
#define MAXPLASMA 1
#define PKEY 128


#ifdef SMALL_SCREEN
#define GWINSIDE 400
#define TWINSIDE 500
#else
#define GWINSIDE 500
#define TWINSIDE 500				 /* Size of strategic and * * 
						  * tactical windows */
#endif

#define BORDER 3				 /* border width for option * 
						  * 
						  * * windows */
#define PSEUDOSIZE 16
#define CLOAK_PHASES 7          /* number of drawing phases
                                 * in a cloak
                                 * engage/disengage */
#define NUMRANKS 9

/* These are configuration definitions */

#define GWIDTH 100000				 /* galaxy is 100000 spaces * 
						  * 
						  * * on a side */
#define WARP1 20				 /* warp one will move 20 * * 
						  * spaces per update */
#define SCALE 40				 /* Window will be one pixel
						  * * * for 20 spaces */
#define EXPDIST 350				 /* At this range a torp will
						  * * * explode */
#define DETDIST 1700				 /* At this range a player *
						  * * can detonate a torp */
#define NUM_PLANET_BITMAPS 7
#define NUM_PLANET_BITMAPS2 8

#define PHASEDIST 6000				 /* At this range a player *
						  * * can do damage with * *
						  * phasers */
#define ENTORBDIST 900				 /* At this range a player *
						  * * can orbit a planet */
#define ORBDIST 800				 /* A player will orbit at *
						  * * this radius */
#define ORBSPEED 2				 /* This is the fastest a * * 
						  * person can go into orbit */
#define PFIREDIST 1500				 /* At this range a planet *
						  * * will shoot at a player */
#define UPDATE 100000				 /* Update time is 100000 * * 
						  * micro-seconds */

/* 6 minutes is maximum for autoquit -- anything more causes problems in the
 * server.  (?) */
#define AUTOQUIT 9*60           /* auto logout in 5 minutes */

#define VACANT -1               /* indicates vacant port on
                                 * a starbase */
#define DOCKDIST 600
#define DOCKSPEED 2             /* If base is moving, there
                                 * will be some finesse
                                 * involved to dock */
#define NUMPORTS 4
#define SBFUELMIN 10000         /* If starbase's fuel is
                                 * less than this, it will
                                 * not refuel docked vessels */
#define TRACTDIST   6000        /* maximum effective tractor
                                 * beam range */
#define TRACTEHEAT  5           /* ammount tractor beams
                                 * heat engines */
#define TRACTCOST   20          /* fuel cost of activated
                                 * tractor beam */


/* RSA stuff */
#define RSA_VERSION "RSA v2.0 CLIENT"   /* string must begin with
                                         * characters "RSA" */
#define KEY_SIZE 32
#define RESERVED_SIZE 16
#define NAME_LEN 16
#define KEYMAP_LEN 96

#define MSG_LEN 80


/* These are memory sections */
#define PLAYER 1
#define MAXMESSAGE 50
#define MAXREVIEWMESSAGE 20

#define rosette(x)   ((((x) + 8) / 16) & 15)
/* #define rosette(x)   ((((x) + 256/VIEWS/2) / (256/VIEWS) + VIEWS) % VIEWS) */
/* (((x + 8) / 16 + 16)  %  16)  */

/* These are the teams */
/* Note that I used bit types for these mostly for messages and war status.
 * This was probably a mistake.  It meant that Ed had to add the 'remap' area
 * to map these (which are used throughout the code as the proper team
 * variable) into a nice four team deep array for his color stuff.  Oh well. */
#define NOBODY 0x0
#define FED 0x1
#define ROM 0x2
#define KLI 0x4
#define ORI 0x8
#define ALLTEAM (FED|ROM|KLI|ORI)
#define MAXTEAM (ORI)
#define NUMTEAM 4
/* * These are random configuration variables */
#define VICTORY 3               /* Number of systems needed
                                 * to conquer the galaxy */
#define WARNTIME 30             /* Number of updates to have
                                 * a warning on the screen */
#define MESSTIME 30             /* Number of updates to have
                                 * a message on the screen */

#define TARG_PLAYER	0x1     /* Flags for gettarget */
#define TARG_PLANET	0x2
#define TARG_CLOAK	0x4     /* Include cloaked ships in
                                 * search */
#define TARG_SELF	0x8
#define TARG_ENEMY      0x10
#define TARG_FRIEND     0x20


#define DEFAULT_PORT	2592

/* Other stuff that Ed added */

#define ABS(a)			/* abs(a) */ (((a) < 0) ? -(a) : (a))

#ifndef MAX
#define MAX(a,b)		((a) > (b) ? (a) : (b))
#endif

#define myPlasmaTorp(t)		(me->p_no == (t)->pt_owner)
#define myTorp(t)		(me->p_no == (t)->t_owner)
#define friendlyPlasmaTorp(t)	((!(me->p_team & (t)->pt_war)) || (myPlasmaTorp(t)))
#define friendlyTorp(t)		((!(me->p_team & (t)->t_war)) || (myTorp(t)))
#define myPhaser(p)		(&phasers[me->p_no] == (p))
#define friendlyPhaser(p)	(me->p_team == players[(p) - phasers].p_team)
#define myPlayer(p)		(me == (p))
#define myPlanet(p)		(me->p_team == (p)->pl_owner)
#define friendlyPlayer(p)	((!(me->p_team & \
				    ((p)->p_swar | (p)->p_hostile))) && \
				    (!((p)->p_team & \
				    (me->p_swar | me->p_hostile))))
#define isAlive(p)		(((p)->p_status == PALIVE) || ((p)->p_status == POBSERV))
#define friendlyPlanet(p)	((p)->pl_info & me->p_team && \
			     !((p)->pl_owner & (me->p_swar | me->p_hostile)))

#define isLockPlanet(p)		((me->p_flags & PFPLLOCK) && (me->p_planet == p->pl_no))
#define isLockPlayer(p)		((me->p_flags & PFPLOCK) && (me->p_playerl == p->p_no))
#define isObsLockPlayer(p)      ((me->p_flags & PFOBSERV) && (me->p_flags & PFPLOCK) && \
                                    (me->p_playerl == p->p_no))
#define torpColor(t)		\
	(myTorp(t) ? myColor : shipCol[remap[players[(t)->t_owner].p_team]])
#define plasmatorpColor(t)		\
	(myPlasmaTorp(t) ? myColor : shipCol[remap[players[(t)->pt_owner].p_team]])
#define phaserColor(p)		\
	(myPhaser(p) ? myColor : shipCol[remap[players[(p) - phasers].p_team]])
/* Cloaking phase (and not the cloaking flag) is the factor in determining
 * the color of the ship.  Color 0 is white (same as 'myColor' used to be). */
#define playerColor(p)		\
	(myPlayer(p) ? myColor : shipCol[remap[(p)->p_team]])

#ifdef RECORDGAME
#define planetColor(p)    \
  ((((p)->pl_info & me->p_team) || playback) \
   ? shipCol[remap[(p)->pl_owner]] : unColor)
#else
#define planetColor(p)		\
	(((p)->pl_info & me->p_team) ? shipCol[remap[(p)->pl_owner]] : unColor)
#endif

#define planetFont(p)           \
        (myPlanet(p) ? W_BoldFont : friendlyPlanet(p) ? W_HighlightFont \
            : W_RegularFont)

//SRS - changed this from UnderlineFont to HighlightFont
#define shipFont(p)		\
	(myPlayer(p) ? W_BoldFont : friendlyPlayer(p) ? W_HighlightFont \
	    : W_RegularFont)
#define bombingRating(p)	\
	((float) (p)->p_stats.st_tarmsbomb * status->timeprod / \
	 ((float) (p)->p_stats.st_tticks * status->armsbomb))
#define planetRating(p)		\
	((float) (p)->p_stats.st_tplanets * status->timeprod / \
	 ((float) (p)->p_stats.st_tticks * status->planets))
#define offenseRating(p)	\
	((float) (p)->p_stats.st_tkills * status->timeprod / \
	 ((float) (p)->p_stats.st_tticks * status->kills))
#define defenseRating(p)	\
	((float) (p)->p_stats.st_tticks * status->losses / \
	 ((p)->p_stats.st_tlosses!=0 ? \
	  ((float) (p)->p_stats.st_tlosses * status->timeprod) : \
	  (status->timeprod)))

#define INVISIBLE(j)        (((j)->p_flags & PFCLOAK) && \
        ((j)->p_cloakphase == (CLOAK_PHASES-1)))

#define ANNOUNCESOCKET      if (me!=NULL)printf("***  socket %d, player %d  ***\n",nextSocket,me->p_no); else


#if !defined(_IBMR2)
/* typedef enum { FALSE = 0, TRUE }
 * 
 * boolean; */
#else
/* Hmmm, this will only work if TRUE/FALSE were undef'ed first. Would that
 * break things?  I don't know.  Since "boolean" type is never used, we will
 * just leave it commented out for now.
 * 
 * typedef enum {FALSE=0, TRUE} boolean; */
#endif

#ifndef ROTATERACE
#define sendTorpReq(dir) sendShortPacket(CP_TORP, (char) dir)
#define sendPhaserReq(dir) sendShortPacket(CP_PHASER, (char) dir)
#define sendDirReq(dir) sendShortPacket(CP_DIRECTION, (char) dir)
#define sendPlasmaReq(dir) sendShortPacket(CP_PLASMA, (char) dir)
#else
#define sendTorpReq(dir) sendShortPacket(CP_TORP, (char) RotateDirSend(dir))
#define sendPhaserReq(dir) sendShortPacket(CP_PHASER, (char) RotateDirSend(dir))
#define sendDirReq(dir) sendShortPacket(CP_DIRECTION, (char) RotateDirSend(dir))
#define sendPlasmaReq(dir) sendShortPacket(CP_PLASMA, (char) RotateDirSend(dir))
#endif /* ROTATERACE */

#define sendSpeedReq(speed) sendShortPacket(CP_SPEED, (char) speed)
#define sendShieldReq(state) sendShortPacket(CP_SHIELD, (char) state)
#define sendOrbitReq(state) sendShortPacket(CP_ORBIT, (char) state)
#define sendRepairReq(state) sendShortPacket(CP_REPAIR, (char) state)
#define sendBeamReq(state) sendShortPacket(CP_BEAM, (char) state)
#define sendCopilotReq(state) sendShortPacket(CP_COPILOT, (char) state)
#define sendDetonateReq() sendShortPacket(CP_DET_TORPS, 0)
#define sendCloakReq(state) sendShortPacket(CP_CLOAK, (char) state)
#define sendBombReq(state) sendShortPacket(CP_BOMB, (char) state)
#define sendPractrReq() sendShortPacket(CP_PRACTR, 0)
#define sendWarReq(mask) sendShortPacket(CP_WAR, (char) mask)
#define sendRefitReq(ship) {sendShortPacket(CP_REFIT, (char) ship); shipchange(ship);}
#define sendPlaylockReq(pnum) sendShortPacket(CP_PLAYLOCK, (char) pnum)
#define sendPlanlockReq(pnum) sendShortPacket(CP_PLANLOCK, (char) pnum)
#define sendCoupReq() sendShortPacket(CP_COUP, 0)
#define sendQuitReq() sendShortPacket(CP_QUIT, 0)
#define sendByeReq() sendShortPacket(CP_BYE, 0)
#define sendDockingReq(state) sendShortPacket(CP_DOCKPERM, (char) state)
#define sendResetStatsReq(verify) sendShortPacket(CP_RESETSTATS, (char) verify)
#define sendScanReq(who) sendShortPacket(CP_SCAN, (char) who)  /* ATM */

#ifdef SHORT_PACKETS
/* #define sendShortReq(state)   sendShortPacket(CP_S_REQ, state) */
#endif

/* This macro allows us to time things based upon # frames / sec. */
#define ticks(x) ((x)*200000/timerDelay)

char   *getdefault(char *str);


/* UDP control stuff */

#ifdef GATEWAY
#define UDP_NUMOPTS    11
#define UDP_GW         UDP_NUMOPTS-1
#else
#define UDP_NUMOPTS    10
#endif

#define UDP_CURRENT     0
#define UDP_STATUS      1
#define UDP_DROPPED     2
#define UDP_SEQUENCE    3
#define UDP_SEND	4
#define UDP_RECV	5
#define UDP_DEBUG       6
#define UDP_FORCE_RESET	7
#define UDP_UPDATE_ALL	8
#define UDP_DONE        9
#define COMM_TCP        0
#define COMM_UDP        1
#define COMM_VERIFY     2
#define COMM_UPDATE	3
#define COMM_MODE	4
#define SWITCH_TCP_OK   0
#define SWITCH_UDP_OK   1
#define SWITCH_DENIED   2
#define SWITCH_VERIFY   3
#define CONNMODE_PORT   0
#define CONNMODE_PACKET 1
#define STAT_CONNECTED  0
#define STAT_SWITCH_UDP 1
#define STAT_SWITCH_TCP 2
#define STAT_VERIFY_UDP 3
#define MODE_TCP        0
#define MODE_SIMPLE     1
#define MODE_FAT	2
#define MODE_DOUBLE     3

#define UDP_RECENT_INTR 300
#define UDP_UPDATE_WAIT	5

/* client version of UDPDIAG */
#define UDPDIAG(x)      { if (udpDebug == 2) { printf("UDP: "); printf x; }}
#define V_UDPDIAG(x)    UDPDIAG(x)

#ifdef ROTATERACE
#define RotateDirSend(d)        (rotate?d-rotate_deg:d)
#endif

#ifdef nodef
#define RotateDir(d)            d -= 64

#define RotateDirSend(d)        (rotate?(d+64):d)

#define RotateCoord(x,y)        _RotateCoord(x,y,(3.1415927/2.))

#define UnrotateCoord(x,y)      _RotateCoord(x,y,-(3.1415927/2.))

#define _RotateCoord(x,y,rd)                            \
   {                                                    \
      extern double atan2(), hypot(), sin(), cos();     \
      double    __dir;                                  \
      double    __r, __dx, __dy;                        \
                                                        \
      __dx = (double)(x - GWIDTH/2);                    \
      __dy = (double)(GWIDTH/2 - y);                    \
      __dir = atan2(__dx,  __dy) - 3.1415927/2.;        \
      __r = hypot(__dx, __dy);                          \
      __dir -= rd;                                      \
      x = (int)(__r * cos(__dir) + GWIDTH/2);           \
      y = (int)(__r * sin(__dir) + GWIDTH/2);           \
   }
#endif /* nodef */

#define         NETSTAT_NUMFIELDS       7

/* field names */
#define         NETSTAT_SWITCH          0
#define         NETSTAT_RESET           1
#define         NETSTAT_TOTAL           2
#define         NETSTAT_LOCAL           3
#define         NETSTAT_FAILURES        4
#define         NETSTAT_NFTHRESH        5
#define         NETSTAT_DONE            6

/* misc */

#define         NETSTAT_DF_NFT          2000
#define         NETSTAT_DF_NFT_S        "2000"


#ifdef SHORT_PACKETS
#define         SPK_VOFF        0       /* variable packets off */
#define         SPK_VON         1       /* variable packets on */
#define         SPK_MOFF        2       /* message packets off */
#define         SPK_MON         3       /* message packets on */
#define         SPK_M_KILLS     4       /* send kill mesgs */
#define         SPK_M_NOKILLS   5       /* don't send kill mesgs */
#define         SPK_THRESHOLD   6       /* threshold */
#define         SPK_M_WARN      7       /* warnings */
#define         SPK_M_NOWARN    8       /* no warnings */
#define SPK_SALL 9              /* only planets,kills and
                                 * weapons */
#define         SPK_ALL 10      /* Full Update - SP_STATS */

#define         SPK_NUMFIELDS   6

#define         SPK_VFIELD      0
#define         SPK_MFIELD      1
#define         SPK_KFIELD      2
#define         SPK_WFIELD      3
#define         SPK_TFIELD      4
#define         SPK_DONE        5
#endif

#ifndef NBT
#define NBT
#endif /* NBT */

#define MAXMACLEN       85
#define NBTM            0
#define NEWM            1
#define NEWMSPEC        2
#define NEWMMOUSE       3
#define NEWMULTIM	4

#define MACRO_ME        0
#define MACRO_PLAYER    1
#define MACRO_TEAM      2
#define MACRO_FRIEND    3
#define MACRO_ENEMY     4

#ifdef TOOLS
#define TOOLSWINLEN 25
#define MAX_KEY         20
#endif

#ifdef HOCKEY_LINES
#define NUM_HOCKEY_LINES 13
#define S_LINE_HORIZONTAL 1
#define S_LINE_VERTICAL 2
#endif

#ifdef BEEPLITE
#define LITE_PLAYERS_MAP        0x01
#define LITE_PLAYERS_LOCAL      0x02
#define LITE_SELF               0x04
#define LITE_PLANETS            0x08
#define LITE_SOUNDS             0x10
#define LITE_COLOR              0x20
#define LITE_TTS                0x40
#endif

#ifdef WARP_DEAD
#define DEADPACKETS	3
#endif

#endif /* _h_defs */
