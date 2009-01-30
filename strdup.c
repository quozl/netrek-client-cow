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
