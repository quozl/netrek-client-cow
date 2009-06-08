#include "config.h"

#ifdef SOUND

#if defined(HAVE_SDL)
#include "SDL.h"
#include "SDL_mixer.h"
#endif

#include "copyright.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include INC_LIMITS
#include INC_SYS_TIME

#if defined(sgi)
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <invent.h>
#include <dmedia/audio.h>
#include <dmedia/audiofile.h>
#include <signal.h>
#include <stropts.h>
#include <poll.h>
#include "sgi-sound.h"
#endif

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "audio.h"

#if defined(HAVE_SDL) || defined(sgi)
enum {
  INTRO_WAV,			/*  0 */
  FIRE_TORP_WAV,		/*  1 */
  PHASER_WAV,			/*  2 */
  FIRE_PLASMA_WAV,		/*  3 */
  EXPLOSION_WAV,		/*  4 */
  SBEXPLOSION_WAV,		/*  5 */
  CLOAK_WAV,			/*  6 */
  UNCLOAK_WAV,			/*  7 */
  SHIELD_DOWN_WAV,		/*  8 */
  SHIELD_UP_WAV,		/*  9 */
  TORP_HIT_WAV,			/* 10 */
  REDALERT_WAV,			/* 11 */
  BUZZER_WAV,			/* 12 */
  ENTER_SHIP_WAV,		/* 13 */
  SELF_DESTRUCT_WAV,		/* 14 */
  PLASMA_HIT_WAV,		/* 15 */
  MESSAGE_WAV,			/* 16 */
  ENGINE_WAV,			/* 17 */
  THERMAL_WAV,			/* 18 */
  FIRE_TORP_OTHER_WAV,		/* 19 */
  PHASER_OTHER_WAV,		/* 20 */
  FIRE_PLASMA_OTHER_WAV,	/* 21 */
  EXPLOSION_OTHER_WAV,		/* 22 */
  SBEXPLOSION_OTHER_WAV,	/* 23 */
  NUM_WAVES			/* 24 */
};
#endif

#if defined(HAVE_SDL)
/* This is probably unix specific paths */
static Mix_Chunk *sounds[NUM_WAVES];
#endif

#if defined(sgi)
static ALport sfxInitAudioPort(int init);
static short *sfxLoadAudioFile(char *dirName, char *fileName, AFframecount *size);
static int sfxCheckForAudio(void);
static int sfxOpenAudioPorts(ALport ports[], int maxAudioPorts);
static void sfxCheckVolume(void);
static void sfxResetAudioHw(void);
static void sfxSetGain(float l, float r);
static void sfxSetGainIndex(unsigned long gain);
static void sfxSignalSound(Sfx sp);
static void sfxDieGracefully(int sig);
static void sfxSoundDied(int sig);
static void sfxSoundErrFunc(long err, const char *msg, ...);
static void sfxSoundHandler(void *arg);

static Sfx sounds[NUM_WAVES];			/* waveform storage */
static ALport audioPort[MAX_AUDIO_PORTS];	/* audio port structure */
static int nAudioPorts;				/* number of audio ports */
static int soundOther = 1;			/* sound toggle for other ships */
static int soundChild;				/* set if child process is running */
static int gainIndex;				/* current gain index */
static int endingOnPurpose = 0;			/* set if sound should end on purpose */
static int spigot[2];				/* pipe to the child process */
static int soundId = 0;				/* ID of current sound */
static char *sfxAudioDir;			/* directory with sound files */
static unsigned long origLeftGain;		/* initial gain setting for left channel */
static unsigned long origRightGain;		/* initial gain setting for right channel */
static unsigned long origOutputRate;		/* initial output sample rate */
static unsigned long currLeftGain;		/* current gain setting for left channel */
static unsigned long currRightGain;		/* current gain setting for right channel */
static unsigned long gainSettings[] = { 0, 2, 3, 5, 9, 16, 29, 50, 91, 156, 255 };
#endif

