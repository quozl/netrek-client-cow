/* 
 * Include file for socket I/O xtrek.
 *
 * Kevin P. Smith 1/29/89
 */
#include "copyright2.h"

#define STATUS_TOKEN    "\t@@@"			 /* ATM */

/*
 * TCP and UDP use identical packet formats; the only difference is that,
 * when in UDP mode, all packets sent from server to client have a sequence
 * number appended to them.
 *
 * (note: ALL packets, whether sent on the TCP or UDP channel, will have
 * the sequence number.  Thus it's important that client & server agree on
 * when to switch.  This was done to keep critical and non-critical data
 * in sync.)
 */

/*
	general protocol state outline

	starting state

	CP_SOCKET
	CP_FEATURE, optional, to indicate feature packets are known
	SP_MOTD
	SP_FEATURE, only if CP_FEATURE was seen
	SP_QUEUE, optional, repeats until slot is available
	SP_YOU, indicates slot number assigned

	login state, player slot status is POUTFIT
	client shows name and password prompt and accepts input

	CP_LOGIN
	CP_FEATURE
	SP_LOGIN
	SP_YOU
	SP_PLAYER_INFO
	various other server packets

	outfit state, player slot status is POUTFIT
	client shows team selection window

	SP_MASK, sent regularly during outfit

	client accepts team selection input
	CP_OUTFIT
	SP_PICKOK, signals server acceptance of alive state

	alive state,
	server places ship in game and play begins

	SP_PSTATUS, indicates PDEAD state
	client animates explosion

	SP_PSTATUS, indicates POUTFIT state
	clients returns to team selection window

	CP_QUIT
	CP_BYE
*/

/* When making an index of a recording, if we need to save this packet. */
#define PB_CONTEXT(p) (\
        p == 60 || /* handleFeature */ \
        p == 14 || /* handleStatus */ \
        p == 24 || /* handlePlyrLogin */ \
        p == 2 ||  /* handlePlyrInfo */ \
        p == 20 || /* handlePStatus */ \
        p == 58 || /* handleVKills */ \
        p == 15 || /* handlePlanet */ \
        p == 26 || /* handlePlanetLoc */ \
        p == 59 || /* handle_s_Stats */ \
        p == 50 || /* handleVPlanet */ \
        p == 3)    /* handleKills */

#define RECORDPACKET(p) (\
        p==SP_MESSAGE ||\
 	p==SP_PLAYER_INFO ||\
 	p==SP_KILLS ||\
 	p==SP_PLAYER ||\
 	p==SP_YOU ||\
 	p==SP_STATUS ||\
 	p==SP_PLANET ||\
 	p==SP_LOGIN ||\
 	p==SP_FLAGS ||\
 	p==SP_MASK ||\
 	p==SP_PSTATUS ||\
 	p==SP_BADVERSION ||\
 	p==SP_HOSTILE ||\
 	p==SP_STATS ||\
 	p==SP_PL_LOGIN ||\
 	p==SP_RESERVED ||\
 	p==SP_PLANET_LOC)

/* packets sent from xtrek server to remote client */
#define SP_MESSAGE 	1
#define SP_PLAYER_INFO 	2			 /* general player info not * 
						  * 
						  * * elsewhere */
#define SP_KILLS	3			 /* # kills a player has */
#define SP_PLAYER	4			 /* x,y for player */
#define SP_TORP_INFO	5			 /* torp status */
#define SP_TORP		6			 /* torp location */
#define SP_PHASER	7			 /* phaser status and * *
						  * direction */
#define SP_PLASMA_INFO	8			 /* player login information */
#define SP_PLASMA	9			 /* like SP_TORP */
#define SP_WARNING	10			 /* like SP_MESG */
#define SP_MOTD		11			 /* line from .motd screen */
#define SP_YOU		12			 /* info on you? */
#define SP_QUEUE	13			 /* estimated loc in queue? */
#define SP_STATUS	14			 /* galaxy status numbers */
#define SP_PLANET 	15			 /* planet armies & * *
						  * facilities */
#define SP_PICKOK	16			 /* your team & ship was * *
						  * accepted */
#define SP_LOGIN	17			 /* login response */
#define SP_FLAGS	18			 /* give flags for a player */
#define SP_MASK		19			 /* tournament mode mask */
#define SP_PSTATUS	20			 /* give status for a player */
#define SP_BADVERSION   21			 /* invalid version number */
#define SP_HOSTILE	22			 /* hostility settings for a
						  * * * player */
#define SP_STATS	23			 /* a player's statistics */
#define SP_PL_LOGIN	24			 /* new player logs in */
#define SP_RESERVED	25			 /* for future use */
#define SP_PLANET_LOC	26			 /* planet name, x, y */

/* NOTE: not implemented */
#define SP_SCAN         27			 /* ATM: results of player *
						  * * scan */

#define SP_UDP_REPLY    28			 /* notify client of UDP * *
						  * status */
