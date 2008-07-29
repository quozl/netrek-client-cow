#include "config.h"
#include "copyright2.h"

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"


void myf(int x, int y, W_Color color, W_Font font, const char *fmt, ...)
{
  char buf[101];
  int len;
  va_list args;

  va_start(args, fmt);
  len = vsnprintf(buf, 100, fmt, args);
  buf[len] = '\0';
  W_WriteText(w, x, y, color, buf, len, font);
  va_end(args);
}


void myc(int x, int y)
{
  W_ClearArea(w, x, y, TWINSIDE-x, W_Textheight);
}


