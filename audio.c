

/* Background Sound Library
 * 
 * Copyright 1993 by Kurt Siegl <007@netrek.org> Permission to use,
 * modify, copy and distribute this software without fee is hereby granted as
 * long as this notice is left here.
 *
 * $Log: audio.c,v $
 * Revision 1.5  2002/06/20 04:18:38  tanner
 * Merged COW_SDL_MIXER_BRANCH to TRUNK.
 *
 * Revision 1.2.2.1  2002/06/13 04:10:16  tanner
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
 * Revision 1.3  2002/06/11 05:55:13  tanner
 * Following XP made a simple change.
 *
 * I want cow to play the STTNG intro when started. That's it. Nothing else.
 *
 * Revision 1.2  2000/01/02 11:46:32  siegl
 * Audio fixes
 *
 * Revision 1.1.1.1  1998/11/01 17:24:08  siegl
 * COW 3.0 initial revision
 * */

#include "config.h"

#ifdef SOUND
#include <stdio.h>
#include INC_UNISTD
#include INC_STDLIB
#include INC_LIMITS
#include INC_FCNTL
#include INC_STRINGS
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "audio.h"

static int snd_pid, SoundRet = -1, shmid;

static struct shm_sound *shared_data;

static  RETSIGTYPE
        RetOk(int _dummy)
{
  SoundRet = 0;
}

static  RETSIGTYPE
        RetErr(int _dummy)
{
  SoundRet = -1;
}

extern void
        ExitSound(void)
{
  kill(snd_pid, SIGINT);
  shmctl(shmid, IPC_RMID, 0);
}

extern int InitSound(void) {

  char   *argv[5], shmids[10], mypid[10], *sound_player;
  int     argc = 0;
  int     pid, shmKey = IPC_PRIVATE;
  struct shmid_ds smbuf;

  /* if ((shmid=shmget(shmKey, sizeof(struct shm_sound),0))>=0) { * *
   * fprintf(stderr,"Killing Sound Shared Memory\n"); shmctl(shmid, IPC_RMID,
   * * * 0); } */

  if ((shmid = shmget(shmKey, sizeof(struct shm_sound), (IPC_CREAT | 0777))) < 0)
    {
      perror("Can't open Sound Shared Memory\n");
      return (-1);
    }

  shmctl(shmid, IPC_STAT, &smbuf);
  smbuf.shm_perm.uid = getuid();
  smbuf.shm_perm.mode = 0777;
  shmctl(shmid, IPC_SET, &smbuf);

  if ((shared_data = (struct shm_sound *) shmat(shmid, 0, 0)) == (void *) -1)
    {
      perror("shm attach failed");
      return (-1);
    }

  SoundRet = 1;
  SIGNAL(SIGUSR1, RetOk);
  SIGNAL(SIGUSR2, RetErr);
  SIGNAL(SIGCHLD, RetErr);
  sprintf(shmids, "%i", shmid);
  sprintf(mypid, "%i", getpid());

  if ((pid = vfork()) == 0)
    {

      /* setsid();  Make it independend of the main program */

      if ((sound_player = (char *) getdefault("soundplayer")) == NULL)
	sound_player = SOUNDPLAYER;
      argv[argc++] = sound_player;
      argv[argc++] = (char *) shmids;
      argv[argc++] = (char *) mypid;
      argv[argc] = NULL;

      SIGNAL(SIGINT, SIG_DFL);
      execvp(sound_player, argv);
      perror(sound_player);
      fflush(stderr);
      SoundRet = -2;
      _exit(-1);
    }

  if (pid == -1)
    {
      perror("forking");
      return (-1);
    }

  snd_pid = pid;
  if (SoundRet >0 )
    pause();
  if (SoundRet)
    ExitSound();
  return (SoundRet);

}

extern void
        StopSound(void)
{
  kill(snd_pid, SIGUSR1);
}

extern int StartSound(char *name) {
  strcpy(shared_data->name, name);
  kill(snd_pid, SIGUSR2);
  return (0);
}

extern int
        SoundPlaying()
{
  return (shared_data->play_sound);
}
#endif
