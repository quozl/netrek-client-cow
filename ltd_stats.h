/*
 *
 * Dave Ahn
 *
 * Leonard/Tom/Dave (LTD) Extended statistics.  For detailed info, read
 * the README.LTD file.
 *
 * For inline optimization details, read the README.LTD and
 * ltd_stats.c files.
 *
 * struct ltd_stats is a per-ship statistics group for each player.
 * This means that the player DB will increase by several factors in
 * size at the benefit of detailed stats that can be used to analyze
 * individual play for clued games.
 *
 */

#ifndef __INCLUDED_ltd_stats_h__
#define __INCLUDED_ltd_stats_h__

#include "defs.h"	/* We need defines from this file */

typedef enum {

  LTD_TZONE_0		= 0,	/* Zone 0 = backfield */
  LTD_TZONE_1		= 1,	/* Zone 1 = core + 2 open side */
  LTD_TZONE_2		= 2,	/* Zone 2 = front line */
  LTD_TZONE_3		= 3,	/* Zone 3 = enemy front line */
  LTD_TZONE_4		= 4,	/* Zone 4 = enemy core + 2 open side */
  LTD_TZONE_5		= 5,	/* Zone 5 = enemy backfield */
  LTD_TZONE_6		= 6,	/* Zone 6 = 3rd space */
  LTD_TZONE_7		= 7,	/* Zone 7 = unknown/diag */
  LTD_NUM_TZONES	= 8	/* number of zones */

} LTD_TZONE_T;

/* XXNOTE: find a better way to do the inlining
   */

#ifdef LTD_INLINE
#define __LTD_INLINE inline static
#else
#define __LTD_INLINE
#endif

/* for extende stats, one struct for each ship except GA and AT, which
   share the same slot */

typedef enum {

  LTD_TOTAL	= 0,
  LTD_SC	= 1,
  LTD_DD	= 2,
  LTD_CA	= 3,
  LTD_BB	= 4,
  LTD_AS	= 5,
  LTD_SB	= 6,
  LTD_GA	= 7,
  LTD_AT	= 7,
  LTD_NUM_SHIPS	= 8

} LTD_SHIP_T;


#ifdef LTD_PER_RACE

typedef enum {

  LTD_FED	= 0,
  LTD_ROM	= 1,
  LTD_KLI	= 2,
  LTD_ORI	= 3,
  LTD_NOBODY	= 4,
  LTD_NUM_RACES	= 5

} LTD_RACE_T;

#else

typedef enum {

  LTD_FED	= 0,
  LTD_ROM	= 0,
  LTD_KLI	= 0,
  LTD_ORI	= 0,
  LTD_NOBODY	= 0,
  LTD_NUM_RACES	= 1

} LTD_RACE_T;

#endif /* LTD_PER_RACE */


/* LTD stats structure */

struct ltd_stats {

  /* How many times have I killed in this ship */

  struct _kills {

    unsigned int total;			/* total number of kills ever */
    double max;				/* max kills ever achieved */
    unsigned int first;			/* number of first kills made */
    unsigned int first_potential;	/* first kills that could be
                                           converted to carries */
    unsigned int first_converted;	/* first kills that actually were
                                           converted to carries */
    unsigned int second;		/* number of second kills made */
    unsigned int second_potential;	/* second kills that could be
                                           converted to carries */
    unsigned int second_converted;	/* second kills that actually were
                                           converted to carries */
    unsigned int phasered;		/* number of kills made with a
                                           phaser death blow */
    unsigned int torped;		/* number of kills made with a
                                           torp blow */
    unsigned int plasmaed;		/* number of kills made with a
                                           plamsa */
#if defined(_64BIT) && defined(linux)
  } __attribute__((packed)) kills;
#else
 } kills;
