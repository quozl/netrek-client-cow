
/* Sound defines
 *
 * $Log: sound.h,v $
 * Revision 1.4  2002/06/20 04:18:38  tanner
 * Merged COW_SDL_MIXER_BRANCH to TRUNK.
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

#define NO_SOUND                0
#define FIRE_TORP_SOUND         1
#define PHASER_SOUND            2
#define FIRE_PLASMA_SOUND       3
#define EXPLOSION_SOUND         4
#define CLOAK_SOUND             5
#define UNCLOAK_SOUND           6
#define SHIELD_DOWN_SOUND       7
#define SHIELD_UP_SOUND         8
#define TORP_HIT_SOUND          9
#define WARNING_SOUND           10
#define ENGINE_SOUND            11
#define ENTER_SHIP_SOUND        12
#define SELF_DESTRUCT_SOUND     13
#define PLASMA_HIT_SOUND        14
#define MESSAGE_SOUND           15
#define MESSAGE1_SOUND          16
#define MESSAGE2_SOUND          17
#define MESSAGE3_SOUND          18
#define MESSAGE4_SOUND          19
#define MESSAGE5_SOUND          10
#define MESSAGE6_SOUND          21
#define MESSAGE7_SOUND          22
#define MESSAGE8_SOUND          23
#define MESSAGE9_SOUND          24

#define OTHER_SOUND_OFFSET      24

/* Other people's sounds; not all of these are currently used */
#define OTHER_FIRE_TORP_SOUND   25
#define OTHER_PHASER_SOUND      26
#define OTHER_FIRE_PLASMA_SOUND 27
#define OTHER_EXPLOSION_SOUND   28


#define NUM_SOUNDS 28

struct Sound
  {
    char   *name;
    int     priority;
    int     flag;
  };

extern void sounddone(void);
extern void soundwindow(void);

/* extern void soundaction (W_Event * data); */

extern void Play_Sound(int type);
extern void Abort_Sound(int type);
extern void Init_Sound(void);
extern void Exit_Sound(void);

enum {
  CLOAKED_WAV,
  ENGINE_WAV,
  ENTER_SHIP_WAV,
  EXPLOSION_WAV,
  EXPLOSION_OTHER_WAV,
  FIRE_PLASMA_WAV,
  FIRE_TORP_WAV,
  FIRE_TORP_OTHER_WAV,
  INTRO_WAV,
  MESSAGE_WAV,
  PHASER_WAV,
  PHASER_OTHER_WAV,
  PLASMA_HIT_WAV,
  RED_ALERT_WAV,
  SELF_DESTRUCT_WAV,
  SHIELD_DOWN_WAV,
  SHIELD_UP_WAV,
  TORP_HIT_WAV,
  UNCLOAK_WAV,
  WARNING_WAV,
  NUM_WAVES
};

#endif /* __SOUND_H */
