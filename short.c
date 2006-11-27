
/* This file implements all SHORT_PACKETS functions */
/* HW 19.07.93
 *
 * $Log: short.c,v $
 * Revision 1.5  2006/09/19 10:20:39  quozl
 * ut06 full screen, det circle, quit on motd, add icon, add desktop file
 *
 * Revision 1.4  2006/05/22 13:13:24  quozl
 * initialise packet buffers
 *
 * Revision 1.3  1999/06/11 16:14:17  siegl
 * cambot replay patches
 *
 * Revision 1.2  1999/03/25 20:56:26  siegl
 * CygWin32 autoconfig fixes
 *
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 * */
#include "config.h"

#ifdef SHORT_PACKETS
#include INC_MACHINE_ENDIAN
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include INC_SYS_SELECT
#include INC_NETINET_IN
#include INC_NETINET_TCP
#include <netdb.h>
#include <math.h>
#include <errno.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"
#include "playerlist.h"
#include "map.h"
#include "local.h"

/* from here on all SHORT_PACKETS */
#include "wtext.h"				 /* here are all * *
						  * warningdefines */

/* Here are all warnings that are send with SP_S_WARNING */
/* HW 93           */

/* DaemonMessages */
char   *daemon_texts[] =
{
/* Game_Paused() */
  "Game is paused.  CONTINUE to continue.",	 /* 0 */
  "Game is no-longer paused!",			 /* 1 */
  "Game is paused. Captains CONTINUE to continue.",	/* 2 */
  "Game will continue in 10 seconds",		 /* 3 */
/* send_about_to_start() */
  "Teams chosen.  Game will start in 1 minute.", /* 4 */
  "----------- Game will start in 1 minute -------------",	/* 5 */
};

/* VARITEXTE = warnings with 1 or more arguments argument */
char   *vari_texts[] =
{
/* redraw.c */
  "Engineering:  Energizing transporters in %d seconds",	/* 0 */
  "Stand By ... Self Destruct in %d seconds",	 /* 1 */
  "Helmsman:  Docking manuever completed Captain.  All moorings secured at port %d.",	/* 2 
											 * 
											 */
/* interface.c from INL server */
  "Not constructed yet. %d minutes required for completion",	/* 3 */

};



char   *w_texts[] =
{
/* socket.c             */
  "Tractor beams haven't been invented yet.",	 /* 0 */
  "Weapons's Officer:  Cannot tractor while cloaked, sir!",	/* 1 */
  "Weapon's Officer:  Vessel is out of range of our tractor beam.",	/* 2 */

/* handleRepressReq */
/****************       coup.c  ***********************/
/* coup()  */
  "You must have one kill to throw a coup",	 /* 3 */
  "You must orbit your home planet to throw a coup",	/* 4 */
  "You already own a planet!!!",		 /* 5 */
  "You must orbit your home planet to throw a coup",	/* 6 */
  "Too many armies on planet to throw a coup",	 /* 7 */
  "Planet not yet ready for a coup",		 /* 8 */
/* getentry.c              */
/* getentry()              */
  "I cannot allow that.  Pick another team",	 /* 9 */
  "Please confirm change of teams.  Select the new team again.",	/* 10 
									 * 
									 */
  "That is an illegal ship type.  Try again.",	 /* 11 */
  "That ship hasn't beed designed yet.  Try again.",	/* 12 */
  "Your new starbase is still under construction",	/* 13 */
  "Your team is not capable of defending such an expensive ship!",	/* 14 
									 * 
									 */
  "Your team's stuggling economy cannot support such an expenditure!",	/* 15 
									 * 
									 */
  "Your side already has a starbase!",		 /* 16 */
/* plasma.c        */
/* nplasmatorp(course, type)    */
  "Plasmas haven't been invented yet.",		 /* 17 */
"Weapon's Officer:  Captain, this ship is not armed with plasma torpedoes!",	/* 18 
										 * 
										 */
  "Plasma torpedo launch tube has exceeded the maximum safe temperature!",	/* 19 
										 * 
										 */
  "Our fire control system limits us to 1 live torpedo at a time captain!",	/* 20 
										 * 
										 */
  "Our fire control system limits us to 1 live torpedo at a time captain!",	/* 21 
										 * 
										 */
  "We don't have enough fuel to fire a plasma torpedo!",	/* 22 */
  "We cannot fire while our vessel is undergoing repairs.",	/* 23 */
  "We are unable to fire while in cloak, captain!",	/* 24 */
/********       torp.c  *********/
/* ntorp(course, type)     */
  "Torpedo launch tubes have exceeded maximum safe temperature!",	/* 25 
									 * 
									 */
  "Our computers limit us to having 8 live torpedos at a time captain!",	/* 26 
										 * 
										 */
  "We don't have enough fuel to fire photon torpedos!",		/* 27 */
  "We cannot fire while our vessel is in repair mode.",		/* 28 */
  "We are unable to fire while in cloak, captain!",	/* 29 */
  "We only have forward mounted cannons.",	 /* 30 */
/* phasers.c       */
/* phaser(course) */
  "Weapons Officer:  This ship is not armed with phasers, captain!",	/* 31 
									 * 
									 */
  "Phasers have not recharged",			 /* 32 */
  "Not enough fuel for phaser",			 /* 33 */
  "Can't fire while repairing",			 /* 34 */
  "Weapons overheated",				 /* 35 */
  "Cannot fire while cloaked",			 /* 36 */
  "Phaser missed!!!",				 /* 37 */
  "You destroyed the plasma torpedo!",		 /* 38 */
/* interface.c     */
/* bomb_planet()        */
  "Must be orbiting to bomb",			 /* 39 */
  "Can't bomb your own armies.  Have you been reading Catch-22 again?",		/* 40 
										 * 
										 */
  "Must declare war first (no Pearl Harbor syndrome allowed here).",	/* 41 
									 * 
									 */
  "Bomb out of T-mode?  Please verify your order to bomb.",	/* 42 */
  "Hoser!",					 /* 43 */
/* beam_up()       */
  "Must be orbiting or docked to beam up.",	 /* 44 */
  "Those aren't our men.",			 /* 45 */
  "Comm Officer: We're not authorized to beam foriegn troops on board!",	/* 46 
										 * 
										 */
/* beam_down() */
  "Must be orbiting or docked to beam down.",	 /* 47 */
  "Comm Officer: Starbase refuses permission to beam our troops over.",		/* 48 
										 * 
										 */
/* declare_war(mask)       */
  "Pausing ten seconds to re-program battle computers.",	/* 49 */
/* do_refit(type) */
  "You must orbit your HOME planet to apply for command reassignment!",		/* 50 
										 * 
										 */
  "You must orbit your home planet to apply for command reassignment!",		/* 51 
										 * 
										 */
  "Can only refit to starbase on your home planet.",	/* 52 */
  "You must dock YOUR starbase to apply for command reassignment!",	/* 53 
									 * 
									 */
  "Must orbit home planet or dock your starbase to apply for command reassignment!",	/* 54 
											 * 
											 */
  "Central Command refuses to accept a ship in this condition!",	/* 55 
									 * 
									 */
  "You must beam your armies down before moving to your new ship",	/* 56 
									 * 
									 */
  "That ship hasn't been designed yet.",	 /* 57 */
  "Your side already has a starbase!",		 /* 58 */
  "Your team is not capable of defending such an expensive ship",	/* 59 
									 * 
									 */
  "Your new starbase is still under construction",	/* 60 */
  "Your team's stuggling economy cannot support such an expenditure!",	/* 61 
									 * 
									 */
  "You are being transported to your new vessel .... ",		/* 62 */
/* redraw.c */
/* auto_features()  */
  "Engineering:  Energize. [ SFX: chimes ]",	 /* 63 */
  "Wait, you forgot your toothbrush!",		 /* 64 */
  "Nothing like turning in a used ship for a new one.",		/* 65 */
  "First officer:  Oh no, not you again... we're doomed!",	/* 66 */
  "First officer:  Uh, I'd better run diagnostics on the escape pods.",		/* 67 
										 * 
										 */
  "Shipyard controller:  This time, *please* be more careful, okay?",	/* 68 
									 * 
									 */
  "Weapons officer:  Not again!  This is absurd...",	/* 69 */
  "Weapons officer:  ... the whole ship's computer is down?",	/* 70 */
  "Weapons officer:  Just to twiddle a few bits of the ship's memory?",		/* 71 
										 * 
										 */
  "Weapons officer:  Bah! [ bangs fist on inoperative console ]",	/* 72 
									 * 
									 */
  "First Officer:  Easy, big guy... it's just one of those mysterious",		/* 73 
										 * 
										 */
  "First Officer:  laws of the universe, like 'tires on the ether'.",	/* 74 
									 * 
									 */
  "First Officer:  laws of the universe, like 'Klingon bitmaps are ugly'.",	/* 75 
										 * 
										 */
  "First Officer:  laws of the universe, like 'all admirals have scummed'.",	/* 76 
										 * 
										 */
  "First Officer:  laws of the universe, like 'Mucus Pig exists'.",	/* 77 
									 * 
									 */
  "First Officer:  laws of the universe, like 'guests advance 5x faster'.",	/* 78 
										 * 
										 */
/* orbit.c */
/* orbit() */
  "Helmsman: Captain, the maximum safe speed for docking or orbiting is warp 2!",	/* 79 
											 * 
											 */
  "Central Command regulations prohibits you from orbiting foreign planets",	/* 80 
										 * 
										 */
  "Helmsman:  Sensors read no valid targets in range to dock or orbit sir!",	/* 81 
										 * 
										 */
/* redraw.c */
  "No more room on board for armies",		 /* 82 */
  "You notice everyone on the bridge is staring at you.",	/* 83 */
/* startdaemon.c */
/* practice_robo() */
  "Can't send in practice robot with other players in the game.",	/* 84 
									 * 
									 */
/* socket.c */
/* doRead(asock) */
  "Self Destruct has been canceled",		 /* 85 */
/* handleMessageReq(packet) */
  "Be quiet",					 /* 86 */
  "You are censured.  Message was not sent.",	 /* 87 */
  "You are ignoring that player.  Message was not sent.",	/* 88 */
  "That player is censured.  Message was not sent.",	/* 89 */
/* handleQuitReq(packet) */
  "Self destruct initiated",			 /* 90 */
/* handleScan(packet) */
  "Scanners haven't been invented yet",		 /* 91 */
/* handleUdpReq(packet) */
  "WARNING: BROKEN mode is enabled",		 /* 92 */
  "Server can't do that UDP mode",		 /* 93 */
  "Server will send with TCP only",		 /* 94 */
  "Server will send with simple UDP",		 /* 95 */
  "Request for fat UDP DENIED (set to simple)",	 /* 96 */
  "Request for double UDP DENIED (set to simple)",	/* 97 */
/* forceUpdate() */
  "Update request DENIED (chill out!)",		 /* 98 */
/* INL redraw.c */
  "Player lock lost while player dead.",	 /* 99 */
  "Can only lock on own team.",			 /* 100 */
  "You can only warp to your own team's planets!",	/* 101 */
  "Planet lock lost on change of ownership.",	 /* 102 */
  " Weapons officer: Finally! systems are back online!",	/* 103 */

};

