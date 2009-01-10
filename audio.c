/* Background Sound Library
 * 
 * Copyright 1993 by Kurt Siegl <007@netrek.org> Permission to use,
 * modify, copy and distribute this software without fee is hereby granted as
 * long as this notice is left here.
 *
 */

#include "config.h"

#if defined(SOUND) && !defined(sgi)
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
#include "defaults.h"

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
