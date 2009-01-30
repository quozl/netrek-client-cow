/* Sound defines
 *
 */

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
