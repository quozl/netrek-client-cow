


/* struct.h for the client of an xtrek socket protocol.
 * 
 * Most of the unneeded stuff in the structures has been thrown away.
 *
 * $Log: struct.h,v $
 * Revision 1.5  2006/09/19 10:20:39  quozl
 * ut06 full screen, det circle, quit on motd, add icon, add desktop file
 *
 * Revision 1.4  2006/05/16 06:20:18  quozl
 * add PLCORE
 *
 * Revision 1.3  2002/06/21 00:26:03  quozl
 * fix description of PFOBSERV
 *
 * Revision 1.2  1999/08/05 16:46:32  siegl
 * remove several defines (BRMH, RABBITEARS, NEWDASHBOARD2)
 *
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 * */
#include "copyright.h"

#ifndef _h_struct
#define _h_struct

struct distress
  {
    unsigned char sender;
    unsigned char dam, shld, arms, wtmp, etmp, fuelp, sts;
    unsigned char wtmpflag, etempflag, cloakflag, distype, macroflag;
    unsigned char close_pl, close_en, tclose_pl, tclose_en, pre_app, i;
    unsigned char close_j, close_fr, tclose_j, tclose_fr;
    unsigned char cclist[6];			 /* allow us some day to cc a
						  * message up to 5 people */
    /* sending this to the server allows the server to do the cc action */
    /* otherwise it would have to be the client ... less BW this way */
    char    preappend[80];			 /* text which we pre or append */
  };

struct dmacro_list
  {
    unsigned char c;
    char   *name;
    char   *macro;
  };

struct status
  {
    unsigned char tourn;			 /* Tournament mode? */
    /* These stats only updated during tournament mode */
    unsigned int armsbomb, planets, kills, losses, time;
    /* Use LONG for this, so it never wraps */
    unsigned LONG timeprod;
  };

enum dist_type
  {
    /* help me do series */
    take = 1, ogg, bomb, space_control,
    save_planet,
    base_ogg,
    help3, help4,

    /* doing series */
    escorting, ogging, bombing, controlling,
    asw,
    asbomb,
    doing3, doing4,

    /* other info series */
    free_beer,					 /* ie. player x is totally 
						  * hosed now */
    no_gas,					 /* ie. player x has no gas */
    crippled,					 /* ie. player x is way hurt
						  * but may have gas */
    pickup,					 /* player x picked up armies 
						  */
    pop,					 /* there was a pop somewhere 
						  */
    carrying,					 /* I am carrying */
    other1, other2,

    /* just a generic distress call */
    generic

#ifdef RCM
    ,rcm
#endif
  };

/* The General distress has format:
 * 
 * byte1: 00yzzzzz where zzzzz is dist_type, and y is 1 if this is a more
 * complicated macro and not just a simple distress (a simple distress will
 * ONLY send ship info like shields, armies, status, location, etc.). I guess
 * y=1 can be for !    future expansion.
 * 
 * byte2: 1fff ffff - f = percentage fuel remaining (0-100) byte3: 1ddd dddd - %
 * damage byte4: 1sss ssss - % shields remaining byte5: 1eee eeee - % etemp
 * byte6: 1www wwww - % wtemp byte7: 100a aaaa - armies carried byte8: (lsb
 * of me->p_status) & 0x80 byte9: 1ppp pppp - planet closest to me byte10:
 * 1eee eeee - enemy closest to me byte11: 1ppp pppp - planet closest to
 * target byte12: 1eee eeee - enemy closest to target byte13: 1ttt tttt -
 * tclose_j byte14: 1jjj jjjj - close_j byte15: 1fff ffff - tclose_fr byte16:
 * 1ccc cccc - close_fr byte17+: cc list (each player to cc this message to
 * is 11pp ppp) cc list is terminated by 0x80 (pre-pend) or 0100 0000
 * (append) ) byte18++: the text to pre or append .. depending on termination
 * above. text is null terminated and the last thing in this distress */

#define PFREE 0
#define POUTFIT 1
#define PALIVE 2
#define PEXPLODE 3
#define PDEAD 4
#define POBSERV 5

