#include "config.h"

#include <stdio.h>
#include <time.h>
#include <timeb.h>
#include <ssdef.h>
#include <jpidef.h>
#include <iodef.h>

typedef struct itmlst_str {
  short buffer_length;
  short item_code;
  long buffer_address;
  long retlen_address;
} Itmlst;

long random(void) {
  long rand();
  return (rand());
}

void srandom(int seed) {
  srand(seed);
}

char *crypt(char *cryptstr, char *key) {
  return (cryptstr);                         /* not really crypted */
}

bcopy(char *from_str, char *to_str, int length) {
  int i;
  for (i=0; i<length; i++)
    to_str[i] = from_str[i];
}
bzero( char *str, int length) {
  int i;
  for (i=0; i<length; i++)
    str[i] = '\0';
}

char *index(char *str, char c) {
  while (*str != '\0') {
    if (*str == c)
      return (str);
    else
      str++;
  }
  return ((char *) 0);
}

char *rindex(char *str, char c) {
  char *firstc = str;

  str += strlen(str);       /* go to the end of the string  */
  while (str >= str) {      /* while not over the beginning */
    if (*str == c)
      return (str);
    else
      str--;
  }
  return ((char *) 0);
}

int bcmp(char *b1, char *b2, int length) {
  int i;

  for (i=0; i<length; i++) {
    if (b1[i] != b2[i])
      return (-1);  /* strings aren't alike */
  }
  return (0);   /* strings are alike */
}

struct passwd *getpwuid(int not_used) {
  struct passwd *passwd= (struct passwd *) malloc(sizeof(struct passwd));
  short retlen;
  char *cptr  = malloc(20*sizeof(char));
  Itmlst itmlst;
  unsigned int status;

  itmlst.buffer_length = 19;
  itmlst.item_code = JPI$_USERNAME;
  itmlst.buffer_address = (long)cptr;
  itmlst.retlen_address = (long)&retlen;
  
  passwd->pw_name = cptr;

  status = sys$getjpiw(NULL, NULL, NULL, &itmlst, NULL, NULL, NULL);
  if ( ( status && 1 ) == 1 ) {
    passwd->pw_name[retlen] = '\0';
    while (*cptr) {          /* find first whitespace and terminate there */
      if (*cptr == ' ') {
        *cptr = '\0';
      }
      cptr++;
    }
    return (passwd);
  } else {
    printf ( "vmsutils: getpwuid(): sys$getjpiw returned %08.8x\n", status );
    strcpy(passwd->pw_name, "VMS-nousername");
    return (passwd);
  }
}

struct tv {
    long tv_sec;
    long tv_usec;
};

gettimeofday ( struct tv *tv, struct tv *tz )
{
    /* ignore tz */
    struct timeb tb;

    ftime (&tb);
    tv->tv_sec = (long) tb.time;
    tv->tv_usec = (long) tb.millitm;
}

