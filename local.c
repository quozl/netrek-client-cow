/*
 * Local.c
 *
 * Functions to maintain the local map.
 */

#include "config.h"
#include "copyright2.h"

#include <stdio.h>
#include <math.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "local.h"

#ifdef WIN32
/* Fixup for minor inconsistencies between SAC's interger
 * linedraw and Win32 LineTo() */
int     tpline = -1;

#endif

/* Local Variables */

static int clearcount = 0;
static int clearzone[4][(MAXTORP + 1) * MAXPLAYER +
                        (MAXPLASMA + 1) * MAXPLAYER + MAXPLANETS];
static int clearlcount = 0;
#ifdef HOCKEY_LINES
static int clearline[4][MAXPLAYER + 2 * MAXPLAYER + NUM_HOCKEY_LINES];
#else
static int clearline[4][MAXPLAYER + 2 * MAXPLAYER];
#endif

#ifdef SOUND
static int sound_torps = 0;
static int sound_other_torps = 0;
static int num_other_torps = 0;
static int sound_plasma = 0;
static int sound_other_plasma = 0;
static int num_other_plasma = 0;

#if 0 /* unused? */
static int sound_other_explode = 0;
#endif

unsigned int sound_flags = PFSHIELD;

#endif

extern void *S_Plasma(int);
extern void *S_Ship(int);
extern void *S_Torp(int);
extern int W_DrawSprite(void *, int, int, int);
int clearsize;


/* Function Defininitions */

#define NORMALIZE(d) (((d) + 256) % 256)


static inline W_Icon
        planetBitmap(register struct planet *p)
{
  int     i;

  if (showlocal == 0)				 /* nothing */
    {
      return (bplanets[0]);
    }
  else if ((p->pl_info & me->p_team)

#ifdef RECORDGAME
	   || playback
#endif

      )
    {
      if (showlocal == 1)			 /* owner */
	{
	  return (bplanets[remap[p->pl_owner]]);
	}
      else
	/* resources */
	{
	  i = 0;
	  if (p->pl_armies > 4)
	    i += 4;
	  if (p->pl_flags & PLREPAIR)
	    i += 2;
	  if (p->pl_flags & PLFUEL)
	    i += 1;
	  switch (showlocal)
	    {
	    case 2:		                 /* standard */
	      return (bplanets2[i]);
	    case 3:		                 /* ZZ aka MOO */
	      return (bplanets3[i]);
	    case 4:                              /* rabbit ears */
	      return (bplanets4[i]);
	    default:
	      return (bplanets[0]);
	    }
	}
    }
  else
    {
      return (noinfoplanet);
    }
}


static void DrawPlanets(void)
/*
 * Draw the planets on the local map.
 */
{
  register int dx, dy;
  register struct planet *l;
  const int view = SCALE * TWINSIDE / 2;

  for (l = planets + MAXPLANETS - 1; l >= planets; --l)
    {
      dx = l->pl_x - me->p_x;
      dy = l->pl_y - me->p_y;
      if (dx > view || dx < -view || dy > view || dy < -view)
	continue;

      dx = dx / SCALE + TWINSIDE / 2;
      dy = dy / SCALE + TWINSIDE / 2;


      W_WriteBitmap(dx - (planet_width / 2), dy - (planet_height / 2),
		    planetBitmap(l), planetColor(l));

      if (showIND && ((l->pl_info & me->p_team)

#ifdef RECORDGAME
		      || playback
#endif

	  ) && (l->pl_owner == NOBODY))
	{
	  W_CacheLine(w, dx - (planet_width / 2), dy - (planet_height / 2),
		  dx + (planet_width / 2 - 1), dy + (planet_height / 2 - 1),
		      W_White);
	  W_CacheLine(w, dx + (planet_width / 2 - 1), dy - (planet_height / 2),
		      dx - (planet_width / 2), dy + (planet_height / 2 - 1),
		      W_White);
	}

      if (namemode)
	{
	  W_MaskText(w, dx - (planet_width / 2), dy + (planet_height / 2),
		     planetColor(l), l->pl_name, l->pl_namelen,
		     planetFont(l));
	  clearzone[0][clearcount] = dx - (planet_width / 2);
	  clearzone[1][clearcount] = dy + (planet_height / 2);
	  clearzone[2][clearcount] = W_Textwidth * l->pl_namelen;
	  clearzone[3][clearcount] = W_Textheight;
	  clearcount++;
	}

      clearzone[0][clearcount] = dx - (planet_width / 2);
      clearzone[1][clearcount] = dy - (planet_height / 2);
      clearzone[2][clearcount] = planet_width;
      clearzone[3][clearcount] = planet_height;
      clearcount++;
    }
}