#define PFSHIELD	0x0001
#define PFREPAIR	0x0002
#define PFBOMB		0x0004
#define PFORBIT		0x0008
#define PFCLOAK		0x0010
#define PFWEP		0x0020
#define PFENG		0x0040
#define PFROBOT		0x0080
#define PFBEAMUP	0x0100
#define PFBEAMDOWN	0x0200
#define PFSELFDEST	0x0400
#define PFGREEN		0x0800
#define PFYELLOW	0x1000
#define PFRED		0x2000
#define PFPLOCK		0x4000			 /* Locked on a player */
#define PFPLLOCK	0x8000			 /* Locked on a planet */
#define PFCOPILOT	0x10000			 /* Allow copilots */
#define PFWAR		0x20000			 /* computer reprogramming
						  * for war */
#define PFPRACTR	0x40000			 /* practice type robot (no
						  * kills) */
#define PFDOCK          0x80000			 /* true if docked to a
						  * starbase */
#define PFREFIT         0x100000		 /* true if about to refit */
#define PFREFITTING	0x200000		 /* true if currently
						  * refitting */
#define PFTRACT  	0x400000		 /* tractor beam activated */
#define PFPRESS  	0x800000		 /* pressor beam activated */
#define PFDOCKOK	0x1000000		 /* docking permission */
#define PFOBSERV	0x8000000		 /* observer */

#define KQUIT		0x01			 /* Player quit */
#define KTORP		0x02			 /* killed by torp */
#define KPHASER		0x03			 /* killed by phaser */
#define KPLANET		0x04			 /* killed by planet */
#define KSHIP		0x05			 /* killed by other ship */
#define KDAEMON		0x06			 /* killed by dying daemon */
#define KWINNER		0x07			 /* killed by a winner */
#define KGHOST		0x08			 /* killed because a ghost */
#define KGENOCIDE	0x09			 /* killed by genocide */
#define KPROVIDENCE	0x0a			 /* killed by a hacker */
#define KPLASMA         0x0b			 /* killed by a plasma * *
						  * torpedo */
#define TOURNEND	0x0c			 /* tournament game ended */
#define KOVER		0x0d			 /* game over  */
#define TOURNSTART	0x0e			 /* tournament game starting */
#define KBADBIN		0x0f			 /* bad binary */

#define NUM_TYPES 8
#define SCOUT 0
#define DESTROYER 1
#define CRUISER 2
#define BATTLESHIP 3
#define ASSAULT 4
#define STARBASE 5
#define SGALAXY	6
#define ATT	7

struct ship
  {
    short   s_phaserdamage;
    int     s_maxspeed;
    int     s_maxfuel;
    int     s_maxshield;
    int     s_maxdamage;
    int     s_maxegntemp;
    int     s_maxwpntemp;
    short   s_maxarmies;
    short   s_width;
    short   s_height;
    short   s_type;
    int     s_torpspeed;
  };

struct stats
  {
    double  st_maxkills;			 /* max kills ever */
    int     st_kills;				 /* how many kills */
    int     st_losses;				 /* times killed */
    int     st_armsbomb;			 /* armies bombed */
    int     st_planets;				 /* planets conquered */
    int     st_ticks;				 /* Ticks I've been in game */
    int     st_tkills;				 /* Kills in tournament play */
    int     st_tlosses;				 /* Losses in tournament play */
    int     st_tarmsbomb;			 /* Tournament armies bombed */
    int     st_tplanets;			 /* Tournament planets
						  * conquered */
    int     st_tticks;				 /* Tournament ticks */
    /* SB stats are entirely separate */
    int     st_sbkills;				 /* Kills as starbase */
    int     st_sblosses;			 /* Losses as starbase */
    int     st_sbticks;				 /* Time as starbase */
    double  st_sbmaxkills;			 /* Max kills as starbase */
    LONG    st_lastlogin;			 /* Last time this player was
						  * played */
    int     st_flags;				 /* Misc option flags */

#ifdef MOUSE_AS_SHIFT
    unsigned char st_keymap[480];		 /* keymap for this player */
#else
    unsigned char st_keymap[96];		 /* keymap for this player */
#endif
    int     st_rank;				 /* Ranking of the player */
  };