#define SP_SEQUENCE     29			 /* sequence # packet */
#define SP_SC_SEQUENCE  30			 /* this trans is * *
						  * semi-critical info */

#ifdef RSA
#define SP_RSA_KEY	31			 /* handles binary */
#endif

#define SP_GENERIC_32	32

#define SP_SHIP_CAP 39				 /* Handles server ship mods */

#ifdef SHORT_PACKETS
#define SP_S_REPLY      40			 /* reply to send-short * *
						  * request */
#define SP_S_MESSAGE    41			 /* var. Message Packet */
#define SP_S_WARNING    42			 /* Warnings with 4  Bytes */
#define SP_S_YOU        43			 /* hostile,armies,whydead,etc
						  * * * .. */
#define SP_S_YOU_SS     44			 /* your ship status */
#define SP_S_PLAYER     45			 /* variable length player *
						  * * packet */
#endif

#ifdef PING
#define SP_PING         46			 /* ping packet */
#endif

#ifdef SHORT_PACKETS
#define SP_S_TORP       47			 /* variable length torp * *
						  * packet */
#define SP_S_TORP_INFO  48			 /* SP_S_TORP with TorpInfo */
#define SP_S_8_TORP     49			 /* optimized SP_S_TORP */
#define SP_S_PLANET     50			 /* see SP_PLANET */

/* S_P2 */
#define SP_S_SEQUENCE   56			 /* SP_SEQUENCE for * *
						  * compressed packets */
#define SP_S_PHASER     57			 /* see struct */
#define SP_S_KILLS      58			 /* # of kills player have */
#define SP_S_STATS      59			 /* see SP_STATS */

#ifdef FEATURE_PACKETS
#define CP_FEATURE	60
#define SP_FEATURE	60
#endif

#define SP_RANK         61
#define SP_LTD          62      /* LTD stats for character */

/* variable length packets */
#define VPLAYER_SIZE    4
#define SHORTVERSION    11			 /* other number blocks, like
						  * * * UDP Version */
#define OLDSHORTVERSION 10			 /* S_P2 */
#endif


/* packets sent from remote client to xtrek server */
#define CP_MESSAGE      1			 /* send a message */
#define CP_SPEED	2			 /* set speed */
#define CP_DIRECTION	3			 /* change direction */
#define CP_PHASER	4			 /* phaser in a direction */
#define CP_PLASMA	5			 /* plasma (in a direction) */
#define CP_TORP		6			 /* fire torp in a direction */
#define CP_QUIT		7			 /* self destruct */
#define CP_LOGIN	8			 /* log in (name, password) */
#define CP_OUTFIT	9			 /* outfit to new ship */
#define CP_WAR		10			 /* change war status */
#define CP_PRACTR	11			 /* create practice robot? */
#define CP_SHIELD	12			 /* raise/lower sheilds */
#define CP_REPAIR	13			 /* enter repair mode */
#define CP_ORBIT	14			 /* orbit planet/starbase */
#define CP_PLANLOCK	15			 /* lock on planet */
#define CP_PLAYLOCK	16			 /* lock on player */
#define CP_BOMB		17			 /* bomb a planet */
#define CP_BEAM		18			 /* beam armies up/down */
#define CP_CLOAK	19			 /* cloak on/off */
#define CP_DET_TORPS	20			 /* detonate enemy torps */
#define CP_DET_MYTORP	21			 /* detonate one of my torps */
#define CP_COPILOT	22			 /* toggle copilot mode */
#define CP_REFIT	23			 /* refit to different ship * 
						  * 
						  * * type */
#define CP_TRACTOR	24			 /* tractor on/off */
#define CP_REPRESS	25			 /* pressor on/off */
#define CP_COUP		26			 /* coup home planet */
#define CP_SOCKET	27			 /* new socket for * *
						  * reconnection */
#define CP_OPTIONS	28			 /* send my options to be * * 
						  * saved */
#define CP_BYE		29			 /* I'm done! */
#define CP_DOCKPERM	30			 /* set docking permissions */
#define CP_UPDATES	31			 /* set number of usecs per * 
						  * 
						  * * update */
#define CP_RESETSTATS	32			 /* reset my stats packet */
#define CP_RESERVED	33			 /* for future use */

/* NOTE: not implemented. */
#define CP_SCAN         34			 /* ATM: request for player * 
						  * 
						  * * scan */

#define CP_UDP_REQ      35			 /* request UDP on/off */
#define CP_SEQUENCE     36			 /* sequence # packet */

#ifdef RSA
#define CP_RSA_KEY      37			 /* handles binary * *
						  * verification */
#endif

#define CP_PING_RESPONSE 42			 /* client response */

#ifdef SHORT_PACKETS
#define CP_S_REQ                43
#define CP_S_THRS               44
#define CP_S_MESSAGE    45			 /* vari. Message Packet */
#define CP_S_RESERVED       46
#define CP_S_DUMMY      47
#endif