#if !defined(HAVE_SDL) && !defined(sgi)

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
static struct Sound sounds[NUM_SOUNDS+1] = {
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
static int soundOther = 1;		 /* play other ship's sounds?  */

#endif /* HAVE_SDL */

static int isDirectory(char* dir) {	/* check wheter `dir' is a directory */
	struct stat buf;

	if (stat(dir, &buf) < 0) {
		return 0;
	}
	return S_ISDIR(buf.st_mode);
}

#if defined(sgi)

/*
 * Initialize the audio hardware.
 */
int sfxInit(char *audioDir, int numPorts) {
	int idx;
	char *str;

	if (audioDir != NULL) {
		str = strdup(audioDir);
		if (str)
			sfxAudioDir = str;
	}
	if (numPorts < 1)
		return SFX_ERR_NO_PORTS_AVAIL;

	if (numPorts > MAX_AUDIO_PORTS)
		numPorts = MAX_AUDIO_PORTS;

	(void) ALseterrorhandler(sfxSoundErrFunc);

	nAudioPorts = sfxOpenAudioPorts(audioPort, numPorts);

	if (nAudioPorts == 0)
		return SFX_ERR_NO_PORTS_AVAIL;

	if (nAudioPorts < 0)
		return SFX_ERR_NO_AUDIO_HW;

	(void) signal(SIGCHLD, sfxSoundDied);

	if (pipe(spigot) < 0 || (soundChild = sproc(sfxSoundHandler, PR_SADDR, audioPort)) < 0) {
		for (idx=0; idx < nAudioPorts; idx++)
			ALcloseport(audioPort[idx]);

		return SFX_ERR_NO_SPROC;
	}
	return nAudioPorts;
}

/*
 * Load an audio file.
 */
Sfx sfxLoad(char *filename) {
	Sfx sp;

	if ((sp=(Sfx)malloc(sizeof(struct _sfx))) == NULL)
		return NULL;

	sp->soundData = NULL;
	sp->soundSize = NULL;
	sp->loop = 0;
	sp->repeat = 0;
	sp->count = 1;
	sp->pitch = 0;
	sp->enabled = 1;

	if ((sp->soundData=(short **)malloc(sizeof(short *))) == NULL) {
		sfxFree(sp);
		return NULL;
	}
	if ((sp->soundSize=(AFframecount *)malloc(sizeof(AFframecount))) == NULL) {
		sfxFree(sp);
		return NULL;
	}
	if ((sp->soundData[0] = sfxLoadAudioFile(sfxAudioDir, filename, &(sp->soundSize[0]))) == NULL) {
		sfxFree(sp);
		return NULL;
	}
	sp->id = soundId++;
	return sp;
}

/*
 * Indicate that a sound is to loop.
 */
void sfxLoop(Sfx sp) {
	if (sp != NULL)
		sp->loop = 1;
	return;
}

/*
 * Play a sound effect.
 */
void sfxPlay(Sfx sp) {
	if (sound_init && sp != NULL && sp->soundData[0] && sp->enabled) {
		if (sp->loop)
			sp->repeat = 1;

		sfxCheckVolume();
		sfxSignalSound(sp);
	}
	return;
}

/*
 * Play a pitch-bent audio clip.
 */
void sfxPlayPitch(Sfx sp, float val) {
	int i;

	if (sound_init && sp != NULL && sp->soundData[0] && sp->enabled) {
		if (val <= sp->loVal)
			i = 0;
		else if (val >= sp->hiVal)
			i = sp->count-1;
		else
			i = (long)(0.4f + (sp->count - 1) *
				   (val - sp->loVal)/(sp->hiVal - sp->loVal));

		if (sp->loop && sp->repeat && sp->pitch == i)
                        return;

		sp->pitch = i;
		sfxPlay(sp);
	}
	return;
}

/*
 * Create a pitch bend audio clip.
 */
void sfxPitchBend(Sfx sp, float startVal, float endVal,
			  float startPitch, float endPitch,
			  float basePitch, int steps) {
	int	i, k;
	int	size;
	short	**ptr;
	short	*data;
	short	*origData;
	double	pitch;
	double	a, b;
	long	frameIndex;
	AFframecount *lptr;

	if (sp == NULL || steps < 2 || sp->count != 1)
		return;

	if ((lptr=(AFframecount *)malloc(steps * sizeof(AFframecount))) == NULL)
		return;

	size = 0;
	a = (endPitch - startPitch) / (double)(steps - 1);
	b = 1.0 / (double)basePitch;

	for (i=0; i < steps; i++) {
		pitch = (startPitch + (double)i * a) * b;
		k = (long)((double)sp->soundSize[0] / pitch);
		lptr[i] = k;
		size += k;
	}
	if ((ptr=(short **)malloc(steps * sizeof(short *))) == NULL) {
		free(lptr);
		return;
	}

	/*
	 * Allocate new space for pitch-bent sound data.
	 */
	if ((data=(short *)malloc(size * 2 * sizeof(short))) == NULL) {
		free(lptr);
		free(ptr);
		return;
	}

	size = (int)sp->soundSize[0];
	free(sp->soundSize);
	sp->soundSize = lptr;

	origData = sp->soundData[0];
	free(sp->soundData);
	sp->soundData = ptr;

	sp->count = steps;
	sp->loVal = startVal;
	sp->hiVal = endVal;

	for (i=0; i < steps; i++) {
		pitch = (startPitch + (double)i * a) * b;

		if (i == 0)
			sp->soundData[i] = data;
		else
			sp->soundData[i] = sp->soundData[i-1] + 2 * sp->soundSize[i-1];

		for (k=0; k < sp->soundSize[i]; k++) {
			frameIndex = (long)(k * pitch + 0.5);
			if (frameIndex >= size)
				frameIndex = size - 1;

			sp->soundData[i][2*k  ] = origData[2*frameIndex	 ];
			sp->soundData[i][2*k+1] = origData[2*frameIndex+1];
		}
	}
	free(origData);
	return;
}

/*
 * Silence an audio loop.
 */
void sfxSilenceLoop(Sfx sp) {
	if (sp != NULL && sp->repeat) {
		sp->repeat = 0;
		sfxSignalSound(sp);
	}
	return;
}

/*
 * Disable a sound clip.
 */
void sfxDisable(Sfx sp) {
	if (sp != NULL) {
		sp->enabled = 0;
		if (sp->loop)
			sfxSilenceLoop(sp);
	}
	return;
}

/*
 * Enable a sound clip.
 */
void sfxEnable(Sfx sp) {
	if (sp != NULL)
		sp->enabled = 1;
	return;
}

/*
 * Check whether a sound clip is enabled.
 */
int sfxIsEnabled(Sfx sp) {
	return (sp != NULL) && sp->enabled;
}

/*
 * Set the volume to a specified level.
 */
void sfxSetVolume(int level) {
	if (level >= 0 && level < sizeof( gainSettings ) / sizeof( gainSettings[0])) {
		gainIndex = level;
		currLeftGain = currRightGain = gainSettings[gainIndex];
	}
	return;
}

/*
 * Clean up sound routines.
 */
void sfxEnd(int waitForSounds) {
	int idx;

	endingOnPurpose = 1;

	if (waitForSounds) {		/* wait for sounds to complete */
		for (idx=0; idx < nAudioPorts; idx++) {
			while (ALgetfilled(audioPort[idx]) > 0)
				sginap(1);

			ALcloseport(audioPort[idx]);
		}
	} else if (soundChild > 0)	/* kill childs playing sounds */
		kill(soundChild, SIGKILL);

	if (nAudioPorts > 0)		/* reset audio subsystem */
		sfxResetAudioHw();
}

/*
 * Open up the audio ports.
 */
static int sfxOpenAudioPorts(ALport ports[], int maxAudioPorts) {
	int n;

	if (sfxCheckForAudio() == 0)
		return -1;

	for (n=0; n < maxAudioPorts; n++)
		if ((audioPort[n] = sfxInitAudioPort(n == 0)) == NULL)
			break;
	return n;
}

/*
 * Check for audio hardware.
 */
static int sfxCheckForAudio(void) {
	int st = 0;
	inventory_t *base;
	inventory_t *inv;

	inv = base = getinvent();
	while (inv != NULL) {
		/*
		 * Ok if any type of audio hardware available.
		 */
		if (inv->inv_class == INV_AUDIO)
			st = 1;

		inv = getinvent();
	}
	if( base )
		endinvent();

	return st;
}

/*
 * Open and initialize an audio port.
 */
static ALport sfxInitAudioPort(int init) {
	ALport ap;
	ALconfig audioPortConfig;
	long pvbuf[6];

	if (init) {
		pvbuf[0] = AL_LEFT_SPEAKER_GAIN;
		pvbuf[2] = AL_RIGHT_SPEAKER_GAIN;
		pvbuf[4] = AL_OUTPUT_RATE;
		ALgetparams(AL_DEFAULT_DEVICE, pvbuf, 6L);
		currLeftGain = origLeftGain = pvbuf[1];
		currRightGain = origRightGain = pvbuf[3];
		origOutputRate = pvbuf[5];
		sfxSetGainIndex((origLeftGain+origRightGain)/2);
	}

	/*
	 * Configure and open audio port.
	 */
	audioPortConfig = ALnewconfig();
	ALsetwidth(audioPortConfig, AL_SAMPLE_16);
	ALsetchannels(audioPortConfig, AL_STEREO);
	ALsetqueuesize(audioPortConfig, 16000);
	ap = ALopenport("spacetrek", "w", audioPortConfig);
	ALfreeconfig(audioPortConfig);

	return ap;
}

/*
 * Reset the audio hardware to where we found it when we started.
 */
static void sfxResetAudioHw(void) {
	long pvbuf[6];

	pvbuf[0] = AL_LEFT_SPEAKER_GAIN;
	pvbuf[1] = origLeftGain;
	pvbuf[2] = AL_RIGHT_SPEAKER_GAIN;
	pvbuf[3] = origRightGain;
	pvbuf[4] = AL_OUTPUT_RATE;
	pvbuf[5] = origOutputRate;

	ALsetparams(AL_DEFAULT_DEVICE, pvbuf, 6L);
	return;
}

/*
 * Free the Sfx structure.
 */
void sfxFree(Sfx sp) {
	if (sp) {
		if (sp->soundData) {
			free(sp->soundData);
			sp->soundData = NULL;
		}
		if (sp->soundSize) {
			free(sp->soundSize);
			sp->soundSize = NULL;
		}
		free(sp);
	}
	return;
}

/*
 * Set the gain index.
 */
static void sfxSetGainIndex(unsigned long gain) {
	int idx;
	long min = 256;
	long diff;

	for (idx=0; idx < sizeof(gainSettings) / sizeof(gainSettings[0]); idx++) {
		diff = gain - gainSettings[idx];
		if (diff < 0)
			diff = -diff;

		if (diff < min) {
			min = diff;
			gainIndex = idx;
		}
	}
	return;
}

/*
 * Set the volume level.
 */
static void sfxSetGain(float l, float r) {
	long pvbuf[4];

	pvbuf[0] = AL_LEFT_SPEAKER_GAIN;
	pvbuf[1] = l * currLeftGain;
	pvbuf[2] = AL_RIGHT_SPEAKER_GAIN;
	pvbuf[3] = r * currRightGain;

	ALsetparams(AL_DEFAULT_DEVICE, pvbuf, 4L);
}

/*
 * Check for external volume changes.
 */
static void sfxCheckVolume(void) {
	long pvbuf[6];

	/*
	 * Check to see if the volume was changed externally.
	 */
	pvbuf[0] = AL_LEFT_SPEAKER_GAIN;
	pvbuf[2] = AL_RIGHT_SPEAKER_GAIN;

	ALgetparams(AL_DEFAULT_DEVICE, pvbuf, 4L);
	if (pvbuf[1] != currLeftGain || pvbuf[3] != currRightGain) {
		origLeftGain = currLeftGain = pvbuf[1];
		origRightGain = currRightGain = pvbuf[3];
		sfxSetGainIndex((pvbuf[1] + pvbuf[3]) / 2);
	}
	return;
}

/*
 * Adjust the volume.
 */
int sfxVolumeChange(int direction) {
	if (sound_init) {
		sfxCheckVolume();
		if (direction == 1) {
			if (gainIndex < sizeof(gainSettings)/sizeof(gainSettings[0])-1)
				gainIndex++;

		} else if (gainIndex > 0)
			gainIndex--;

		currLeftGain = currRightGain = gainSettings[gainIndex];
		sfxSetGain(1., 1.);

		return gainIndex;
	}
	return 0;
}

/*
 * Wake up the sound handler.
 */
static void sfxSignalSound(Sfx sp) {
	if (soundChild && write(spigot[1], sp, sizeof(*sp)) != sizeof(*sp))
		(void) fprintf(stderr, "sfxSignalSound failed: %s\n", strerror(errno));

	return;
}

/*
 * Audio library error handler.
 */
static void sfxSoundErrFunc(long err, const char *msg, ...) { }

/*
 * Handler for sound child dying.
 */
static void sfxSoundDied(int sig) {
	if (!endingOnPurpose) {
		nAudioPorts = 0;
		sound_init = 0;

		(void) fprintf(stderr,
				"Sound handling child process was killed by signal %d\n", sig);
	}
	return;
}

/*
 * Exit sound handler on SIGHUP.
 */
static void sfxDieGracefully(int sig) {
	exit(0);
}

/*
 * Sound handler.
 */
static void sfxSoundHandler(void *arg) {
	ALport *ap = (ALport *)arg;
	Sample samp[MAX_AUDIO_PORTS];

	int idx, nap;
	int treated;
	int nSounds = 1;
	int nextPort = 0;
	long maxSampsPerPass;
	long sampCount;
	long pvbuf[2];
	struct _sfx ss;
	struct pollfd pf;

	prctl(PR_TERMCHILD, 0);

	(void) signal(SIGHUP, sfxDieGracefully);

	maxSampsPerPass = 1600;

	for (idx=0; idx < nAudioPorts; idx++) {
		samp[idx].id = -1;
		samp[idx].sample = NULL;
		samp[idx].sampsToPlay = 0;
		samp[idx].repeat = 0;
	}

	/*
	 * Set sample rate for output device.
	 */
	pvbuf[0] = AL_OUTPUT_RATE;
	pvbuf[1] = AL_RATE_16000;

	(void) ALsetparams(AL_DEFAULT_DEVICE, pvbuf, 2L);

	/*
	 * Prepare to read from pipe.
	 */
	pf.fd = spigot[0];
	pf.events = POLLIN | POLLRDNORM | POLLRDBAND;

#define EVER ;;
	for (EVER) {
		if (nSounds == 0 || (idx=poll(&pf, 1, 0)) > 0) {
			(void) read(spigot[0], &ss, sizeof(ss));
			treated = 0;
			if (ss.loop == 1 && ss.repeat == 0) {
				treated = 1;
				for (idx=0; idx < nAudioPorts; idx++) {
					if (samp[idx].id == ss.id) {
						samp[idx].id = -1;
						samp[idx].repeat = 0;
						samp[idx].sampsToPlay = 0;
						samp[idx].sample = NULL;
					}
				}
			} else if (ss.loop == 1 && ss.count > 1) {
				for (idx=0; idx < nAudioPorts; idx++) {
					if (samp[idx].id == ss.id) {
						treated = 1;
						samp[idx].repeat = ss.repeat;
						samp[idx].sampsToPlay = (long)ss.soundSize[ss.pitch];
						samp[idx].sampsPlayed = 0;
						samp[idx].sample = ss.soundData[ss.pitch];
					}
				}
			}
			if (!treated) {
				for (idx=0; idx < nAudioPorts; idx++) {
					nextPort = (nextPort+1) % nAudioPorts;
					if (samp[nextPort].repeat == 0)
						break;
				}
				samp[nextPort].sample = ss.soundData[ss.pitch];
				samp[nextPort].sampsToPlay = (long)ss.soundSize[ss.pitch];
				samp[nextPort].sampsPlayed = 0;
				samp[nextPort].repeat = ss.repeat;
				samp[nextPort].id = ss.id;
			}
		} else if (idx < 0)
			(void) fprintf(stderr, "panic: input poll failed: %s\n", strerror(errno));

		nSounds = 0;
		nap = 0;

		for (idx=0; idx < nAudioPorts; idx++) {
			if (samp[idx].sampsToPlay > 0) {
				nSounds++;
				if (ALgetfilled(ap[idx]) > 4000) {
					nap++;
					continue;
				}
			}
			if (samp[idx].sampsToPlay >= maxSampsPerPass) {
				(void) ALwritesamps(ap[idx],
						    samp[idx].sample + samp[idx].sampsPlayed, maxSampsPerPass);
				samp[idx].sampsPlayed += maxSampsPerPass;
				samp[idx].sampsToPlay -= maxSampsPerPass;

			} else if (samp[idx].sampsToPlay > 0) {
				if ((samp[idx].sampsToPlay%2) == 1) {
					samp[idx].sampsToPlay -= 1;
					samp[idx].sampsPlayed += 1;
				}
				if (samp[idx].sampsToPlay > 0)
				    (void) ALwritesamps(ap[idx],
							samp[idx].sample+samp[idx].sampsPlayed, samp[idx].sampsToPlay);

				if (samp[idx].repeat) {
					sampCount = maxSampsPerPass - samp[idx].sampsToPlay;
					samp[idx].sampsToPlay += samp[idx].sampsPlayed - sampCount;
					samp[idx].sampsPlayed = sampCount;
					(void) ALwritesamps(ap[idx], samp[idx].sample, sampCount);
				} else
					samp[idx].sampsToPlay = 0;
			}
		}
		if (nap == nSounds)
			sginap(1);
	}
	return;
}

/*
 * Open an audio file, check for recognized type, and read.
 */
static short *sfxLoadAudioFile(char *dirName, char *fileName, AFframecount *size) {
	int fd;
	int sampleWidth;
	int sampleFmt;
	char *name;
	short *data;
	AFfilehandle file;

	if (!(name = malloc(strlen(dirName)+strlen(fileName)+2)))
		return NULL;

	(void) sprintf(name, "%s/%s", dirName, fileName);

	if ((fd=open(name, O_RDONLY)) < 0) {
		(void) fprintf(stderr, "Could not open audio file `%s': %s\n", name, strerror(errno));
		free(name);
		return NULL;
	}

	switch(AFidentifyfd(fd)) {
	  case AF_FILE_AIFF:
	  case AF_FILE_AIFFC:
		break;

	  default:
		(void) fprintf(stderr, "%s: unrecognized file type -- convert to AIFF or AIFC\n", name);
		free(name);
		return NULL;
	}
	file = AFopenfd(fd, "r", AF_NULL_FILESETUP);

	if (file == AF_NULL_FILEHANDLE) {
		fprintf(stderr, "%s: failed to attach an audio file struct\n", name);
		free(name);
		return NULL;
	}

	if ((int)AFgetchannels(file, AF_DEFAULT_TRACK) != 2) {
		fprintf(stderr, "%s: does not have 2 channels\n", name);
		free(name);
		AFclosefile(file);
		return NULL;
	}

	if (AFgetrate(file, AF_DEFAULT_TRACK) != 16000) {
		fprintf(stderr, "%s: is not recorded at 16 KHz\n", name);
		free(name);
		AFclosefile(file);
		return NULL;
	}

	AFgetsampfmt(file, AF_DEFAULT_TRACK, &sampleFmt, &sampleWidth);
	if (sampleWidth != 16) {
		fprintf(stderr, "%s: is not recorded with 16 bit samples\n", name);
		free(name);
		AFclosefile(file);
		return NULL;
	}
	free(name);

	*size = AFgetframecnt(file, AF_DEFAULT_TRACK);

	if ((data=(short *)malloc((size_t)(*size * 2 * sizeof(short)))) == NULL) {
		(void) fprintf(stderr, "%s: out of memory for %lld samples\n", name, *size);
		*size = 0;
		AFclosefile(file);
		return NULL;
	}

	if (AFreadframes(file, AF_DEFAULT_TRACK, data, (int)*size) != (int)*size) {
		(void) fprintf(stderr, "%s: error reading\n", name);
		free(data);
		data = NULL;
		*size = 0L;
	}

	*size *= 2L;
	AFclosefile(file);
	return data;
}

#elif defined(HAVE_SDL)
/*
 * Build the path to the sound files 
 */
static char *DATAFILE(const char* wav) {
	static char buf[PATH_MAX];

	(void) snprintf(buf, sizeof buf, "%s/%s", sounddir, wav);
	return (char *)buf;
} 

/*
 * Load the .wave files into the sounds array
 */
static int loadSounds(void) {
	int i;

	sounds[CLOAK_WAV] = Mix_LoadWAV(DATAFILE("nt_cloaked.wav"));
	sounds[ENTER_SHIP_WAV] = Mix_LoadWAV(DATAFILE("nt_enter_ship.wav"));
	sounds[EXPLOSION_WAV] = Mix_LoadWAV(DATAFILE("nt_explosion.wav"));
	sounds[SBEXPLOSION_WAV] = Mix_LoadWAV(DATAFILE("nt_sbexplosion.wav"));
	sounds[EXPLOSION_OTHER_WAV] = Mix_LoadWAV(DATAFILE("nt_explosion_other.wav"));
	sounds[FIRE_PLASMA_WAV] = Mix_LoadWAV(DATAFILE("nt_fire_plasma.wav"));
	sounds[FIRE_TORP_WAV] = Mix_LoadWAV(DATAFILE("nt_fire_torp.wav"));
	sounds[FIRE_TORP_OTHER_WAV] = Mix_LoadWAV(DATAFILE("nt_fire_torp_other.wav"));
	sounds[INTRO_WAV] = Mix_LoadWAV(DATAFILE("nt_intro.wav"));
	sounds[MESSAGE_WAV] = Mix_LoadWAV(DATAFILE("nt_message.wav"));
	sounds[PHASER_WAV] = Mix_LoadWAV(DATAFILE("nt_phaser.wav"));
	sounds[PHASER_OTHER_WAV] = Mix_LoadWAV(DATAFILE("nt_phaser_other.wav"));
	sounds[PLASMA_HIT_WAV] = Mix_LoadWAV(DATAFILE("nt_plasma_hit.wav"));
	sounds[SELF_DESTRUCT_WAV] = Mix_LoadWAV(DATAFILE("nt_self_destruct.wav"));
	sounds[SHIELD_DOWN_WAV] = Mix_LoadWAV(DATAFILE("nt_shield_down.wav"));
	sounds[SHIELD_UP_WAV] = Mix_LoadWAV(DATAFILE("nt_shield_up.wav"));
	sounds[TORP_HIT_WAV] = Mix_LoadWAV(DATAFILE("nt_torp_hit.wav"));
	sounds[UNCLOAK_WAV] = Mix_LoadWAV(DATAFILE("nt_uncloak.wav"));
	sounds[REDALERT_WAV] = Mix_LoadWAV(DATAFILE("nt_warning.wav"));

#ifdef SOUND_WARN_MISSING
	for (i=0; i < NUM_WAVES; i++) {
		if (!sounds[i]) {
			(void) fprintf(stderr, "Mix_LoadWAV sound[%d] could not be loaded.\n"
						"Check soundDir in your .netrekrc: %s\n", i, Mix_GetError());
		}
	}
#endif
	return 1;
}

void sound_cleanup (void) {
	int i;

	/* Free the sound effects */
	for (i = 0; i < NUM_WAVES; i++)
		Mix_FreeChunk(sounds[i]);

	/* Quit SDL_mixer */
	Mix_CloseAudio();

	/* Quit SDL */	/* Oh boy, what a meaningful comment! */
	SDL_Quit();
}
#endif /* HAVE_SDL */

void Exit_Sound(void) {

#if defined(sgi)
	if (sound_init)
		sfxEnd(1);

#elif !defined(HAVE_SDL)
	if (sound_init)
		ExitSound();
#endif
	sound_init = 0;
	sound_toggle = 0;
}

void Init_Sound(void) {
	char *sd;

#if defined(sgi)
	int err;
#else
	char buf[PATH_MAX];
#endif

#ifdef DEBUG
	printf("Init_Sound\n");
#endif
	/*
	 * If sound_init is on in the .xtrekrc file (set in defaults.c)
	 * look for sounds in .xtrekrc sounddir parameter. If that fails
	 * look for an environment variable called SOUNDDIR and if that
	 * fails, try to open the hardcoded sound directory.
	 */
	if (sound_init) {
		if ((sounddir = getdefault("sounddir")) == NULL) {
			if ((sd=getenv("SOUNDDIR")) != NULL)
				sounddir = strdup(sd);
			else
#if defined(sgi)
				sounddir = "/usr/local/games/netrek-sgi/sounds";
#else
				sounddir = "/usr/share/sounds/netrek-client-cow";
#endif
		}
		if (!isDirectory(sounddir)) {
			sounddir = "sounds";
				if (!isDirectory(sounddir)) {
					(void) fprintf(stderr, "sound directory missing\n", sounddir);
					return;
			}
		}
	}
#if defined(sgi)
	err = sfxInit(sounddir, 3);			/* initialize up to three audio ports */
	if (err == SFX_ERR_NO_PORTS_AVAIL) {
		(void) fprintf(stderr, "No audio ports available.\n");
		sound_init = 0;
		sound_toggle = 0;
		return;
	}
	if (err == SFX_ERR_NO_SPROC) {
		(void) fprintf(stderr, "Unable to execute sound process.\n");
		sound_init = 0;
		sound_toggle = 0;
		return;
	}
	if (err == SFX_ERR_NO_MEM) {
		(void) fprintf(stderr, "No memory available for sound data.\n");
		sound_init = 0;
		sound_toggle = 0;
		return;
	}
	if (err > 0) {				/* load mandatory sounds f we got at least one audio port */
		sounds[FIRE_TORP_WAV] = sfxLoad("fire_torp.aiff");
		sounds[PHASER_WAV] = sfxLoad("phaser.aiff");
		sounds[FIRE_PLASMA_WAV] = sfxLoad("fire_plasma.aiff");
		sounds[EXPLOSION_WAV] = sfxLoad("explosion.aiff");
		sounds[FIRE_TORP_OTHER_WAV] = sfxLoad("fire_torp_other.aiff");
		sounds[PHASER_OTHER_WAV] = sfxLoad("phaser_other.aiff");
		sounds[FIRE_PLASMA_OTHER_WAV] = sfxLoad("fire_plasma_other.aiff");
		sounds[EXPLOSION_OTHER_WAV] = sfxLoad("explosion_other.aiff");
		sounds[PLASMA_HIT_WAV] = sfxLoad("plasma_hit.aiff");
		sounds[TORP_HIT_WAV] = sfxLoad("torp_hit.aiff");

		if (err > 1) {			/* load optional sounds only if we got two audio ports */
			sounds[CLOAK_WAV] = sfxLoad("cloak.aiff");
			sounds[UNCLOAK_WAV] = sfxLoad("cloak.aiff");
			sounds[SHIELD_DOWN_WAV] = sfxLoad("shield_down.aiff");
			sounds[SHIELD_UP_WAV] = sfxLoad("shield_up.aiff");
			sounds[REDALERT_WAV] = sfxLoad("klaxon.aiff");
			sounds[INTRO_WAV] = sfxLoad("paradise.aiff");
			sounds[MESSAGE_WAV] = sfxLoad("message.aiff");

			/* load sound loops only if we got three audio ports */
			if (err > 2) {
				sounds[THERMAL_WAV] = sfxLoad("thermal_warn.aiff");
				sounds[ENTER_SHIP_WAV] = sfxLoad("enter_ship.aiff");
				sounds[SELF_DESTRUCT_WAV] = sfxLoad("self_destruct.aiff");

				if ((sounds[ENGINE_WAV] = sfxLoad("bridge.aiff")) != NULL) {
					sfxLoop(sounds[ENGINE_WAV]);
					sfxPitchBend(sounds[ENGINE_WAV], 0.0f, 1.0f, 1.0f, 2.0f, 1.1f, 20);
				}
			}
		}
		sfxPlay(sounds[INTRO_WAV]);
	}

#elif defined(HAVE_SDL)
#ifdef DEBUG
	printf("Init_Sound using SDL\n");
#endif

	/* Initialize the SDL library */
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
	  fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
	}
	atexit(SDL_Quit);