#define ST_MAPMODE      1
#define ST_NAMEMODE     2
#define ST_SHOWSHIELDS  4
#define ST_KEEPPEACE    8
#define ST_SHOWLOCAL    16			 /* two bits for these two */
#define ST_SHOWGLOBAL   64

struct player
  {
    int     p_no;
    int     p_updates;				 /* Number of updates ship
						  * has survived */
    int     p_status;				 /* Player status */
    unsigned int p_flags;			 /* Player flags */
    char    p_name[16];
    char    p_login[16];
    char    p_monitor[16];			 /* Monitor being played on */
    char    p_mapchars[2];			 /* Cache for map window
						  * image */
    struct ship p_ship;				 /* Personal ship statistics */
    int     p_x;
    int     p_y;
    unsigned char p_dir;			 /* Real direction */
    unsigned char p_desdir;			 /* desired direction */
    int     p_subdir;				 /* fraction direction change 
						  */
    int     p_speed;				 /* Real speed */
    short   p_desspeed;				 /* Desired speed */
    int     p_subspeed;				 /* Fractional speed */
    short   p_team;				 /* Team I'm on */
    int     p_damage;				 /* Current damage */
    int     p_subdamage;			 /* Fractional damage repair */
    int     p_shield;				 /* Current shield power */
    int     p_subshield;			 /* Fractional shield
						  * recharge */
    short   p_cloakphase;			 /* Drawing stage of cloaking
						  * engage/disengage. */
    short   p_ntorp;				 /* Number of torps flying */
    short   p_nplasmatorp;			 /* Number of plasma torps
						  * active */
    char    p_hostile;				 /* Who my torps will hurt */
    char    p_swar;				 /* Who am I at sticky war
						  * with */
    float   p_kills;				 /* Enemies killed */
    short   p_planet;				 /* Planet orbiting or locked
						  * onto */
    short   p_playerl;				 /* Player locked onto */

#ifdef ARMY_SLIDER
    int     p_armies;				 /* XXX: for stats */
#else
    short   p_armies;
#endif						 /* ARMY_SLIDER */
    int     p_fuel;
    short   p_explode;				 /* Keeps track of final
						  * explosion */
    int     p_etemp;
    short   p_etime;
    int     p_wtemp;
    short   p_wtime;
    short   p_whydead;				 /* Tells you why you died */
    short   p_whodead;				 /* Tells you who killed you */
    struct stats p_stats;			 /* player statistics */
    short   p_genoplanets;			 /* planets taken since last
						  * genocide */
    short   p_genoarmsbomb;			 /* armies bombed since last
						  * genocide */
    short   p_planets;				 /* planets taken this game */
    short   p_armsbomb;				 /* armies bombed this game */
    int     p_docked;				 /* If starbase, # docked to,
						  * else pno base host */
    int     p_port[4];				 /* If starbase, pno of ship
						  * docked to that port,
						  * else p_port[0] = port 
						  * # docked to on host. 
						  */
    short   p_tractor;				 /* What player is in tractor
						  * lock */
    int     p_pos;				 /* My position in the player
						  * file */
  };

struct statentry
  {
    char    name[16], password[16];
    struct stats stats;
  };

/* Torpedo states */

#define TFREE 0
#define TMOVE 1
#define TEXPLODE 2
#define TDET 3
#define TOFF 4
#define TSTRAIGHT 5				 /* Non-wobbling torp */


struct torp
  {
    unsigned char t_status;			 /* State information */
    short   t_owner;
    char    t_war;				 /* enemies */
    int     t_x;
    int     t_y;
    short   t_fuse;				 /* Life left in current *
						  * state */
    unsigned char t_updateFuse;			 /* Updates before torp will
						  * expire */
    unsigned char t_dir;			 /* direction */
  };


/* Plasma Torpedo states */

#define PTFREE 0
#define PTMOVE 1
#define PTEXPLODE 2
#define PTDET 3