static void DrawShips(void) {
	register struct player *j;
	register struct phaser *php;

	char idbuf[10];
	const int view = SCALE * TWINSIDE / 2;
	int dx, dy, px, py, wx, wy, tx, ty, lx, ly;

#ifndef DYNAMIC_BITMAPS
	W_Icon(*ship_bits)[VIEWS];
#endif

	/*
	** Kludge to try to fix missing ID chars on tactical (short range) display. 
	*/
	idbuf[0] = '0';
	idbuf[1] = '\0';

	for (j=players + MAXPLAYER - 1; j >= players; --j) {
		int cloak_phases = CLOAK_PHASES * server_ups / 10;

		void *sprite = S_Ship(j->p_no);

		/* jmn - observer support.. tried to diplay tractors but no works */

		if (j->p_flags & PFCLOAK) {
			if (j->p_cloakphase < (cloak_phases-1)) {
#ifdef SOUND
				if (myPlayer(j) && (j->p_cloakphase == 0)) {
					Play_Sound(CLOAK_SOUND);
				}
#endif
				j->p_cloakphase++;
			}
		} else {
			if (j->p_cloakphase) {
#ifdef SOUND
				if (myPlayer(j)) {
					if (j->p_cloakphase == cloak_phases - 1) {
						Play_Sound(UNCLOAK_SOUND);
					}
# if !defined(sgi)
					else
						Abort_Sound(CLOAK_SOUND);
# endif
				}
#endif

				j->p_cloakphase--;
			}
		}

		dx = j->p_x - me->p_x;
		dy = j->p_y - me->p_y;

		if ((sprite == NULL) && (dx > view || dx < -view || dy > view || dy < -view))
			continue;

		dx = dx / SCALE + TWINSIDE / 2;
		dy = dy / SCALE + TWINSIDE / 2;

		if ((sprite == NULL) || (pixFlags & NO_CLK_PIX))
		  if (j->p_flags & PFCLOAK && (j->p_cloakphase == (cloak_phases - 1))) {
			if (myPlayer(j)
#ifdef RECORDGAME
					|| playback
#endif
			) {
				W_WriteBitmap(dx - (cloak_width / 2), dy - (cloak_height / 2), cloakicon, myColor);
#ifdef VARY_HULL
				clearzone[0][clearcount] = dx - (shield_width / 2 + 1);
				clearzone[1][clearcount] = dy - (shield_height / 2 + 1);
				clearzone[2][clearcount] = shield_width + 3;
				clearzone[3][clearcount] = shield_height + 3;
				clearcount++;
#else
				clearzone[0][clearcount] = dx - (shield_width / 2);
				clearzone[1][clearcount] = dy - (shield_height / 2);
				clearzone[2][clearcount] = shield_width;
				clearzone[3][clearcount] = shield_height;
				clearcount++;
#endif
				goto shieldlabel;	 /* draw the shield even when cloaked */
			}
			continue;
		}
		if (j->p_status == PALIVE && !(myPlayer(j) && (me->p_flags & PFOBSERV))) {
			/*
			 * not an observing me
			 */
#ifndef DYNAMIC_BITMAPS
			switch (j->p_team) {
			  case FED:
#ifdef TNG_FED_BITMAPS
				if (use_tng_fed_bitmaps)
					ship_bits = tng_fed_bitmaps;
				else	ship_bits = fed_bitmaps;
#else
				ship_bits = fed_bitmaps;
#endif
				break;

			  case ROM:
				ship_bits = rom_bitmaps;
				break;

			  case KLI:
				ship_bits = kli_bitmaps;
				break;

			  case ORI:
				ship_bits = ori_bitmaps;
				break;

			  default:
				ship_bits = ind_bitmaps;
				break;
			}
#endif

#if defined (VARY_HULL) || defined (BEEPLITE)
			clearzone[0][clearcount] = dx - (shield_width / 2 + 6);
			clearzone[1][clearcount] = dy - (shield_height / 2 + 6);
			clearzone[2][clearcount] = shield_width + 12;
			clearzone[3][clearcount] = shield_height + 12;
			clearcount++;
#else
			clearzone[0][clearcount] = dx - (shield_width / 2);
			clearzone[1][clearcount] = dy - (shield_height / 2);
			clearzone[2][clearcount] = shield_width;
			clearzone[3][clearcount] = shield_height;
			clearcount++;
#endif

			if (sprite != NULL) {
				clearsize = W_DrawSprite(sprite, dx, dy, TWINSIDE);
#if defined (VARY_HULL)
				if (clearsize > shield_width + 12
#else
				if (clearsize > shield_width
#endif
				) {
					clearcount--;
					clearzone[0][clearcount] = dx - (clearsize / 2);
					clearzone[1][clearcount] = dy - (clearsize / 2);
					clearzone[2][clearcount] = clearsize;
					clearzone[3][clearcount] = clearsize;
					clearcount++;
				}
			} else

			  if (j->p_team == ROM && j->p_ship.s_type == CRUISER && ROMVLVS)
				W_WriteBitmap(dx - (j->p_ship.s_width / 2),
				dy - (j->p_ship.s_height / 2), ROMVLVS_bitmap[rosette(j->p_dir)], playerColor(j));
			  else
				W_WriteBitmap(dx - (j->p_ship.s_width / 2), dy - (j->p_ship.s_height / 2),

#ifndef DYNAMIC_BITMAPS
			ship_bits[j->p_ship.s_type][rosette(j->p_dir)],
#else
			ship_bitmaps[PlayerBitmap(j)][rosette(j->p_dir)],
#endif
			playerColor(j));

			if (j->p_cloakphase > 0) {
				if (sprite == NULL)

				W_WriteBitmap(dx - (cloak_width / 2),
					      dy - (cloak_height / 2), cloakicon, playerColor(j));

				if (!myPlayer(j))		 /* if myplayer draw the shield */
					continue;
			}

	shieldlabel:

#ifdef BEEPLITE
			if ((UseLite && emph_player_seq_n[j->p_no] > 0) && (liteflag & LITE_PLAYERS_LOCAL)) {
				int seq_n = (emph_player_seq_n[j->p_no] * 10 / server_ups) % emph_player_seql_frames;

				W_WriteBitmap(dx - (emph_player_seql_width / 2),
					      dy - (emph_player_seql_height / 2),
					      emph_player_seql[seq_n], W_White);
			}
#endif

#ifdef VARY_HULL
			if ((myPlayer(j) || isObsLockPlayer(j)) && vary_hull) {
				int hull_left = (100 * (me->p_ship.s_maxdamage - me->p_damage)) / me->p_ship.s_maxdamage;
				int hull_num = 7;
				int hull_color;

				if (hull_left <= 16) {
					hull_num = 0;
					hull_color = W_Red;

				} else if (hull_left <= 28) {
					hull_num = 1;
					hull_color = W_Yellow;

				} else if (hull_left <= 40) {
					hull_num = 2;
					hull_color = W_Green;

				} else if (hull_left <= 52) {
					hull_num = 3;
					hull_color = W_Green;

				} else if (hull_left <= 64) {
					hull_num = 4;
					hull_color = W_Green;

				} else if (hull_left <= 76) {
					hull_num = 5;
					hull_color = W_White;
				} else if (hull_left <= 88) {
					hull_num = 6;
					hull_color = W_White;
				} else
					hull_color = playerColor(j);

				W_WriteBitmap(dx - (shield_width / 2 + 1),
					      dy - (shield_height / 2 + 1),
					      hull[hull_num], hull_color);
			}
#endif

#ifdef SOUND
			if (j->p_no == me->p_no) {
				if ((sound_flags & PFSHIELD) && !(j->p_flags & PFSHIELD))
					Play_Sound(SHIELD_DOWN_SOUND);

				else if (!(sound_flags & PFSHIELD) && (j->p_flags & PFSHIELD))
					Play_Sound(SHIELD_UP_SOUND);
			}
#endif

			/*
			 * It used to be that if "showShields" was false, shields were not
			 * shown.  COW has already stopped accepting showShields flags from
			 * the server, now stop using showShields altogether.
			 * James Soutter (Zork)     4 Jan 95
			 */

			if (j->p_flags & PFSHIELD) {
				int color = playerColor(j);
#ifdef VSHIELD_BITMAPS
				int shieldnum;

				if ((myPlayer(j) || isObsLockPlayer(j)) && VShieldBitmaps) {
					int value;

					shieldnum = SHIELD_FRAMES * me->p_shield / me->p_ship.s_maxshield;
					if (shieldnum >= SHIELD_FRAMES)
						shieldnum = SHIELD_FRAMES - 1;

					value = 100 * me->p_shield / me->p_ship.s_maxshield;
					if (value < 34)
						color = rColor;
					else if (value < 67)
						color = yColor;
					else	color = gColor;
				} else {
					color = playerColor(j);
					shieldnum = 2;
				}
#endif
				if (warnShields && (myPlayer(j) || isObsLockPlayer(j))) {
					switch (me->p_flags & (PFGREEN | PFYELLOW | PFRED)) {

					  case PFGREEN:
						color = gColor;
						break;

					  case PFYELLOW:
						color = yColor;
						break;

					  case PFRED:
						color = rColor;
						break;
					}
				}
#ifdef VSHIELD_BITMAPS
				W_WriteBitmap(dx - (shield_width / 2),
					      dy - (shield_height / 2), shield[shieldnum], color);
#else
				W_WriteBitmap(dx - (shield_width / 2),
					      dy - (shield_height / 2), shield, color);
#endif
			}

			/* Det circle */
			if (detCircle) {
				if (myPlayer(j)) {
					int dcr = DETDIST*2/SCALE;
					int dcx = dx - (dcr/2);
					int dcy = dy - (dcr/2);
					W_WriteCircle(w, dcy, dcy, dcr, W_Red);         
					clearzone[0][clearcount] = dcx;
					clearzone[1][clearcount] = dcy;
					clearzone[2][clearcount] = dcr + dcr;
					clearzone[3][clearcount] = dcr + dcr;
					clearcount++;
					detCircle--;
				}
			}
			if (j->p_flags & PFCLOAK)		 /* when cloaked stop here */
				continue;

		      {
			/* drawing of ship number */
			int color = playerColor(j);

			idbuf[0] = *(shipnos + j->p_no);

			if (myPlayer(j) || isObsLockPlayer(j)) {
				switch (me->p_flags & (PFGREEN | PFYELLOW | PFRED)) {

				  case PFGREEN:
					color = gColor;
					break;

				  case PFYELLOW:
					color = yColor;
					break;

				  case PFRED:
					color = rColor;
					break;
				}
			}
			W_MaskText(w, dx + (j->p_ship.s_width / 2),
				      dy - (j->p_ship.s_height / 2),
				      color, idbuf, 1, shipFont(j));

			clearzone[0][clearcount] = dx + (j->p_ship.s_width / 2);
			clearzone[1][clearcount] = dy - (j->p_ship.s_height / 2);
			clearzone[2][clearcount] = W_Textwidth;
			clearzone[3][clearcount] = W_Textheight;
			clearcount++;
		      }

		} else if (j->p_status == PEXPLODE) {
			int i = j->p_explode * 10 / server_ups;
#ifdef SOUND
			if (j->p_explode == 1) {
				if (j == me) {
#if defined(sgi)
					Engine_Sound(ENG_OFF);		/* stop engine sound */
#endif
					if (j->p_ship.s_type == STARBASE)
						Play_Sound(SBEXPLOSION_SOUND);
					else
						Play_Sound(EXPLOSION_SOUND);
				} else if (j->p_ship.s_type == STARBASE)
					Play_Sound(OTHER_SBEXPLOSION_SOUND);
				else
					Play_Sound(OTHER_EXPLOSION_SOUND);
			}
#endif

			if (sprite != NULL) {
				clearsize = W_DrawSprite(sprite, dx, dy, TWINSIDE);
				clearzone[0][clearcount] = dx - (clearsize / 2);
				clearzone[1][clearcount] = dy - (clearsize / 2);
				clearzone[2][clearcount] = clearsize;
				clearzone[3][clearcount] = clearsize;
				clearcount++;
			} else {

			if (i < EX_FRAMES || (i < SBEXPVIEWS && j->p_ship.s_type == STARBASE)) {
				if (j->p_ship.s_type == STARBASE) {
					W_WriteBitmap(dx - (sbexp_width / 2),
						      dy - (sbexp_height / 2), sbexpview[i], playerColor(j));

					clearzone[0][clearcount] = dx - (sbexp_width / 2);
					clearzone[1][clearcount] = dy - (sbexp_height / 2);
					clearzone[2][clearcount] = sbexp_width;
					clearzone[3][clearcount] = sbexp_height;
				} else {
					W_WriteBitmap(dx - (ex_width / 2),
						      dy - (ex_height / 2), expview[i], playerColor(j));
					clearzone[0][clearcount] = dx - (ex_width / 2);
					clearzone[1][clearcount] = dy - (ex_height / 2);
					clearzone[2][clearcount] = ex_width;
					clearzone[3][clearcount] = ex_height;
				}
				clearcount++;
			}
			j->p_explode++;
			}
		}

		/* Now draw his phaser (if it exists) */
		php = &phasers[j->p_no];

		if (php->ph_status != PHFREE) {
#ifdef SOUND
			if (php->sound_phaser) {
				Play_Sound(j == me ? PHASER_SOUND : OTHER_PHASER_SOUND);
				php->sound_phaser = 0;
			}
#endif
			if ((php->ph_updateFuse -= weaponUpdate) == 0) {	/* expire the phaser */
				php->ph_status = PHFREE;
#ifdef SOUND
				php->sound_phaser = 0;
#endif
			} else {
				if (php->ph_status == PHMISS) {
					/* Here I will have to compute the end coordinate */
					tx = PHASEDIST * j->p_ship.s_phaserdamage / 100 * Cos[php->ph_dir];
					ty = PHASEDIST * j->p_ship.s_phaserdamage / 100 * Sin[php->ph_dir];

					tx = (j->p_x + tx - me->p_x) / SCALE + TWINSIDE / 2;
					ty = (j->p_y + ty - me->p_y) / SCALE + TWINSIDE / 2;
					php->ph_fuse = 0;
				} else if (php->ph_status == PHHIT2) {
					tx = (php->ph_x - me->p_x) / SCALE + TWINSIDE / 2;
					ty = (php->ph_y - me->p_y) / SCALE + TWINSIDE / 2;
				} else {					/* start point is dx, dy */
					tx = (players[php->ph_target].p_x - me->p_x) / SCALE + TWINSIDE / 2;
					ty = (players[php->ph_target].p_y - me->p_y) / SCALE + TWINSIDE / 2;
				}

				/*
				 * Shrink the phasers if necessary: Measure length in 16ths 
				 * to make the maths a little easier for the computer (div
				 * 16 is a 4 bit shift). Add 8 to each sum to round properly.
				 */
				if (shrinkPhaserOnMiss || php->ph_status != PHMISS) {
					if ((myPlayer(j) || isObsLockPlayer(j))) {
						px = (dx * (16 - phaserShrink) + tx * phaserShrink + 8) / 16;
						py = (dy * (16 - phaserShrink) + ty * phaserShrink + 8) / 16;
					} else {
						px = (dx * (16 - theirPhaserShrink) + tx * theirPhaserShrink + 8) / 16;
						py = (dy * (16 - theirPhaserShrink) + ty * theirPhaserShrink + 8) / 16;
					}
				} else {
					px = dx;
					py = dy;
				}

				/* Now draw the phasers */

				if (friendlyPlayer(j)) {
					if (highlightFriendlyPhasers && (php->ph_status == PHHIT))
						W_CacheLine(w, px, py, tx, ty, foreColor);
					else {
						if ((php->ph_fuse % 2) == 1)
							W_CacheLine(w, px, py, tx, ty, foreColor);
						else
							W_CacheLine(w, px, py, tx, ty, phaserColor(php));
					}
					php->ph_fuse++;

					clearline[0][clearlcount] = px;
					clearline[1][clearlcount] = py;
					clearline[2][clearlcount] = tx;
					clearline[3][clearlcount] = ty;
					clearlcount++;
				} else {
					if ((enemyPhasers > 0) && (enemyPhasers <= 10)) {
						unsigned char dir;

						if (tx == px && ty == py)
							continue;

#ifdef SHORT_PACKETS
						if (php->ph_status != PHMISS) {		/* KOC 10/20/95  */
											/* hack for SP_2 */
#define XPI     3.1415926
							dir = (unsigned char) nint(atan2((double) (ty-py),
										         (double) (tx-px))/XPI * 128.0);
#undef XPI
						} else
#endif
						{
							dir = NORMALIZE(php->ph_dir + 64);
						}

						wx = px + enemyPhasers * Cos[dir];
						wy = py + enemyPhasers * Sin[dir];
						lx = px - enemyPhasers * Cos[dir];
						ly = py - enemyPhasers * Sin[dir];

						W_MakePhaserLine(w, wx, wy, tx, ty, shipCol[remap[j->p_team]]);
						W_MakePhaserLine(w, lx, ly, tx, ty, shipCol[remap[j->p_team]]);

						php->ph_fuse++;

						clearline[0][clearlcount] = wx;
						clearline[1][clearlcount] = wy;
						clearline[2][clearlcount] = tx;
						clearline[3][clearlcount] = ty;
						clearlcount++;

						clearline[0][clearlcount] = lx;
						clearline[1][clearlcount] = ly;
						clearline[2][clearlcount] = tx;
						clearline[3][clearlcount] = ty;
						clearlcount++;

					} else {
						W_MakePhaserLine(w, px, py, tx, ty, shipCol[remap[j->p_team]]);

						php->ph_fuse++;

						clearline[0][clearlcount] = px;
						clearline[1][clearlcount] = py;
						clearline[2][clearlcount] = tx;
						clearline[3][clearlcount] = ty;
						clearlcount++;
					}
				}
			}
		}

		/*
		 * ATM - show tractor/pressor beams (modified by James Collins)
		 * showTractorPressor is a variable set by xtrekrc.
		 */

		if (showTractorPressor) {
			if ((myPlayer(j) || (F_show_all_tractors)) && isAlive(me) &&
			     isAlive(j) && (j->p_flags & PFTRACT || j->p_flags & PFPRESS)) {
				double  theta;
				unsigned char dir;
				int lx[2], ly[2], target_width;

				struct player *tractee;

				if (j->p_tractor < 0 || j->p_tractor >= MAXPLAYER)
					continue;

				if (j->p_flags & PFOBSERV)
					continue;

				tractee = &players[j->p_tractor];

				if (tractee->p_status != PALIVE ||
				    ((tractee->p_flags & PFCLOAK) && (tractee->p_cloakphase == (cloak_phases-1))))
					continue;

				if (tcounter >= 2) {				/* continue tractor stuff */
					if (!continuetractor)
						tcounter--;

					px = (tractee->p_x - me->p_x) / SCALE + TWINSIDE / 2;
					py = (tractee->p_y - me->p_y) / SCALE + TWINSIDE / 2;
					if (px == dx && py == dy)
						continue;			/* this had better be last in for(;;) */
#define XPI     3.1415926
					theta = atan2((double) (px-dx), (double) (dy-py))+XPI / 2.0;
					dir = (unsigned char) nint(theta/XPI * 128.0);

					if (tractee->p_flags & PFSHIELD)
						target_width = shield_width;
					else	target_width = tractee->p_ship.s_width / 2;

					lx[0] = px + (Cos[dir] * (target_width / 2));
					ly[0] = py + (Sin[dir] * (target_width / 2));
					lx[1] = px - (Cos[dir] * (target_width / 2));
					ly[1] = py - (Sin[dir] * (target_width / 2));
#undef XPI
					if (j->p_flags & PFPRESS) {
						W_MakeTractLine(w, dx, dy, lx[0], ly[0], W_Yellow);
						W_MakeTractLine(w, dx, dy, lx[1], ly[1], W_Yellow);
					} else {
						W_MakeTractLine(w, dx, dy, lx[0], ly[0], W_Green);
						W_MakeTractLine(w, dx, dy, lx[1], ly[1], W_Green);
					}
#ifdef WIN32
					/*
					 * Fixup for minor inconsistencies between SAC's interger
					 * linedraw and Win32 LineTo()
					 */
					tpline = clearlcount;
#endif
					clearline[0][clearlcount] = dx;
					clearline[1][clearlcount] = dy;
					clearline[2][clearlcount] = lx[0];
					clearline[3][clearlcount] = ly[0];
					clearlcount++;
					clearline[0][clearlcount] = dx;
					clearline[1][clearlcount] = dy;
					clearline[2][clearlcount] = lx[1];
					clearline[3][clearlcount] = ly[1];
					clearlcount++;
				}

			} else if (!(me->p_flags & PFPRESS || me->p_flags & PFTRACT))
				tcounter = 2;
		}
	}
	return;
}

static void DrawTorps(void) {
	register struct torp *k, *t;
	register int dx, dy;
	int numdetframes, frame;

	register int tno, tsub;
	void   *sprite;
	struct player *j;

	int torpCount;
	const int view = SCALE * TWINSIDE / 2;

	for (t=torps, j=players, tno=0; j != players + MAXPLAYER; t += MAXTORP, ++j, tno += MAXTORP)
	{

#ifdef SOUND
		if (j != me)
			num_other_torps += j->p_ntorp;
#endif
		torpCount = j->p_ntorp;

		for (tsub=0, k=t; torpCount > 0; ++k, ++tsub)
		{
			/*
			 * Work until all the torps for a given player have been examined.
			 * In the current INL server torps are allocated from low to high
			 * so this loop must work so that k is incrimented rather than
			 * decrimented.
			 */

			if (!k->t_status)
				continue;

			--torpCount;

			/*
			 * Age a torp only if some weapon has been updated
			 * (eg this is not a pause).
			 */

			if ((k->t_updateFuse -= weaponUpdate) == 0) {
				if (k->t_status != TEXPLODE) {		/* expire the torp */
					k->t_status = TFREE;
					j->p_ntorp--;
					continue;
				}
				k->t_updateFuse = 100;			/* leave the torp to explode on its own */
			}

			dx = k->t_x - me->p_x;
			dy = k->t_y - me->p_y;

			if (dx > view || dx < -view || dy > view || dy < -view) {
				/*
				 * Call any torps off screen "free" (if owned by other)
				 */
				if (k->t_status == TEXPLODE && j != me) {
					k->t_status = TFREE;
					j->p_ntorp--;
				}
				continue;
			}

			dx = dx / SCALE + TWINSIDE / 2;
			dy = dy / SCALE + TWINSIDE / 2;
			if ((sprite = S_Torp(tno + tsub)) != NULL) {
				clearsize = W_DrawSprite(sprite, dx, dy, TWINSIDE);
				clearzone[0][clearcount] = dx - (clearsize / 2);
				clearzone[1][clearcount] = dy - (clearsize / 2);
				clearzone[2][clearcount] = clearsize;
				clearzone[3][clearcount] = clearsize;
				clearcount++;
			} else
			if (k->t_status == TEXPLODE) {
				k->t_fuse--;

				numdetframes = NUMDETFRAMES * server_ups / 10;
				frame = k->t_fuse * 10 / server_ups;

				if (frame > NUMDETFRAMES - 1)
					frame = NUMDETFRAMES - 1;

				if (k->t_fuse <= 0) {
					k->t_status = TFREE;
					j->p_ntorp--;
					continue;
				}

				if (k->t_fuse >= numdetframes)
					k->t_fuse = numdetframes-1;
#ifdef SOUND
				if (k->t_fuse == numdetframes-1)
					Play_Sound(TORP_HIT_SOUND);
#endif

				W_WriteBitmap(dx - (cloud_width / 2),
					      dy - (cloud_height / 2), cloud[frame], torpColor(k));

				clearzone[0][clearcount] = dx - (cloud_width / 2);
				clearzone[1][clearcount] = dy - (cloud_height / 2);
				clearzone[2][clearcount] = cloud_width;
				clearzone[3][clearcount] = cloud_height;
				clearcount++;
			} else if (j != me && ((k->t_war & me->p_team) ||
			          (j->p_team & (me->p_hostile | me->p_swar)))) {

				/*
				 * Solid.  Looks strange.
				   W_FillArea(w, dx-(etorp_width/2),
						 dy-(etorp_height/2), etorp_width, etorp_height, torpColor(k));
				 */

				/* XFIX */
				W_CacheLine(w, dx-(etorp_width/2), dy-(etorp_height/2),
					       dx+(etorp_width/2), dy+(etorp_height/2), torpColor(k));
				W_CacheLine(w, dx+(etorp_width/2), dy-(etorp_height/2),
					       dx-(etorp_width/2), dy+(etorp_height/2), torpColor(k));

				/* W_WriteBitmap(dx-(etorp_width/2), dy-(etorp_height/2), etorp, torpColor(k)); */
				clearzone[0][clearcount] = dx - (etorp_width / 2);
				clearzone[1][clearcount] = dy - (etorp_height / 2);
				clearzone[2][clearcount] = etorp_width;
				clearzone[3][clearcount] = etorp_height;
				clearcount++;
			} else {
				W_CacheLine(w, dx-(mtorp_width/2), dy,
					       dx+(mtorp_width/2), dy, torpColor(k));
				W_CacheLine(w, dx, dy-(mtorp_width/2), dx,
					       dy+(mtorp_width/2), torpColor(k));

				/* W_WriteBitmap(dx - (mtorp_width/2), dy - (mtorp_height/2), mtorp, torpColor(k)); */
				clearzone[0][clearcount] = dx - (mtorp_width / 2);
				clearzone[1][clearcount] = dy - (mtorp_height / 2);
				clearzone[2][clearcount] = mtorp_width;
				clearzone[3][clearcount] = mtorp_height;
				clearcount++;
			}
		}
	}
	return;
}


void DrawPlasmaTorps(void) {
	register struct plasmatorp *pt;
	register int dx, dy;
	int numdetframes, frame;

	register int ptno;
	void *sprite;
	const int view = SCALE * TWINSIDE / 2;

	/*
	 * MAXPLASMA is small so work through all the plasmas rather than
	 * look at the number of outstanding plasma torps for each player.
	 */

	for (pt=plasmatorps+(MAXPLASMA * MAXPLAYER)-1, ptno=(MAXPLASMA * MAXPLAYER)-1;
	     pt >= plasmatorps;
	     --pt, --ptno)
	{
		if (!pt->pt_status)
			continue;
#ifdef SOUND
		if (pt->pt_owner != me->p_no)
			num_other_plasma++;
#endif
		if ((pt->pt_updateFuse -= weaponUpdate) == 0) {
			if (pt->pt_status != PTEXPLODE) {		/* expire the plasma torpedo */
				pt->pt_status = PTFREE;
				players[pt->pt_owner].p_nplasmatorp--;
				continue;
			}
			pt->pt_updateFuse = 100;			/* leave the torp to explode on its own */
		}

		dx = pt->pt_x - me->p_x;
		dy = pt->pt_y - me->p_y;

		if (dx > view || dx < -view || dy > view || dy < -view)
			continue;

		dx = dx / SCALE + TWINSIDE / 2;
		dy = dy / SCALE + TWINSIDE / 2;

		if ((sprite = S_Plasma(ptno)) != NULL) {
			clearsize = W_DrawSprite(sprite, dx, dy, TWINSIDE);
			clearzone[0][clearcount] = dx - (clearsize / 2);
			clearzone[1][clearcount] = dy - (clearsize / 2);
			clearzone[2][clearcount] = clearsize;
			clearzone[3][clearcount] = clearsize;
			clearcount++;
		} else
		if (pt->pt_status == PTEXPLODE) {
			pt->pt_fuse--;

			numdetframes = NUMDETFRAMES * server_ups / 10;
			frame = pt->pt_fuse * 10 / server_ups;

			if (frame > NUMDETFRAMES-1)
				frame = NUMDETFRAMES-1;

			if (pt->pt_fuse <= 0) {
				pt->pt_status = PTFREE;
				players[pt->pt_owner].p_nplasmatorp--;
				continue;
			}

			if (pt->pt_fuse >= numdetframes)
				pt->pt_fuse = numdetframes-1;
#ifdef SOUND
			if (pt->pt_fuse == numdetframes-1)
				Play_Sound(PLASMA_HIT_SOUND);
#endif

			W_WriteBitmap(dx - (plasmacloud_width / 2),
				      dy - (plasmacloud_height / 2), plasmacloud[frame], plasmatorpColor(pt));
			clearzone[0][clearcount] = dx - (plasmacloud_width / 2);
			clearzone[1][clearcount] = dy - (plasmacloud_height / 2);
			clearzone[2][clearcount] = plasmacloud_width;
			clearzone[3][clearcount] = plasmacloud_height;
			clearcount++;
		}

		/*
		 * needmore: if (pt->pt_war & me->p_team)
		 */
		else if (pt->pt_owner != me->p_no && ((pt->pt_war & me->p_team) ||
			(players[pt->pt_owner].p_team & (me->p_hostile | me->p_swar)))) {

			W_WriteBitmap(dx - (eplasmatorp_width / 2),
				      dy - (eplasmatorp_height / 2),
				      eplasmatorp, plasmatorpColor(pt));
			clearzone[0][clearcount] = dx - (eplasmatorp_width / 2);
			clearzone[1][clearcount] = dy - (eplasmatorp_height / 2);
			clearzone[2][clearcount] = eplasmatorp_width;
			clearzone[3][clearcount] = eplasmatorp_height;
			clearcount++;
		} else {
			W_WriteBitmap(dx - (mplasmatorp_width / 2),
				      dy - (mplasmatorp_height / 2),
				      mplasmatorp, plasmatorpColor(pt));
			clearzone[0][clearcount] = dx - (mplasmatorp_width / 2);
			clearzone[1][clearcount] = dy - (mplasmatorp_height / 2);
			clearzone[2][clearcount] = mplasmatorp_width;
			clearzone[3][clearcount] = mplasmatorp_height;
			clearcount++;
		}
	}
	return;
}

/* draws hockey lines, galaxy edges, tactical border alert colour, and
   lock icon */
static void DrawMisc(void) {
	register struct player *j;
	register int dx, dy;
	const int view = SCALE * TWINSIDE / 2;

#ifdef HOCKEY_LINES
	register struct s_line *sl;
	const int HALF_WINSIDE = TWINSIDE / 2;
	int ex, ey, sx, sy;
#endif


#ifdef HOCKEY_LINES
  if (hockey_s_lines && 1)
    for (sl = s_lines + NUM_HOCKEY_LINES - 1; sl >= s_lines; --sl)
      {
	/* Treat the line differently based on the orientation */
	if (sl->orientation == S_LINE_VERTICAL)
	  {
	    if (((sx = (sl->begin_x - me->p_x) / SCALE) < HALF_WINSIDE)
		&& (sx > -HALF_WINSIDE))
	      {
		sx += HALF_WINSIDE;
		ex = sx;
		if ((sy = HALF_WINSIDE - (me->p_y - sl->begin_y) / SCALE) < 0)
		  sy = 0;
		if (sy > (TWINSIDE - 1))
		  sy = TWINSIDE - 1;
		if ((ey = HALF_WINSIDE - (me->p_y - sl->end_y) / SCALE) < 0)
		  ey = 0;
		if (ey > (TWINSIDE - 1))
		  ey = TWINSIDE - 1;
		if (sy == ey)
		  continue;
	      }
	    else
	      continue;
	  }

	else if (sl->orientation == S_LINE_HORIZONTAL)
	  {
	    if (((sy = (sl->begin_y - me->p_y) / SCALE) < HALF_WINSIDE)
		&& (sy > -HALF_WINSIDE))
	      {
		sy += HALF_WINSIDE;
		ey = sy;
		if ((sx = HALF_WINSIDE - (me->p_x - sl->begin_x) / SCALE) < 0)
		  sx = 0;
		if (sx > (TWINSIDE - 1))
		  sx = TWINSIDE - 1;
		if ((ex = HALF_WINSIDE - (me->p_x - sl->end_x) / SCALE) < 0)
		  ex = 0;
		if (ex > (TWINSIDE - 1))
		  ex = TWINSIDE - 1;
		if (sx == ex)
		  continue;
	      }
	    else
	      continue;
	  }
	else
	  continue;

	W_CacheLine(w, sx, sy, ex, ey, sl->color);
	clearline[0][clearlcount] = sx;
	clearline[1][clearlcount] = sy;
	clearline[2][clearlcount] = ex;
	clearline[3][clearlcount] = ey;
	clearlcount++;
      }						 /* End for Hockey Lines * *
						  * * Ends the if, too */
#endif /* HOCKEY_LINES */

  /* Draw Edges */
  if (me->p_x < (TWINSIDE / 2) * SCALE)
    {
      dx = (TWINSIDE / 2) - (me->p_x) / SCALE;
      sy = (TWINSIDE / 2) + (0 - me->p_y) / SCALE;
      ey = (TWINSIDE / 2) + (GWIDTH - me->p_y) / SCALE;
      if (sy < 0)
	sy = 0;
      if (ey > TWINSIDE - 1)
	ey = TWINSIDE - 1;
      /* XFIX */
      W_CacheLine(w, dx, sy, dx, ey, warningColor);
      /* W_MakeLine(w, dx, sy, dx, ey, warningColor); */
      clearline[0][clearlcount] = dx;
      clearline[1][clearlcount] = sy;
      clearline[2][clearlcount] = dx;
      clearline[3][clearlcount] = ey;
      clearlcount++;
    }

  if ((GWIDTH - me->p_x) < (TWINSIDE / 2) * SCALE)
    {
      dx = (TWINSIDE / 2) + (GWIDTH - me->p_x) / SCALE;
      sy = (TWINSIDE / 2) + (0 - me->p_y) / SCALE;
      ey = (TWINSIDE / 2) + (GWIDTH - me->p_y) / SCALE;
      if (sy < 0)
	sy = 0;
      if (ey > TWINSIDE - 1)
	ey = TWINSIDE - 1;
      /* XFIX */
      W_CacheLine(w, dx, sy, dx, ey, warningColor);
      /* W_MakeLine(w, dx, sy, dx, ey, warningColor); */
      clearline[0][clearlcount] = dx;
      clearline[1][clearlcount] = sy;
      clearline[2][clearlcount] = dx;
      clearline[3][clearlcount] = ey;
      clearlcount++;
    }

  if (me->p_y < (TWINSIDE / 2) * SCALE)
    {
      dy = (TWINSIDE / 2) - (me->p_y) / SCALE;
      sx = (TWINSIDE / 2) + (0 - me->p_x) / SCALE;
      ex = (TWINSIDE / 2) + (GWIDTH - me->p_x) / SCALE;
      if (sx < 0)
	sx = 0;
      if (ex > TWINSIDE - 1)
	ex = TWINSIDE - 1;
      /* XFIX */
      W_CacheLine(w, sx, dy, ex, dy, warningColor);
      /* W_MakeLine(w, sx, dy, ex, dy, warningColor); */
      clearline[0][clearlcount] = sx;
      clearline[1][clearlcount] = dy;
      clearline[2][clearlcount] = ex;
      clearline[3][clearlcount] = dy;
      clearlcount++;
    }

  if ((GWIDTH - me->p_y) < (TWINSIDE / 2) * SCALE)
    {
      dy = (TWINSIDE / 2) + (GWIDTH - me->p_y) / SCALE;
      sx = (TWINSIDE / 2) + (0 - me->p_x) / SCALE;
      ex = (TWINSIDE / 2) + (GWIDTH - me->p_x) / SCALE;
      if (sx < 0)
	sx = 0;
      if (ex > TWINSIDE - 1)
	ex = TWINSIDE - 1;
      /* XFIX */
      W_CacheLine(w, sx, dy, ex, dy, warningColor);
      /* W_MakeLine(w, sx, dy, ex, dy, warningColor); */
      clearline[0][clearlcount] = sx;
      clearline[1][clearlcount] = dy;
      clearline[2][clearlcount] = ex;
      clearline[3][clearlcount] = dy;
      clearlcount++;
    }


  /* Change border color to signify alert status */
  if (oldalert != (me->p_flags & (PFGREEN | PFYELLOW | PFRED)))
    {
      oldalert = (me->p_flags & (PFGREEN | PFYELLOW | PFRED));
      switch (oldalert)
	{
	case PFGREEN:
	  if (extraBorder)
	    W_ChangeBorder(w, gColor);
	  W_ChangeBorder(baseWin, gColor);
	  W_ChangeBorder(iconWin, gColor);

#if defined(SOUND) && !defined(sgi)
	  Abort_Sound(REDALERT_SOUND);
#endif
	  break;

	case PFYELLOW:
	  if (extraBorder)
	    W_ChangeBorder(w, yColor);
	  W_ChangeBorder(baseWin, yColor);
	  W_ChangeBorder(iconWin, yColor);

#if defined(SOUND) && !defined(sgi)
	  Abort_Sound(REDALERT_SOUND);
#endif
	  break;

	case PFRED:
	  if (extraBorder)
	    W_ChangeBorder(w, rColor);
	  W_ChangeBorder(baseWin, rColor);
	  W_ChangeBorder(iconWin, rColor);

#ifdef SOUND
	  Play_Sound(REDALERT_SOUND);
#endif
	  break;
	}
    }

#ifdef SOUND
	if (sound_torps < me->p_ntorp)
		Play_Sound(FIRE_TORP_SOUND);
	if (sound_other_torps < num_other_torps)
		Play_Sound(OTHER_FIRE_TORP_SOUND);

	if (sound_plasma < me->p_nplasmatorp)
		Play_Sound(FIRE_PLASMA_SOUND);
	if (sound_other_plasma < num_other_plasma)
		Play_Sound(OTHER_FIRE_PLASMA_SOUND);

	sound_flags = me->p_flags;
	sound_torps = me->p_ntorp;
	sound_other_torps = num_other_torps;
	num_other_torps = 0;

	sound_plasma = me->p_nplasmatorp;
	sound_other_plasma = num_other_plasma;
	num_other_plasma = 0;
#endif

  /* show 'lock' icon on local map (Actually an EM hack ) */
  if ((showLock & 2) && !(me->p_flags & PFOBSERV))
    {
      int     tri_x = -1, tri_y = -1, facing = 0;
      int     tri_size = 4;

      if (me->p_flags & PFPLOCK)
	{
	  /* locked onto a ship */
	  j = &players[me->p_playerl];
	  if (!(j->p_flags & PFCLOAK))
	    {
	      dx = j->p_x - me->p_x;
	      dy = j->p_y - me->p_y;
	      if (ABS(dx) < view && ABS(dy) < view)
		{
		  dx = dx / SCALE + TWINSIDE / 2;
		  dy = dy / SCALE + TWINSIDE / 2;
		  tri_x = dx + 0;
		  tri_y = dy + 20;		 /* below ship */
		  facing = 1;
		}
	      /* printf("Drawing local triangle at %d %d\n", tri_x, tri_y); */
	    }
	}
      else if (me->p_flags & PFPLLOCK)
	{
	  /* locked onto a planet */
	  struct planet *l = &planets[me->p_planet];

	  dx = l->pl_x - me->p_x;
	  dy = l->pl_y - me->p_y;
	  if (ABS(dx) < view && ABS(dy) < view)
	    {
	      dx = dx / SCALE + TWINSIDE / 2;
	      dy = dy / SCALE + TWINSIDE / 2;
	      tri_x = dx;
	      tri_y = dy - 20;			 /* below planet */
	      facing = 0;
	    }
	  /* printf("Drawing local triangle at %d %d\n", tri_x, tri_y); */
	}
      if (tri_x != -1)
	{
	  W_WriteTriangle(w, tri_x, tri_y, 4, facing, foreColor);
	  clearzone[0][clearcount] = tri_x - tri_size - 1;
	  clearzone[1][clearcount] = tri_y - 1 +
	      (facing ? 0 : -tri_size);
	  clearzone[2][clearcount] = tri_size * 2 + 2;
	  clearzone[3][clearcount] = tri_size + 2;
	  clearcount++;
	}
    }
}


inline void local(void)
/*
 * Draw out the 'tactical' map
 */
{
  if (me->p_x < 0)
    return;

  DrawPlanets();
  DrawShips();
  DrawTorps();
  DrawPlasmaTorps();

  weaponUpdate = 0;
  DrawMisc();
}


inline void clearLocal(void)
/*
 * Clear the local map (intelligently rather than just simply wiping
 * the map).
 */
{
  if (W_FastClear)
    {
      W_ClearWindow(w);
      clearcount = 0;
      clearlcount = 0;
    }
  else
    {

#ifndef WIN32
      while (clearcount)
	{
	  clearcount--;
	  /* XFIX */
	  W_CacheClearArea(w, clearzone[0][clearcount],
			 clearzone[1][clearcount], clearzone[2][clearcount],
			   clearzone[3][clearcount]);

	  /* W_ClearArea(w, clearzone[0][clearcount], * *
	   * clearzone[1][clearcount], clearzone[2][clearcount], * *
	   * clearzone[3][clearcount]); */
	}
      while (clearlcount)
	{
	  clearlcount--;
	  /* XFIX */
	  W_CacheLine(w, clearline[0][clearlcount], clearline[1][clearlcount],
		      clearline[2][clearlcount], clearline[3][clearlcount],
		      backColor);
	  /* W_MakeLine(w, clearline[0][clearlcount], * *
	   * clearline[1][clearlcount], clearline[2][clearlcount], * *
	   * clearline[3][clearlcount], backColor); */
	}
      /* XFIX */
      W_FlushClearAreaCache(w);
      W_FlushLineCaches(w);
#else
      // Much more efficient way of clearing -- X programmers take note!
      W_ClearAreas(w, clearzone[0], clearzone[1], clearzone[2], clearzone[3], clearcount);
      clearcount = 0;
      if (tpline != -1)
	{
	  /* Fixup for SAC's integer linedraw */
	  W_MakeTractLine(w, clearline[0][tpline], clearline[1][tpline],
		     clearline[2][tpline], clearline[3][tpline], backColor);
	  W_MakeTractLine(w, clearline[0][tpline + 1], clearline[1][tpline + 1],
	     clearline[2][tpline + 1], clearline[3][tpline + 1], backColor);
	  tpline = -1;
	}
      W_MakeLines(w, clearline[0], clearline[1], clearline[2], clearline[3], clearlcount, backColor);
      clearlcount = 0;
#endif
    }
}