#endif

  /* How many times have I died in this ship */

  struct _deaths {

    unsigned int total;			/* total number of deaths ever */
    unsigned int potential;		/* number of times died as a
                                           potential carrier */
    unsigned int converted;		/* number of times died as a
                                           a converted carrier */
    unsigned int dooshed;		/* number of times died while ++ */
    unsigned int phasered;		/* number of times died from phaser */
    unsigned int torped;		/* number of times died from torp */
    unsigned int plasmaed;		/* number of times died from plasma */
    unsigned int acc;			/* number of times you died, and
                                           someone picked up armies with
                                           the kill */

  } deaths;


  /* How many planets have I destroyed or taken */

  struct _planets {

    unsigned int taken;			/* number of planets taken */
    unsigned int destroyed;		/* number of planets destroyed */

  } planets;


  /* How many enemy armies have you bombed */

  struct _bomb {

    unsigned int planets;		/* number of planets bombed */
    unsigned int planets_8;		/* number of planets with <= 8
                                           armies bombed */
    unsigned int planets_core;		/* number of core planets bombed */
    unsigned int armies;		/* total number of armies bombed */
    unsigned int armies_8;		/* armies bombed where the planet
                                           being bombed had <= 8 armies */
    unsigned int armies_core;		/* armies bombed in a core planet */

  } bomb;


  /* How many enemy carriers have you killed and armies ogged */

  struct _ogged {

    unsigned int armies;		/* number of enemy armies ogged */
    unsigned int dooshed;		/* number of dooshed real carriers */
    unsigned int converted;		/* number of converted enemy
                                           carriers */
    unsigned int potential;		/* number of potential enemy
                                           carriers */
    unsigned int bigger_ship;		/* number of times a real carrier
                                           was in a bigger ship */
    unsigned int same_ship;		/* number of times a real carrier
                                           was in the same sized ship */
    unsigned int smaller_ship;		/* number of times a real carrier
                                           was in the same sized ship */
    unsigned int sb_armies;		/* number of armies on enemy SB
                                           ogged */
    unsigned int friendly;		/* number of friendly carriers
                                           you killed */
    unsigned int friendly_armies;	/* number of friendly armies you
                                           killed due to blatant stupidity */

  } ogged;


  /* How many friendly armies have you carried */

  struct _armies {

    unsigned int total;			/* number of friendly armies carried */
    unsigned int attack;		/* armies droped on enemy planets */
    unsigned int reinforce;		/* armies used to reinforce friendly
                                           planet < 4 */
    unsigned int ferries;		/* armies ferried to friendly planet
                                           >= 4 or SB */
    unsigned int killed;		/* armies killed */

  } armies;


  /* How many carries have you made */

  struct _carries {

    unsigned int total;			/* number of carries you attempted */
    unsigned int partial;		/* number of times you delivered
                                           at least 1 army before dying ++ */
    unsigned int completed;		/* number of times you completed
                                           your carry by delivering all
                                           the armies */
    unsigned int attack;		/* number of times you dropped on
                                           a neutral or enemy planet */
    unsigned int reinforce;		/* number of times you reinforced
                                           a friendly planet < 4 */
    unsigned int ferries;		/* number of times you ferried to
                                           a friendly planet >= 4 or SB */

  } carries;


  /* How much time we spent in relevant roles, modes or space */

  struct _ticks {

    unsigned int total;			/* total = green + yellow + red */
    /* unsigned int green; */		/* time spent in green alert */
    unsigned int yellow;		/* time spent in yellow alert */
    unsigned int red;			/* time spent in red alert */

    unsigned int zone[LTD_NUM_TZONES];	/* time spent in a particular zone */

    unsigned int potential;		/* time spent as potential carrier */
    unsigned int carrier;		/* time spent as carrier */
    unsigned int repair;		/* time spent in repair mode */

  } ticks;

  unsigned int damage_repaired;		/* total damage repaired */


  /* Weapon stats */

  struct _weapons {

    /* phasers */

    struct _phaser {

      unsigned int fired;		/* number of times the weapon was
                                           fired */
      unsigned int hit;			/* number of times the weapon hit */

      struct _damage {

        unsigned int inflicted;		/* damage inflicted with this weapon */
        unsigned int taken;		/* damage taken from this weapon */

      } damage;

    } phaser;

    /* torps */

    struct _torp {

      unsigned int fired;		/* number of torps that were fired */
      unsigned int hit;			/* number of torps hit */
      unsigned int detted;		/* number of torps that were detted
                                           by enemy */
      unsigned int selfdetted;		/* number of torps self detted */
      unsigned int wall;		/* number of torps that hit the wall */

      struct _damage damage;		/* damage inflicted/taken */

    } torps;


    /* plasma */

    struct _plasma {

      unsigned int fired;		/* number of plasmas that were fired */
      unsigned int hit;			/* number that hit */
      unsigned int phasered;		/* number that were phasered */
      unsigned int wall;		/* number that hit the wall */

      struct _damage damage;		/* damage inflicted/taken */

    } plasma;

    /* damage repaired */

  } weapons;

#if defined(_64BIT) && defined(linux)
} __attribute__((packed));
#else
};
#endif

/* LTD history structure - this is needed to calculate the LTD stats
   every tick. */

enum {
  LTD_NUM_HIST = 5		/* keep track of up to 5 events */
};

struct ltd_history {

  /* kill history */

  struct _kill {

    unsigned int tick;		/* tick kill was acquired */
    char potential;		/* kill was retained long enough to
                                   become potential or converted
                                   carrier */
    struct player *victim;	/* the player that was killed */
    float before;		/* kills i had before this one, perhaps
                                   from bombing, etc */
    int ship;			/* ship type of the victim */

  } kills[LTD_NUM_HIST];

  int num_kills;		/* number of last_kills */

  char kill_potential;		/* i am a potential carrier
                                   0 = no
                                   1 = potential 1st kill
                                   2 = potential 2nd kill
                                   3 = converted 1st kill
                                   4 = converted 2nd kill */