struct plasmatorp
  {
    unsigned char pt_status;			 /* State information */
    char    pt_war;				 /* enemies */
    short   pt_owner;
    short   pt_fuse;				 /* Life left in current * *
						  * state */
    short   pt_updateFuse;			 /* Time till expiry */
    int     pt_x;
    int     pt_y;
  };

#define PHFREE 0x00
#define PHHIT  0x01				 /* When it hits a person */
#define PHMISS 0x02
#define PHHIT2 0x04				 /* When it hits a photon */

struct phaser
  {
    unsigned char ph_status;			 /* What it's up to */
    unsigned char ph_dir;			 /* direction */
    short   ph_target;				 /* Who's being hit (for * *
						  * drawing) */
    short   ph_updateFuse;			 /* Time till expiry */
    short   ph_fuse;				 /* Life left for drawing */
    int     ph_x, ph_y;				 /* For when it hits a torp */
  };


#ifdef RSA
struct rsa_key
  {
    unsigned char client_type[KEY_SIZE];
    unsigned char architecture[KEY_SIZE];
    unsigned char global[KEY_SIZE];
    unsigned char public[KEY_SIZE];
  };

#endif

/* An important note concerning planets:  The game assumes that the planets
 * are in a 'known' order.  Ten planets per team, the first being the home
 * planet. */

/* the lower bits represent the original owning team */
#define PLREPAIR 0x010
#define PLFUEL 0x020
#define PLAGRI 0x040
#define PLREDRAW 0x080				 /* Player close for redraw */
#define PLHOME 0x100				 /* home planet for a given
						  * team */
#define PLCOUP 0x200				 /* Coup has occured */
#define PLCHEAP 0x400				 /* Planet was taken from 
						  * undefended team */
#define PLCORE 0x800
#define PLCLEAR 0x1000

struct planet
  {
    int     pl_no;
    int     pl_flags;				 /* State information */
    int     pl_owner;
    int     pl_x;
    int     pl_y;
    char    pl_name[16];
    int     pl_namelen;				 /* Cuts back on strlen's */
    int     pl_armies;
    int     pl_info;				 /* Teams which have info on
						  * planets */
    int     pl_deadtime;			 /* Time before planet will 
						  * support life */
    int     pl_couptime;			 /* Time before coup may take
						  * place */
  };

#define MVALID 0x01
#define MGOD   0x10
#define MMOO   0x12

#ifdef TOOLS
#define MTOOLS 0x14
#endif

/* order flags by importance (0x100 - 0x400) */
/* restructuring of message flags to squeeze them all into 1 byte - jmn */
/* hopefully quasi-back-compatible: MVALID, MINDIV, MTEAM, MALL, MGOD use up
 * 5 bits. this leaves us 3 bits. since the server only checks for those
 * flags when deciding message related things and since each of the above
 * cases only has 1 flag on at a time we can overlap the meanings of the
 * flags */

#define MINDIV 0x02
/* these go with MINDIV flag */

#ifdef STDBG
#define MDBG   0x20
#endif

#define MCONFIG 0x40				 /* config messages from * *
						  * server */
#define MDIST 0x60				 /* flag distress type * *
						  * messages properly */

#ifdef MULTILINE_MACROS
#define MMACRO 0x80
#endif

#define MTEAM  0x04
/* these go with MTEAM flag */
#define MTAKE  0x20
#define MDEST  0x40
#define MBOMB  0x60
#define MCOUP1 0x80
#define MCOUP2 0xA0
#define MDISTR 0xC0				 /* flag distress type
						  * messages */

#define MALL   0x08
/* these go with MALL flag */
#define MGENO  0x20				 /* MGENO is not used in INL
						  * server but beLONGs
						  * here  */