#define SOCKVERSION 	4

#define UDPVERSION      10

struct packet_handler
  {
    int     size;
    void    (*handler) ();
  };

struct mesg_spacket
  {
    char    type;				 /* SP_MESSAGE */
    unsigned char m_flags;
    unsigned char m_recpt;
    unsigned char m_from;
    char    mesg[80];
  };

struct plyr_info_spacket
  {
    char    type;				 /* SP_PLAYER_INFO */
    char    pnum;
    char    shiptype;
    char    team;
  };

struct plyr_login_spacket
  {
    char    type;				 /* SP_PL_LOGIN */
    char    pnum;
    char    rank;
    char    pad1;
    char    name[16];
    char    monitor[16];
    char    login[16];
  };

struct hostile_spacket
  {
    char    type;				 /* SP_HOSTILE */
    char    pnum;
    char    war;
    char    hostile;
  };

struct stats_spacket
  {
    char    type;				 /* SP_STATS */
    char    pnum;
    char    pad1;
    char    pad2;
    LONG    tkills;				 /* Tournament kills */
    LONG    tlosses;				 /* Tournament losses */
    LONG    kills;				 /* overall */
    LONG    losses;				 /* overall */
    LONG    tticks;				 /* ticks of tournament play
						  * * * time */
    LONG    tplanets;				 /* Tournament planets */
    LONG    tarmies;				 /* Tournament armies */
    LONG    sbkills;				 /* Starbase kills */
    LONG    sblosses;				 /* Starbase losses */
    LONG    armies;				 /* non-tourn armies */
    LONG    planets;				 /* non-tourn planets */
    LONG    maxkills;				 /* max kills as player * 100 
						  * 
						  */
    LONG    sbmaxkills;				 /* max kills as sb * 100 */
  };

struct flags_spacket
  {
    char    type;				 /* SP_FLAGS */
    char    pnum;				 /* whose flags are they? */
    char    tractor;
    char    pad2;
    unsigned flags;
  };

struct kills_spacket
  {
    char    type;				 /* SP_KILLS */
    char    pnum;
    char    pad1;
    char    pad2;
    unsigned kills;				 /* where 1234=12.34 kills *
						  * * and 0=0.00 kills */
  };

struct player_spacket
  {
    char    type;				 /* SP_PLAYER */
    char    pnum;
    unsigned char dir;
    char    speed;
    LONG    x, y;
  };

struct torp_info_spacket
  {
    char    type;				 /* SP_TORP_INFO */
    char    war;
    char    status;				 /* TFREE, TDET, etc... */
    char    pad1;				 /* pad needed for cross cpu
						  * * * compatibility */
    unsigned short tnum;
    unsigned short pad2;
  };

struct torp_spacket
  {
    char    type;				 /* SP_TORP */
    unsigned char dir;
    unsigned short tnum;
    LONG    x, y;
  };

struct phaser_spacket
  {
    char    type;				 /* SP_PHASER */
    char    pnum;
    char    status;				 /* PH_HIT, etc... */
    unsigned char dir;
    LONG    x, y;
    LONG    target;
  };

struct you_spacket
  {
    char    type;				 /* SP_YOU */
    char    pnum;				 /* Guy needs to know this... 
						  * 
						  */
    char    hostile;
    char    swar;
    char    armies;
    char    tractor;
    char    pad2;
    char    pad3;
    unsigned flags;
    LONG    damage;
    LONG    shield;
    LONG    fuel;
    unsigned short etemp;
    unsigned short wtemp;
    unsigned short whydead;
    unsigned short whodead;
  };

struct status_spacket
  {
    char    type;				 /* SP_STATUS */
    char    tourn;
    char    pad1;
    char    pad2;
    unsigned armsbomb;
    unsigned planets;
    unsigned kills;
    unsigned losses;
    unsigned time;
    unsigned LONG timeprod;
  };

struct warning_spacket
  {
    char    type;				 /* SP_WARNING */
    char    pad1;
    char    pad2;
    char    pad3;
    char    mesg[80];
  };

struct planet_spacket
  {
    char    type;				 /* SP_PLANET */
    char    pnum;
    char    owner;
    char    info;
    unsigned short flags;
    unsigned short pad2;
    LONG    armies;
  };

struct torp_cpacket
  {
    char    type;				 /* CP_TORP */
    unsigned char dir;				 /* direction to fire torp */
    char    pad1;
    char    pad2;
  };

struct phaser_cpacket
  {
    char    type;				 /* CP_PHASER */
    unsigned char dir;
    char    pad1;
    char    pad2;
  };

struct speed_cpacket
  {
    char    type;				 /* CP_SPEED */
    char    speed;
    char    pad1;
    char    pad2;
  };

struct dir_cpacket
  {
    char    type;				 /* CP_DIRECTION */
    unsigned char dir;
    char    pad1;
    char    pad2;
  };

