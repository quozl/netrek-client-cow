/*
 * data.c
 */
#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"

struct player *players;
struct player *me = NULL;
struct torp *torps;
struct plasmatorp *plasmatorps;
struct status *status;
struct ship *myship;
struct stats *mystats;
struct planet *planets;
struct phaser *phasers;
struct message *messages;
struct mctl *mctl;
struct context *context;
struct memory universe;

int     ghoststart = 0;				 /* is this a ghostbust *

						  * 
						  * * restart? */
int     ghost_pno = 0;				 /* is this a ghostbust *

						  * 
						  * * restart? */
int     keepInfo = 15;				 /* how many updates to keep

						  * 
						  * * * infowins 6/1/93 LAB */
int     showPlanetOwner = 0;

int     phaserShrink = 0;
int     theirPhaserShrink = 0;
int     shrinkPhaserOnMiss = 0;

int     newDashboard = 3;			 /* use new graphic *

						  * 
						  * * dashboard, 6/2/93 LAB */
int     old_db = 3;				 /* should be same as *

						  * 
						  * * newDashboard */
int     detCircle = 0;		/* Show det circle on tactical */
int     fastQuit = 0;
int     gen_distress = 0;			 /* generic distress/macro *

						  * 
						  * * system support */
int     niftyNewMessages = 1;
int     oldalert = PFGREEN;
int     remap[16] =
{0, 1, 2, 0, 3, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0};
int     messpend = 0;

#ifdef XTRA_MESSAGE_UI
int     messageHUD = 0;				 /* Show message being typed

						  * 
						  * * on the local display *
						  *     */
int     messHoldThresh = 0;			 /* Threshold value for

						  * 
						  * * putting a message on
						  * hold  * (squared) */
int     messMouseDelta = 0;			 /* To keep track of mouse

						  * 
						  * * movement delta *
						  * */
#endif
int     lastcount = 0;
int     mdisplayed = 0;
int     udcounter = 0;
int     showTractorPressor = 1;
int     showLock = 3;
int     showPhaser = 2;
int     autoKey = 0;
int     extraBorder = 1;

/* udp options */
int     tryUdp = 1;
int     tryUdp1 = 1;
struct plupdate pl_update[MAXPLANETS];
char    buttonmap[W_BUTTON_RANGE] =
{'\0', '\0', '\0', '\0', '\0', '\0'};
int     lastm = 0;
int     delay = 0;				 /* delay for decaring war */
int     rdelay = 0;				 /* delay for refitting */
int     namemode = 1;
int     quittime = 60;
int     showStats = 0;
int     warnShields = 0;
int     ROMVLVS = 0;
int     warncount = 0;
int     warntimer = -1;
int     infomapped = 0;
int     mustexit = 0;
int     messtime = 5;
int     keeppeace = 0;

#ifdef GATEWAY
unsigned LONG netaddr = 0;			 /* for blessing */

#endif

int     msgBeep = 1;				 /* ATM - msg beep */

int     logmess = 0;
int     continuetractor = 1;
int     tcounter = 2;
int     showlocal = 2;
int     showgalactic = 2;
int     pixMissing = 0;
int     pixFlags = 0;
char   *title = NULL;
char   *shipnos = "0123456789abcdefghijklmnopqrstuvwxyz";

#ifndef RSA
/* Um, not that I could find... SAC 1 Aug 1996 */
/* Search for it in the rsa_box*.c files ... 007 15 Aug 1996 */
int     sock = -1;				 /* randomized into RSA code

						  * 
						  * *  08/24/95 [007] */
#endif
int     xtrekPort = -1;
int     queuePos = -1;
int     pickOk = -1;
int     lastRank = -1;
int     promoted = 0;

#ifdef ROTATERACE
int     rotate = 0;
int     rotate_deg = 0;

#endif