#define NUMWTEXTS (sizeof w_texts / sizeof w_texts[0])
#define NUMVARITEXTS ( sizeof vari_texts / sizeof   vari_texts[0])
#define NUMDAEMONTEXTS ( sizeof daemon_texts / sizeof daemon_texts[0])

extern void sendShortReq(char);
void    new_flags(unsigned int data, int which);

extern char numofbits[];			 /* How many 1 bits in a char 

						  * 
						  * 
						  */
extern int vtisize[];				 /* 4 byte Header + torpdata */
int     Plx[MAXPLAYER], Ply[MAXPLAYER], Pgx[MAXPLAYER], Pgy[MAXPLAYER];
unsigned char Pdir[MAXPLAYER];
int     my_x, my_y;				 /* for rotation we need to * 

						  * 
						  * 
						  * * keep track of our real
						  * * * coordinates */

/* SP_S_WARNING vari texte */
char   *s_texte[256];				 /* Better with a malloc *

						  * 
						  * * scheme */
char    no_memory[] =
{"Not enough memory for warning string!"};

/* For INL Server */
char   *shiptype[NUM_TYPES] =
{"SC", "DD", "CA", "BB", "AS", "SB", "??"};
int     spwinside = 500;			 /* WINSIDE from Server */

#define SPWINSIDE 500				 /* To make it safe */
LONG    spgwidth = GWIDTH;


sendThreshold(short unsigned int v)
{
  struct threshold_cpacket p;

  p.type = CP_S_THRS;
  p.thresh = v;
  sendServerPacket((struct player_spacket *) &p);
}

void    handleVTorp(unsigned char *sbuf)
{
  unsigned char *which, *data;
  unsigned char bitset;
  struct torp *thetorp;
  int     dx, dy;
  int     shiftvar;

  int     i;
  register int shift = 0;			 /* How many torps are *

						  * 
						  * * extracted (for shifting 
						  * )  */

  /* now we must find the data ... :-) */
  if (sbuf[0] == SP_S_8_TORP)
    {						 /* MAX packet */
      bitset = 0xff;
      which = &sbuf[1];
      data = &sbuf[2];
    }
  else
    {						 /* Normal Packet */
      bitset = sbuf[1];
      which = &sbuf[2];
      data = &sbuf[3];
    }

#ifdef CORRUPTED_PACKETS
  /* we probably should do something clever here - jmn */
#endif

  weaponUpdate = 1;
  thetorp = &torps[((unsigned char) *which * 8)];
  for (shift = 0, i = 0; i < 8;
       i++, thetorp++, bitset >>= 1)
    {
      thetorp->t_updateFuse = TORP_UPDATE_FUSE;

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

	  /* This is necessary because TFREE/TMOVE is now encoded in the * *
	   * bitset */
	  if (thetorp->t_status == TFREE)
	    {
	      thetorp->t_status = TMOVE;	 /* guess */
	      players[thetorp->t_owner].p_ntorp++;
	    }
	  else if (thetorp->t_owner == me->p_no
		   && thetorp->t_status == TEXPLODE)
	    {
	      thetorp->t_status = TMOVE;	 /* guess */
	    }

	  /* Check if torp is visible */
	  if (dx > SPWINSIDE || dy > SPWINSIDE)
	    {
	      thetorp->t_x = -100000;		 /* Not visible */
	      thetorp->t_y = -100000;
	    }
	  else
	    {					 /* visible */
	      /* thetorp->t_x = me->p_x + ((dx - SPWINSIDE / 2) * SCALE); * * 
	       * thetorp->t_y = me->p_y + ((dy - SPWINSIDE / 2) * SCALE); */
	      thetorp->t_x = my_x + ((dx - SPWINSIDE / 2) * SCALE);
	      thetorp->t_y = my_y + ((dy - SPWINSIDE / 2) * SCALE);

#ifdef ROTATERACE
	      if (rotate)
		{
		  rotate_coord(&thetorp->t_x, &thetorp->t_y, rotate_deg,
			       GWIDTH / 2, GWIDTH / 2);
		}
#endif
	    }
	}					 /* if */
      else
	{					 /* We got a TFREE */
	  if (thetorp->t_status && thetorp->t_status != TEXPLODE)
	    {
	      players[thetorp->t_owner].p_ntorp--;
	      thetorp->t_status = TFREE;	 /* That's no guess */
	    }
	}
    }						 /* for */
}

void    handleSelfShort(struct youshort_spacket *packet)
{
  struct player* pl;

  pl = &players[packet->pnum];

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
  pl->p_flags = ntohl(packet->flags);
  pl->p_whydead = packet->whydead;
  pl->p_whodead = packet->whodead;
}

void    handleSelfShip(struct youss_spacket *packet)
{
  struct player* pl;

  if (F_many_self)
    {
      pl = &players[packet->pad1];
    }
  else
    {
      if (!me)
	return;                                      /* wait.. */
      pl = me;

      if (F_self_8flags)
	me->p_flags = (me->p_flags & 0xffffff00) | (unsigned char) packet->pad1;
      else if (F_self_8flags2)
	{
	  unsigned int new_flags = me->p_flags & ~(PFSHIELD | PFREPAIR | PFCLOAK |
						   PFGREEN | PFYELLOW | PFRED |
						   PFTRACT | PFPRESS);

	  new_flags |= ((packet->pad1 & PFSHIELD) |
			(packet->pad1 & PFREPAIR) |
			((packet->pad1 & (PFCLOAK << 2)) >> 2) |
			((packet->pad1 & (PFGREEN << 7)) >> 7) |
			((packet->pad1 & (PFYELLOW << 7)) >> 7) |
			((packet->pad1 & (PFRED << 7)) >> 7) |
			((packet->pad1 & (PFTRACT << 15)) >> 15) |
			((packet->pad1 & (PFPRESS << 15)) >> 15));

	  me->p_flags = new_flags;
	}
    }
  pl->p_damage = ntohs(packet->damage);
  pl->p_shield = ntohs(packet->shield);
  pl->p_fuel = ntohs(packet->fuel);
  pl->p_etemp = ntohs(packet->etemp);
  pl->p_wtemp = ntohs(packet->wtemp);
}