	/* Open the audio device at 8000 Hz 8 bit Microsoft PCM */
	if (Mix_OpenAudio(8000, AUDIO_U8, 1, 512) < 0) {
	  fprintf(stderr,"Mix_OpenAudio: %s\n", Mix_GetError());
	  sound_init = 0;
	}

        Mix_AllocateChannels(16);

	/* If we successfully loaded the wav files, so shut-off
	   sound_init and play the introduction */
	if (loadSounds()) {
	  if (sounds[INTRO_WAV])
	    if (Mix_PlayChannel(-1, sounds[INTRO_WAV], 0) < 0) {
	      fprintf(stderr, "Mix_PlayChannel: %s\n", Mix_GetError());
	    }
	}
#else
	if (InitSound() == -1) {
	  sound_toggle = 0;
	  sound_init = 0;
	} else {
	  sound_init = 1;
	  sound_toggle = 1;
	}

	strcpy(sound_prefix, sounddir);
	strcat(sound_prefix, "/");

	if (sound_toggle) {
	  strcpy(buf, sounddir);
	  strcat(buf, "/nt_intro");
	  StartSound(buf);
	}
#endif
}

#if defined(HAVE_SDL) || defined(sgi)
static int sound2wave[NUM_SOUNDS+1] = {
	/* NO_SOUND */			-1,
	/* FIRE_TORP_SOUND */		FIRE_TORP_WAV,
	/* PHASER_SOUND */		PHASER_WAV,
	/* FIRE_PLASMA_SOUND */		FIRE_PLASMA_WAV,
	/* EXPLOSION_SOUND */		EXPLOSION_WAV,
	/* SBEXPLOSION_SOUND */		SBEXPLOSION_WAV,
	/* CLOAK_SOUND */		CLOAK_WAV,
	/* UNCLOAK_SOUND */		UNCLOAK_WAV,
	/* SHIELD_DOWN_SOUND */		SHIELD_DOWN_WAV,
	/* SHIELD_UP_SOUND */		SHIELD_UP_WAV,
	/* TORP_HIT_SOUND */		TORP_HIT_WAV,
	/* REDALERT_SOUND */		REDALERT_WAV,
	/* BUZZER_SOUND */		BUZZER_WAV,
#if defined(sgi)
	/* ENGINE_SOUND */		ENGINE_WAV,
#else
	/* text in soundrefresh() says engine sound is not supported
	 * so we'll disable it although there is an ENGINE_WAV */
	/* ENGINE_SOUND */		-1,
#endif
	/* THERMAL_SOUND */		THERMAL_WAV,
	/* ENTER_SHIP_SOUND */		ENTER_SHIP_WAV,
	/* SELF_DESTRUCT_SOUND */	SELF_DESTRUCT_WAV,
	/* PLASMA_HIT_SOUND */		PLASMA_HIT_WAV,
	/* MESSAGE_SOUND */		MESSAGE_WAV,
	/* MESSAGE1_SOUND */		MESSAGE_WAV,
	/* MESSAGE2_SOUND */		MESSAGE_WAV,
	/* MESSAGE3_SOUND */		MESSAGE_WAV,
	/* MESSAGE4_SOUND */		MESSAGE_WAV,
	/* MESSAGE5_SOUND */		MESSAGE_WAV,
	/* MESSAGE6_SOUND */		MESSAGE_WAV,
	/* MESSAGE7_SOUND */		MESSAGE_WAV,
	/* MESSAGE8_SOUND */		MESSAGE_WAV,
	/* MESSAGE9_SOUND */		MESSAGE_WAV,
	/* OTHER_FIRE_TORP_SOUND */	FIRE_TORP_OTHER_WAV,
	/* OTHER_PHASER_SOUND */	PHASER_OTHER_WAV,
	/* OTHER_FIRE_PLASMA_SOUND */	FIRE_PLASMA_OTHER_WAV,
	/* OTHER_EXPLOSION_SOUND */	EXPLOSION_OTHER_WAV,
	/* OTHER_SBEXPLOSION_SOUND */	SBEXPLOSION_OTHER_WAV
};
#endif

