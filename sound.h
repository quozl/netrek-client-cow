
/* Sound defines
 *
 * $Log: sound.h,v $
 * Revision 1.4  2002/06/20 04:18:38  tanner
 * Merged COW_SDL_MIXER_BRANCH to TRUNK.
 *
 * Revision		 2008/12/15 23:16:27
 * Mon Dec 15 23:16:27 CET 2008 Stefan Stapelberg <stefan@rent-a-guru.de>
 *
 *	* added (native) sound handler for SGI workstations
 *
 * Revision 1.1.1.1.2.1  2002/06/13 04:10:16  tanner
 * Wed Jun 12 22:52:13 2002  Bob Tanner  <tanner@real-time.com>
 *
 * 	* playback.c (pbmain):  Converted enter_ship.wav
 *
 * 	* input.c (Key113): Converted self_destruct.wav
 *
 * 	* input.c (Key109): Converted message.wav
 *
 * 	* local.c (DrawMisc): Converted warning.wav
 *
 * 	* local.c (DrawPlasmaTorps): Converted plasma_hit.wav
 *
 * 	* local.c (DrawTorps): Converted torp_hit.wav
 *
 * 	* sound.h: added EXPLOSION_OTHER_WAV, PHASER_OTHER_WAV,
 * 	FIRE_TORP_OTHER. and the code to load these new sounds.
 *
 * 	* local.c (DrawShips): Converted cloak.wav, uncloak.wav,
 * 	shield_down.wav, shield_up.wav, explosion.wav,
 * 	explosion_other.wav, phaser.wav, phaser_other.wav
 *
 * 	* cowmain.c (cowmain): Converted enter_ship.wav and engine.wav
 *
 * 	* sound.c: added isDirectory to check that the sounddir is
 * 	actually a directory.
 *
 * Tue Jun 11 01:10:51 2002  Bob Tanner  <tanner@real-time.com>
 *
 * 	* system.mk.in: Added SDL_CFLAGS, SDL_CONFIG, SDL_LIBS,
 * 	SDL_MIXER_LIBS
 *
 * 	* sound.c: Added HAVE_SDL wrapper, initialization of SDL system,
 * 	opening of audio device, and loading of 17 cow sounds.
 *
 * 	* cowmain.c (cowmain): HAVE_SDL wrapper to Init_Sound using SDL. I
 * 	moved the Init_Sound method to right after readdefaults() so the
 * 	intro can start playing ASAP.
 *
 * 	* configure.in: Added AC_CANONICAL_SYSTEM, added check for SDL,
 * 	add check for SDL_mixer.
 *
 * 	* config.h.in: add HAVE_SDL
 *
 * 	* spike: See spike/README for details
 *
 * Revision 1.2  2002/06/13 03:45:19  tanner
 * Wed Jun 12 22:35:44 2002  Bob Tanner  <tanner@real-time.com>
 *
 * 	* local.c (DrawMisc): Converted warning.wav
 *
 * 	* local.c (DrawPlasmaTorps): Converted plasma_hit.wav
 *
 * 	* local.c (DrawTorps): Converted torp_hit.wav
 *
 * 	* sound.h: added EXPLOSION_OTHER_WAV, PHASER_OTHER_WAV,
 * 	FIRE_TORP_OTHER. and the code to load these new sounds.
 *
 * 	* local.c (DrawShips): Converted cloak.wav, uncloak.wav,
 * 	shield_down.wav, shield_up.wav, explosion.wav,
 * 	explosion_other.wav, phaser.wav, phaser_other.wav
 *
 * 	* cowmain.c (cowmain): Converted enter_ship.wav and engine.wav
 *
 * 	* sound.c: added isDirectory to check that the sounddir is
 * 	actually a directory.
 *
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 * */

#ifndef __SOUND_H
#define __SOUND_H

#define NO_SOUND		 0
#define FIRE_TORP_SOUND		 1
#define PHASER_SOUND		 2
#define FIRE_PLASMA_SOUND	 3
#define EXPLOSION_SOUND		 4
#define SBEXPLOSION_SOUND	 5
#define CLOAK_SOUND		 6
#define UNCLOAK_SOUND		 7
#define SHIELD_DOWN_SOUND	 8
#define SHIELD_UP_SOUND		 9
#define TORP_HIT_SOUND		10
#define REDALERT_SOUND		11
#define BUZZER_SOUND		12
#define ENGINE_SOUND		13
#define THERMAL_SOUND		14
#define ENTER_SHIP_SOUND	15
#define SELF_DESTRUCT_SOUND	16
#define PLASMA_HIT_SOUND	17
#define MESSAGE_SOUND		18
#define MESSAGE1_SOUND		19
#define MESSAGE2_SOUND		20
#define MESSAGE3_SOUND		21
#define MESSAGE4_SOUND		22
#define MESSAGE5_SOUND		23
#define MESSAGE6_SOUND		24
#define MESSAGE7_SOUND		25
#define MESSAGE8_SOUND		26
#define MESSAGE9_SOUND		27

/* Other people's sounds; not all of these are currently used */
#define OTHER_SOUND_OFFSET	27
#define OTHER_FIRE_TORP_SOUND	28
#define OTHER_PHASER_SOUND	29
#define OTHER_FIRE_PLASMA_SOUND	30
#define OTHER_EXPLOSION_SOUND	31
#define OTHER_SBEXPLOSION_SOUND	32

#define NUM_SOUNDS		32

#if !defined(sgi)
struct Sound {
    char *name;
    int  priority;
    int  flag;
};
#endif

/* Window stuff */
extern void sounddone(void);
extern void soundwindow(void);
extern int  sound_window_height(void);
extern void soundaction (W_Event * data);

/* Global sound functions */
extern void Play_Sound(int type);
extern void Init_Sound(void);
extern void Exit_Sound(void);
extern void Abort_Sound(int type);

#if defined(sgi)
#define ENG_OFF		-1, -1
#define ENG_ON		 0,  0

extern void Engine_Sound(int speed, int maxspeed);
#endif

#endif /* __SOUND_H */
