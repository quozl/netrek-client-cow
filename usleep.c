
#include "config.h"

#include <stdio.h>
#include INC_MATH
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include INC_STRINGS
#include INC_UNISTD

usleep(microSeconds)
U_LONG microSeconds;
{
        u_int            Seconds, uSec;
        int                     nfds, readfds, writefds, exceptfds;
        struct  timeval         Timer;

        nfds = readfds = writefds = exceptfds = 0;

        if( (microSeconds == (U_LONG) 0)
                || microSeconds > (U_LONG) 4000000 )
        {
                errno = ERANGE;         /* value out of range */
                perror( "usleep time out of range ( 0 -> 4000000 ) " );
                return -1;
        }

        Seconds = microSeconds / (U_LONG) 1000000;
        uSec    = microSeconds % (U_LONG) 1000000;

        Timer.tv_sec            = Seconds;
        Timer.tv_usec           = uSec;

        if( select( nfds, &readfds, &writefds, &exceptfds, &Timer ) < 0 )
        {
		if (errno != EINTR) {
                	perror( "usleep (select) failed" );
                	return -1;
		}
        }

        return 0;
}