struct shield_cpacket
  {
    char    type;				 /* CP_SHIELD */
    char    state;				 /* up/down */
    char    pad1;
    char    pad2;
  };

struct repair_cpacket
  {
    char    type;				 /* CP_REPAIR */
    char    state;				 /* on/off */
    char    pad1;
    char    pad2;
  };

struct orbit_cpacket
  {
    char    type;				 /* CP_ORBIT */
    char    state;				 /* on/off */
    char    pad1;
    char    pad2;
  };

struct practr_cpacket
  {
    char    type;				 /* CP_PRACTR */
    char    pad1;
    char    pad2;
    char    pad3;
  };

struct bomb_cpacket
  {
    char    type;				 /* CP_BOMB */
    char    state;
    char    pad1;
    char    pad2;
  };

struct beam_cpacket
  {
    char    type;				 /* CP_BEAM */
    char    state;
    char    pad1;
    char    pad2;
  };

struct cloak_cpacket
  {
    char    type;				 /* CP_CLOAK */
    char    state;
    char    pad1;
    char    pad2;
  };

struct det_torps_cpacket
  {
    char    type;				 /* CP_DET_TORPS */
    char    pad1;
    char    pad2;
    char    pad3;
  };

struct copilot_cpacket
  {
    char    type;				 /* CP_COPLIOT */
    char    state;
    char    pad1;
    char    pad2;
  };

struct queue_spacket
  {
    char    type;				 /* SP_QUEUE */
    char    pad1;
    unsigned short pos;
  };

struct outfit_cpacket
  {
    char    type;				 /* CP_OUTFIT */
    char    team;
    char    ship;
    char    pad1;
  };

struct pickok_spacket
  {
    char    type;				 /* SP_PICKOK */
    char    state;
    char    pad2;
    char    pad3;
  };

struct login_cpacket
  {
    char    type;				 /* CP_LOGIN */
    char    query;
    char    pad2;
    char    pad3;
    char    name[16];
    char    password[16];
    char    login[16];
  };

struct login_spacket
  {
    char    type;				 /* SP_LOGIN */
    char    accept;				 /* 1/0 */
    char    pad2;
    char    pad3;
    LONG    flags;
    char    keymap[96];
  };

struct tractor_cpacket
  {
    char    type;				 /* CP_TRACTOR */
    char    state;
    char    pnum;
    char    pad2;
  };

struct repress_cpacket
  {
    char    type;				 /* CP_REPRESS */
    char    state;
    char    pnum;
    char    pad2;
  };

struct det_mytorp_cpacket
  {
    char    type;				 /* CP_DET_MYTORP */
    char    pad1;
    unsigned short tnum;
  };

struct war_cpacket
  {
    char    type;				 /* CP_WAR */
    char    newmask;
    char    pad1;
    char    pad2;
  };

struct refit_cpacket
  {
    char    type;				 /* CP_REFIT */
    char    ship;
    char    pad1;
    char    pad2;
  };

struct plasma_cpacket
  {
    char    type;				 /* CP_PLASMA */
    unsigned char dir;
    char    pad1;
    char    pad2;
  };

struct plasma_info_spacket
  {
    char    type;				 /* SP_PLASMA_INFO */
    char    war;
    char    status;				 /* TFREE, TDET, etc... */
    char    pad1;				 /* pad needed for cross cpu
						  * * * compatibility */
    unsigned short pnum;
    unsigned short pad2;
  };

struct plasma_spacket
  {
    char    type;				 /* SP_PLASMA */
    char    pad1;
    unsigned short pnum;
    LONG    x, y;
  };

struct playlock_cpacket
  {
    char    type;				 /* CP_PLAYLOCK */
    char    pnum;
    char    pad1;
    char    pad2;
  };

struct planlock_cpacket
  {
    char    type;				 /* CP_PLANLOCK */
    char    pnum;
    char    pad1;
    char    pad2;
  };

struct coup_cpacket
  {
    char    type;				 /* CP_COUP */
    char    pad1;
    char    pad2;
    char    pad3;
  };

struct pstatus_spacket
  {
    char    type;				 /* SP_PSTATUS */
    char    pnum;
    char    status;
    char    pad1;
  };

struct motd_spacket
  {
    char    type;				 /* SP_MOTD */
    char    pad1;
    char    pad2;
    char    pad3;
    char    line[80];
  };

struct quit_cpacket
  {
    char    type;				 /* CP_QUIT */
    char    pad1;
    char    pad2;
    char    pad3;
  };

struct mesg_cpacket
  {
    char    type;				 /* CP_MESSAGE */
    char    group;
    char    indiv;
    char    pad1;
    char    mesg[80];
  };

struct mask_spacket
  {
    char    type;				 /* SP_MASK */
    char    mask;
    char    pad1;
    char    pad2;
  };

struct socket_cpacket
  {
    char    type;				 /* CP_SOCKET */
    char    version;
    char    udp_version;			 /* was pad2 */
    char    pad3;
    unsigned socket;
  };

