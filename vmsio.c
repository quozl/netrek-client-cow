/*
**  OpenVMS does not provide the "hack" that UNIX provides that allows one to
**  obtain the file descriptor of the X connection and include it in a select()
**  call, so as to wait for either network activity or X events.
**  
**  Instead, we have to ask the OpenVMS X implementation to signal us when an
**  event arrives, set a flag, and then detect that in a replacement select()
**  function.
**  
**  cameron@stl.dec.com 16-Jan-1998 
*/
#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include <time.h>
#include <timeb.h>
#include <ssdef.h>
#include <jpidef.h>
#include <iodef.h>

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"

/*
**  Event stuff
*/

/*
**  Event flag number for use by vms_select()
**  
**  (The VMS event flag is reset by vms_select(), and set by any of the AST
**  routines that are executed when data becomes available)
*/
static char efns;

/*
**  Message received flags; set to 1 when a message is received
*/
static unsigned char tim = 0;   /* timer expiry         */
static unsigned char win = 0;   /* x window event       */
static unsigned char tcp = 0;   /* tcp socket to server */
static unsigned char udp = 0;   /* udp socket to server */

void vms_event ( unsigned char *flag )
{
    sys$setef ( efns );
    *flag = 1;
}

void vms_event_window(void)
{
    sys$setef ( efns );
    win = 1;
}

void vms_event_window_done(void)
{
    win = 0;
}

void vms_queue_socket_read_attention
( int socket, void *(routine(void)), unsigned char *parameter )
{
    unsigned int status;
    
    status = sys$qiow (
        0,		           /* event flag number */
        vaxc$get_sdc(socket),      /* channel */
        IO$_SETMODE|IO$M_READATTN, /* function code */
        0,                         /* iosb */
        0,                         /* astadr */
        0,		           /* astprm */
        routine,                   /* p1 - ast routine */
        parameter,                 /* p2 - ast parameter */
        0,                         /* p3 - ast access mode */
        0,                         /* p4 - not used */
        0,                         /* p5 - not used */
        0                          /* p6 - not used */
    );

    /* check status and report err */
    if ( ( status & 1 ) != 1 )
    {
        lib$signal ( status );
    }
}

/*
**  Replacement for select()
**  
**  Given a bitmask of the sockets to be checked for unread data,
**  return a bitmask of those sockets that *have* unread data.
**  
**  If the timeout argument is a NULL pointer, stall until at least one
**  socket has unread data.
**  
**  If the timeout argument points to a zero timeout structure, do not
**  stall; return immediately.  (i.e. imagine immediate timeout).
**  
**  If the timeout argument points to a non-zero timeout structure, stall
**  until the time specified has elapsed.
**  
**  (This routine does not handle write or exception sockets).
*/
int
vms_select
( int ndfs, int *readfds, int *writefds, int *exceptfds, struct timeval *tm )
{
    int mask;
    int swin;
    unsigned int status;
    
    /* read attention queued flags per ip socket */
    static unsigned char tcpraqd = 0;
    static unsigned char udpraqd = 0;
    
    /* obtain the socket understood to be the X socket */
    swin = W_Socket();
    
    /*
    **  Ensure that the read attention ASTs are requeued if they had fired
    **  since the last call to us, or if this is the first call.
    **  
    **  We assume that the AST will be called immediately if the socket has
    **  unread data on it, even if a prior AST had indicated that such data
    **  has arrived.  [Check this?]
    */
    if ( ! tcpraqd )
    {
        vms_queue_socket_read_attention ( sock, vms_event, &tcp );
        tcpraqd = 1;
    }

    if ( ! udpraqd )
    {
        if ( udpSock >= 0 )
        {
            vms_queue_socket_read_attention ( udpSock, vms_event, &udp );
            udpraqd = 1;
        }
    }
    
    mask = 0;
    tim = 0;
    
    /* if timeout specified and non zero, queue a timer */
    if ( tm != NULL )
        if ( tm->tv_sec != 0 || tm->tv_usec != 0 )
        {
            unsigned int delay[2];
            
            /* calculate delay; loss of significance; 7 minutes maximum wait */
            delay[0] = 0 - ( tm->tv_sec * 1000000 + tm->tv_usec ) * 10;
            delay[1] = -1;
            
            /* set a timer for the delay period, which will set the event flag
               (twice) and set the timer flag to 1 */
            status = sys$setimr ( efns, &delay, vms_event, &tim, 0 );
            if ( ( status & 1 ) != 1 ) lib$signal ( status );
        }
        else
        {
            /* timeout period zero; force immediate timeout */
            tim = 1;
        }

#ifdef TRACE1
    printf ( "?" );
    if (FD_ISSET(swin, readfds)) printf ( "x" );
    if (FD_ISSET(sock, readfds)) printf ( "t" );
    if (udpSock >= 0 && FD_ISSET(udpSock, readfds)) printf ( "u" );
#endif

    while ( mask == 0 )
    {
        /* turn off interrupts during check */
        status = sys$setast(0);
        if ( ( status & 1 ) != 1 ) lib$signal ( status );
        
        /* detect tcp/ip and udp/ip socket input */
        if ( *readfds & ( 1 << sock ) )
            if ( tcp != 0 )
                { tcp = 0; tcpraqd = 0; mask |= ( 1<<sock ); }

        if ( *readfds & ( 1 << udpSock ) )
            if ( udp != 0 )
                { udp = 0; udpraqd = 0; mask |= ( 1<<udpSock ); }
        
        /* detect window input */
        if ( *readfds & ( 1 << swin ) )
            if ( win != 0 )
                { mask |= ( 1<<swin ); }

        /* if timeout, stop checking */
        if ( tim == 1 ) break;
        
        /* if any of the requested sockets are active, return */
        if ( mask & *readfds ) break;
        
        /* nothing that set the event flag was what we are waiting for */
        /* so reset it */
        status = sys$clref ( efns );
        if ( ( status & 1 ) != 1 ) lib$signal ( status );
        
        /* enable interrupts again */
        status = sys$setast(1);
        if ( ( status & 1 ) != 1 ) lib$signal ( status );
        
        /* sleep until woken by either unread data or timeout */
        status = sys$waitfr ( efns );
        if ( ( status & 1 ) != 1 ) lib$signal ( status );
    }

    /* enable interrupts again */
    status = sys$setast(1);
    if ( ( status & 1 ) != 1 ) lib$signal ( status );
        
    /* cancel the timeout if it didn't expire */
    if ( tm != NULL )
        if ( tim == 0 && ( tm->tv_sec != 0 || tm->tv_usec != 0 ) )
        {
            status = sys$cantim ( &tim, 0 );
            if ( ( status & 1 ) != 1 ) lib$signal ( status );
        }

    /* set return status according to whether activity is detected */
    if ( *readfds & mask )
    {
        status = 1;
    }
    else
    {
        status = 0;
    }
    
    *readfds = mask;
#ifdef TRACE2
    printf ( "-" );
    if (FD_ISSET(swin, readfds)) printf ( "x" );
    if (FD_ISSET(sock, readfds)) printf ( "t" );
    if (udpSock >= 0 && FD_ISSET(udpSock, readfds)) printf ( "u" );
#endif
    return ( status );
}