void    handleVPlayer(unsigned char *sbuf)
{
  register int speed, x, y, i, numofplayers, pl_no, save, galactic;
  unsigned char *savebuf = sbuf;
  register struct player *pl;

  numofplayers = (unsigned char) sbuf[1] & 0x3f;

#ifdef CORRUPTED_PACKETS
  /* should do something clever here - jmn if(pl_no < 0 || pl_no >= * *
   * MAXPLAYER){ fprintf(stderr, "handleVPlayer: bad index %d\n", pl_no); * * 
   * return; } */
#endif

  if (sbuf[1] & (unsigned char) 128)
    {						 /* Short Header + Extended */
      sbuf += 4;
      for (i = 0; i < numofplayers; i++)
	{
	  pl_no = ((unsigned char) *sbuf & 0x1f) + 32;
	  if (pl_no >= MAXPLAYER)
	    continue;				 /* a little error check */
	  save = (unsigned char) *sbuf;
	  sbuf++;
	  pl = &players[pl_no];

	  pl->p_speed = (unsigned char) *sbuf & 15;	/* SPEED */
	  PlistNoteSpeed(pl_no);

	  if (F_cloak_maxwarp && pl != me)
	    {
	      if (pl->p_speed == 0xf)
		pl->p_flags |= PFCLOAK;
	      else if (pl->p_flags & PFCLOAK)
		pl->p_flags &= ~PFCLOAK;
	    }

	  Pdir[pl_no] = (unsigned char) *sbuf >> 4;	/* DIR */
	  pl->p_dir = (unsigned char) Pdir[pl_no] * 16;		/* real DIR */
	  sbuf++;
	  x = (unsigned char) *sbuf++;
	  y = (unsigned char) *sbuf++;		 /* The lower 8 Bits are * *
						  * saved */
	  /* Now we must preprocess the coordinates */
	  if ((unsigned char) save & 64)
	    x |= 256;
	  if ((unsigned char) save & 128)
	    y |= 256;

#ifdef WARP_DEAD
	  /* -10000 doesn't work for SP 501 is invisible needs server change
	   * * * [007] */
	  if (F_dead_warp && pl->p_speed == 14 && x == 501 && y == 501 && (pl->p_status != PEXPLODE))
	    {
	      pl->p_status = PEXPLODE;
	      x = pl->p_x;
	      y = pl->p_y;
	      if (pl->p_dir > DEADPACKETS)
		pl->p_explode = EX_FRAMES;
	      else
		pl->p_explode = 0;

	      redrawPlayer[pl_no] = 1;
	      PlistNoteUpdate(pl_no);
	    }
#endif

	  /* Now test if it's galactic or local coord */
	  if (save & 32)
	    {					 /* It's galactic */
	      if (x == 501 || y == 501)
		{
		  Pgx[pl_no] = -500;
		  Pgy[pl_no] = -500;
		}
	      else
		{
		  Pgx[pl_no] = x;
		  Pgy[pl_no] = y;
		}
	      Plx[pl_no] = -1;			 /* Not visible */
	      Ply[pl_no] = -1;
	      redrawPlayer[pl->p_no] = 1;
	      pl->p_x = Pgx[pl_no] * GWIDTH / SPWINSIDE;
	      pl->p_y = Pgy[pl_no] * GWIDTH / SPWINSIDE;

#ifdef ROTATERACE
	      if (rotate)
		{
		  rotate_coord(&pl->p_x, &pl->p_y,
			       rotate_deg, GWIDTH / 2, GWIDTH / 2);
		  rotate_dir(&pl->p_dir, rotate_deg);
		}
#endif
	    }
	  else
	    {					 /* Local */
	      Plx[pl_no] = x;
	      Ply[pl_no] = y;
	      redrawPlayer[pl->p_no] = 1;
	      pl->p_x = me->p_x + ((Plx[pl_no] - SPWINSIDE / 2) * SCALE);
	      pl->p_y = me->p_y + ((Ply[pl_no] - SPWINSIDE / 2) * SCALE);
	      Pgx[pl_no] = pl->p_x * SPWINSIDE / GWIDTH;
	      Pgy[pl_no] = pl->p_y * SPWINSIDE / GWIDTH;
	      pl->p_x = my_x + ((x - SPWINSIDE / 2) * SCALE);
	      pl->p_y = my_y + ((y - SPWINSIDE / 2) * SCALE);

#ifdef ROTATERACE
	      if (rotate)
		{
		  rotate_coord(&pl->p_x, &pl->p_y,
			       rotate_deg, GWIDTH / 2, GWIDTH / 2);
		  rotate_dir(&pl->p_dir, rotate_deg);
		}
#endif
	    }
	}					 /* for */
    }						 /* if */
  else if (sbuf[1] & 64)
    {						 /* Short Header  */
      if (shortversion == SHORTVERSION)
	{					 /* flags S_P2 */
	  if (sbuf[2] == 2)
	    {
	      int    *tmp = (int *) &sbuf[4];

	      new_flags(ntohl(*tmp), sbuf[3]);
	      tmp++;
	      new_flags(ntohl(*tmp), 0);
	      sbuf += 8;
	    }
	  else if (sbuf[2] == 1)
	    {
	      int    *tmp = (int *) &sbuf[4];

	      new_flags(ntohl(*tmp), sbuf[3]);
	      sbuf += 4;
	    }
	}
      sbuf += 4;
      for (i = 0; i < numofplayers; i++)
	{
	  pl_no = ((unsigned char) *sbuf & 0x1f);
	  if (pl_no >= MAXPLAYER)
	    continue;
	  save = (unsigned char) *sbuf;
	  sbuf++;
	  pl = &players[pl_no];

	  pl->p_speed = (unsigned char) *sbuf & 15;	/* SPEED */
	  PlistNoteSpeed(pl_no);

	  if (F_cloak_maxwarp && pl != me)
	    {
	      if (pl->p_speed == 0xf)
		pl->p_flags |= PFCLOAK;
	      else if (pl->p_flags & PFCLOAK)
		pl->p_flags &= ~PFCLOAK;
	    }

	  Pdir[pl_no] = (unsigned char) *sbuf >> 4;	/* DIR */
	  pl->p_dir = (unsigned char) Pdir[pl_no] * 16;		/* real DIR */
	  sbuf++;
	  x = (unsigned char) *sbuf++;
	  y = (unsigned char) *sbuf++;		 /* The lower 8 Bits are * *
						  * saved */
	  /* Now we must preprocess the coordinates */
	  if ((unsigned char) save & 64)
	    x |= 256;
	  if ((unsigned char) save & 128)
	    y |= 256;

#ifdef WARP_DEAD
	  /* waiting for server change [007] */
	  if (F_dead_warp && pl->p_speed == 14 && x == 501 && y == 501 && (pl->p_status != PEXPLODE))
	    {
	      pl->p_status = PEXPLODE;
	      x = pl->p_x;
	      y = pl->p_y;
	      if (pl->p_dir > DEADPACKETS)
		pl->p_explode = EX_FRAMES;
	      else
		pl->p_explode = 0;
	      redrawPlayer[pl_no] = 1;
	      PlistNoteUpdate(pl_no);
	    }
#endif

	  /* Now test if it's galactic or local coord */
	  if (save & 32)
	    {					 /* It's galactic */
	      if (x == 501 || y == 501)
		{
		  Pgx[pl_no] = -500;
		  Pgy[pl_no] = -500;
		}
	      else
		{
		  Pgx[pl_no] = x;
		  Pgy[pl_no] = y;
		}
	      Plx[pl_no] = -1;			 /* Not visible */
	      Ply[pl_no] = -1;
	      redrawPlayer[pl->p_no] = 1;
	      pl->p_x = Pgx[pl_no] * GWIDTH / SPWINSIDE;
	      pl->p_y = Pgy[pl_no] * GWIDTH / SPWINSIDE;

#ifdef ROTATERACE
	      if (rotate)
		{
		  rotate_coord(&pl->p_x, &pl->p_y,
			       rotate_deg, GWIDTH / 2, GWIDTH / 2);
		  rotate_dir(&pl->p_dir, rotate_deg);
		}
#endif
	    }
	  else
	    {					 /* Local */
	      Plx[pl_no] = x;
	      Ply[pl_no] = y;
	      redrawPlayer[pl->p_no] = 1;
	      pl->p_x = me->p_x + ((Plx[pl_no] - SPWINSIDE / 2) * SCALE);
	      pl->p_y = me->p_y + ((Ply[pl_no] - SPWINSIDE / 2) * SCALE);
	      Pgx[pl_no] = pl->p_x * SPWINSIDE / GWIDTH;
	      Pgy[pl_no] = pl->p_y * SPWINSIDE / GWIDTH;
	      pl->p_x = my_x + ((x - SPWINSIDE / 2) * SCALE);
	      pl->p_y = my_y + ((y - SPWINSIDE / 2) * SCALE);

#ifdef ROTATERACE
	      if (rotate)
		{
		  rotate_coord(&pl->p_x, &pl->p_y,
			       rotate_deg, GWIDTH / 2, GWIDTH / 2);
		  rotate_dir(&pl->p_dir, rotate_deg);
		}
#endif
	    }
	}					 /* for */
    }						 /* 2. if */
  else
    {						 /* Big Packet */
      struct player_s_spacket *packet = (struct player_s_spacket *) sbuf;

      pl = &players[me->p_no];
      pl->p_dir = (unsigned char) packet->dir;
      Pdir[me->p_no] = (unsigned char) rosette(pl->p_dir);

      pl->p_speed = packet->speed;
      PlistNoteSpeed(me->p_no);

      if (F_cloak_maxwarp && pl != me)
	{
	  if (pl->p_speed == 0xf)
	    pl->p_flags |= PFCLOAK;
	  else if (pl->p_flags & PFCLOAK)
	    pl->p_flags &= ~PFCLOAK;
	}
      if (shortversion == SHORTVERSION)
	{					 /* S_P2 */
	  struct player_s2_spacket *pa2 = (struct player_s2_spacket *) sbuf;

	  x = SCALE * (short) ntohs(pa2->x);
	  y = SCALE * (short) ntohs(pa2->y);
	  new_flags(ntohl(pa2->flags), 0);
	}
      else
	{					 /* OLDSHORTVERSION */
	  x = ntohl(packet->x);
	  y = ntohl(packet->y);
	}

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
	  redrawPlayer[me->p_no] = 1;
	  PlistNoteUpdate(me->p_no);
	}
#endif

      pl->p_x = my_x = x;
      pl->p_y = my_y = y;
      Plx[me->p_no] = TWINSIDE / 2;
      Ply[me->p_no] = TWINSIDE / 2;
      Pgx[me->p_no] = pl->p_x * GWINSIDE / GWIDTH;
      Pgy[me->p_no] = pl->p_y * GWINSIDE / GWIDTH;
      redrawPlayer[me->p_no] = 1;

#ifdef ROTATERACE
      if (rotate)
	{
	  rotate_coord(&pl->p_x, &pl->p_y,
		       rotate_deg, GWIDTH / 2, GWIDTH / 2);
	  rotate_dir(&pl->p_dir, rotate_deg);
	}
#endif

      if (sbuf[1] == 0)
	return;
      sbuf += 12;				 /* Now the small packets */
      for (i = 0; i < numofplayers; i++)
	{
	  pl_no = ((unsigned char) *sbuf & 0x1f);
	  if (pl_no >= MAXPLAYER)
	    continue;
	  save = (unsigned char) *sbuf;
	  sbuf++;
	  pl = &players[pl_no];

	  pl->p_speed = (unsigned char) *sbuf & 15;	/* SPEED */
	  PlistNoteSpeed(pl_no);

	  if (F_cloak_maxwarp && pl != me)
	    {
	      if (pl->p_speed == 0xf)
		pl->p_flags |= PFCLOAK;
	      else if (pl->p_flags & PFCLOAK)
		pl->p_flags &= ~PFCLOAK;
	    }

	  Pdir[pl_no] = (unsigned char) *sbuf >> 4;	/* DIR */
	  pl->p_dir = (unsigned char) Pdir[pl_no] * 16;		/* real DIR */
	  sbuf++;
	  x = (unsigned char) *sbuf++;
	  y = (unsigned char) *sbuf++;		 /* The lower 8 Bits are * *
						  * saved */
	  /* Now we must preprocess the coordinates */
	  if ((unsigned char) save & 64)
	    x |= 256;
	  if ((unsigned char) save & 128)
	    y |= 256;

#ifdef WARP_DEAD
	  if (F_dead_warp && pl->p_speed == 14 && x == 501 && y == 501 && (pl->p_status != PEXPLODE))
	    {
	      pl->p_status = PEXPLODE;
	      x = pl->p_x;
	      y = pl->p_y;
	      if (pl->p_dir > DEADPACKETS)
		pl->p_explode = EX_FRAMES;
	      else
		pl->p_explode = 0;
	      redrawPlayer[me->p_no] = 1;
	      PlistNoteUpdate(me->p_no);
	    }
#endif

	  /* Now test if it's galactic or local coord */
	  if (save & 32)
	    {					 /* It's galactic */
	      if (x == 501 || y == 501)
		{
		  Pgx[pl_no] = -500;
		  Pgy[pl_no] = -500;
		}
	      else
		{
		  Pgx[pl_no] = x;
		  Pgy[pl_no] = y;
		}
	      Plx[pl_no] = -1;			 /* Not visible */
	      Ply[pl_no] = -1;
	      redrawPlayer[pl->p_no] = 1;
	      pl->p_x = Pgx[pl_no] * GWIDTH / SPWINSIDE;
	      pl->p_y = Pgy[pl_no] * GWIDTH / SPWINSIDE;

#ifdef ROTATERACE
	      if (rotate)
		{
		  rotate_coord(&pl->p_x, &pl->p_y,
			       rotate_deg, GWIDTH / 2, GWIDTH / 2);
		  rotate_dir(&pl->p_dir, rotate_deg);
		}
#endif
	    }
	  else
	    {					 /* Local */
	      Plx[pl_no] = x;
	      Ply[pl_no] = y;
	      redrawPlayer[pl->p_no] = 1;
	      pl->p_x = me->p_x + ((Plx[pl_no] - SPWINSIDE / 2) * SCALE);
	      pl->p_y = me->p_y + ((Ply[pl_no] - SPWINSIDE / 2) * SCALE);
	      Pgx[pl_no] = pl->p_x * SPWINSIDE / GWIDTH;
	      Pgy[pl_no] = pl->p_y * SPWINSIDE / GWIDTH;
	      pl->p_x = my_x + (x - SPWINSIDE / 2) * SCALE;
	      pl->p_y = my_y + (y - SPWINSIDE / 2) * SCALE;

#ifdef ROTATERACE
	      if (rotate)
		{
		  rotate_coord(&pl->p_x, &pl->p_y,
			       rotate_deg, GWIDTH / 2, GWIDTH / 2);
		  rotate_dir(&pl->p_dir, rotate_deg);
		}
#endif
	    }
	}					 /* for */
    }
}

