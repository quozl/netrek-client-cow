
/* Portable Sound Library
 * 
 * Copyright 1993 by Kurt Siegl <007@netrek.org> Permission to use,
 * modify, copy and distribute this software without fee is hereby granted as
 * long as this notice is left here.
 *
 * $Log: audio.h,v $
 * Revision 1.2  1999/12/31 17:27:58  siegl
 * Win32 fixes
 *
 * Revision 1.1.1.1  1998/11/01 17:24:08  siegl
 * COW 3.0 initial revision
 * */

#ifndef __SNDLIB_H
#define __SNDLIB_H


/* Background Sound player */

/* Starts the Background Sound player Returns:  0 on succes -1 in case of an
 * error */
extern int InitSound(void);

/* Terminate Sound player */
extern void ExitSound(void);

/* Is a sound currently playing? */
extern int SoundPlaying();

/* In WINBASE.H (at least in MS VC++) StartSound and StopSound are
 * defined, even though they are supposed to be deleted functions.
 * Being the lazy sort, I just edited WINBASE.H to remove the redef
 * and incorrect def errors. -SAC 96-Jul-06
 */

#ifdef HAVE_WIN32
/* Play a Soundfile */
extern int myStartSound(char *name);

/* Stop the currently played sound */
extern void myStopSound(void);
#define StartSound myStartSound
#define StopSound myStopSound

#else

/* Play a Soundfile */
extern int StartSound(char *name);

/* Stop the currently played sound */
extern void StopSound(void);

/* Internal audio configurations  */

#define SOUNDPLAYER "bgsndplay"

struct shm_sound
  {
    char    name[PATH_MAX];
    int     play_sound;
  };

#endif /* __SNDLIB_H */

#endif
