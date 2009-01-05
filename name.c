#include <stdio.h>
#include <stdlib.h>
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

  printf ("\n");
  exit(0);
}