#if defined(HAVE_SDL) || defined(sgi)
static int get_waveform(int type) {
	int waveform;

	if (type <= 0 || type > NUM_SOUNDS) {			/* check sound index */
#ifdef DEBUG
		(void) fprintf(stderr, "panic: non-existent sound number: %i\n", type);
#endif
		return -1;
	}

	if ((waveform = sound2wave[type]) >= NUM_WAVES) {	/* check waveform index */
#ifdef DEBUG
		(void) fprintf(stderr, "panic: non-existent waveform number: %i\n", waveform);
#endif
		return -1;
	}
	return waveform;
}
#endif

void Play_Sound(int type) {
#if defined(HAVE_SDL) || defined(sgi)
	int waveform;

	if (!sound_init || (waveform=get_waveform(type)) < 0)
		return;

#if defined(sgi)
	if (type > OTHER_SOUND_OFFSET && !soundOther)
		return;

	sfxPlay(sounds[waveform]);

#else
	if (sounds[waveform])
	  if (Mix_PlayChannel(-1, sounds[waveform], 0) < 0) {
	    (void) fprintf(stderr, "Mix_PlayChannel: %s\n", Mix_GetError());
	  }
#endif

#else
	char buf[PATH_MAX];

	/* Don't play other ship's sounds if turned off */
	if (type > OTHER_SOUND_OFFSET && !soundOther)
		return;

	if (sound_toggle && sounds[type].flag &&
	   ((sounds[type].priority >= sounds[current_sound].priority) || !SoundPlaying())) {
		STRNCPY(buf, sound_prefix, PATH_MAX);
		strcat(buf, sounds[type].name);
		StartSound(buf);
		current_sound = type;
	}
	if (!(sound_toggle))
		current_sound = NO_SOUND;
#endif
}

