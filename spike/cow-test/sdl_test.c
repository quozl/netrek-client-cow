#include <stdlib.h>
#include <stdio.h>

#include "SDL.h"
#include "SDL_mixer.h"

#define DIR_SEP	"/"
#define DIR_CUR	""
#define DATAFILE(X)	DIR_CUR "sounds" DIR_SEP X

enum {
  PHASER_WAV,
  CLOAKED_WAV,
  ENGINE_WAV,
  ENTER_SHIP_WAV,
  EXPLOSION_WAV,
  FIRE_PLASMA_WAV,
  FIRE_TORP_WAV,
  MESSAGE_WAV,
  PLASMA_HIT_WAV,
  RED_ALERT_WAV,
  SELF_DESTRUCT_WAV,
  SHIELD_DOWN_WAV,
  SHIELD_UP_WAV,
  TORP_HIT_WAV,
  UNCLOAK_WAV,
  WARNING_WAV,
  INTRO_WAV,
  NUM_WAVES
};
Mix_Chunk *sounds[NUM_WAVES];

int loadSounds(void) {
  int i;

  sounds[PHASER_WAV] = Mix_LoadWAV(DATAFILE("phaser.wav"));
  sounds[CLOAKED_WAV] = Mix_LoadWAV(DATAFILE("cloaked.wav"));
  sounds[ENGINE_WAV] = Mix_LoadWAV(DATAFILE("engine.wav"));
  sounds[ENTER_SHIP_WAV] = Mix_LoadWAV(DATAFILE("enter_ship.wav"));
  sounds[EXPLOSION_WAV] = Mix_LoadWAV(DATAFILE("explosion.wav"));
  sounds[FIRE_PLASMA_WAV] = Mix_LoadWAV(DATAFILE("fire_plasma.wav"));
  sounds[FIRE_TORP_WAV] = Mix_LoadWAV(DATAFILE("fire_torp.wav"));
  sounds[MESSAGE_WAV] = Mix_LoadWAV(DATAFILE("message.wav"));
  sounds[PLASMA_HIT_WAV] = Mix_LoadWAV(DATAFILE("plasma_hit.wav"));
  sounds[RED_ALERT_WAV] = Mix_LoadWAV(DATAFILE("red_alert.wav"));
  sounds[SELF_DESTRUCT_WAV] = Mix_LoadWAV(DATAFILE("self_destruct.wav"));
  sounds[SHIELD_DOWN_WAV] = Mix_LoadWAV(DATAFILE("shield_down.wav"));
  sounds[SHIELD_UP_WAV] = Mix_LoadWAV(DATAFILE("shield_up.wav"));
  sounds[TORP_HIT_WAV] = Mix_LoadWAV(DATAFILE("torp_hit.wav"));
  sounds[UNCLOAK_WAV] = Mix_LoadWAV(DATAFILE("uncloak.wav"));
  sounds[WARNING_WAV] = Mix_LoadWAV(DATAFILE("warning.wav"));
  sounds[INTRO_WAV] = Mix_LoadWAV(DATAFILE("intro.wav"));

  for (i=0; i < NUM_WAVES; i++) {
    if (!sounds[i]) {
      fprintf(stderr, "Mix_LoadWAV sound[%d]: %s\n", i, Mix_GetError());
    }
  }

  return(1);
}

void freeSounds() {
  int i;
  for (i=0; i < NUM_WAVES; ++i) {
    Mix_FreeChunk(sounds[i]);
  }
}

void playSound() {
  char buffer[16];
  int index = 0;

  printf("Sound [0-%d]:", NUM_WAVES-1);
  fgets(buffer, 16, stdin);
  index = atoi(buffer);

  if ((index < 0) || (index >= NUM_WAVES)) {
    fprintf(stderr, "Invalid range: %d\n", index);
  } else {
    if (Mix_PlayChannel(-1, sounds[index], 0) < 0) {
      fprintf(stderr, "Mix_PlayChannel: %s\n", Mix_GetError());
    }
  }
}

main(int argc, char *argv[]) {
  /* Initialize the SDL library */
  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
    exit(2);
  }
  atexit(SDL_Quit);

  /* Open the audio device */
  if (Mix_OpenAudio(8000, AUDIO_U8, 1, 512) < 0) {
    fprintf(stderr,"Mix_OpenAudio: %s\n", Mix_GetError());
  }

  if (loadSounds()) {
    while (1) {
      playSound();
    }
    
    freeSounds();
  }

  Mix_CloseAudio();
  exit(0);
}
