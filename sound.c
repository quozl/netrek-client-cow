
/* sound.c - Sound stuff
 *
 * $Log: sound.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 * */
#include "config.h"

#ifdef SOUND
#include "copyright.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include INC_LIMITS
#include INC_SYS_TIME
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "audio.h"

/* Each sound has a priority which controls what can override what
 * Currently these are set as follows:
 * 
 * 10: explosion
 * 9: phaser,plasma
 * 8: torp/plasma hit
 * 7: torp fire, cloak
 * 6: alert
 * 4: "informational" sounds (self destruct, enter ship, message, etc.)
 * 3: shield up/down, other people's sounds (phaser, torp, plasma, explode)
 * 0: background or ambient (engine?)
 */
static struct Sound sounds[NUM_SOUNDS + 1] =
{
  {"", 0, 0},
  {"nt_fire_torp", 7, 1},
  {"nt_phaser", 9, 1},
  {"nt_fire_plasma", 9, 1},
  {"nt_explosion", 10, 1},
  {"nt_cloaked", 7, 1},
  {"nt_uncloak", 7, 1},
  {"nt_shield_down", 3, 1},
  {"nt_shield_up", 3, 1},
  {"nt_torp_hit", 8, 1},
  {"nt_warning", 5, 1},
  {"nt_engine", 0, 0},
  {"nt_enter_ship", 4, 1},
  {"nt_self_destruct", 6, 1},
  {"nt_plasma_hit", 8, 1},
  {"nt_message", 4, 1},
  {"nt_message1", 4, 1},
  {"nt_message2", 4, 1},
  {"nt_message3", 4, 1},
  {"nt_message4", 4, 1},
  {"nt_message5", 4, 1},
  {"nt_message6", 4, 1},
  {"nt_message7", 4, 1},
  {"nt_message8", 4, 1},
  {"nt_message9", 4, 1},
  {"nt_fire_torp_other", 3, 1},
  {"nt_phaser_other", 3, 1},
  {"nt_fire_plasma_other", 3, 1},
  {"nt_explosion_other", 10, 1}
};

static char sound_prefix[PATH_MAX];
static int current_sound = NO_SOUND;
static int sound_other = 1;			 /* Play other ship's sounds? 

						  * 
						  * 
						  */

extern void Exit_Sound(void)
{
  if (sound_init)
    ExitSound();
  sound_init = 0;
  sound_toggle = 0;
}

extern void Init_Sound(void)
{
  char    buf[PATH_MAX];

#ifdef DEBUG
  printf("Init_Sound\n");
#endif

  if (sound_init)
    {
      if ((sounddir = getdefault("sounddir")) == NULL)
	{
	  if (getenv("SOUNDDIR") != NULL)
	    {
	      sounddir = strdup(getenv("SOUNDDIR"));
	    }
	  else
	    sounddir = "./sounds";
	}

      if (InitSound() == -1)
	{
	  sound_toggle = 0;
	  sound_init = 0;
	}
      else
	{
	  sound_init = 1;
	  sound_toggle = 1;
	}

      strcpy(sound_prefix, sounddir);
      strcat(sound_prefix, "/");

      if (sound_toggle)
	{
	  strcpy(buf, sounddir);
	  strcat(buf, "/nt_intro");
	  StartSound(buf);
	}
    }
}

extern void Play_Sound(int type)
{
  char    buf[PATH_MAX];

  /* Don't play other ship's sounds if turned off */
  if (type > OTHER_SOUND_OFFSET && !sound_other)
    return;

  if (sound_toggle && sounds[type].flag &&
      ((sounds[type].priority >= sounds[current_sound].priority) ||
       !SoundPlaying()))
    {
      STRNCPY(buf, sound_prefix, PATH_MAX);
      strcat(buf, sounds[type].name);
      StartSound(buf);
      current_sound = type;
    }

  if (!(sound_toggle))
    current_sound = NO_SOUND;
}

extern void Abort_Sound(int type)
{
  if ((current_sound != NO_SOUND) && (type == current_sound))
    StopSound();
}


/* Sound options window stuff */

#define SOUND_TOGGLE  0
#define SOUND_OTHER   MESSAGE_SOUND + 1
#define SOUND_INIT    MESSAGE_SOUND + 2
#define SOUND_DONE    MESSAGE_SOUND + 3

static void soundrefresh(int i);

extern void soundwindow(void)
{
  int     i;

  for (i = 0; i <= SOUND_DONE; i++)
    soundrefresh(i);

  /* Map window */
  W_MapWindow(soundWin);
}

