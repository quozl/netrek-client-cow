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
 * $Log: winsndlib.c,v $
 * Revision 1.2  2001/09/08 13:27:13  siegl
 * Support for state of the art cygwin
 *
 * Revision 1.1.1.1  1998/11/01 17:24:12  siegl
 * COW 3.0 initial revision
 * */

#undef DEBUG

#include "config.h"

#ifdef SOUND
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include INC_LIMITS
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
int ParseSoundFile(char *fname, PCMWAVEFORMAT *header, DWORD *datalen, char **data)
{
   FILE *in;
   char chunk[8],fname2[PATH_MAX];
   DWORD len;
   int chunksread = 0;

   int flen = strlen (fname);
   if (fname[flen-4] != '.')  // check for ext on filename
      {
      strcpy(fname2, fname);
      strcat(fname2, ".wav");
      fname = fname2;
      }
      
   in = fopen(fname, "rb");
   if(!in)
      {
      fprintf(stderr, "Could not open wave file %s\n", fname);
      return 0;
      }

   fread(chunk, 8, 1, in);  /* read signature, and file length  (which we discard) */
#ifdef DEBUG   
   printf("Chunk: %c%c%c%c\n",chunk[0],chunk[1],chunk[2],chunk[3]);
#endif   
   if (strncmp(chunk, "RIFF", 4))
      {
      fprintf(stderr, "%s is not a valid wave file\n", fname);
      return 0;
      }
         
   /* Find RIFF chunk "WAVE". */
   do /* while (chunksread < 2) */
      {
      fread(chunk, 4, 1, in);
      if (feof(in))
         goto NoGood;
#ifdef DEBUG         
      printf("Chunk: %c%c%c%c\n",chunk[0],chunk[1],chunk[2],chunk[3]);
#endif      
      if (!strncmp(chunk, "WAVE", 4))  
         do
            {
            fread(chunk, 4, 1, in);
            fread((char *)&len, 4, 1, in);
            if (feof(in))
               goto NoGood;
#ifdef DEBUG               
            printf("Chunk: %c%c%c%c\n",chunk[0],chunk[1],chunk[2],chunk[3]);
#endif   
            if (!strncmp(chunk, "fmt ", 4))  /* found fmt chunk, read header*/
               {
               if (len != sizeof(PCMWAVEFORMAT))
                  goto NoGood;
               if (fread((char *)header, sizeof(PCMWAVEFORMAT), 1, in) != 1)
                  goto NoGood;
#ifdef DEBUG                  
               printf("Yup gosh read the header\n");
#endif               
               chunksread++;
               }
            else if (!strncmp(chunk, "data", 4)) /* found data chunk, read it */
               {
               *datalen = len;
               *data = (char *) malloc(len);
               if (fread(*data, len, 1, in) != 1)
                  goto NoGood;
#ifdef DEBUG                  
               printf("Read %d bytes of data\n", *datalen);
#endif               
               chunksread++;
               }
            else
               fseek(in, len, SEEK_CUR);  /* unknown chunk, skip */
            }
         while (chunksread < 2);
      }
   while (chunksread < 2);

   fclose(in);
   return 1;

 NoGood:
   fprintf(stderr, "%s is not a valid wave file\n", fname);
   fclose(in);
   return 0;
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

            newest->newer = itr;
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
   if (SoundPlaying())
      StopSound();
   waveOutClose(hw);
   }
   hw = NULL;

   /* Delete all the sounds in the cache */
   while (newest)
      {
      struct sound *tmp = newest->older;
#ifdef DEBUG
      /* Avoid circular lists */
      newest->older=0;
#endif
      free(newest->hdr.lpData);
      free(newest);
      newest = tmp;
      }
   oldest=newest=NULL;
}

int InitSound ()
{
   return 0;
}

void StopSound ()
{
   if (SoundPlaying())
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
   DWORD err;

#ifdef DEBUG
   printf("Request to play sound %s\n", name);
#endif      
   StopSound();
   
   if (snd=GetSound(name))
      {
      current = snd;

      if (!hw)
         {
#ifdef DEBUG
   printf("Initial open of wave device\n");
#endif                  
         if (err=waveOutOpen(&hw, WAVE_MAPPER, (WAVEFORMAT *)&snd->fmt, NULL, 0, 0))
            fprintf(stderr, "Could not open wave device, error %d\n", err);
         memcpy(&lastfmt, &snd->fmt, sizeof(PCMWAVEFORMAT));         
         }
      else if (memcmp(&snd->fmt, &lastfmt, sizeof(PCMWAVEFORMAT)))
         /* Re-use the currently open sound handle of the formats are the same */
         {
#ifdef DEBUG
   printf("Re-open of wave device\n");
#endif   
         waveOutClose(hw);
         memcpy(&lastfmt, &snd->fmt, sizeof(PCMWAVEFORMAT));
         if (err=waveOutOpen(&hw, WAVE_MAPPER, (WAVEFORMAT *)&snd->fmt, NULL, 0, 0))
            fprintf(stderr, "Could not open wave device, error %d\n", err);

         }
#ifdef DEBUG            
      else
         printf("Re-using open sound handle\n");
#endif
      waveOutPrepareHeader(hw, &snd->hdr, sizeof(WAVEHDR));
      waveOutWrite(hw, &snd->hdr, sizeof(WAVEHDR));


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
      waveOutUnprepareHeader(hw, &current->hdr, sizeof(WAVEHDR));
      current = NULL;
      }
#ifdef DEBUG
      printf("%d\n", playing);
#endif               
  return playing;
}

#endif