struct options_cpacket
  {
    char    type;				 /* CP_OPTIONS */
    char    pad1;
    char    pad2;
    char    pad3;
    unsigned flags;
    char    keymap[96];
  };

struct bye_cpacket
  {
    char    type;				 /* CP_BYE */
    char    pad1;
    char    pad2;
    char    pad3;
  };

struct badversion_spacket
  {
    char    type;				 /* SP_BADVERSION */
    char    why;
    char    pad2;
    char    pad3;
  };

struct dockperm_cpacket
  {
    char    type;				 /* CP_DOCKPERM */
    char    state;
    char    pad2;
    char    pad3;
  };

struct updates_cpacket
  {
    char    type;				 /* CP_UPDATES */
    char    pad1;
    char    pad2;
    char    pad3;
    unsigned usecs;
  };

struct resetstats_cpacket
  {
    char    type;				 /* CP_RESETSTATS */
    char    verify;				 /* 'Y' - just to make sure * 
						  * 
						  * * he meant it */
    char    pad2;
    char    pad3;
  };

struct reserved_spacket
  {
    char    type;				 /* SP_RESERVED */
    char    pad1;
    char    pad2;
    char    pad3;
    char    data[16];
  };

struct reserved_cpacket
  {
    char    type;				 /* CP_RESERVED */
    char    pad1;
    char    pad2;
    char    pad3;
    char    data[16];
    char    resp[16];
  };

struct udp_req_cpacket
  {						 /* UDP */
    char    type;				 /* CP_UDP_REQ */
    char    request;
    char    connmode;				 /* respond with port # or *
						  * * just send UDP packet? */
    char    pad2;
    int     port;				 /* compensate for hosed * *
						  * recvfrom() */
  };
struct ping_cpacket
  {
    char    type;				 /* CP_PING_RESPONSE */
    unsigned char number;			 /* id */
    char    pingme;				 /* if client wants server to
						  * * * ping */
    char    pad1;

    LONG    cp_sent;				 /* # packets sent to server */
    LONG    cp_recv;				 /* # packets recv from * *
						  * server */
  };
struct ping_spacket
  {
    char    type;				 /* SP_PING */
    unsigned char number;			 /* id (ok to wrap) */
    unsigned short lag;				 /* delay of last ping in ms */

    unsigned char tloss_sc;			 /* total loss server-client
						  * * * 0-100% */
    unsigned char tloss_cs;			 /* total loss client-server
						  * * * 0-100% */

    unsigned char iloss_sc;			 /* inc. loss server-client * 
						  * 
						  * * 0-100% */
    unsigned char iloss_cs;			 /* inc. loss client-server * 
						  * 
						  * * 0-100% */
  };
struct sequence_cpacket
  {						 /* UDP */
    char    type;				 /* CP_SEQUENCE */
    char    pad1;
    unsigned short sequence;
  };


struct sc_sequence_spacket
  {						 /* UDP */
    char    type;				 /* SP_CP_SEQUENCE */
    char    pad1;
    unsigned short sequence;
  };

struct udp_reply_spacket
  {						 /* UDP */
    char    type;				 /* SP_UDP_REPLY */
    char    reply;
    char    pad1;
    char    pad2;
    int     port;
  };

struct sequence_spacket
  {						 /* UDP */
    char    type;				 /* SP_SEQUENCE */
    unsigned char flag16;
    unsigned short sequence;
  };


struct planet_loc_spacket
  {
    char    type;				 /* SP_PLANET_LOC */
    char    pnum;
    char    pad2;
    char    pad3;
    LONG    x;
    LONG    y;
    char    name[16];
  };

#ifdef FEATURE_PACKETS
struct feature_cpacket
  {
    char    type;
    char    feature_type;
    char    arg1, arg2;
    int     value;
    char    name[80];
  };

#endif

struct rank_spacket { /* SP_RANK py-struct pending #61 */
    char        type;
    char        rnum;           /* rank number */
    char        rmax;           /* rank number maximum */
    char        pad;
    char        name[NAME_LEN]; /* full rank name */
    int         hours;          /* hundredths of hours required */
    int         ratings;        /* hundredths of ratings required */
    int         offense;        /* hundredths of offense required */
    char        cname[8];       /* short 'curt' rank name */
};

#define LTD_VERSION 'a' /* version for SP_LTD packet */

