/*
   Background Sound Library, 32 bit Windows Version

   Interface based on the (Unix) Background Sound Library:
   
   "Copyright 1993 by Kurt Siegl <007@netrek.org>
   Permission to use, modify, copy and distribute this software without
   fee is hereby granted as long as this notice is left here."

   by Jonathan Shekter, June 1995 

   At first I went the hideously easy route of using the Windows sndPlaySound()
   call. This made the whole thing a 40 line file, everything was just a wrapper.
   Sadly this didn't give the performance I expected; there was a noticable
   pause as the file was loaded. So this code implements an LRU cache of sounds.
 * 
 * $Log: wsl.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:12  siegl
 * COW 3.0 initial revision
 * */

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <string.h>
#include "audio.h"


/* Currently open device ID */
static HWAVEOUT hw=0;

/* Linked list of loaded sounds */
struct sound
   {
   char name[PATH_MAX];
   WAVEHDR hdr;
   PCMWAVEFORMAT fmt;
   struct sound *newer,*older;
   };
struct sound *newest, *oldest, *current;
int bytesused;

/* Sound cache size */
#define MAXBYTES (256*1024)


/* Here we use the Windows multimedia file io (mmio) library which makes reading
   RIFF waveform files really easy */
int ParseSoundFile(char *fname, PCMWAVEFORMAT *header, DWORD *len, char **data)
{
   HMMIO       hmmio;                    /* file handle for open file */
   MMCKINFO    mmckinfoParent;  /* parent chunk information structure */
   MMCKINFO    mmckinfoSubchunk; /* subchunk information structure    */
   char        fname2[PATH_MAX];

   int flen = strlen (fname);
   if (fname[flen-4] != '.')  // check for ext on filename
      {
      strcpy(fname2, fname);
      strcat(fname2, ".wav");
      fname = fname2;
      }
   hmmio = mmioOpen(fname, NULL, MMIO_READ | MMIO_ALLOCBUF);

   if(!hmmio)
      {
#ifdef DEBUG         
      fprintf(stderr, "Could not open wave file %s\n", fname);
#endif      
      return 0;
      }

   /* Find RIFF chunk "WAVE" */
   mmckinfoParent.fccType = mmioFOURCC('W', 'A', 'V', 'E');
   if (mmioDescend(hmmio, (LPMMCKINFO) &mmckinfoParent, NULL, MMIO_FINDRIFF))
      {
      fprintf(stderr, "\"Wave\" file has no WAVE chunk!\n");
      return 0;
      mmioClose(hmmio, 0);
      return 0;
      }

   /* Find format chunk */
   mmckinfoSubchunk.ckid = mmioFOURCC('f', 'm', 't', ' ');
   if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoParent, MMIO_FINDCHUNK))
      {
      fprintf(stderr, "\"Wave\" file has no fmt sub-chunk!\n");
      mmioClose(hmmio, 0);
      return 0;
      }

   /* Read PCM header */
   if (mmckinfoSubchunk.cksize != sizeof(PCMWAVEFORMAT))
      {
      fprintf(stderr, "PCMWAVEFORMAT chunk is the wrong size.\n");
      mmioClose(hmmio, 0);
      return 0;
      }
      
   if (mmioRead(hmmio, header, sizeof(PCMWAVEFORMAT)) != sizeof(PCMWAVEFORMAT))
      {
      fprintf(stderr, "Failed to read wave file header\n");
      mmioClose(hmmio, 0);
      return 0;
      }
      
    /* Ascend out of the "fmt " subchunk. */
    mmioAscend(hmmio, &mmckinfoSubchunk, 0);

    mmckinfoSubchunk.ckid = mmioFOURCC('d', 'a', 't', 'a');
    if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoParent, MMIO_FINDCHUNK))
      {
      fprintf(stderr, "\"Wave\" file has no data sub-chunk!\n");
      mmioClose(hmmio, 0);
      return 0;
      }

    /* Get the size of the data subchunk and allocate */
    *len = mmckinfoSubchunk.cksize;
    *data = (char *) malloc(len);

   if (mmioRead(hmmio, *data, *len) != *len)
      {
      fprintf(stderr, "Failed to read wave file data\n");
      free(*data);
      mmioClose(hmmio, 0);
      return 0;
      }

   mmioClose(hmmio, 0);
   return 1;
}


/* Finds a sound in the cache or loads it. */
struct sound *GetSound(char *name)
{
#ifdef DEBUG
   int loopcount=0;      
#endif         
   struct sound *itr = newest, *newsnd;

#ifdef DEBUG         
   printf("Searching for sound %s in cache\n", name);
#endif

