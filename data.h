
/* data.h
 *
 * $Log: data.h,v $
 * Revision 1.9  2006/09/19 10:20:39  quozl
 * ut06 full screen, det circle, quit on motd, add icon, add desktop file
 *
 * Revision 1.8  2002/06/21 00:29:02  quozl
 * describe playback states
 *
 * Revision 1.7  2001/04/28 04:03:56  quozl
 * change -U to also adopt a local port number for TCP mode.
 * 		-- Benjamin `Quisar' Lerman  <quisar@quisar.ambre.net>
 *
 * Revision 1.6  2000/05/19 14:24:52  jeffno
 * Improvements to playback.
 * - Can jump to any point in recording.
 * - Can lock on to cloaked players.
 * - Tactical/galactic repaint when paused.
 * - Can lock on to different players when recording paused.
 *
 * Revision 1.5  1999/08/05 16:46:32  siegl
 * remove several defines (BRMH, RABBITEARS, NEWDASHBOARD2)
 *
 * Revision 1.4  1999/07/24 19:23:43  siegl
 * New default portSwap for UDP_PORTSWAP feature
 *
 * Revision 1.3  1999/06/11 16:14:17  siegl
 * cambot replay patches
 *
 * Revision 1.2  1999/01/31 16:38:17  siegl
 * Hockey rink background XPM on galactic map in hockey mode.
 *
 * Revision 1.1.1.1  1998/11/01 17:24:09  siegl
 * COW 3.0 initial revision
 * */
#include "copyright.h"

#ifndef _h_data
#define _h_data

#define MAP_PIX   0
#define LOCAL_PIX 1
#define GHOST_PIX 2
#define GENO_PIX  3
#define GREET_PIX 4
#ifdef HOCKEY_LINES
#define HOCKEY_PIX 5
#endif

#define EX_FRAMES               5
#define SBEXPVIEWS              7
#define NUMDETFRAMES            5		 /* # frames in torp * *
						  * explosion */
#define ex_width                64
#define ex_height               64
#define sbexp_width             80
#define sbexp_height            80
#define cloud_width             8
#define cloud_height            8
#define plasmacloud_width       13
#define plasmacloud_height      13
#define etorp_width             3
#define etorp_height            3
#define eplasmatorp_width       7
#define eplasmatorp_height      7
#define mplasmatorp_width       5
#define mplasmatorp_height      5
#define mtorp_width             3
#define mtorp_height            3
#define crossmask_width         16
#define crossmask_height        16
#define planet_width            30
#define planet_height           30
#define mplanet_width           16
#define mplanet_height          16
#define shield_width            20
#define shield_height           20
#define cloak_width             20
#define cloak_height            20
#define icon_width              112
#define icon_height             80

extern struct player *players;
extern struct player *me;
extern struct torp *torps;
extern struct plasmatorp *plasmatorps;
extern struct status *status;
extern struct ship *myship;
extern struct stats *mystats;
extern struct planet *planets;
extern struct phaser *phasers;
extern struct message *messages;
extern struct mctl *mctl;
extern struct team *teams;
extern struct ship shipvals[];
extern struct memory universe;
extern struct planet pdata[];

extern int oldalert;
extern int remap[];
extern int udcounter;
extern char *title;
extern struct plupdate pl_update[];
extern char buttonmap[];
extern int messpend;

#ifdef XTRA_MESSAGE_UI
extern int messageHUD;				 /* Show message being typed

						  * 
						  * * on the local display */
extern int messHoldThresh;			 /* Threshold value for

						  * 
						  * * putting a message on
						  * hold  */
extern int messMouseDelta;			 /* To keep track of mouse

						  * 
						  * * movement delta        */
#endif
extern int lastcount;
extern int mdisplayed;
extern int lastm;
extern int delay;
extern int rdelay;
extern int namemode;
extern int warnShields;
extern int ROMVLVS;
extern int showStats;
extern int myPlanetBitmap;