struct ltd_spacket { /* SP_LTD py-struct pending #62 */
    char        type;
    char        version;
    char        pad[2];
    unsigned int kt;    /* kills total, kills.total */
    unsigned int kmax;  /* kills max, kills.max */
    unsigned int k1;    /* kills first, kills.first */
    unsigned int k1p;   /* kills first potential, kills.first_potential */
    unsigned int k1c;   /* kills first converted, kills.first_converted */
    unsigned int k2;    /* kills second, kills.second */
    unsigned int k2p;   /* kills second potential, kills.second_potential */
    unsigned int k2c;   /* kills second converted, kills.second_converted */
    unsigned int kbp;   /* kills by phaser, kills.phasered */
    unsigned int kbt;   /* kills by torp, kills.torped */
    unsigned int kbs;   /* kills by smack, kills.plasmaed */
    unsigned int dt;    /* deaths total, deaths.total */
    unsigned int dpc;   /* deaths as potential carrier, deaths.potential */
    unsigned int dcc;   /* deaths as converted carrier, deaths.converted */
    unsigned int ddc;   /* deaths as dooshed carrier, deaths.dooshed */
    unsigned int dbp;   /* deaths by phaser, deaths.phasered */
    unsigned int dbt;   /* deaths by torp, deaths.torped */
    unsigned int dbs;   /* deaths by smack, deaths.plasmaed */
    unsigned int acc;   /* actual carriers created, deaths.acc */
    unsigned int ptt;   /* planets taken total, planets.taken */
    unsigned int pdt;   /* planets destroyed total, planets.destroyed */
    unsigned int bpt;   /* bombed planets total, bomb.planets */
    unsigned int bp8;   /* bombed planets <=8, bomb.planets_8 */
    unsigned int bpc;   /* bombed planets core, bomb.planets_core */
    unsigned int bat;   /* bombed armies total, bomb.armies */
    unsigned int ba8;   /* bombed_armies <= 8, bomb.armies_8 */
    unsigned int bac;   /* bombed armies core, bomb.armies_core */
    unsigned int oat;   /* ogged armies total, ogged.armies */
    unsigned int odc;   /* ogged dooshed carrier, ogged.dooshed */
    unsigned int occ;   /* ogged converted carrier, ogged.converted */
    unsigned int opc;   /* ogged potential carrier, ogged.potential */
    unsigned int ogc;   /* ogged bigger carrier, ogged.bigger_ship */
    unsigned int oec;   /* ogged same carrier, ogged.same_ship */
    unsigned int olc;   /* ogger smaller carrier, ogged.smaller_ship */
    unsigned int osba;  /* ogged sb armies, ogged.sb_armies */
    unsigned int ofc;   /* ogged friendly carrier, ogged.friendly */
    unsigned int ofa;   /* ogged friendly armies, ogged.friendly_armies */
    unsigned int at;    /* armies carried total, armies.total */
    unsigned int aa;    /* armies used to attack, armies.attack */
    unsigned int ar;    /* armies used to reinforce, armies.reinforce */
    unsigned int af;    /* armies ferried, armies.ferries */
    unsigned int ak;    /* armies killed, armies.killed */
    unsigned int ct;    /* carries total, carries.total */
    unsigned int cp;    /* carries partial, carries.partial */
    unsigned int cc;    /* carries completed, carries.completed */
    unsigned int ca;    /* carries to attack, carries.attack */
    unsigned int cr;    /* carries to reinforce, carries.reinforce */
    unsigned int cf;    /* carries to ferry, carries.ferries */
    unsigned int tt;    /* ticks total, ticks.total */
    unsigned int tyel;  /* ticks in yellow, ticks.yellow */
    unsigned int tred;  /* ticks in red, ticks.red */
    unsigned int tz0;   /* ticks in zone 0, ticks.zone[0] */
    unsigned int tz1;   /* ticks in zone 1, ticks.zone[1] */
    unsigned int tz2;   /* ticks in zone 2, ticks.zone[2] */
    unsigned int tz3;   /* ticks in zone 3, ticks.zone[3] */
    unsigned int tz4;   /* ticks in zone 4, ticks.zone[4] */
    unsigned int tz5;   /* ticks in zone 5, ticks.zone[5] */
    unsigned int tz6;   /* ticks in zone 6, ticks.zone[6] */
    unsigned int tz7;   /* ticks in zone 7, ticks.zone[7] */
    unsigned int tpc;   /* ticks as potential carrier, ticks.potential */
    unsigned int tcc;   /* ticks as carrier++, ticks.carrier */
    unsigned int tr;    /* ticks in repair, ticks.repair */
    unsigned int dr;    /* damage repaired, damage_repaired */
    unsigned int wpf;   /* weap phaser fired, weapons.phaser.fired */
    unsigned int wph;   /* weap phaser hit, weapons.phaser.hit */
    unsigned int wpdi;  /* weap phaser damage inflicted, weapons.phaser.damage.inflicted */
    unsigned int wpdt;  /* weap phaser damage taken, weapons.phaser.damage.taken */
    unsigned int wtf;   /* weap torp fired, weapons.torps.fired */
    unsigned int wth;   /* weap torp hit, weapons.torps.hit */
    unsigned int wtd;   /* weap torp detted, weapons.torps.detted */
    unsigned int wts;   /* weap torp self detted, weapons.torps.selfdetted */
    unsigned int wtw;   /* weap torp hit wall, weapons.torps.wall */
    unsigned int wtdi;  /* weap torp damage inflicted, weapons.torps.damage.inflicted */
    unsigned int wtdt;  /* weap torp damage taken, weapons.torps.damage.taken */
    unsigned int wsf;   /* weap smack fired, weapons.plasma.fired */
    unsigned int wsh;   /* weap smack hit, weapons.plasma.hit */
    unsigned int wsp;   /* weap smack phasered, weapons.plasma.phasered */
    unsigned int wsw;   /* weap smack hit wall, weapons.plasma.wall */
    unsigned int wsdi;  /* weap smack damage inflicted, weapons.plasma.damage.inflicted */
    unsigned int wsdt;  /* weap smack damage taken, weapons.plasma.damage.taken */
} __attribute__ ((packed));