   while (itr)
      {
#ifdef DEBUG
      if (loopcount >0 && itr == newest)
         {
         printf("Sound list is circular with %d items!\n", loopcount);
         return 0;
         }
         
      if (loopcount++ > 100)
         {
         printf("Stuck in GetSound loop!\n");
         return 0;
         }

   printf("Found %s in cache\n", itr->name);
#endif
      
      if (!strcmp(name ,itr->name)) 
         {                     /* If found, move to front of list */
         if (itr!= newest)
            {
            if (itr->older)
               itr->older->newer = itr->newer;
            else
               oldest = itr->newer;
            itr->newer->older = itr->older;
               
            itr->older = newest;
            itr->newer=0;
            
            newest = itr;
            }
         return itr;
         }
         
      itr = itr->older;
      }

   /* Sound not in cache, must load */
#ifdef DEBUG         
   printf("Sound %s not in cache, loading\n", name);
#endif

   newsnd = (struct sound *) malloc (sizeof(struct sound));
   memset(newsnd, 0, sizeof(struct sound));

   if (ParseSoundFile(name, &newsnd->fmt, &newsnd->hdr.dwBufferLength, &newsnd->hdr.lpData))
      {
      bytesused += newsnd->hdr.dwBufferLength;  

      /* trim cache down if too large, starting from oldest */      
      itr = oldest;
      while (bytesused > MAXBYTES && itr)
         {
         struct sound *next = itr->newer;
         bytesused -= itr->hdr.dwBufferLength;
#ifdef DEBUG
         printf("Trimming sound cache, file %s, %d bytes\n", itr->name, itr->hdr.dwBufferLength);
#endif
         if (next)           /* Remove from list, simple as always at end */
            {
            next->older = 0;
            oldest = next;
            }
         else
            newest = 0;   

         free(itr->hdr.lpData);
         free(itr);
         oldest = itr = next;
         }
         
      strcpy(newsnd->name, name);
      newsnd->older = newest;   /* insert into list */
      newsnd->newer = 0;
      if (newest)
         newest->newer = newsnd;
      else
         oldest = newsnd;
      newest = newsnd;

      return newest;
      }

   free(newsnd);
   return 0;
}

void ExitSound ()
{
  if (hw)
   {
   if (soundPlaying())
      StopSound();
   waveOutClose(hw);
   }
   hw = NULL;

   /* Delete all the sounds in the cache */
   while (newest)
      {
      struct sound *tmp = newest->older;
      free(newest->hdr.lpData);
      free(newest);
#ifdef DEBUG
      /* Avoid circular lists */
      newest->older=0;
#endif      
      }
   oldest=newest=NULL;
}

int InitSound ()
{
   return 0;
}

void StopSound ()
{
   if (soundPlaying())
      {
#ifdef DEBUG
      printf("Interuppting sound\n");
#endif               
      waveOutReset(hw);
      waveOutUnprepareHeader(hw, &current->hdr, sizeof(WAVEHDR));
      current = NULL;   
      }
}

int StartSound (char *name)
{
   static PCMWAVEFORMAT lastfmt;
   struct sound *snd;

#ifdef DEBUG
   printf("Request to play sound %s\n", name);
#endif      
   stopSound();
   
   if (snd=GetSound(name))
      {
      current = snd;

      if (!hw)
         {
#ifdef DEBUG
   printf("Initial open of wave device\n");
#endif                  
         waveOutOpen(&hw, 0, (WAVEFORMAT *)&snd->fmt, NULL, 0, 0);
         memcpy(&lastfmt, &snd->fmt, sizeof(PCMWAVEFORMAT));         
         }
      else /*if (memcmp(&snd->fmt, &lastfmt, sizeof(PCMWAVEFORMAT)))
         /* Re-use the currently open sound handle of the formats are the same */
         {
#ifdef DEBUG
   printf("Re-open of wave device\n");
#endif   
         printf("close: %d, ", waveOutClose(hw));
         memcpy(&lastfmt, &snd->fmt, sizeof(PCMWAVEFORMAT));
         printf("open %d\n",waveOutOpen(&hw, 0, (WAVEFORMAT *)&snd->fmt, NULL, 0, 0));
         }
#ifdef DEBUG            
//      else
//         printf("Re-using open sound handle\n");
#endif
      printf("prepare header: %\d, ", waveOutPrepareHeader(hw, &snd->hdr, sizeof(WAVEHDR)));
      printf("write: %d\n", waveOutWrite(hw, &snd->hdr, sizeof(WAVEHDR)));


      return 0;
      }
   return -1;
}

int SoundPlaying()
{
   int playing;

#ifdef DEBUG
   printf("playing = ...");
#endif      
   if (!hw || !current)
      return 0;
      
   playing = !(current->hdr.dwFlags & WHDR_DONE);
   if (!playing)
      {
      printf(" (unprepare: %d) ",waveOutUnprepareHeader(hw, &current->hdr, sizeof(WAVEHDR)));
      current = NULL;
      }
#ifdef DEBUG
      printf("%d\n", playing);
#endif               
  return playing;
}
