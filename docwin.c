
/*
 * $Log: docwin.c,v $
 * Revision 1.2  2001/04/28 04:05:27  quozl
 * remove minor compiler warning.
 *
 * Revision 1.1.1.1  1998/11/01 17:24:09  siegl
 * COW 3.0 initial revision
 * */

#include "config.h"
#include "copyright.h"

#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h>

#include <time.h>
#include INC_SYS_TIME
#include INC_SYS_SELECT
#include INC_STRINGS

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "packets.h"

										/* #ifndef 
										 * _IBMR2 
										 * char 
										 * *malloc 
										 * (size_t); 
										 * #else 
										 * void 
										 * *malloc 
										 * (size_t); 
										 * #endifi 
																				 *//* _
																				 * *
																				 * *
																				 * *
																				 * IBMR2 
																				 * 
																				 */

extern char cbugs[];

#define NORMAL          0
#define BOLD            1
#define ITALIC          2

struct list
  {
    int     face;
    struct list *next;
    char   *data;
  };

static struct list *docdata = NULL;
static struct list *xtrekrcdata = NULL;

showdocs(int atline)
{
  FILE   *fptr;
  int     i, length, top, center;
  struct list *data;
  int     count;
  char    buf[128];
  W_Font  font;

  if (!docwin)
    docwin = W_MakeWindow("DocWin", 0, 181, 500, 500, 0, 2,
			  foreColor);

  W_ClearWindow(docwin);

  if (!W_IsMapped(docwin))
    W_MapWindow(docwin);

  sprintf(buf, "---  %s  ---", (char *) query_cowid());
  length = strlen(buf);

  /* using GWINSIDE instead of TWINSIDE because with SMALL_SCREEN defined it
   * * makes more sense to use the smaller width in the interest of saving *
   * screen real estate */
  center = GWINSIDE / 2 - (length * W_Textwidth) / 2;
  W_WriteText(docwin, center, W_Textheight, textColor,
	      buf, length, W_BoldFont);
  sprintf(buf, cbugs);
  length = strlen(buf);
  center = GWINSIDE / 2 - (length * W_Textwidth) / 2;
  W_WriteText(docwin, center, 3 * W_Textheight, textColor,
	      buf, length, W_RegularFont);

  if (!docdata)
    loaddocs();

  top = 10;

  if (atline > maxdoclines)
    atline = maxdoclines - 28;

  data = docdata;

  for (i = 0; i < atline; i++)
    {
      if (data == NULL)
	{
	  atline = 0;
	  data = docdata;
	  break;
	}
      data = data->next;
    }

  count = 28;					 /* Magical # of lines to * * 
						  * display */

  for (i = top; i < 50; i++)
    {
      if (data == NULL)
	break;

      if (data->data == NULL)
	continue;

      switch (data->face)
	{
	case BOLD:
	  font = W_BoldFont;
	  break;
	case ITALIC:
	  font = W_UnderlineFont;
	  break;
	case NORMAL:
	  font = W_RegularFont;
	  break;
	}


      W_WriteText(docwin, 20, i * W_Textheight, textColor, data->data,
		  strlen(data->data), font);
      data = data->next;
      count--;

      if (count <= 0)
	break;
    }
}

loaddocs(void)
{
  FILE   *fptr;
  struct list *temp = NULL;
  struct list *fl = NULL;
  char    line[80], *filename = NULL;
  int     i;

  filename = getdefault("documentation");

  if (filename)
    {
      if ((fptr = fopen(filename, "r")) == NULL)
	return;
    }
  else if ((fptr = fopen("BRM.DOC", "r")) == NULL)
    return;

  temp = (struct list *) malloc(sizeof(struct list));

  if (temp == NULL)
    {						 /* malloc error checking --
						  * * * 10/30/92 EM */
      printf("Warning:  Couldn't malloc space for a new doc line!");
      return;
    }

  while (fgets(line, 80, fptr) != NULL)
    {
      if (fl == NULL)
	fl = temp;

      if (line[strlen(line) - 1] == '\n')
	line[strlen(line) - 1] = '\0';

      temp->face = NORMAL;

      if (line[0] == '\f')
	line[0] = ' ';

      if (line[0] == 0x1b)
	{
	  switch (line[1])
	    {
	    case 'b':
	      temp->face = BOLD;
	      break;
	    case 'i':
	      temp->face = ITALIC;
	      break;
	    }

	  line[0] = line[1] = ' ';
	}

      for (i = 0; i < strlen(line); i++)
	if (line[i] == '\t')
	  line[i] = ' ';

      temp->data = (char *) malloc(strlen(line) + 1);
      strcpy(temp->data, line);

      temp->next = (struct list *) malloc(sizeof(struct list));

      if (temp->next == NULL)
	{					 /* malloc error checking --
						  * * * 10/30/92 EM */
	  printf("Warning:  Couldn't malloc space for a new doc line!");
	  return;
	}

      maxdoclines++;
      temp = temp->next;
      temp->data = NULL;
      temp->next = NULL;
    }

  temp->next = NULL;
  docdata = fl;
}