void    handleSMessage(struct mesg_s_spacket *packet)
{
  char    buf[100];
  char    addrbuf[9];
  unsigned char flags;

  if (debug)
    printf("Length of Message is: %d  total Size %d \n", strlen(&packet->mesg), (int) packet->length);
  if (packet->m_from >= MAXPLAYER)
    packet->m_from = 255;

  if (packet->m_from == 255)
    strcpy(addrbuf, "GOD->");
  else
    {
      sprintf(addrbuf, " %c%c->", teamlet[players[packet->m_from].p_team],
	      shipnos[players[packet->m_from].p_no]);
    }

  switch (packet->m_flags & (MTEAM | MINDIV | MALL))
    {
    case MALL:
      sprintf(addrbuf + 5, "ALL");
      break;
    case MTEAM:
      sprintf(addrbuf + 5, teamshort[me->p_team]);
      break;
    case MINDIV:
      /* I know that it's me -> xxx but i copied it straight ... */
      sprintf(addrbuf + 5, "%c%c ", teamlet[players[packet->m_recpt].p_team],
	      shipnos[packet->m_recpt]);
      break;
    default:
      sprintf(addrbuf + 5, "ALL");
      break;
    }
  sprintf(buf, "%-9s%s", addrbuf, &packet->mesg);
  dmessage(buf, packet->m_flags, packet->m_from, packet->m_recpt);
}

void    handleShortReply(struct shortreply_spacket *packet)
{
  switch (packet->repl)
    {
    case SPK_VOFF:
      /* S_P2 */
      if (shortversion == SHORTVERSION &&
	  recv_short == 0)
	{					 /* retry for S_P 1 */
	  printf("Using Short Packet Version 1.\n");
	  shortversion = OLDSHORTVERSION;
	  sendShortReq(SPK_VON);
	}
      else
	{
	  recv_short = 0;
	  sprefresh(SPK_VFIELD);
	  /* 
	   * Get a `=' style update to fix the kills shown on the playerlist
	   * when you first enter the game.  It is more than likely that we
	   * were waiting for short packets so that a `-' update could be
	   * used instead.
	   */
	  sendUdpReq(COMM_UPDATE);
	}
      break;
    case SPK_VON:
      recv_short = 1;
      sprefresh(SPK_VFIELD);
      spwinside = ntohs(packet->winside);
      spgwidth = ntohl(packet->gwidth);
      printf("Receiving Short Packet Version %d\n", shortversion);
      /* 
       * Get a `-' style update to fix the kills shown on the playerlist
       * when you first enter and to fix other loss if short packets
       * have just been turned back on.
       */
      sendShortReq(SPK_SALL);
      break;
    case SPK_MOFF:
      recv_mesg = 0;
      sprefresh(SPK_MFIELD);
      W_SetSensitive(reviewWin, 0);
      break;
    case SPK_MON:
      recv_mesg = 1;
      sprefresh(SPK_MFIELD);
      W_SetSensitive(reviewWin, 1);
      break;
    case SPK_M_KILLS:
      recv_kmesg = 1;
      sprefresh(SPK_KFIELD);
      break;
    case SPK_M_NOKILLS:
      recv_kmesg = 0;
      sprefresh(SPK_KFIELD);
      break;
    case SPK_M_WARN:
      recv_warn = 1;
      sprefresh(SPK_WFIELD);
      break;
    case SPK_M_NOWARN:
      recv_warn = 0;
      sprefresh(SPK_WFIELD);
      break;

    case SPK_THRESHOLD:
      break;
    default:
      fprintf(stderr, "%s: unknown response packet value short-req: %d\n",
	      "netrek", packet->repl);
    }
}


