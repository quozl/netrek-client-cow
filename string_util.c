
/* string_util.c
 * 
 * The client actually uses lots of string functions, mostly to format the
 * information it displays.  This module provides housing for all these
 * string functions. 
 *
 * $Log: string_util.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 * */

#include "config.h"
#include "copyright.h"
#include <stdio.h>


char   *itoapad(int val, char *result, int pad, int prec)
{
  int     lead_digit = 0, i, j, too_big = 1, h = 1;

  if (prec != 0)
    result[prec] = '\0';


  /* Careful!!! maximum number convertable must be <=100000000. size < 9 */

  for (i = 100000000, j = 0; i && h <= prec; i /= 10, j++)
    {
      if ((9 - prec) > j && too_big)
	continue;
      else if (h)
	{
	  j = 0;
	  too_big = 0;
	  h = 0;
	}

      result[j] = (val % (i * 10)) / i + '0';

      if (result[j] != '0' && !lead_digit)
	lead_digit = 1;

      if (!lead_digit && !pad)
	if ((result[j] = (val % (i * 10)) / i + '0') == '0')
	  result[j] = ' ';
    }

  if (val == 0)
    result[prec - 1] = '0';

  return (result);
}

char   *ftoa(float fval, char *result, int pad, int iprec, int dprec)
{
  int     i, ival;
  float   val = fval;

  if ((iprec + dprec) != 0)
    result[iprec + dprec + 1] = '\0';

  for (i = 0; i < dprec; i++)
    val *= 10.0;

  ival = val;
  itoapad(ival, result, pad, iprec + dprec);

  for (i = (iprec + dprec); i >= iprec; i--)
    if (result[i] == ' ')
      result[i + 1] = '0';
    else
      result[i + 1] = result[i];

  result[iprec] = '.';

  if (fval < 1.0)
    result[iprec - 1] = '0';

  return (result);
}

char   *
        format(char *buf, char *from, int width, int right_justify)
{
  int     len = strlen(from), i;

  if (len > width)
    len = width;

  buf[width] = '\0';

  if (right_justify)
    {
      STRNCPY(&(buf[width - len]), from, len);

      for (i = 0; i < (width - len); i++)
	buf[i] = ' ';
    }
  else
    {
      STRNCPY(buf, from, len);

      for (i = len; i < width; i++)
	buf[i] = ' ';
    }

  return (buf);
}