#ifdef RSA
struct rsa_key_spacket
  {
    char    type;				 /* SP_RSA_KEY */
    char    pad1;
    char    pad2;
    char    pad3;
    unsigned char data[KEY_SIZE];
  };

struct rsa_key_cpacket
  {
    char    type;				 /* CP_RSA_KEY */
    char    pad1;
    char    pad2;
    char    pad3;
    unsigned char global[KEY_SIZE];
    unsigned char public[KEY_SIZE];
    unsigned char resp[KEY_SIZE];
  };

#endif

struct ship_cap_spacket
  {						 /* Server configuration of * 
						  * 
						  * * client */
    char    type;				 /* screw motd method */
    char    operation;				 /* 0 = add/change a ship, 1
						  * * * = remove a ship */
    unsigned short s_type;			 /* SP_SHIP_CAP */
    unsigned short s_torpspeed;
    unsigned short s_phaserrange;
    int     s_maxspeed;
    int     s_maxfuel;
    int     s_maxshield;
    int     s_maxdamage;
    int     s_maxwpntemp;
    int     s_maxegntemp;
    unsigned short s_width;
    unsigned short s_height;
    unsigned short s_maxarmies;
    char    s_letter;
    char    pad2;
    char    s_name[16];
    char    s_desig1;
    char    s_desig2;
    unsigned short s_bitmap;
  };

#ifdef SHORT_PACKETS
struct shortreq_cpacket
  {						 /* CP_S_REQ */
    char    type;
    char    req;
    char    version;
    char    pad2;
  };

struct threshold_cpacket
  {						 /* CP_S_THRS */
    char    type;
    char    pad1;
    unsigned short thresh;
  };

struct shortreply_spacket
  {						 /* SP_S_REPLY */
    char    type;
    char    repl;
    unsigned short winside;
    LONG    gwidth;
  };

struct youshort_spacket
  {						 /* SP_S_YOU */
    char    type;

    char    pnum;
    char    hostile;
    char    swar;

    char    armies;
    char    whydead;
    char    whodead;

    char    pad1;

    unsigned flags;
  };

struct youss_spacket
  {						 /* SP_S_YOU_SS */
    char    type;
    char    pad1;

    unsigned short damage;
    unsigned short shield;
    unsigned short fuel;
    unsigned short etemp;
    unsigned short wtemp;
  };

#define VPLANET_SIZE 6

struct planet_s_spacket
  {						 /* body of SP_S_PLANET  */
    char    pnum;
    char    owner;
    char    info;
    unsigned char armies;			 /* more than 255 Armies ? *
						  * * ...  */
    unsigned short flags;
  };
struct warning_s_spacket
  {						 /* SP_S_WARNING */
    char    type;
    unsigned char whichmessage;
    char    argument, argument2;		 /* for phaser  etc ... */
  };

struct player_s_spacket
  {
    char    type;				 /* SP_S_PLAYER Header */
    char    packets;				 /* How many player-packets * 
						  * 
						  * * are in this packet (
						  * only * * the first 6 bits 
						  * are * * relevant ) */
    unsigned char dir;
    char    speed;
    LONG    x, y;				 /* To get the absolute * *
						  * Position */
  };

/* S_P2 */
struct player_s2_spacket
  {
    char    type;				 /* SP_S_PLAYER Header */
    char    packets;				 /* How many player-packets * 
						  * 
						  * * are in this packet  ( *
						  * * only the firs t 6 bits *
						  * * are relevant ) */
    unsigned char dir;
    char    speed;
    short   x, y;				 /* absolute position / 40 */
    unsigned int flags;				 /* 16 playerflags */
  };

/* The format of the body: struct player_s_body_spacket {       Body of new
 * Player Packet unsigned char pnum;   0-4 = pnum, 5 local or galactic, 6 =
 * 9. x-bit, 7 9. y-bit unsigned char speeddir;       0-3 = speed , 4-7
 * direction of ship unsigned char x;      low 8 bits from X-Pixelcoordinate
 * unsigned char y;      low 8 bits from Y-Pixelcoordinate }; */