  /* bombing history */

  unsigned int last_bombed_tick;/* tick i bombed the last planet */
  short last_bombed_planet;	/* last planet that i bombed */

  /* army/carries history */

  short last_beamup_planet;
  short last_beamdown_planet;	/* last planet i beamed up/down armies from
                                   value is planet number
                                   -1 = SB
                                   -2 = no planet */

  int enemy_team;		/* the enemy team, for ltd_zone */

};


/* forward declarations of structs for function prototypes */

struct player;
struct stats;
struct planet;

/* function prototypes for non-inlined functions */

void ltd_reset(struct player *);
void ltd_reset_struct(struct ltd_stats (*) [LTD_NUM_SHIPS]);
void ltd_reset_hist(struct player *);
int ltd_can_rank(struct player *);
void ltd_update_totals(struct player *);

/* function prototypes for inlined functions */

__LTD_INLINE float ltd_total_rating(struct player *);
__LTD_INLINE float ltd_bombing_rating(struct player *);
__LTD_INLINE float ltd_planet_rating(struct player *);
__LTD_INLINE float ltd_defense_rating(struct player *);
__LTD_INLINE float ltd_offense_rating(struct player *);

__LTD_INLINE int ltd_kills(struct player *, const LTD_SHIP_T);
__LTD_INLINE int ltd_deaths(struct player *, const LTD_SHIP_T);
__LTD_INLINE int ltd_armies_bombed(struct player *, const LTD_SHIP_T);
__LTD_INLINE int ltd_planets_taken(struct player *, const LTD_SHIP_T);
__LTD_INLINE int ltd_ticks(struct player *, const LTD_SHIP_T);
__LTD_INLINE int ltd_kills_max(struct player *, const LTD_SHIP_T);

__LTD_INLINE void ltd_update_ticks(struct player *);
__LTD_INLINE void ltd_update_kills(struct player *credit_killer,
                                   struct player *actual_killer,
                                   struct player *victim);
__LTD_INLINE void ltd_update_kills_max(struct player *);
__LTD_INLINE void ltd_update_deaths(struct player *victim,
                                    struct player *actual_killer);
__LTD_INLINE void ltd_update_bomb(struct player *bomber,
                                  struct planet *planet_bombed,
                                  int armies_bombed);
__LTD_INLINE void ltd_update_planets(struct player *taker,
                                     struct planet *planet);
__LTD_INLINE void ltd_update_armies(struct player *carrier,
                                    struct planet *planet);
__LTD_INLINE void ltd_update_armies_carried(struct player *carrier,
                                            struct player *sb);
__LTD_INLINE void ltd_update_armies_ferried(struct player *carrier,
                                            struct player *sb);
__LTD_INLINE void ltd_update_repaired(struct player *, const int damage);

__LTD_INLINE void ltd_update_phaser_fired(struct player *shooter);
__LTD_INLINE void ltd_update_phaser_hit(struct player *shooter);
__LTD_INLINE void ltd_update_phaser_damage(struct player *shooter,
                                           struct player *victim,
                                           const int damage);

__LTD_INLINE void ltd_update_torp_fired(struct player *shooter);
__LTD_INLINE void ltd_update_torp_hit(struct player *shooter);
__LTD_INLINE void ltd_update_torp_detted(struct player *shooter);
__LTD_INLINE void ltd_update_torp_selfdetted(struct player *shooter);
__LTD_INLINE void ltd_update_torp_wall(struct player *shooter);
__LTD_INLINE void ltd_update_torp_damage(struct player *shooter,
                                         struct player *victim,
                                         const int damage);

__LTD_INLINE void ltd_update_plasma_fired(struct player *shooter);
__LTD_INLINE void ltd_update_plasma_hit(struct player *shooter);
__LTD_INLINE void ltd_update_plasma_phasered(struct player *shooter);
__LTD_INLINE void ltd_update_plasma_wall(struct player *shooter);
__LTD_INLINE void ltd_update_plasma_damage(struct player *shooter,
                                           struct player *victim,
                                           const int damage);

/* find a better way to do this inline stuff */

#ifdef LTD_INLINE		/* this stuff is ugly */
#define __LTD_INCLUDE_SOURCE
#include "ltd_stats.c"
#undef  __LTD_INCLUDE_SOURCE
#endif


#ifdef LTD_PER_RACE

static inline LTD_RACE_T ltd_race(const short team) {

  switch(team) {

    case FED:	return LTD_FED;
    case ROM:	return LTD_ROM;
    case KLI:	return LTD_KLI;
    case ORI:	return LTD_ORI;
    default:	return LTD_NOBODY;

  }

}

#else

#define ltd_race(XXX) 0

#endif /* LTD_PER_RACE */

#endif /* __INCLUDED_ltd_stats_h__ */