void    handleVTorpInfo(unsigned char *sbuf)
{
  unsigned char *bitset, *which, *data, *infobitset, *infodata;
  struct torp *thetorp;
  int     dx, dy;
  int     shiftvar;
  char    status, war;
  register int i;
  register int shift = 0;			 /* How many torps are *

						  * 
						  * * extracted (for shifting 
						  * )  */

  /* now we must find the data ... :-) */
  bitset = &sbuf[1];
  which = &sbuf[2];
  infobitset = &sbuf[3];
  /* Where is the data ? */
  data = &sbuf[4];
  infodata = &sbuf[vtisize[numofbits[(unsigned char) sbuf[1]]]];

  weaponUpdate = 1;
  thetorp = &torps[((unsigned char) *which * 8)];

  for (shift = 0, i = 0; i < 8;
       thetorp++, *bitset >>= 1, *infobitset >>= 1, i++)
    {
      thetorp->t_updateFuse = TORP_UPDATE_FUSE;

      if (*bitset & 01)
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
	  /* Check for torp with no TorpInfo */
	  if (!(*infobitset & 01))
	    {
	      if (thetorp->t_status == TFREE)
		{
		  thetorp->t_status = TMOVE;	 /* guess */
		  players[thetorp->t_owner].p_ntorp++;
		}
	      else if (thetorp->t_owner == me->p_no &&
		       thetorp->t_status == TEXPLODE)
		{				 /* If TFREE got lost */
		  thetorp->t_status = TMOVE;	 /* guess */
		}
	    }

	  /* Check if torp is visible */
	  if (dx > SPWINSIDE || dy > SPWINSIDE)
	    {
	      thetorp->t_x = -100000;		 /* Not visible */
	      thetorp->t_y = -100000;
	    }
	  else
	    {					 /* visible */
	      /* thetorp->t_x = me->p_x + ((dx - SPWINSIDE / 2) * SCALE); * * 
	       * thetorp->t_y = me->p_y + ((dy - SPWINSIDE / 2) * SCALE); */
	      thetorp->t_x = my_x + ((dx - SPWINSIDE / 2) *
				     SCALE);
	      thetorp->t_y = my_y + ((dy - SPWINSIDE / 2) *
				     SCALE);

#ifdef ROTATERACE
	      if (rotate)
		{
		  rotate_coord(&thetorp->t_x, &thetorp->t_y, rotate_deg,
			       GWIDTH / 2, GWIDTH / 2);
		}
#endif
	    }
	}					 /* if */
      else
	{					 /* Got a TFREE ? */
	  if (!(*infobitset & 01))
	    {					 /* No other TorpInfo for * * 
						  * this Torp */
	      if (thetorp->t_status && thetorp->t_status != TEXPLODE)
		{
		  players[thetorp->t_owner].p_ntorp--;
		  thetorp->t_status = TFREE;	 /* That's no guess */
		}
	    }
	}
      /* Now the TorpInfo */
      if (*infobitset & 01)
	{
	  war = (unsigned char) *infodata & 15 /* 0x0f */ ;
	  status = ((unsigned char) *infodata & 0xf0) >> 4;
	  infodata++;
	  if (status == TEXPLODE && thetorp->t_status == TFREE)
	    {
	      /* FAT: redundant explosion; don't update p_ntorp */
	      continue;
	    }
	  if (thetorp->t_status == TFREE && status)
	    {
	      players[thetorp->t_owner].p_ntorp++;
	    }
	  if (thetorp->t_status && status == TFREE)
	    {
	      players[thetorp->t_owner].p_ntorp--;
	    }
	  thetorp->t_war = war;
	  if (status != thetorp->t_status)
	    {
	      /* FAT: prevent explosion reset */
	      thetorp->t_status = status;
	      if (thetorp->t_status == TEXPLODE)
		{
		  thetorp->t_fuse = NUMDETFRAMES;
		}
	    }
	}					 /* if */

    }						 /* for */
}


void    handleVPlanet(unsigned char *sbuf)
{
  register int i;
  register int numofplanets;			 /* How many Planets are in * 

						  * 
						  * 
						  * * the packet */
  struct planet *plan;
  struct planet_s_spacket *packet = (struct planet_s_spacket *) &sbuf[2];

#ifdef ATM
  /* FAT: prevent excessive redraw */
  int     redraw = 0;

#endif /* ATM */

  numofplanets = (unsigned char) sbuf[1];

  if (numofplanets > MAXPLANETS + 1)
    return;

  for (i = 0; i < numofplanets; i++, packet++)
    {
      if (packet->pnum >= MAXPLANETS)
	continue;

      plan = &planets[packet->pnum];

#ifdef ATM
      if (plan->pl_owner != packet->owner)
	redraw = 1;
#endif /* ATM */

      plan->pl_owner = packet->owner;

#ifdef TSH
      if (plan->pl_owner < FED || plan->pl_owner > ORI)
	plan->pl_owner = NOBODY;
#endif /* TSH */

#ifdef ATM
      if (plan->pl_info != packet->info)
	redraw = 1;
#endif /* ATM */

      plan->pl_info = packet->info;
      /* Redraw the planet because it was updated by server */

#ifdef ATM
      if (plan->pl_flags != (int) ntohs(packet->flags))
	redraw = 1;
      plan->pl_flags = (int) ntohs(packet->flags);
#else
      plan->pl_flags = (int) ntohs(packet->flags) | PLREDRAW;
#endif /* ATM */

#ifdef ATM
      if (plan->pl_armies != (unsigned char) packet->armies)
	{

#ifdef EM
	  /* don't redraw when armies change unless it crosses the '4' * army
	   * * * limit. Keeps people from watching for planet 'flicker' * when 
	   * *  * players are beaming */
	  int     planetarmies = (unsigned char) packet->armies;

	  if ((plan->pl_armies < 5 && planetarmies > 4) ||
	      (plan->pl_armies > 4 && planetarmies < 5))
#endif

	    redraw = 1;
	}
#endif /* ATM */

      plan->pl_armies = (unsigned char) packet->armies;

#ifndef RECORDGAME
      if (plan->pl_info == 0)
	{
	  plan->pl_owner = NOBODY;
	}
#endif

#ifdef ATM
      if (redraw)
	plan->pl_flags |= PLREDRAW;
#endif /* ATM */

    }						 /* FOR */
}


resetWeaponInfo(void)
/*
 *  Give all weapons for all ships the status of not being active.
 */
{
  register int i;

  for (i = 0; i < MAXPLAYER * MAXTORP; i++)
    torps[i].t_status = TFREE;

  for (i = 0; i < MAXPLAYER * MAXPLASMA; i++)
    plasmatorps[i].pt_status = PTFREE;

  for (i = 0; i < MAXPLAYER; i++)
    {
      players[i].p_ntorp = 0;
      players[i].p_nplasmatorp = 0;
      phasers[i].ph_status = PHFREE;
    }
}


void    sendShortReq(char state)
{
  struct shortreq_cpacket shortReq;

  bzero(&shortReq, sizeof(shortReq));
  shortReq.type = CP_S_REQ;
  shortReq.req = state;
  shortReq.version = shortversion;		 /* need a var now because 2
						  * * * S_P versions exist
						  * S_P2  */
  switch (state)
    {
    case SPK_VON:
      warning("Sending short packet request");
      break;
    case SPK_VOFF:
      warning("Sending old packet request");
      break;
    default:
      break;
    }
  if ((state == SPK_SALL || state == SPK_ALL) && recv_short)
    {
      /* Let the client do the work, and not the network :-) */

      resetWeaponInfo();

      if (state == SPK_SALL)
	warning("Sent request for small update (weapons+planets+kills)");
      else if (state == SPK_ALL)
	warning("Sent request for medium update (all except stats)");
      else
	warning("Sent some unknown request...");
    }

  sendServerPacket((struct shortreq_cpacket *) &shortReq);
}

char   *whydeadmess[] =
{"", "[quit]", "[photon]", "[phaser]", "[planet]", "[explosion]",
 "[daemon]", "[winner]", "[ghostbust]", "[genocide]", "[hacker]", "[plasma]",
 "[tournend]", "[gameover]", "[gamestart]", "[bad binary]",
 "[detted photon]", "[chain explosion]",
 "[zapped plasma]", "", "[team det]", "[team explosion]"};