#ifdef RECORDGAME
extern FILE *recordFile;			 /* recorder */
extern FILE *recordIndexFile;   /* To jump around recordings. */
extern FILE *recordContextFile; /* To jump around recordings. */
extern int playback; /* State playback is in (pause, forward, reverse) */

#define PL_OFF 0        /* not playing back, but in a real game	*/
#define PL_PAUSE 1      /* playing back, paused			*/
#define PL_FORWARD 2    /* playing back, in forward direction	*/
#define PL_REVERSE 3    /* playing back, in reverse direction	*/
#endif
extern FILE *logFile;				 /* message log */
extern int msgBeep;				 /* ATM - msg beep */
extern int warncount;
extern int warntimer;
extern int infomapped;
extern int scanmapped;				 /* ATM - scanner stuff */
extern int mustexit;
extern int messtime;
extern int keeppeace;
extern int gen_distress;

#ifdef GATEWAY
extern unsigned LONG netaddr;			 /* for blessing */

#endif

extern int messageon;

#ifdef RSA
extern char testdata[];
extern int RSA_Client;
extern char key_name[];
extern char client_type[];
extern char client_arch[];
extern char client_creator[];
extern char client_comments[];
extern char client_key_date[];

#endif

#ifdef META
extern char *metaserver;
extern int metaport;

#endif

#ifdef NBT
extern int MacroMode;
extern int macrocnt;
extern struct macro_list macro[];		 /* NBT 2/26/93 */

#endif

#ifdef ROTATERACE
extern int rotate;
extern int rotate_deg;

#endif

extern int netstat;
extern int netstatfreq;
extern W_Window netstatWin, lMeter;
extern int updatespeed;

extern int SBhours;

#ifdef SHORT_PACKETS
extern int why_dead;
extern int tryShort, tryShort1;
extern int recv_short;
extern int recv_mesg;
extern int recv_kmesg;
extern int recv_threshold;
extern char recv_threshold_s[];
extern int recv_warn;

/* S_P2 */
extern int shortversion;			 /* Which version do we use? */

#endif

extern int ghoststart;
extern int ghost_pno;
extern int keepInfo;
extern int showPlanetOwner;
extern int phaserShrink;
extern int theirPhaserShrink;
extern int shrinkPhaserOnMiss;
extern int newDashboard, old_db;
extern int niftyNewMessages;
extern int detCircle;
extern int fastQuit;
extern int babes;
extern int showlocal, showgalactic;

#ifdef HAVE_XPM
#define NO_IND_PIX 0x0001
#define NO_FED_PIX 0x0002
#define NO_ROM_PIX 0x0004
#define NO_KLI_PIX 0x0008
#define NO_ORI_PIX 0x0010
#define NO_WEP_PIX 0x0020
#define NO_EXP_PIX 0x0040
#define NO_CLK_PIX 0x0080
#define NO_MAP_PIX 0x0100
#define NO_BG_PIX  0x0400
#define NO_HALOS   0x1000
#define NO_PIXMAPS 0x8000
extern int pixMissing;
extern int pixFlags;

#endif
extern char *shipnos;
extern int sock;
extern int xtrekPort;
extern int queuePos;
extern int pickOk;
extern int lastRank;
extern int promoted;
extern int loginAccept;
extern unsigned localflags;
extern int tournMask;
extern int nextSocket;
extern char *serverName;
extern char defaultsFile[80];
extern int loggedIn;
extern int reinitPlanets;
extern int lastUpdate[];
extern int timerDelay;
extern int redrawDelay;
extern int reportKills;
extern int phaserWindow;

#ifdef PHASER_STATS
extern int phaserShowStats;
extern int phaserStatTry;
extern int phaserStatHit;

#endif
extern int censorMessages;

extern int scanplayer;
extern int showTractor;
extern int commMode;				 /* UDP */
extern int commModeReq;				 /* UDP */
extern int commStatus;				 /* UDP */
extern int commSwitchTimeout;			 /* UDP */
extern int udpTotal;				 /* UDP */
extern int udpDropped;				 /* UDP */
extern int udpRecentDropped;			 /* UDP */
extern int udpSock;				 /* UDP */
extern int udpDebug;				 /* UDP */
extern int udpClientSend;			 /* UDP */
extern int udpClientRecv;			 /* UDP */
extern int udpSequenceChk;			 /* UDP */
extern int weaponUpdate;

