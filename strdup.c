
/*
 * $Log: strdup.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 * */
#include <stdio.h>


char *
strdup (strptr)
     char *strptr;
{
  char *charptr;

  charptr = (char *) malloc (sizeof (char) * strlen (strptr) + 1);
  if (charptr == NULL)
    return (charptr);
  strcpy (charptr, strptr);

  return (charptr);
}