#if !defined(sgi)	/* avoid calling null functions at all */
void Abort_Sound(int type) {
#if !defined(HAVE_SDL)
	if ((current_sound != NO_SOUND) && (type == current_sound))
		StopSound();
#endif
}
#endif

#if defined(sgi)
/*
 * Play a pitch-bended engine sound loop while player is alive.
 */
void Engine_Sound(int speed, int maxspeed) {
	static int engine_on = 0;
	static float opitch = 0.0f;
	float pitch;

	if (!sound_init)
		return;

	if ((speed < 0) && (maxspeed < 0)) {	/* stop sound */
		sfxDisable(sounds[ENGINE_WAV]);
		engine_on = 0;
		opitch = 0.0f;
		return;
	}
	if (speed == 0)
		pitch = 0.0f;
	else	pitch = 0.25f * (float)speed * (1.0f / (float)maxspeed);

	if (!engine_on || opitch != pitch) {
		sfxEnable(sounds[ENGINE_WAV]);
		sfxPlayPitch(sounds[ENGINE_WAV], pitch);
		engine_on = 1;
		opitch = pitch;
	}
	return;
}
#endif

/* Sound options window stuff */

#define SOUND_TOGGLE  0
#define SOUND_OTHER   MESSAGE_SOUND + 1
#define SOUND_INIT    MESSAGE_SOUND + 2
#define SOUND_DONE    MESSAGE_SOUND + 3