int     loginAccept = -1;
unsigned localflags = 0;
int     tournMask = 15;
int     nextSocket = 0;				 /* socket to use when we get

						  * 
						  * * * ghostbusted... */
char   *serverName = NULL;

/* if there's an rc file, defaultsFile gets set by initDefaults; 
 * if there isn't one, we want to know */
char    defaultsFile[80] = "";

char   *myname = NULL;
int     loggedIn = 0;
int     reinitPlanets = 0;
int     timerDelay = 200000;			 /* micro secs between *

						  * 
						  * * updates */
int     redrawDelay = 0;			 /* 1/10 secs beetween *

						  * 
						  * * redraws */
int     reportKills = 1;			 /* report kill messages (in

						  * 
						  * * * review window)? */
int     phaserWindow = 0;			 /* What window to show

						  * 
						  * * phaser msgs in */

#ifdef PHASER_STATS
int     phaserShowStats = 0;			 /* How to show phaser stats */
int     phaserStatTry = 0;			 /* Try/attemps to phaser */
int     phaserStatHit = 0;			 /* Number of hits */

#endif
int     censorMessages = 0;

#ifdef RECORDGAME
FILE   *recordFile = NULL;			 /* recorder: slurp packets * 

						  * 
						  * 
						  * * here */
FILE   *recordIndexFile = NULL;			  
FILE   *recordContextFile = NULL;			 

int     playback = 0;

#endif

FILE   *logFile = NULL;				 /* log messages to this file 

						  * 
						  * 
						  */
int     scanplayer = 0;				 /* who to scan */
int     showTractor = 1;			 /* show visible tractor *

						  * 
						  * * beams */
int     commMode = 0;				 /* UDP: 0=TCP only, 1=UDP *

						  * 
						  * * updates */
int     commModeReq = 0;			 /* UDP: req for comm *

						  * 
						  * * protocol change */
int     commStatus = 0;				 /* UDP: used when switching

						  * 
						  * * * protocols */
int     commSwitchTimeout = 0;			 /* UDP: don't wait forever */
int     udpTotal = 1;				 /* UDP: total #of packets *

						  * 
						  * * received */
int     udpDropped = 0;				 /* UDP: count of packets *

						  * 
						  * * dropped */
int     udpRecentDropped = 0;			 /* UDP: #of packets dropped

						  * 
						  * * * recently */
int     udpSock = -1;				 /* UDP: the socket */
int     udpDebug = 0;				 /* UDP: debugging info *

						  * 
						  * * on/off */
int     udpClientSend = 1;			 /* UDP: send our packets *

						  * 
						  * * using UDP? */
int     udpClientRecv = 1;			 /* UDP: receive with simple

						  * 
						  * * * UDP */
int     udpSequenceChk = 1;			 /* UDP: check sequence *

						  * 
						  * * numbers */
int     weaponUpdate = 0;			 /* Have any weapon packets * 

						  * 
						  * 
						  * * been received recently */

#ifdef GATEWAY
int     gw_serv_port, gw_port, gw_local_port;	 /* UDP */
char   *gw_mach = NULL;				 /* UDP */

#endif

/* for router-based firewalls, we need to tunnel through at a specific port */
int     baseLocalPort = 0;			 /* UDP */

int     debug = 0;

int     messageon = 0;

#ifdef RSA
char    testdata[16];
int     RSA_Client = 1;

#endif

int     SBhours = 0;

#ifdef SHORT_PACKETS
int     why_dead = 0;
int     tryShort = 1;				 /* for .xtrekrc option */
int     tryShort1 = 1;
int     recv_short = 0;
int     recv_mesg = 1;
int     recv_kmesg = 1;
int     recv_threshold = 0;
char    recv_threshold_s[8] =
{'0', '\0'};
int     recv_warn = 1;

#endif


int     netstat = 0;
int     netstatfreq = 5;
W_Window netstatWin, lMeter;
int     client_ups = 50;	/* client requested updates per second */
int     server_ups = 50;	/* server responded updates per second */
int     server_fps = 50;	/* server configured frames per second */

