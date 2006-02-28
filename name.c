/*
 * $Log: name.c,v $
 * Revision 1.2  2006/02/28 01:45:56  quozl
 * prerelease
 *
 * Revision 1.1.1.1  1998/11/01 17:24:10  siegl
 * COW 3.0 initial revision
 * */
#include "version.h"
#include "patchlevel.h"

main (int argc, char **argv)
{

  if (argc > 1)
  {
    switch (argv[1][0])
    {
      case '1':	
	printf("%d",LIBMAJOR);
	break;
      case '2':
	printf("%d",LIBMINOR);
        break;
      case 'p':
	printf("%d",PATCHLEVEL);
        break;
      default: ;
    }
    exit(0);
  }

  printf ("%s.%d", mvers, PATCHLEVEL);

#ifdef ALPHACODER
  printf (".%s(%s)", ALPHACODER, ALPHAREF);
#endif

  printf ("\n");
  exit (0);
}