#define MCONQ  0x20				 /* not enought bits to 
						  * distinguish MCONQ/MGENO :-( */
#define MKILLA 0x40
#define MKILLP 0x60
#define MKILL  0x80
#define MLEAVE 0xA0
#define MJOIN  0xC0
#define MGHOST 0xE0
/* MMASK not used in INL server */

#define MWHOMSK  0x1f				 /* mask with this to find
						  * who msg to */
#define MWHATMSK 0xe0				 /* mask with this to find
						  * what message about */

/* old flags... 
 * #define MVALID 0x01 
 * #define MINDIV 0x02 
 * #define MTEAM  0x04
 * #define MALL   0x08 
 * #define MGOD   0x10
 * 
 * #define MGENO  0x100            order these by importance (0x100 - 0x400)
 * #define MCONQ  0x110 
 * #define MTAKE  0x120 
 * #define MDEST  0x130 
 * #define MKILLA 0x200 
 * #define MBOMB  0x210 
 * #define MKILLP 0x220 
 * #define MKILL  0x230
 * #define MLEAVE 0x300 
 * #define MJOIN  0x310 
 * #define MGHOST 0x320 
 * #define MCOUP1 0x330 
 * #define MCOUP2 0x340    
 * end of old flags  */



struct message
  {
    int     m_no;
    int     m_flags;
    int     m_time;
    int     m_recpt;
    char    m_data[80];
  };

/* message control structure */

struct mctl
  {
    int     mc_current;
  };

/* This is a structure used for objects returned by mouse pointing */

#define PLANETTYPE 0x1
#define PLAYERTYPE 0x2

struct obtype
  {
    int     o_type;
    int     o_num;
  };

struct rank
  {
    float   hours, ratings, defense;
    char   *name, *cname;
  };

struct memory
  {
    struct player players[MAXPLAYER];
    struct torp torps[MAXPLAYER * MAXTORP];
    struct plasmatorp plasmatorps[MAXPLAYER * MAXPLASMA];
    struct status status[1];
    struct planet planets[MAXPLANETS];
    struct phaser phasers[MAXPLAYER];
    struct mctl mctl[1];
    struct message messages[MAXMESSAGE];
    struct ship shipvals[NUM_TYPES];
  };

struct plupdate
  {
    int     plu_update;
    int     plu_x, plu_y;
  };

struct macro_list
  {
    int     type;
    unsigned char key;
    char    who;
    char   *string;
  };

/******************************************************************************/
/***                   Distress structure definitions                       ***/
/***                                                                        ***/
/***  The LOW_DISTRESS, MID_DISTRESS, and HIGH_DISTRESS are all used to     ***/
/***  index the correct items within the DISTRESS_BLOCK's item array.  The  ***/
/***  distress block contains a minimum and maximum value for each item, a  ***/
/***  flag to indicate whether the distress on this value is active or not, ***/
/***  and three single character pointers for different severity levels.    ***/
/******************************************************************************/

#define DIST_LOW            ( 0 )
#define DIST_MID            ( 1 )
#define DIST_HIGH           ( 2 )

#define DIST_SHIELDS        ( 0 )
#define DIST_DAMAGE         ( 1 )
#define DIST_WTEMP          ( 2 )
#define DIST_ETEMP          ( 3 )
#define DIST_ARMYS          ( 4 )
#define DIST_FUEL           ( 5 )

typedef struct distress_block
  {
    int     min, max;
    int     on;
    char   *item[3];
  }

DISTRESS_DESC;

struct distress_list
  {						 /* need one for ships and *
						  * * one for SBs */
    DISTRESS_DESC problem[6];
  };

#ifdef HOCKEY_LINES
struct s_line
  {
    int     begin_x, begin_y;			 /* Start point of the line */
    int     end_x, end_y;			 /* End point of the line   */
    W_Color color;				 /* The color of the line   */
    int     orientation;			 /* Horizontal or Vertical? */
    int    *flag;				 /* Should line be drawn?   */
  };

#endif /* HOCKEY_LINES */

struct shipdef
  {
    char   *name;
    char   *rcfile;
    unsigned char *keymap;
    unsigned char *buttonmap;
    unsigned char *ckeymap;
  };

#ifdef TOOLS
struct key_list
  {
    unsigned char dest;
    char   *name;
  };

#endif

#endif /* _h_struct */