#ifdef GATEWAY
extern int gw_serv_port, gw_port, gw_local_port; /* UDP */
extern char *gw_mach;				 /* UDP */

#endif
extern int baseLocalPort;			 /* UDP and TCP */


extern int showTractorPressor;
extern int showLock;
extern int showPhaser;
extern int logmess;
extern int continuetractor;
extern int tcounter;
extern int autoKey;
extern int extraBorder;

/* udp options */
extern int tryUdp, tryUdp1;

extern int debug;

extern double Sin[], Cos[];

extern W_Icon stipple, clockpic, icon;

#define VIEWS 16
#define NUM_TYPES 8
extern W_Icon expview[EX_FRAMES];
extern W_Icon sbexpview[SBEXPVIEWS];
extern W_Icon cloud[NUMDETFRAMES];
extern W_Icon plasmacloud[NUMDETFRAMES];
extern W_Icon etorp, mtorp;
extern W_Icon eplasmatorp, mplasmatorp;

#ifdef VSHIELD_BITMAPS
#define SHIELD_FRAMES 5
extern W_Icon shield[SHIELD_FRAMES], cloakicon;
extern int VShieldBitmaps;

#else
extern W_Icon shield, cloakicon;

#endif

extern W_Icon tractbits, pressbits;
extern W_Icon fed_bitmaps[NUM_TYPES][VIEWS], kli_bitmaps[NUM_TYPES][VIEWS],
        rom_bitmaps[NUM_TYPES][VIEWS], ori_bitmaps[NUM_TYPES][VIEWS], ind_bitmaps[NUM_TYPES][VIEWS],
        ROMVLVS_bitmap[VIEWS];
extern W_Icon bplanets[7];
extern W_Icon mbplanets[7];
extern W_Icon bplanets2[8];
extern W_Icon mbplanets2[8];
extern W_Icon bplanets3[NUM_PLANET_BITMAPS2];	 /* isae: added this */
extern W_Icon mbplanets3[NUM_PLANET_BITMAPS2];	 /* isae: added this */
extern W_Icon noinfoplanet;
extern W_Color borderColor, backColor, textColor, myColor, warningColor,
        shipCol[5], rColor, yColor, gColor, unColor, foreColor;

/* jn - SMARTMACRO */
extern char lastMessage[];
extern int MacroNum;
extern char *classes[];
extern char teamlet[];
extern char *teamshort[];
extern char pseudo[PSEUDOSIZE];
extern char defpasswd[PSEUDOSIZE];
extern char login[PSEUDOSIZE];

extern struct rank ranks[NUMRANKS];

extern W_Window messagew, w, mapw, statwin, baseWin, infow, iconWin, tstatw,
        war, warnw, helpWin, teamWin[4], qwin, messwa, messwt, messwi,
        messwk, planetw, rankw, playerw, optionWin, reviewWin;
extern W_Window scanw, scanwin, udpWin, phaserwin;

#ifdef SHORT_PACKETS
extern W_Window spWin;

#endif

#ifdef NBT
extern W_Window macroWin;

#endif

#ifdef META
extern W_Window metaWin;

#endif

extern int ping;				 /* to ping or not to ping */
extern LONG packets_sent;			 /* # all packets sent to *

						  * 
						  * * server */
extern LONG packets_received;			 /* # all packets received */
extern W_Window pStats;

extern char deathmessage[80];
extern char outmessage[];
extern char *xdisplay_name;

extern int UseNewDistress;
extern int UseNewMacro;
extern int UseSmartMacro;
extern int rejectMacro;
extern int maskrecip;
extern unsigned char singleMacro[MAX_MACRO];


extern int enemyPhasers;

extern char cloakChars[3];

extern int showIND;
extern int newPlist;


