
#include "config.h"

/* randomize argv's - stolen from beorn
 *
 * $Log: randomize.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 * */
main (ac, av)
     int ac;
     char *av[];
{
  int i, j;

  SRANDOM ((getuid () << 7) ^ (getpid () << 4) ^ time (0));

  for (i = ac - 1; i; i--)
    {
      j = (RANDOM () % i) + 1;
      printf ("%s ", av[j]);
      av[j] = av[i];
    }
  printf ("\n");
  exit (0);
}
