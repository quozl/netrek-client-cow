
/* ranklist.c
 * 
 * Kevin P. Smith 12/5/88
 *
 * $Log: ranklist.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:11  siegl
 * COW 3.0 initial revision
 * */
#include "config.h"
#include "copyright2.h"

#include <stdio.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"

void    ranklist(void)
{
  register int i;
  char    buf[80];

  /* W_ClearWindow(rankw); */
  (void) strcpy(buf, "  Rank       Hours  Defense  Ratings      DI");
  W_WriteText(rankw, 1, 1, textColor, buf, strlen(buf), W_BoldFont);
  for (i = 0; i < NUMRANKS; i++)
    {
      sprintf(buf, "%-11.11s %5.0f %8.2f %8.2f   %7.2f",
	      ranks[i].name,
	      ranks[i].hours,
	      ranks[i].defense,
	      ranks[i].ratings,
	      ranks[i].ratings * ranks[i].hours);
      if (mystats->st_rank == i)
	{
	  W_WriteText(rankw, 1, i + 2, W_Cyan, buf, strlen(buf), W_BoldFont);
	}
      else
	{
	  W_WriteText(rankw, 1, i + 2, textColor, buf, strlen(buf), W_RegularFont);
	}
    }
  strcpy(buf, "To achieve a rank, you need the corresponding DI");
  W_WriteText(rankw, 1, i + 3, textColor, buf, strlen(buf), W_RegularFont);
  strcpy(buf, "in less than the hours allowed.");
  W_WriteText(rankw, 1, i + 4, textColor, buf, strlen(buf), W_RegularFont);
  strcpy(buf, "OR, get offense+boming+planets above corresponding Ratings");
  W_WriteText(rankw, 1, i + 5, textColor, buf, strlen(buf), W_RegularFont);
  strcpy(buf, "Promotions also occur at 2xDI with Ratings - 1");
  W_WriteText(rankw, 1, i + 6, textColor, buf, strlen(buf), W_RegularFont);
  strcpy(buf, "and at 4xDI with Ratings - 2");
  W_WriteText(rankw, 1, i + 7, textColor, buf, strlen(buf), W_RegularFont);
  strcpy(buf, " also, some servers require .8 defense for promotion.");
  W_WriteText(rankw, 1, i + 8, textColor, buf, strlen(buf), W_RegularFont);
}