extern struct dmacro_list *distmacro;
extern struct dmacro_list dist_defaults[];
extern struct dmacro_list dist_prefered[];
extern int sizedist;

#ifdef BEEPLITE
extern char *distlite[];
extern int UseLite;
extern int DefLite;
extern int emph_planet_seq_n[];
extern int emph_player_seq_n[];
extern W_Icon emph_planet_seq[];
extern W_Icon emph_player_seq[];
extern W_Icon emph_player_seql[];

#define emph_planet_seq_frames 5
#define emph_planet_seq_width 24
#define emph_planet_seq_height 24
#define emph_player_seq_frames 3
#define emph_player_seq_width 24
#define emph_player_seq_height 24
#define emph_player_seql_frames 3
#define emph_player_seql_width 30
#define emph_player_seql_height 30
extern int beep_lite_cycle_time_player;
extern int beep_lite_cycle_time_planet;
extern int liteflag;
extern char F_beeplite_flags;

extern int tts_len;
extern int tts_max_len;
extern int tts_width;
extern int tts_timer;
extern int tts_time;
extern int tts_pos;
extern char lastIn[100];

#endif /* BEEPLITE */

#ifdef RCM
extern struct dmacro_list rcm_msg[];

#endif

extern int highlightFriendlyPhasers;

#ifdef IGNORE_SIGNALS_SEGV_BUS
/* Handle (hopefully) non-fatal segmentation and bus faults. */
extern int ignore_signals;

#endif

#ifdef MOTION_MOUSE
extern int motion_mouse;
extern int user_motion_thresh;
extern int motion_mouse_enablable;
extern int motion_mouse_steering;

#endif

#ifdef SHIFTED_MOUSE
extern int extended_mouse;

#endif

extern int ignoreCaps;

#ifdef MOUSE_AS_SHIFT
extern int mouse_as_shift;
extern int b1_as_shift;
extern int b2_as_shift;
extern int b3_as_shift;

#endif

#ifdef TNG_FED_BITMAPS
extern int use_tng_fed_bitmaps;
extern W_Icon tng_fed_bitmaps[NUM_TYPES][VIEWS];

#endif

#ifdef VARY_HULL
extern W_Icon hull[];
extern int vary_hull;

#endif

#ifdef XTREKRC_HELP
extern W_Window defWin;

#endif

#ifdef CONTROL_KEY
extern int use_control_key;

#endif

#ifdef DOC_WIN
extern W_Window docwin, xtrekrcwin;
extern int maxdoclines, maxxtrekrclines;

#endif

extern W_Icon bplanets4[8];
extern W_Icon mbplanets4[8];

#ifdef REFRESH_INTERVAL
extern int refresh_interval;

#endif

extern int max_fd;

#ifdef TOOLS
extern W_Window toolsWin;
extern int shelltools;
extern struct key_list macroKeys[MAX_KEY];
extern unsigned char keys[MAX_KEY];
extern char *wwwlink;
extern char *upgradeURL;
extern char *releaseURL;
extern char *bugURL;

#endif

#ifdef SOUND
#include "sound.h"
extern int sound_init;
extern int sound_toggle;
extern char *sounddir;
extern W_Window soundWin;

#endif

#ifdef HOCKEY_LINES
extern struct s_line s_lines[NUM_HOCKEY_LINES + 1];
extern int normal_s_lines;
extern int hockey_s_lines;

#endif

#ifdef MULTILINE_MACROS
extern int multiline_enabled;

#endif

#ifdef FEATURE_PACKETS
extern int F_feature_packets;

#endif

extern int F_cloak_maxwarp;
extern int F_self_8flags;
extern int F_self_8flags2;
extern int F_ship_cap;

#ifdef HAVE_XPM
extern int F_agri_pix;

#endif

#ifdef WARP_DEAD
extern int F_dead_warp;

#endif

extern int F_many_self;

extern int F_many_self;

#ifdef UDP_PORTSWAP
extern int portSwap;
#endif

#endif /* _h_data */
