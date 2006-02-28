#include "config.h"

/* store the version info here */

#ifdef STABLE
static char mvers[] = "2.02";
static char version[] = "COW 2.02";

#define LIBMAJOR 2
#define LIBMINOR 2

#else
static char mvers[] = "3.2";
static char version[] = "COW 3.2";

#define LIBMAJOR 3
#define LIBMINOR 1

#if 0
#define ALPHACODER "007"
#define ALPHAREF "2"
#endif
#endif
