
/*
 * Copyright (c) 1985 Regents of the University of California.
 *
 * Use and reproduction of this software are granted  in  accordance  with
 * the terms and conditions specified in  the  Berkeley  Software  License
 * Agreement (in particular, this entails acknowledgement of the programs'
 * source, and inclusion of this notice) with the additional understanding
 * that  all  recipients  should regard themselves as participants  in  an
 * ongoing  research  project and hence should  feel  obligated  to report
 * their  experiences (good or bad) with these elementary function  codes,
 * using "sendbug 4bsd-bugs@BERKELEY", to the authors.
 */

#ifdef VAX      /* VAX D format */
    static unsigned short msign=0x7fff;
#else           /*IEEE double format */
    static unsigned short msign=0x7fff;
#endif

static
double copysign(x,y)
double x,y;
{
#if defined(NATIONAL) || defined(WIN32)
        unsigned short  *px=(unsigned short *) &x+3,
                        *py=(unsigned short *) &y+3;
#else /* VAX, SUN, ZILOG */
        unsigned short  *px=(unsigned short *) &x,
                        *py=(unsigned short *) &y;
#endif

#ifdef VAX
        if ( (*px & mexp) == 0 ) return(x);
#endif

        *px = ( *px & msign ) | ( *py & ~msign );
        return(x);
}

/*
 * algorithm for rint(x) in pseudo-pascal form ...
 *
 * real rint(x): real x;
 *      ... delivers integer nearest x in direction of prevailing rounding
 *      ... mode
 * const        L = (last consecutive integer)/2
 *        = 2**55; for VAX D
 *        = 2**52; for IEEE 754 Double
 * real s,t;
 * begin
 *      if x != x then return x;                ... NaN
 *      if |x| >= L then return x;              ... already an integer
 *      s := copysign(L,x);
 *      t := x + s;                             ... = (x+s) rounded to integer
 *      return t - s
 * end;
 *
 * Note: Inexact will be signaled if x is not an integer, as is
 *      customary for IEEE 754.  No other signal can be emitted.
 */
#ifdef VAX
static long Lx[] = {0x5c00,0x0};                /* 2**55 */
#define L *(double *) Lx
#else   /* IEEE double */
static double L = 4503599627370496.0E0;         /* 2**52 */
#endif
double
rint(x)
double x;
{
        double s,t,one = 1.0,copysign();
#ifndef VAX
        if (x != x)                             /* NaN */
                return (x);
#endif
        if (copysign(x,one) >= L)               /* already an integer */
            return (x);
        s = copysign(L,x);
        t = x + s;                              /* x+s rounded to integer */
        return (t - s);
}