void    handleSWarning(struct warning_s_spacket *packet)
{
  char    buf[80];
  register struct player *target;
  register int damage;
  static int arg3, arg4;			 /* Here are the arguments *

						  * 
						  * * for warnings with more
						  * * * than 2 arguments */
  static int karg3, karg4, karg5 = 0;
  char    killmess[20];

#ifdef RCM
  struct distress dist;

#endif

  switch (packet->whichmessage)
    {
    case TEXTE:				 /* damage used as tmp var */
      damage = (unsigned char) packet->argument;
      damage |= (unsigned char) packet->argument2 << 8;

#ifdef PHASER_STATS
      if (damage == 38)				 /* Plasma hit */
	{
	  phaserStatTry++;
	  phaserStatHit++;
	}
      if (damage == 37)				 /* Miss */
	phaserStatTry++;
      if ((damage == 37) && phaserShowStats)
	{
	  /* Mung the message */
	  char    phstatmsg[30];

	  sprintf(phstatmsg, "%s [%d%%]", w_texts[damage],
		  phaserStatTry ? (phaserStatHit * 100) / phaserStatTry : 0);
	  /* Divide by zero sucks */
	  warning(phstatmsg);
	}
      else
#endif

      if (damage >= 0 && damage < NUMWTEXTS)
	warning(w_texts[damage]);
      break;
    case PHASER_HIT_TEXT:
      target = &players[(unsigned char) packet->argument & 0x3f];
      damage = (unsigned char) packet->argument2;
      if ((unsigned char) packet->argument & 64)
	damage |= 256;
      if ((unsigned char) packet->argument & 128)
	damage |= 512;

#ifdef PHASER_STATS
      phaserStatTry++;
      phaserStatHit++;
#endif

      (void) sprintf(buf, "Phaser burst hit %s for %d points", target->p_name, damage);
      warning(buf);
      break;
    case BOMB_INEFFECTIVE:
      sprintf(buf, "Weapons Officer: Bombing is ineffective.  Only %d armies are defending.",
	      (int) packet->argument);		 /* nifty info feature * *
						  * 2/14/92 TMC */
      warning(buf);
      break;
    case BOMB_TEXT:
      sprintf(buf, "Weapons Officer: Bombarding %s...  Sensors read %d armies left.",
	      planets[(unsigned char) packet->argument].pl_name,
	      (unsigned char) packet->argument2);
      warning(buf);
      break;
    case BEAMUP_TEXT:
      sprintf(buf, "%s: Too few armies to beam up",
	      planets[(unsigned char) packet->argument].pl_name);
      warning(buf);
      break;
    case BEAMUP2_TEXT:
      sprintf(buf, "Beaming up. (%d/%d)", (unsigned char) packet->argument, (unsigned char) packet->argument2);
      warning(buf);
      break;
    case BEAMUPSTARBASE_TEXT:
      sprintf(buf, "Starbase %s: Too few armies to beam up",
	      players[packet->argument].p_name);
      warning(buf);
      break;
    case BEAMDOWNSTARBASE_TEXT:
      sprintf(buf, "No more armies to beam down to Starbase %s.",
	      players[packet->argument].p_name);
      warning(buf);

      break;
    case BEAMDOWNPLANET_TEXT:
      sprintf(buf, "No more armies to beam down to %s.",
	      planets[(unsigned char) packet->argument].pl_name);
      warning(buf);
      break;
    case SBREPORT:
      sprintf(buf, "Transporter Room:  Starbase %s reports all troop bunkers are full!",
	      players[packet->argument].p_name);
      warning(buf);
      break;
    case ONEARG_TEXT:
      if (packet->argument < NUMVARITEXTS)
	{
	  sprintf(buf, vari_texts[packet->argument], (unsigned char) packet->argument2);
	  warning(buf);
	}
      break;
    case BEAM_D_PLANET_TEXT:
      sprintf(buf, "Beaming down.  (%d/%d) %s has %d armies left",
	      arg3,
	      arg4,
	      planets[(unsigned char) packet->argument].pl_name,
	      packet->argument2);
      warning(buf);
      break;
    case ARGUMENTS:
      arg3 = (unsigned char) packet->argument;
      arg4 = (unsigned char) packet->argument2;
      break;
    case BEAM_U_TEXT:
      sprintf(buf, "Transfering ground units.  (%d/%d) Starbase %s has %d armies left",
	      (unsigned char) arg3, (unsigned char) arg4, players[packet->argument].p_name, (unsigned char) packet->argument2);
      warning(buf);
      break;
    case LOCKPLANET_TEXT:
      sprintf(buf, "Locking onto %s", planets[(unsigned char) packet->argument].pl_name);
      warning(buf);
      break;
    case SBRANK_TEXT:
      sprintf(buf, "You need a rank of %s or higher to command a starbase!", ranks[packet->argument].name);
      warning(buf);
      break;
    case SBDOCKREFUSE_TEXT:
      sprintf(buf, "Starbase %s refusing us docking permission captain.",
	      players[packet->argument].p_name);
      warning(buf);
      break;
    case SBDOCKDENIED_TEXT:
      sprintf(buf, "Starbase %s: Permission to dock denied, all ports currently occupied.", players[packet->argument].p_name);
      warning(buf);
      break;
    case SBLOCKSTRANGER:
      sprintf(buf, "Locking onto %s (%c%c)",
	      players[packet->argument].p_name,
	      teamlet[players[packet->argument].p_team],
	      shipnos[players[packet->argument].p_no]);
      warning(buf);
      break;
    case SBLOCKMYTEAM:
      sprintf(buf, "Locking onto %s (%c%c) (docking is %s)",
	      players[packet->argument].p_name,
	      teamlet[players[packet->argument].p_team],
	      shipnos[players[packet->argument].p_no],
	      (players[packet->argument].p_flags & PFDOCKOK) ? "enabled" : "disabled");
      warning(buf);
      break;
    case DMKILL:

#ifdef RCM
    case INLDMKILL:
#endif

      {
	struct mesg_spacket msg;
	unsigned char killer, victim, armies;
	float   kills;

	victim = (unsigned char) packet->argument & 0x3f;
	killer = (unsigned char) packet->argument2 & 0x3f;
	/* that's only a temp */
	damage = (unsigned char) karg3;
	damage |= (karg4 & 127) << 8;

#ifndef RCM
	kills = damage / 100.0;
	if (kills == 0.0)
	  strcpy(killmess, "NO CREDIT");
	else
	  sprintf(killmess, "%0.2f", kills);
#endif

	armies = (((unsigned char) packet->argument >> 6) | ((unsigned char) packet->argument2 & 192) >> 4);
	if (karg4 & 128)
	  armies |= 16;

#ifdef RCM
	dist.distype = rcm;
	dist.sender = victim;
	dist.tclose_j = killer;
	dist.arms = armies;
	dist.dam = damage / 100;
	dist.shld = damage % 100;
	dist.wtmp = karg5;
	makedistress(&dist, msg.mesg, rcm_msg[1].macro);
	msg.m_flags = MALL | MVALID | MKILL;
#else
	if (armies == 0)
	  {
	    (void) sprintf(msg.mesg, "GOD->ALL %s (%c%c) was kill %s for %s (%c%c)",
			   players[victim].p_name,
			   teamlet[players[victim].p_team],
			   shipnos[victim],
			   killmess,
			   players[killer].p_name,
			   teamlet[players[killer].p_team],
			   shipnos[killer]);
	    msg.m_flags = MALL | MVALID | MKILL;
	  }
	else
	  {
	    (void) sprintf(msg.mesg, "GOD->ALL %s (%c%c+%d armies) was kill %s for %s (%c%c)",
			   players[victim].p_name,
			   teamlet[players[victim].p_team],
			   shipnos[victim],
			   armies,
			   killmess,
			   players[killer].p_name,
			   teamlet[players[killer].p_team],
			   shipnos[killer]);
	    msg.m_flags = MALL | MVALID | MKILLA;
	  }
	if (why_dead)
	  {
	    dist->wtmp = karg5
		add_whydead(msg.mesg, karg5);
	    karg5 = 0;
	  }
#endif

	msg.type = SP_MESSAGE;
	msg.mesg[79] = '\0';
	msg.m_recpt = 0;
	msg.m_from = 255;
	handleMessage(&msg);
      }
      break;
    case KILLARGS:
      karg3 = (unsigned char) packet->argument;
      karg4 = (unsigned char) packet->argument2;
      break;
    case KILLARGS2:
      karg5 = (unsigned char) packet->argument;
      break;
    case DMKILLP:

#ifdef RCM
    case INLDMKILLP:
#endif

      {
	struct mesg_spacket msg;

#ifdef RCM
	dist.distype = rcm;
	dist.sender = packet->argument;
	dist.tclose_j = packet->argument;
	dist.arms = '\0';
	dist.dam = '\0';
	dist.shld = '\0';
	dist.tclose_pl = packet->argument2;
	dist.wtmp = karg5;
	makedistress(&dist, msg.mesg, rcm_msg[2].macro);
#else
	(void) sprintf(msg.mesg, "GOD->ALL %s (%c%c) killed by %s (%c)",
		       players[packet->argument].p_name,
		       teamlet[players[packet->argument].p_team],
		       shipnos[packet->argument],
		       planets[(unsigned char) packet->argument2].pl_name,
	      teamlet[planets[(unsigned char) packet->argument2].pl_owner]);
	if (why_dead)
	  {
	    add_whydead(msg.mesg, karg5);
	    karg5 = 0;
	  }
#endif

	msg.type = SP_MESSAGE;
	msg.mesg[79] = '\0';
	msg.m_flags = MALL | MVALID | MKILLP;
	msg.m_recpt = 0;
	msg.m_from = 255;
	handleMessage(&msg);
      }
      break;
    case DMBOMB:
      {
	struct mesg_spacket msg;
	char    buf1[80];

#ifdef RCM
	dist.distype = rcm;
	dist.sender = packet->argument;
	dist.tclose_j = packet->argument;
	dist.arms = '\0';
	dist.dam = arg3;
	dist.shld = '\0';
	dist.tclose_pl = packet->argument2;
	dist.wtmp = karg5;
	makedistress(&dist, msg.mesg, rcm_msg[3].macro);
#else
	(void) sprintf(buf, "%-3s->%-3s", planets[(unsigned char) packet->argument2].pl_name, teamshort[planets[(unsigned char) packet->argument2].pl_owner]);
	(void) sprintf(buf1, "We are being attacked by %s %c%c who is %d%% damaged.",
		       players[packet->argument].p_name,
		       teamlet[players[packet->argument].p_team],
		       shipnos[packet->argument],
		       arg3);
	(void) sprintf(msg.mesg, "%s %s", buf, buf1);
#endif

	msg.type = SP_MESSAGE;
	msg.mesg[79] = '\0';
	msg.m_flags = MTEAM | MBOMB | MVALID;
	msg.m_recpt = planets[(unsigned char) packet->argument2].pl_owner;
	msg.m_from = 255;
	handleMessage(&msg);
      }
      break;
    case DMDEST:
      {
	struct mesg_spacket msg;

#ifdef RCM
	dist.distype = rcm;
	dist.sender = packet->argument2;
	dist.tclose_j = packet->argument2;
	dist.arms = '\0';
	dist.dam = '\0';
	dist.shld = '\0';
	dist.tclose_pl = packet->argument;
	dist.wtmp = karg5;
	makedistress(&dist, msg.mesg, rcm_msg[4].macro);
#else
	char    buf1[80];

	(void) sprintf(buf, "%s destroyed by %s (%c%c)",
		       planets[(unsigned char) packet->argument].pl_name,
		       players[packet->argument2].p_name,
		       teamlet[players[packet->argument2].p_team],
		       shipnos[(unsigned char) packet->argument2]);
	(void) sprintf(buf1, "%-3s->%-3s",
		       planets[(unsigned char) packet->argument].pl_name, teamshort[planets[(unsigned char) packet->argument].pl_owner]);
	(void) sprintf(msg.mesg, "%s %s", buf1, buf);
#endif

	msg.type = SP_MESSAGE;
	msg.mesg[79] = '\0';
	msg.m_flags = MTEAM | MDEST | MVALID;
	msg.m_recpt = planets[(unsigned char) packet->argument].pl_owner;
	msg.m_from = 255;
	handleMessage(&msg);
      }
      break;
    case DMTAKE:
      {
	struct mesg_spacket msg;

#ifdef RCM
	dist.distype = rcm;
	dist.sender = packet->argument2;
	dist.tclose_j = packet->argument2;
	dist.arms = '\0';
	dist.dam = '\0';
	dist.shld = '\0';
	dist.tclose_pl = packet->argument;
	dist.wtmp = karg5;
	makedistress(&dist, msg.mesg, rcm_msg[5].macro);
#else
	char    buf1[80];

	(void) sprintf(buf, "%s taken over by %s (%c%c)",
		       planets[(unsigned char) packet->argument].pl_name,
		       players[packet->argument2].p_name,
		       teamlet[players[packet->argument2].p_team],
		       shipnos[packet->argument2]);
	(void) sprintf(buf1, "%-3s->%-3s",
		       planets[(unsigned char) packet->argument].pl_name, teamshort[players[packet->argument2].p_team]);
	(void) sprintf(msg.mesg, "%s %s", buf1, buf);
#endif

	msg.type = SP_MESSAGE;
	msg.mesg[79] = '\0';
	msg.m_flags = MTEAM | MTAKE | MVALID;
	msg.m_recpt = players[packet->argument2].p_team;
	msg.m_from = 255;
	handleMessage(&msg);
      }
      break;
    case DGHOSTKILL:
      {
	struct mesg_spacket msg;
	ushort  damage;

	damage = (unsigned char) karg3;
	damage |= (unsigned char) (karg4 & 0xff) << 8;

#ifdef RCM
	dist.distype = rcm;
	dist.sender = packet->argument;
	dist.tclose_j = packet->argument;
	dist.arms = '\0';
	dist.dam = damage / 100;
	dist.shld = damage % 100;
	dist.wtmp = karg5;
	makedistress(&dist, msg.mesg, rcm_msg[6].macro);
#else
	(void) sprintf(msg.mesg, "GOD->ALL %s (%c%c) was kill %0.2f for the GhostBusters",
		       players[(unsigned char) packet->argument].p_name, teamlet[players[(unsigned char) packet->argument].p_team],
		       shipnos[(unsigned char) packet->argument],
		       (float) damage / 100.0);
#endif

	msg.type = SP_MESSAGE;
	msg.mesg[79] = '\0';
	msg.m_flags = MALL | MVALID;
	msg.m_recpt = 0;
	msg.m_from = 255;
	handleMessage(&msg);
      }
      break;
      /* INL Daemon Mesages */

#ifndef RCM
    case INLDMKILLP:
      {
	struct mesg_spacket msg;

	sprintf(buf, "");
	if (arg3)
	  {					 /* Armies */
	    sprintf(buf, "+%d", arg3);
	  }
	(void) sprintf(msg.mesg, "GOD->ALL %s(%s) (%c%c%s) killed by %s (%c)",
		       players[(unsigned char) packet->argument].p_name,
	  shiptype[players[(unsigned char) packet->argument].p_ship.s_type],
		  teamlet[players[(unsigned char) packet->argument].p_team],
		       shipnos[(unsigned char) packet->argument],
		       buf,
		       planets[(unsigned char) packet->argument2].pl_name,
	      teamlet[planets[(unsigned char) packet->argument2].pl_owner]);
	if (why_dead)
	  {
	    add_whydead(msg.mesg, karg5);
	    karg5 = 0;
	  }
	msg.type = SP_MESSAGE;
	msg.mesg[79] = '\0';
	msg.m_flags = MALL | MVALID | MKILLP;
	msg.m_recpt = 0;
	msg.m_from = 255;
	handleMessage(&msg);
      }
      break;
    case INLDMKILL:
      {
	struct mesg_spacket msg;
	int     killer, victim, armies;
	float   kills;

	victim = (unsigned char) packet->argument & 0x3f;
	killer = (unsigned char) packet->argument2 & 0x3f;
	/* that's only a temp */
	damage = (unsigned char) karg3;
	damage |= (karg4 & 127) << 8;
	kills = damage / 100.0;
	armies = (((unsigned char) packet->argument >> 6) | ((unsigned char) packet->argument2 & 192) >> 4);
	if (karg4 & 128)
	  armies |= 16;
	if (armies == 0)
	  {
	    (void) sprintf(msg.mesg, "GOD->ALL %s(%s) (%c%c) was kill %0.2f for %s(%s) (%c%c)",
			   players[victim].p_name,
			   shiptype[players[victim].p_ship.s_type],
			   teamlet[players[victim].p_team],
			   shipnos[victim],
			   kills,
			   players[killer].p_name,
			   shiptype[players[killer].p_ship.s_type],
			   teamlet[players[killer].p_team],
			   shipnos[killer]);
	    msg.m_flags = MALL | MVALID | MKILL;
	  }
	else
	  {
	    (void) sprintf(msg.mesg, "GOD->ALL %s(%s) (%c%c+%d armies) was kill %0.2f for %s(%s) (%c%c)",
			   players[victim].p_name,
			   shiptype[players[victim].p_ship.s_type],
			   teamlet[players[victim].p_team],
			   shipnos[victim],
			   armies,
			   kills,
			   players[killer].p_name,
			   shiptype[players[killer].p_ship.s_type],
			   teamlet[players[killer].p_team],
			   shipnos[killer]);
	    msg.m_flags = MALL | MVALID | MKILLA;
	  }
	if (why_dead)
	  {
	    add_whydead(msg.mesg, karg5);
	    karg5 = 0;
	  }
	msg.type = SP_MESSAGE;
	msg.mesg[79] = '\0';
	msg.m_recpt = 0;
	msg.m_from = 255;
	handleMessage(&msg);
      }
      break;
#endif

    case INLDRESUME:
      {
	struct mesg_spacket msg;

	sprintf(msg.mesg, " Game will resume in %d seconds", (unsigned char) packet->argument);
	msg.m_flags = MALL | MVALID;
	msg.type = SP_MESSAGE;
	msg.mesg[79] = '\0';
	msg.m_recpt = 0;
	msg.m_from = 255;
	handleMessage(&msg);
      }
      break;
    case INLDTEXTE:
      if ((unsigned char) packet->argument < NUMDAEMONTEXTS)
	{
	  struct mesg_spacket msg;

	  strcpy(msg.mesg, daemon_texts[(unsigned char) packet->argument]);
	  msg.m_flags = MALL | MVALID;
	  msg.type = SP_MESSAGE;
	  msg.mesg[79] = '\0';
	  msg.m_recpt = 0;
	  msg.m_from = 255;
	  handleMessage(&msg);
	}
      break;
    case STEXTE:
      warning(s_texte[(unsigned char) packet->argument]);
      break;
    case SHORT_WARNING:
      {
	struct warning_spacket *warn = (struct warning_spacket *) packet;

	warning(warn->mesg);
      }
      break;
    case STEXTE_STRING:
      {
	struct warning_spacket *warn = (struct warning_spacket *) packet;

	warning(warn->mesg);
	s_texte[(unsigned char) warn->pad2] = (char *) malloc(warn->pad3 - 4);
	if (s_texte[(unsigned char) warn->pad2] == NULL)
	  {
	    s_texte[(unsigned char) warn->pad2] = no_memory;
	    warning("Could not add warning! (No memory!)");
	  }
	else
	  strcpy(s_texte[(unsigned char) warn->pad2], warn->mesg);
      }
      break;
    default:
      warning("Unknown Short Warning!");
      break;
    }
}