showxtrekrc(int atline)
{
  FILE   *fptr;
  int     i, length, top, center;
  struct list *data;
  int     count;
  char    buf[128];
  W_Font  font;

  if (!xtrekrcwin)
    xtrekrcwin = W_MakeWindow("xtrekrcWin", 0, 200, 500, 500, 0, 2,
			      foreColor);

  W_ClearWindow(xtrekrcwin);

  if (!W_IsMapped(xtrekrcwin))
    W_MapWindow(xtrekrcwin);

  sprintf(buf, "---  %s  ---", (char *) query_cowid());
  length = strlen(buf);
  center = GWINSIDE / 2 - (length * W_Textwidth) / 2;
  W_WriteText(xtrekrcwin, center, W_Textheight, textColor,
	      buf, length, W_BoldFont);
  sprintf(buf, cbugs);
  length = strlen(buf);
  center = GWINSIDE / 2 - (length * W_Textwidth) / 2;
  W_WriteText(xtrekrcwin, center, 3 * W_Textheight, textColor,
	      buf, length, W_RegularFont);

  if (!xtrekrcdata)
    loadxtrekrc();

  top = 10;

  if (atline > maxxtrekrclines)
    atline = maxxtrekrclines - 28;

  data = xtrekrcdata;

  for (i = 0; i < atline; i++)
    {
      if (data == NULL)
	{
	  atline = 0;
	  data = xtrekrcdata;
	  break;
	}
      data = data->next;
    }

  count = 28;					 /* Magical # of lines to * * 
						  * display */

  for (i = top; i < 50; i++)
    {
      if (data == NULL)
	break;

      if (data->data == NULL)
	continue;

      switch (data->face)
	{
	case BOLD:
	  font = W_BoldFont;
	  break;
	case ITALIC:
	  font = W_UnderlineFont;
	  break;
	case NORMAL:
	  font = W_RegularFont;
	  break;
	}


      W_WriteText(xtrekrcwin, 20, i * W_Textheight, textColor, data->data,
		  strlen(data->data), font);
      data = data->next;
      count--;

      if (count <= 0)
	break;
    }
}

loadxtrekrc(void)
{
  FILE   *fptr;
  struct list *temp = NULL;
  struct list *fl = NULL;
  char    line[80], filename[256];
  int     i;

  filename[0] = '\0';

#ifndef WIN32
  if (!findfile(".netrekrc", filename) && !findfile(".xtrekrc", filename))
    return;
#else
  if (!findfile("netrekrc", filename) && !findfile("xtrekrc", filename))
    return;
#endif

  if ((fptr = fopen(filename, "r")) == NULL)
    return;

  temp = (struct list *) malloc(sizeof(struct list));

  if (temp == NULL)
    {						 /* malloc error checking --
						  * * * 10/30/92 EM */
      printf("Warning:  Couldn't malloc space for a new doc line!");
      return;
    }

  while (fgets(line, 80, fptr) != NULL)
    {
      if (fl == NULL)
	fl = temp;

      if (line[strlen(line) - 1] == '\n')
	line[strlen(line) - 1] = '\0';

      temp->face = NORMAL;

      if (line[0] == '\f')
	line[0] = ' ';

      if (line[0] == '#')
	temp->face = ITALIC;

      for (i = 0; i < strlen(line); i++)
	if (line[i] == '\t')
	  line[i] = ' ';

      temp->data = (char *) malloc(strlen(line) + 1);
      strcpy(temp->data, line);

      temp->next = (struct list *) malloc(sizeof(struct list));

      if (temp->next == NULL)
	{					 /* malloc error checking --
						  * * * 10/30/92 EM */
	  printf("Warning:  Couldn't malloc space for a new doc line!");
	  return;
	}

      maxxtrekrclines++;
      temp = temp->next;
      temp->data = NULL;
      temp->next = NULL;
    }

  temp->next = NULL;
  xtrekrcdata = fl;
}