static void soundrefresh(int i);

int sound_window_height(void) {
#if defined(HAVE_SDL)
	return 1;
#else
	return MESSAGE_SOUND + 4;
#endif
}

void soundwindow(void) {
#if defined(HAVE_SDL)
	char *buf="All or nothing with SDL sound. Sorry";
	W_WriteText(soundWin, 0, 0, textColor, buf, strlen(buf), 0);
#else
	int     i;

	for (i=0; i <= SOUND_DONE; i++)
		soundrefresh(i);
#endif

	/* Map window */
	W_MapWindow(soundWin);
}

#if !defined(HAVE_SDL)
static void soundrefresh(int i) {
	char buf[200], *flag;

	if (i == SOUND_TOGGLE) {
		(void) snprintf(buf, sizeof buf, "Sound is turned %s", (sound_toggle == 1) ? "ON" : "OFF");
	} else if (i < SOUND_OTHER) {
#if defined(sgi)
		flag = (sfxIsEnabled(sounds[i]) ? "ON" : "OFF");
#else
		flag = ((sounds[i].flag == 1) ? "ON" : "OFF");
#endif
		switch (i) {
		  case FIRE_TORP_SOUND:
			(void) snprintf(buf, sizeof buf, "Fire torp sound is %s", flag);
			break;

		  case PHASER_SOUND:
			(void) snprintf(buf, sizeof buf, "Phaser sound is %s", flag);
			break;

		  case FIRE_PLASMA_SOUND:
			(void) snprintf(buf, sizeof buf, "Fire plasma sound is %s", flag);
			break;

		  case EXPLOSION_SOUND:
			(void) snprintf(buf, sizeof buf, "Explosion sound is %s", flag);
			break;

		  case CLOAK_SOUND:
			(void) snprintf(buf, sizeof buf, "Cloak sound is %s", flag);
			break;

		  case UNCLOAK_SOUND:
			(void) snprintf(buf, sizeof buf, "Uncloak sound is %s", flag);
			break;

		  case SHIELD_DOWN_SOUND:
			(void) snprintf(buf, sizeof buf, "Shield down  sound is %s", flag);
			break;

		  case SHIELD_UP_SOUND:
			(void) snprintf(buf, sizeof buf, "Shield up sound is %s", flag);
			break;

		  case TORP_HIT_SOUND:
			(void) snprintf(buf, sizeof buf, "Torp hit sound is %s", flag);
			break;

		  case REDALERT_SOUND:
			(void) snprintf(buf, sizeof buf, "Red alert sound is %s", flag);
			break;

		  case ENGINE_SOUND:
#if defined(sgi)
			(void) snprintf(buf, sizeof buf, "Engine sound is %s", flag);
#else
			(void) snprintf(buf, sizeof buf, "Engine sound is not supported");
#endif
			break;

		  case ENTER_SHIP_SOUND:
			(void) snprintf(buf, sizeof buf, "Enter ship sound is %s", flag);
			break;

		  case SELF_DESTRUCT_SOUND:
			(void) snprintf(buf, sizeof buf, "Self destruct sound is %s", flag);
			break;

		  case PLASMA_HIT_SOUND:
			(void) snprintf(buf, sizeof buf, "Plasma hit sound is %s", flag);
			break;

		  case MESSAGE_SOUND:
			(void) snprintf(buf, sizeof buf, "Message sound is %s", flag);
			break;
		}

	} else if (i == SOUND_OTHER)
		(void) snprintf(buf, sizeof buf, "Other ship's sound is %s", soundOther == 1 ? "ON" : "OFF");
#if defined(sgi)
	else if (i == SOUND_INIT)
		(void) snprintf(buf, sizeof buf, sound_init ? "Reset audio hardware"
							   : "Initialize audio hardware");
#else
	else if (i == SOUND_INIT)
		(void) snprintf(buf, sizeof buf, sound_init ? "Restart external sound player"
							   : "Initialize external sound player");
#endif
	else if (i == SOUND_DONE)
		(void) snprintf(buf, sizeof buf, "Done");
	else
		(void) fprintf(stderr, "Uh oh, bogus refresh number in soundrefresh\n");

	W_WriteText(soundWin, 0, i, textColor, buf, strlen(buf), 0);
}
#endif /* HAVE_SDL */

void soundaction(W_Event * data) {
#if defined(sgi)
	printf("sorry, not yet implemented\n");

#elif !defined(HAVE_SDL)
  int     i, j;

  i = data->y;

  if (i == SOUND_TOGGLE) {
      if (sound_init)
	sound_toggle = (sound_toggle == 1) ? 0 : 1;
      soundrefresh(SOUND_TOGGLE);
      /* text in soundrefresh() says engine sound is not supported
      if (!sound_toggle) {
	  Abort_Sound(ENGINE_SOUND);
      } else {
	  Play_Sound(ENGINE_SOUND);
      }
      */
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
      soundOther = !soundOther;
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
#endif /* HAVE_SDL */
}

void sounddone(void) {

	W_UnmapWindow(soundWin);
	return;
}

#endif /* SOUND */