#define MY_SIZEOF(a) (sizeof(a) / sizeof(*(a)))

add_whydead(char *s, int m)			 /* 7/22/93 LAB */


{
  char    b[256];

  if (m < MY_SIZEOF(whydeadmess))
    {
      sprintf(b, "%-50s %s", s, whydeadmess[m]);
      b[79] = '\0';
      strcpy(s, b);
    }
}

/* S_P2 */
void
        handleVKills(sbuf)
unsigned char *sbuf;
{
  register int i, numofkills, pnum;
  register unsigned short pkills;
  register unsigned char *data = &sbuf[2];

  numofkills = (unsigned char) sbuf[1];

  for (i = 0; i < numofkills; i++)
    {
      pkills = (unsigned short) *data++;
      pkills |= (unsigned short) ((*data & 0x03) << 8);
      pnum = (unsigned char) *data++ >> 2;

#ifdef CORRUPTED_PACKETS
      if (pnum < 0 || pnum >= MAXPLAYER)
	{
	  fprintf(stderr, "handleKills: bad index %d\n", pnum);
	  return;
	}
#endif

      if (players[pnum].p_kills != ((float) pkills / 100.0))
	{
	  players[pnum].p_kills = pkills / 100.0;
	  /* FAT: prevent redundant player update */
	  PlistNoteUpdate(pnum);

#ifdef ARMY_SLIDER
	  if (me == &players[(int) pnum])
	    {
	      calibrate_stats();
	      redrawStats();
	    }
#endif /* ARMY_SLIDER */
	}

    }						 /* for */

}						 /* handleVKills */

