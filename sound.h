
/* Sound defines
 *
 * $Log: sound.h,v $
 * Revision 1.3  2002/06/13 05:05:06  tanner
 * Should back out the accidental commits to the head.
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

#endif /* __SOUND_H */