#ifdef META
char   *metaserver = "metaserver.netrek.org";
int     metaport = 3521;
#endif


#ifdef NBT
struct macro_list macro[MAX_MACRO];		 /* NBT 2/26/93 */
int     MacroMode = 0;
int     macrocnt = 0;

#endif

extern double Sin[], Cos[];

W_Icon  stipple, clockpic, icon;

W_Color borderColor, backColor, textColor, myColor, warningColor, shipCol[5],
        rColor, yColor, gColor, unColor, foreColor;

W_Icon  expview[EX_FRAMES];
W_Icon  sbexpview[SBEXPVIEWS];
W_Icon  cloud[NUMDETFRAMES];
W_Icon  plasmacloud[NUMDETFRAMES];
W_Icon  etorp, mtorp;
W_Icon  eplasmatorp, mplasmatorp;

#ifdef VSHIELD_BITMAPS
W_Icon  shield[SHIELD_FRAMES], cloakicon;
int     VShieldBitmaps = 1;

#else
W_Icon  shield, cloakicon;

#endif

W_Icon  tractbits, pressbits;			 /* ATM - visible tractor */
W_Icon  fed_bitmaps[NUM_TYPES][VIEWS], kli_bitmaps[NUM_TYPES][VIEWS], rom_bitmaps[NUM_TYPES][VIEWS],
        ori_bitmaps[NUM_TYPES][VIEWS], ind_bitmaps[NUM_TYPES][VIEWS], ROMVLVS_bitmap[VIEWS];
W_Icon  bplanets[7];
W_Icon  mbplanets[7];
W_Icon  bplanets2[8];
W_Icon  mbplanets2[8];
W_Icon  bplanets3[NUM_PLANET_BITMAPS2];		 /* isae:  Added this */
W_Icon  mbplanets3[NUM_PLANET_BITMAPS2];	 /* isae: Added this */
W_Icon  noinfoplanet;

/* jn - SMARTMACRO */

#ifdef NEWMACRO
int     MacroNum = 0;

#endif /* NEWMACRO */
char    lastMessage[80];
char   *classes[] =
{"SC", "DD", "CA", "BB", "AS", "SB", "GA", "AT"};
char    teamlet[] =
{'I', 'F', 'R', 'X', 'K', 'X', 'X', 'X', 'O', 'X', 'X', 'X',
 'X', 'X', 'X', 'A'};
char   *teamshort[16] =
{"IND", "FED", "ROM", "X", "KLI", "X", "X", "X", "ORI",
 "X", "X", "X", "X", "X", "X", "ALL"};
char    pseudo[PSEUDOSIZE];
char    defpasswd[PSEUDOSIZE];
char    login[PSEUDOSIZE];

struct ship shipvals[NUM_TYPES];

/* 10 Aug 96 - Added curt (short) names -SAC */
struct rank default_ranks[DEFAULT_NUMRANKS] =
{
  {0.0, 0.0, 0.0, "Ensign", "Esgn"},
  {2.0, 1.0, 0.0, "Lieutenant", "Lt "},
  {4.0, 2.0, 0.0, "Lt. Cmdr.", "LtCm"},
  {8.0, 3.0, 0.0, "Commander", "Cder",},
  {15.0, 4.0, 0.0, "Captain", "Capt"},
  {20.0, 5.0, 0.0, "Flt. Capt.", "FltC"},
  {25.0, 6.0, 0.0, "Commodore", "Cdor"},
  {30.0, 7.0, 0.0, "Rear Adm.", "RAdm"},
  {40.0, 8.0, 0.0, "Admiral", "Admr"}};
struct rank *ranks = NULL;
int nranks = 0;