static void
        soundrefresh(int i)
{
  char    buf[BUFSIZ], *flag;

  if (i == SOUND_TOGGLE)
    {
      sprintf(buf, "Sound is turned %s", (sound_toggle == 1) ? "ON" : "OFF");
    }
  else if (i < SOUND_OTHER)
    {
      flag = ((sounds[i].flag == 1) ? "ON" : "OFF");
      switch (i)
	{
	case CLOAK_SOUND:
	  sprintf(buf, "Cloak sound is %s", flag);
	  break;
	case UNCLOAK_SOUND:
	  sprintf(buf, "Uncloak sound is %s", flag);
	  break;
	case ENGINE_SOUND:
	  sprintf(buf, "Engine sound is not supported");
	  /* sprintf (buf, "Engine sound is %s",  flag); */
	  break;
	case ENTER_SHIP_SOUND:
	  sprintf(buf, "Enter ship sound is %s", flag);
	  break;
	case EXPLOSION_SOUND:
	  sprintf(buf, "Explosion sound is %s", flag);
	  break;
	case FIRE_TORP_SOUND:
	  sprintf(buf, "Fire torp sound is %s", flag);
	  break;
	case MESSAGE_SOUND:
	  sprintf(buf, "Message sound is %s", flag);
	  break;
	case PHASER_SOUND:
	  sprintf(buf, "Phaser sound is %s", flag);
	  break;
	case WARNING_SOUND:
	  sprintf(buf, "Warning sound is %s", flag);
	  break;
	case SHIELD_DOWN_SOUND:
	  sprintf(buf, "Shield down  sound is %s", flag);
	  break;
	case SHIELD_UP_SOUND:
	  sprintf(buf, "Shield up sound is %s", flag);
	  break;
	case TORP_HIT_SOUND:
	  sprintf(buf, "Torp hit sound is %s", flag);
	  break;
	case SELF_DESTRUCT_SOUND:
	  sprintf(buf, "Self destruct sound is %s", flag);
	  break;
	case FIRE_PLASMA_SOUND:
	  sprintf(buf, "Fire plasma sound is %s", flag);
	  break;
	case PLASMA_HIT_SOUND:
	  sprintf(buf, "Plasma hit sound is %s", flag);
	  break;
	}
    }
  else if (i == SOUND_OTHER)
    {
      if (sound_other)
	strcpy(buf, "Other ship's sound is ON");
      else
	strcpy(buf, "Other ship's sound is OFF");
    }
  else if (i == SOUND_INIT)
    {
      if (sound_init)
	strcpy(buf, "Restart external sound player");
      else
	strcpy(buf, "Initialize external sound player");
    }
  else if (i == SOUND_DONE)
    {
      strcpy(buf, "Done");
    }
  else
    {
      fprintf(stderr, "Uh oh, bogus refresh number in soundrefresh\n");
    }

  W_WriteText(soundWin, 0, i, textColor, buf, strlen(buf), 0);
}

void    soundaction(W_Event * data)
{
  int     i, j;

  i = data->y;

  if (i == SOUND_TOGGLE)
    {
      if (sound_init)
	sound_toggle = (sound_toggle == 1) ? 0 : 1;
      soundrefresh(SOUND_TOGGLE);
      if (!sound_toggle)
	{
	  Abort_Sound(ENGINE_SOUND);
	}
      else
	{
	  Play_Sound(ENGINE_SOUND);
	}
    }
  else if (i < SOUND_OTHER)
    {
      sounds[i].flag = (sounds[i].flag == 1) ? 0 : 1;
      if (i + OTHER_SOUND_OFFSET <= NUM_SOUNDS)
	sounds[i + OTHER_SOUND_OFFSET].flag = sounds[i].flag;

      soundrefresh(i);
      Play_Sound(i);
      if (i == MESSAGE_SOUND)
	{
	  for (j = MESSAGE1_SOUND; j <= MESSAGE9_SOUND; j++)
	    sounds[j].flag = sounds[MESSAGE_SOUND].flag;
	}
      /* case ENGINE: st_engine = (st_engine == 1) ? 0 : 0; soundrefresh * *
       * (ENGINE); if (st_engine && sound_toggle) { Play_Sound * *
       * (ENGINE_SOUND); } else { Abort_Sound (ENGINE_SOUND); } break; */

    }
  else if (i == SOUND_OTHER)
    {
      sound_other = !sound_other;
      soundrefresh(SOUND_OTHER);
    }
  else if (i == SOUND_INIT)
    {
      Exit_Sound();
      sound_init = 1;
      Init_Sound();
      soundrefresh(SOUND_INIT);
      soundrefresh(SOUND_TOGGLE);
    }
  else
    {
      sounddone();
    }
}

extern void
        sounddone(void)
{
  W_UnmapWindow(soundWin);
}
#endif /* SOUND */
