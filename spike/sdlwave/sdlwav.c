#include "SDL.h"
#include "SDL_thread.h"
#include "SDL_mixer.h"

#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Sint16 stream[2][4096];
int len=4096, done=0, need_refresh=0, bits=0, which=0;
SDL_Surface *s=NULL;
Uint32 flips=0;
Uint32 black,white;
float dy,dx;

/******************************************************************************/
/* some simple exit and error routines                                        */

void errorv(char *str, va_list ap)
{
	vfprintf(stderr,str,ap);
	fprintf(stderr,": %s.\n", SDL_GetError());
}

void cleanExit(char *str,...)
{
	va_list ap;
	va_start(ap, str);
	errorv(str,ap);
	va_end(ap);
	Mix_CloseAudio();
	SDL_Quit();
	exit(1);
}

/******************************************************************************/
/* the postmix processor, only copies the stream buffer and indicates         */
/* a need for a screen refresh                                                */

static void postmix(void *udata, Uint8 *_stream, int _len)
{
	// save the stream buffer and indicate that we need a redraw
	len=_len;
	memcpy(stream[(which+1)%2],_stream,len>s->w*4?s->w*4:len);
	which=(which+1)%2;
	need_refresh=1;
}

/******************************************************************************/
/* redraw the wav and reset the need_refresh indicator                        */

void refresh()
{
	int x,y,Y;
	Sint16 *buf;

	buf=stream[which];
	need_refresh=0;
	
	// clear the screen
	SDL_FillRect(s,NULL,black);

	// draw the wav from the saved stream buffer
	Y=s->h/4;
	for(x=0;x<s->w*2;x++)
	{
		y=(buf[x]*dy);
		{
			if(y<0)
			{
				SDL_Rect r={x/2,Y+y,1,-y};
				SDL_FillRect(s,&r,white);
			}
			else
			{
				SDL_Rect r={x/2,Y,1,y};
				SDL_FillRect(s,&r,white);
			}
		}
		Y=Y>s->h/2?s->h/4:s->h*3/4;
	}
	SDL_Flip(s);
	flips++;
}

/******************************************************************************/

int main(int argc, char **argv)
{
	int audio_rate,audio_channels,
		// set this to any of 512,1024,2048,4096
		// the higher it is, the more FPS shown and CPU needed
		audio_buffers=512;
	Uint16 audio_format;
	Uint32 t;
	Mix_Music *music;
	int volume=SDL_MIX_MAXVOLUME;

	// initialize SDL for audio and video
	if(SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO)<0)
		cleanExit("SDL_Init");

	// open a screen for the wav output
	//if(!(s=SDL_SetVideoMode(1024,768,0,SDL_FULLSCREEN|SDL_ANYFORMAT|SDL_HWSURFACE|SDL_DOUBLEBUF)))
	if(!(s=SDL_SetVideoMode(512,512,0,SDL_ANYFORMAT|SDL_DOUBLEBUF)))
		cleanExit("SDL_SetVideoMode");
	SDL_WM_SetCaption("sdlwav - SDL_mixer demo","sdlwav");
	
	// hide the annoying mouse pointer
	SDL_ShowCursor(SDL_DISABLE);
	// get the colors we use
	white=SDL_MapRGB(s->format,0xff,0xff,0xff);
	black=SDL_MapRGB(s->format,0,0,0);
	
	// initialize sdl mixer, open up the audio device
	if(Mix_OpenAudio(44100,MIX_DEFAULT_FORMAT,2,audio_buffers)<0)
		cleanExit("Mix_OpenAudio");

	// print out some info on the audio device and stream
	Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
	bits=audio_format&0xFF;
	printf("Opened audio at %d Hz %d bit %s, %d bytes audio buffer\n", audio_rate,
			bits, audio_channels>1?"stereo":"mono", audio_buffers );

	// calculate some parameters for the wav display
	dy=s->h/2.0/(float)(0x1<<bits);
	dx=s->w/(float)(0x1<<bits);
	
	// load the song
	if(!(music=Mix_LoadMUS(argv[1])))
		cleanExit("Mix_LoadMUS(\"%s\")",argv[1]);
	
	// set the post mix processor up
	Mix_SetPostMix(postmix,argv[1]);
	
	// start playing and displaying the wav
	// wait for escape key of the quit event to finish
	t=SDL_GetTicks();
	if(Mix_PlayMusic(music, 1)==-1)
		cleanExit("Mix_PlayMusic(0x%p,1)",music);
	Mix_VolumeMusic(volume);
	while((Mix_PlayingMusic() || Mix_PausedMusic()) && !done)
	{
		SDL_Event e;
		while(SDL_PollEvent(&e))
		{
			switch(e.type)
			{
				case SDL_KEYDOWN:
					switch(e.key.keysym.sym)
					{
						case SDLK_ESCAPE:
							done=1;
							break;
						case SDLK_LEFT:
							Mix_RewindMusic();
							break;
						case SDLK_RIGHT:
							switch(Mix_GetMusicType(NULL))
							{
								case MUS_MP3:
									Mix_SetMusicPosition(+5);
									break;
								default:
									printf("cannot fast-forward this type of music\n");
									break;
							}
							break;
						case SDLK_UP:
							volume=(volume+1)<<1;
							if(volume>SDL_MIX_MAXVOLUME)
								volume=SDL_MIX_MAXVOLUME;
							Mix_VolumeMusic(volume);
							break;
						case SDLK_DOWN:
							volume>>=1;
							Mix_VolumeMusic(volume);
							break;
						case SDLK_SPACE:
							if(Mix_PausedMusic())
								Mix_ResumeMusic();
							else
								Mix_PauseMusic();
							break;
					}
					break;
				case SDL_QUIT:
					done=1;
					break;
			}
		}
		// the postmix processor tells us when there's new data to draw
		if(need_refresh)
			refresh();
		else
			SDL_Delay(0);
	}
	t=SDL_GetTicks()-t;
	
	// free & close
	Mix_FreeMusic(music);
	Mix_CloseAudio();
	SDL_Quit();
	// show a silly statistic
	printf("fps=%.2f\n",((float)flips)/(t/1000.0));
	return(0);
}
