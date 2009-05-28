/* ranklist.c
 * 
 * Kevin P. Smith 12/5/88
 *
 */
#include "config.h"
#include "copyright2.h"

#include <stdio.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"

/* Calculate DI to next rank, following server logic */
/* Credit: Bill Balcerski, revision 1.2, 2007/04/02 10:12:38, Netrek XP */
static float toNextRank(int rank)
{
    int hourratio;
    float rankDI, myDI, oRating, pRating, bRating, Ratings;

    /* TODO: add support for INL mode */
    if (!strcmp(me->p_name, "guest") || !strcmp(me->p_name, "Guest"))
        hourratio = 5;
    else
        hourratio = 1;

    oRating = offenseRating (me);
    pRating = planetRating (me);
    bRating = bombingRating (me);
    Ratings = oRating + pRating + bRating;
    myDI = (float) (Ratings * (me->p_stats.st_tticks / 36000.0));
    rankDI = ranks[rank].ratings * ranks[rank].hours / hourratio;

    if (Ratings > ranks[rank].ratings)
    {
        if (myDI > rankDI)
            return (0.0);
        else
            return (rankDI - myDI);
    }
    else if (Ratings > (ranks[rank-1].ratings))
    {
        if (myDI > 2*rankDI)
            return (0.0);
        else
            return (2*rankDI - myDI);
    }
    else if (me->p_stats.st_rank > 0 && Ratings > (ranks[rank-2].ratings))
    {
        if (myDI > 4*rankDI)
            return (0.0);
        else
            return (4*rankDI - myDI);
    }
    else if (me->p_stats.st_rank >= 4 && Ratings > (ranks[rank-3].ratings))
    {
        if (myDI > 8*rankDI)
            return (0.0);
        else
            return (8*rankDI - myDI);
    }
    else
        return (-1);
}

void    ranklist(void)
{
  int i;
  char buf[80];
  int col = F_sp_rank ? textColor : W_Grey;
  static int size = 0;

  if (size != nranks) {
    W_ClearWindow(rankw);
    W_ResizeTextWindow(rankw, 65, nranks + 9);
    size = nranks;
  }
  strcpy(buf, "  Rank       Hours  Offense  Ratings      DI");
  W_WriteText(rankw, 1, 1, col, buf, strlen(buf), W_BoldFont);
  for (i = 0; i < nranks; i++) {
    sprintf(buf, "%-11.11s %5.0f %8.2f %8.2f   %7.2f",
            ranks[i].name,
            ranks[i].hours,
            ranks[i].offense,
            ranks[i].ratings, ranks[i].ratings * ranks[i].hours);
    if (mystats->st_rank == i) {
      if (i < nranks-1) {
        char buf2[35];
        float DI;
        if ((DI = toNextRank(i+1)) != -1) {
            sprintf(buf2, " (need %.2f DI)", DI);
            strcat(buf, buf2);
        } else {
          strcat(buf, " (need ratings)");
        }
      }
      W_WriteText(rankw, 1, i + 2, W_Cyan, buf, strlen(buf), W_BoldFont);
    } else {
      W_WriteText(rankw, 1, i + 2, col, buf, strlen(buf), W_RegularFont);
    }
  }
  strcpy(buf, "To achieve a rank, you need the corresponding DI");
  W_WriteText(rankw, 1, i + 3, col, buf, strlen (buf), W_RegularFont);
  strcpy(buf, "in less than the hours allowed (DI = ratings x hours).");
  W_WriteText(rankw, 1, i + 4, col, buf, strlen (buf), W_RegularFont);
  strcpy(buf, "OR, get offense+bombing+planets above corresponding Ratings");
  W_WriteText(rankw, 1, i + 5, col, buf, strlen (buf), W_RegularFont);
  strcpy(buf, "Promotions also occur at 2xDI with Ratings - 1");
  W_WriteText(rankw, 1, i + 6, col, buf, strlen (buf), W_RegularFont);
  strcpy(buf, "4xDI with Ratings - 2, and 8xDI with Ratings - 3");
  W_WriteText(rankw, 1, i + 7, col, buf, strlen (buf), W_RegularFont);
  W_Flush();
}
