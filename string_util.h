/*
   string_util.h
   
   The client actually uses lots of string functions, mostly to
   format the information it displays.  This module provides
   housing for all these string functions.
 *
 * $Log: string_util.h,v $
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 * */

#ifndef string_util_h
#define string_util_h


char *itoapad (int val, char *result, int pad, int prec);
/*
   Convert an integer `val' to a null terminated string `result'.
   
   Only the `prec' most significant digits will be written out.
   If `val' can be expressed in fewer than `prec' digits then the
   number is padded out with zeros (if pad is true) or spaces
   (if pad is false).
   
   WARNING: val must be <= 100000000 (size < 9).
*/



char *ftoa (float fval, char *result, int pad, int iprec, int dprec);
/*
   Convert a float `fval' to a null terminated string `result'.
   
   Only the `iprec' most significant whole digits and the `dprec'
   most significat fractional digits are printed.
   
   The integer part will be padded with zeros (if pad is true) or
   spaces (if pad is false) if it is shorter than `iprec' digits.
   
   The floating point part will always be padded with zeros.
   
   WARNING: The whole part of `fval' must be <= 100000000 (size < 9).
*/


char *format (char *buf, char *from, int width, int right_justify);
/*
   Right or left justify the string `from' into the next `width'
   characters in the buffer `buf'.
*/

   
#endif  /* defined string_util_h */