struct torp_s_spacket
  {
    char    type;				 /* SP_S_TORP */
    unsigned char bitset;			 /* bit=1 that torp is in * * 
						  * packet */
    unsigned char whichtorps;			 /* Torpnumber of first torp
						  * * * / 8 */
    unsigned char data[21];			 /* For every torp 2*9 bit *
						  * * coordinates */
  };

struct mesg_s_spacket
  {
    char    type;				 /* SP_S_MESSAGE */
    unsigned char m_flags;
    unsigned char m_recpt;
    unsigned char m_from;
    unsigned char length;			 /* Length of whole packet */
    char    mesg;
    char    pad2;
    char    pad3;
    char    pad[76];
  };

struct mesg_s_cpacket
  {
    char    type;				 /* CP_S__MESSAGE */
    char    group;
    char    indiv;
    char    length;				 /* Size of whole packet   */
    char    mesg[80];
  };

/* S_P2 */
struct kills_s_spacket
  {
    char    type;				 /* SP_S_KILLS */
    char    pnum;				 /* How many kills in packet */
    unsigned short kills;			 /* 6 bit player numer   */
    /* 10 bit kills*100     */
    unsigned short mkills[MAXPLAYER];
  };

struct phaser_s_spacket
  {
    char    type;				 /* SP_S_PHASER */
    char    status;				 /* PH_HIT, etc... */
    unsigned char pnum;				 /* both bytes are used for * 
						  * 
						  * * more */
    unsigned char target;			 /* look into the code   */
    short   x;					 /* x coord /40 */
    short   y;					 /* y coord /40 */
    unsigned char dir;
    char    pad1;
    char    pad2;
    char    pad3;
  };

struct stats_s_spacket
  {
    char    type;				 /* SP_S_STATS */
    char    pnum;
    unsigned short tplanets;			 /* Tournament planets */
    unsigned short tkills;			 /* Tournament kills */
    unsigned short tlosses;			 /* Tournament losses */
    unsigned short kills;			 /* overall */
    unsigned short losses;			 /* overall */
    unsigned int tticks;			 /* ticks of tournament play
						  * * * time */
    unsigned int tarmies;			 /* Tournament armies */
    unsigned int maxkills;
    unsigned short sbkills;			 /* Starbase kills */
    unsigned short sblosses;			 /* Starbase losses */
    unsigned short armies;			 /* non-tourn armies */
    unsigned short planets;			 /* non-tourn planets */
    unsigned int sbmaxkills;			 /* max kills as sb * 100 */
  };

#endif

struct generic_32_spacket {
    char        type;
    char        version;        /* alphabetic, 0x60 + version */
    char        pad[30];
};
#define GENERIC_32_LENGTH 32
#define COST_GENERIC_32 (F_sp_generic_32 ? GENERIC_32_LENGTH : 0)
struct generic_32_spacket_a { /* SP_GENERIC_32 py-struct "b1sHH26x" #32 */
    char           type;
    char           version;     /* alphabetic, 0x60 + version */
    unsigned short repair_time; /* server estimate of repair time in seconds */
    unsigned short pl_orbit;    /* what planet player orbiting, -1 if none */
    char           pad1[26];
    /* NOTE: this version didn't use network byte order for the shorts */
};
#define GENERIC_32_VERSION_A 1
struct generic_32_spacket_b { /* SP_GENERIC_32 py-struct "!b1sHbHBBsBsBB18x" #32 */
    char           type;
    char           version;     /* alphabetic, 0x60 + version */
    unsigned short repair_time; /* server estimate of repair time, seconds */
    char           pl_orbit;    /* what planet player orbiting, -1 if none */
    unsigned short gameup;      /* server status flags */
    unsigned char  tournament_teams; /* what teams are involved */
    unsigned char  tournament_age; /* duration of t-mode so far */
    char           tournament_age_units; /* units for above, see s2du */
    unsigned char  tournament_remain; /* remaining INL game time */
    char           tournament_remain_units; /* units for above, see s2du */
    unsigned char  starbase_remain; /* starbase reconstruction, mins   */
    unsigned char  team_remain; /* team surrender time, seconds    */
    char           pad1[18];
} __attribute__ ((packed));
#define GENERIC_32_VERSION_B 2
#define GENERIC_32_VERSION GENERIC_32_VERSION_B /* default */

/* SP_GENERIC_32 versioning instructions:

   we start with version 'a', and each time a structure is changed
   increment the version and reduce the pad size, keeping the packet
   the same size ...

   client is entitled to trust fields in struct that were defined at a
   particular version ...

   client is to send CP_FEATURE with SP_GENERIC_32 value 1 for version
   'a', value 2 for version 'b', etc ...

   server is to reply with SP_FEATURE with SP_GENERIC_32 value set to
   the maximum version it supports (not the version requested by the
   client), ...

   server is to send SP_GENERIC_32 packets of the highest version it
   knows about, but no higher than the version the client asks for.
*/