void
        handleVPhaser(sbuf)
unsigned char *sbuf;
{
  struct phaser *phas;
  register struct player *j;
  struct phaser_s_spacket *packet = (struct phaser_s_spacket *) &sbuf[0];

  /* not nice but.. */
  register int pnum, status, target, x, y, dir;

  status = packet->status & 0x0f;
  pnum = packet->pnum & 0x3f;
  weaponUpdate = 1;

  switch (status)
    {
    case PHFREE:
      break;
    case PHHIT:
      target = (unsigned char) packet->target & 0x3f;
      break;
    case PHMISS:
      dir = (unsigned char) packet->target;
      break;
    case PHHIT2:
      x = SCALE * (ntohs(packet->x));
      y = SCALE * (ntohs(packet->y));
      target = packet->target & 0x3f;
      break;
    default:
      x = SCALE * (ntohs(packet->x));
      y = SCALE * (ntohs(packet->y));
      target = packet->target & 0x3f;
      dir = (unsigned char) packet->dir;
      break;
    }
  phas = &phasers[pnum];
  phas->ph_status = status;
  phas->ph_dir = dir;
  phas->ph_x = x;
  phas->ph_y = y;
  phas->ph_target = target;
  phas->ph_fuse = 0;
  phas->ph_updateFuse = PHASER_UPDATE_FUSE;

#ifdef ROTATERACE
  if (rotate)
    {
      rotate_coord(&phas->ph_x, &phas->ph_y, rotate_deg, GWIDTH / 2, GWIDTH / 2);
      rotate_dir(&phas->ph_dir, rotate_deg);
    }
#endif
}

void
        handle_s_Stats(packet)
struct stats_s_spacket *packet;
{
  register struct player *pl;

#ifdef CORRUPTED_PACKETS
  if (packet->pnum < 0 || packet->pnum >= MAXPLAYER)
    {
      fprintf(stderr, "handleStats: bad index %d\n", packet->pnum);
      return;
    }
#endif

  pl = &players[packet->pnum];

  pl->p_stats.st_tkills = ntohs(packet->tkills);
  pl->p_stats.st_tlosses = ntohs(packet->tlosses);
  pl->p_stats.st_kills = ntohs(packet->kills);
  pl->p_stats.st_losses = ntohs(packet->losses);
  pl->p_stats.st_tticks = ntohl(packet->tticks);
  pl->p_stats.st_tplanets = ntohs(packet->tplanets);
  pl->p_stats.st_tarmsbomb = ntohl(packet->tarmies);
  pl->p_stats.st_sbkills = ntohs(packet->sbkills);
  pl->p_stats.st_sblosses = ntohs(packet->sblosses);
  pl->p_stats.st_armsbomb = ntohs(packet->armies);
  pl->p_stats.st_planets = ntohs(packet->planets);
  if ((pl->p_ship.s_type == STARBASE) && (SBhours))
    {
      pl->p_stats.st_sbticks = ntohl(packet->maxkills);
    }
  else
    {
      pl->p_stats.st_maxkills = ntohl(packet->maxkills) / 100.0;
    }
  pl->p_stats.st_sbmaxkills = ntohl(packet->sbmaxkills) / 100.0;

  PlistNoteUpdate(packet->pnum);
}

void    new_flags(unsigned int data, int which)
{
  register int pnum, status;
  register unsigned int new, tmp;
  unsigned int oldflags;
  struct player *j;

  tmp = data;
  for (pnum = which * 16; pnum < (which + 1) * 16 && pnum < MAXPLAYER; pnum++)
    {
      new = tmp & 0x03;
      tmp >>= 2;
      j = &players[pnum];

      oldflags = j->p_flags;
      switch (new)
	{
	case 0:				 /* PDEAD/PEXPLODE */
	  status = PEXPLODE;
	  j->p_flags &= ~PFCLOAK;
	  break;
	case 1:				 /* PALIVE & PFCLOAK */
	  status = PALIVE;
	  j->p_flags |= PFCLOAK;
	  break;
	case 2:				 /* PALIVE & PFSHIELD */
	  status = PALIVE;
	  j->p_flags |= PFSHIELD;
	  j->p_flags &= ~PFCLOAK;
	  break;
	case 3:				 /* PALIVE & NO shields */
	  status = PALIVE;
	  j->p_flags &= ~(PFSHIELD | PFCLOAK);
	  break;
	default:
	  break;
	}

      if (oldflags != j->p_flags)
	redrawPlayer[pnum] = 1;

      if (j->p_status == status)
	continue;

      if (status == PEXPLODE)
	{
	  if (j->p_status == PALIVE)
	    {
	      j->p_explode = 0;
	      j->p_status = status;
	    }
	  else					 /* Do nothing */
	    continue;
	}
      else
	{					 /* really PALIVE ? */
	  if (j == me)
	    {
	      /* Wait for POUTFIT */
	      if (j->p_status == POUTFIT || j->p_status == PFREE)
		{
		  if (j->p_status != PFREE)
		    j->p_kills = 0.;
		  else
		    PlistNoteArrive(pnum);

		  j->p_status = PALIVE;
		}
	    }
	  else
	    {
	      if (j->p_status != PFREE)
		j->p_kills = 0.;
	      else
		PlistNoteArrive(pnum);

	      j->p_status = status;
	    }
	}
      redrawPlayer[pnum] = 1;
      PlistNoteUpdate(pnum);
    }						 /* for */
}
#endif

/* END SHORT_PACKETS */