W_Window messagew, w, mapw, statwin, baseWin = 0, infow, iconWin, tstatw,
        war, warnw, helpWin, teamWin[4], qwin, messwa, messwt, messwi,
        messwk, playerw, planetw, rankw, optionWin = 0, reviewWin;
W_Window scanw, udpWin, phaserwin;

#ifdef SHORT_PACKETS
W_Window spWin = NULL;

#endif

#ifdef NBT
W_Window macroWin = NULL;

#endif

int     ping = 0;				 /* to ping or not to ping */
LONG    packets_sent = 0;			 /* # all packets sent to *

						  * 
						  * * server */
LONG    packets_received = 0;			 /* # all packets received */
W_Window pStats = NULL;

char    deathmessage[80];
char    outmessage[85];				 /* 80 chars made sun4's core

						  * 
						  * * * dump and I'm too lazy 
						  * to * * calculate the
						  * exact * * number required 
						  * here */
char   *xdisplay_name = NULL;

int     UseNewDistress = 0;
int     UseNewMacro = 1;
int     UseSmartMacro = 1;
int     rejectMacro = 0;
int     maskrecip = 0;

unsigned char singleMacro[MAX_MACRO] = "";

int     enemyPhasers = 1;

char    cloakChars[3] = "??";

int     showIND = 0;
int     newPlist = 0;

/* tried to automate this as much as possible... the entries are * the
 * character, string identifier, and the default macro for * each distress
 * type. */

/* the index into distmacro array should correspond with the correct
 * dist_type */

#define NUM_DIST 27


struct dmacro_list dist_prefered[NUM_DIST];

/* the index into distmacro array should correspond with the correct
 * dist_type */
/* the character specification is ignored now, kept here anyway for reference */
struct dmacro_list dist_defaults[] =
{
  {'X', "no zero", "this should never get looked at"},
/* ^t */
  {'\xd4', "taking", " %T%c->%O (%S) Carrying %a to %l%?%n>-1%{ @ %n%}\0"},
/* ^o */
  {'\xcf', "ogg", " %T%c->%O Help Ogg %p at %l\0"},
/* ^b */
  {'\xc2', "bomb", " %T%c->%O %?%n>4%{bomb %l @ %n%!bomb %l%}\0"},
/* ^c */
  {'\xc3', "space_control", " %T%c->%O Help Control at %L\0"},
/* ^1 */
  {'\x91', "save_planet", " %T%c->%O Emergency at %L!!!!\0"},
/* ^2 */
  {'\x92', "base_ogg", " %T%c->%O Sync with --]> %g <[-- OGG ogg OGG base!!\0"},
/* ^3 */
  {'\x93', "help1", " %T%c->%O Help me! %d%% dam, %s%% shd, %f%% fuel %a armies.\0"},
/* ^4 */
  {'\x94', "help2", " %T%c->%O Help me! %d%% dam, %s%% shd, %f%% fuel %a armies.\0"},
/* ^e */
  {'\xc5', "escorting", " %T%c->%O ESCORTING %g (%d%%D %s%%S %f%%F)\0"},
/* ^p */
  {'\xd0', "ogging", " %T%c->%O Ogging %h\0"},
/* ^m */
  {'\xcd', "bombing", " %T%c->%O Bombing %l @ %n\0"},
/* ^l */
  {'\xcc', "controlling", " %T%c->%O Controlling at %l\0"},
/* ^5 */
  {'\x95', "asw", " %T%c->%O Anti-bombing %p near %b.\0"},
/* ^6 */
  {'\x96', "asbomb", " %T%c->%O DON'T BOMB %l. Let me bomb it (%S)\0"},
/* ^7 */
  {'\x97', "doing1", " %T%c->%O (%i)%?%a>0%{ has %a arm%?%a=1%{y%!ies%}%} at %l.  (%d%% dam, %s%% shd, %f%% fuel)\0"},
/* ^8 */
  {'\x98', "doing2", " %T%c->%O (%i)%?%a>0%{ has %a arm%?%a=1%{y%!ies%}%} at %l.  (%d%% dam, %s%% shd, %f%% fuel)\0"},
/* ^f */
  {'\xc6', "free_beer", " %T%c->%O %p is free beer\0"},
/* ^n */
  {'\xce', "no_gas", " %T%c->%O %p @ %l has no gas\0"},
/* ^h */
  {'\xc8', "crippled", " %T%c->%O %p @ %l crippled\0"},
/* ^9 */
  {'\x99', "pickup", " %T%c->%O %p++ @ %l\0"},
/* ^0 */
  {'\x90', "pop", " %T%c->%O %l%?%n>-1%{ @ %n%}!\0"},
/* F */
  {'F', "carrying", " %T%c->%O %?%S=SB%{Your Starbase is c%!C%}arrying %?%a>0%{%a%!NO%} arm%?%a=1%{y%!ies%}.\0"},
/* ^@ */
  {'\xa0', "other1", " %T%c->%O (%i)%?%a>0%{ has %a arm%?%a=1%{y%!ies%}%} at %l. (%d%%D, %s%%S, %f%%F)\0"},
/* ^# */
  {'\x83', "other2", " %T%c->%O (%i)%?%a>0%{ has %a arm%?%a=1%{y%!ies%}%} at %l. (%d%%D, %s%%S, %f%%F)\0"},
/* E */
  {'E', "help", " %T%c->%O Help(%S)! %s%% shd, %d%% dmg, %f%% fuel,%?%S=SB%{ %w%% wtmp,%!%}%E%{ ETEMP!%}%W%{ WTEMP!%} %a armies!\0"},
  {'\0', '\0', '\0'},
};

struct dmacro_list *distmacro = dist_defaults;

int     sizedist = sizeof(dist_defaults);

#ifdef BEEPLITE
char   *distlite[NUM_DIST] =
{
  NULL, 
  "/c/l/0/|%T%c Taking %L|",
  "/|Ogg %p|",
  "/|Bomb %L|",
  "/|Control %L|",
  "/|Save %L|",
  NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

int     DefLite = 1;
int     UseLite = 1;

int     emph_planet_seq_n[MAXPLANETS] =
{0,};
int     emph_player_seq_n[MAXPLAYER] =
{0,};
W_Icon  emph_planet_seq[10];
W_Icon  emph_player_seq[10];
W_Icon  emph_player_seql[10];
int     beep_lite_cycle_time_player = 10;
int     beep_lite_cycle_time_planet = 10;
int     liteflag = 0;
char    F_beeplite_flags = LITE_PLAYERS_MAP |
LITE_PLAYERS_LOCAL |
LITE_SELF |
LITE_PLANETS |
LITE_SOUNDS |
LITE_TTS;

int     tts_len = 0;
int     tts_max_len = 40;
int     tts_width = 0;
int     tts_timer = 0;
int     tts_time = 25;
int     tts_pos = TWINSIDE / 2 - 16;		 /* as found in redraw.c *

						  * 
						  * * originally */
char    lastIn[100];

#endif /* BEEPLITE */

#ifdef RCM					 /* Receiver configurable * * 
						  * Server messages */
struct dmacro_list rcm_msg[] =
{
  {'0', "none", "Unknown RCM message"},
  {'1', "kill", "GOD->ALL %i (%S) (%T%c%?%a>0%{+%a armies%!%}) was kill %?%d>0%{%k%!NO CREDIT)%} for %u (%r%p) %?%w>0%{%W%!%}"},
  {'2', "planet", "GOD->ALL %i (%S) (%T%c%?%a>0%{+%a armies%!%} killed by %l (%z) %?%w>0%{%W%!%}"},
  {'3', "bomb", "%N->%Z We are being attacked by %i (%T%c) who is %d%% damaged."},
  {'4', "destroy", "%N->%Z %N destroyed by %i (%T%c)"},
  {'5', "take", "%N->%O %N taken by %i (%T%c)"},
  {'6', "ghostbust", "GOD->ALL %i (%S) (%T%c) was kill %k for the GhostBusters"},
  {'\0', '\0', '\0'},
};

#endif /* RCM */

int     highlightFriendlyPhasers = 0;

#ifdef IGNORE_SIGNALS_SEGV_BUS
/* KRP */
/* Handle (hopefully) non-fatal segmentation and bus faults. */
int     ignore_signals = 0;

#endif

#ifdef MOTION_MOUSE
/* KRP */
int     motion_mouse = 0;
int     user_motion_thresh = 16;
int     motion_mouse_enablable = 1;
int     motion_mouse_steering = 0;

#endif

#ifdef SHIFTED_MOUSE
/* KRP */
int     extended_mouse = 0;

#endif

int     ignoreCaps = 1;				 /* Default is to ignore the

						  * 
						  * * * Capslock key SRS */

#ifdef MOUSE_AS_SHIFT
/* KRP */
int     mouse_as_shift = 0;
int     b1_as_shift = 0;
int     b2_as_shift = 0;
int     b3_as_shift = 0;

#endif

#ifdef TNG_FED_BITMAPS
int     use_tng_fed_bitmaps = 0;
W_Icon  tng_fed_bitmaps[NUM_TYPES][VIEWS];

#endif

#ifdef VARY_HULL
W_Icon  hull[8];
int     vary_hull = 0;

#endif

#ifdef XTREKRC_HELP
W_Window defWin = NULL;

#endif

#ifdef CONTROL_KEY
int     use_control_key = 1;

#endif

#ifdef DOC_WIN
W_Window docwin = NULL, xtrekrcwin = NULL;
int     maxdoclines = 0, maxxtrekrclines = 0;

#endif

W_Icon  bplanets4[8];
W_Icon  mbplanets4[8];

#ifdef REFRESH_INTERVAL
int     refresh_interval = 0;

#endif

int     max_fd = 3;

#ifdef TOOLS
W_Window toolsWin = NULL;
int     shelltools = 1;
struct key_list macroKeys[MAX_KEY];
unsigned char keys[MAX_KEY] = "";
char   *wwwlink = "netscape -remote \"openURL(%s)\"";
char   *upgradeURL = "http://cow.netrek.org/current/index.html#%s";
char   *releaseURL = "http://cow.netrek.org/%s/README.html#pl%i";
char   *bugURL = "http://sourceforge.net/bugs/?group_id=968";
/*
char   *bugURL = "http://bugzilla.us.netrek.org/cow?version=%s&pl=%i&arch=%s";
*/
#endif

#ifdef SOUND
int    sound_init = 1;
int    sound_toggle = 0;
char   *sounddir = NULL;
W_Window soundWin = NULL;

#endif

#ifdef HOCKEY_LINES
int     hockey_s_lines = 0;
struct s_line s_lines[NUM_HOCKEY_LINES + 1];
int     normal_s_lines = 0;

#endif

#ifdef MULTILINE_MACROS
int     multiline_enabled = 0;

#endif

#ifdef FEATURE_PACKETS
int     F_feature_packets = 0;

#endif

int     F_cloak_maxwarp = 0;
int     F_self_8flags = 1;
int     F_self_8flags2 = 0;
int     F_ship_cap = 0;
int     F_sp_generic_32 = 1;
char    A_sp_generic_32 = 0;
int     F_agri_pix = 1;

#ifdef WARP_DEAD
int     F_dead_warp = 0;

#endif

int F_many_self = 0;

#ifdef UDP_PORTSWAP
int     portSwap = 1;
#endif

int F_show_all_tractors = 1;
int fastGuest = 0;
int identityBlind = 0;
int versionHide = 0;
int F_sp_rank = 0;
int F_sp_ltd = 0;
int F_tips = 1;
char *program = NULL;
