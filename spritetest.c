/* spritetest.c
 *
 * --sprite-test: show the main window and animate every pixmap.
 *
 * Runs without a server connection.  Each PNG under the pixmap directory is
 * drawn centred in the tactical window and its frames cycle.  A pixmap is
 * held until it has played one full cycle and at least HOLD_TICKS frames,
 * then the next one is shown.
 *
 * Keys: space or n = next, p = previous, q = quit.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fts.h>

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "spritetest.h"

#define FRAME_US    100000	/* 10 frames per second */
#define HOLD_TICKS  15		/* minimum frames per pixmap */

static char **names = NULL;
static int nnames = 0;

static int cmp_names(const void *a, const void *b)
{
  return strcmp(*(char *const *) a, *(char *const *) b);
}

/* Collect the PNG paths relative to the pixmap directory, sorted, since
 * W_ReadImage() takes names relative to it. */
static void collect_names(void)
{
  char *dir = W_PixmapDir();
  char *argv[2];
  FTS *fts;
  FTSENT *ent;
  int cap = 0;

  argv[0] = dir;
  argv[1] = NULL;
  fts = fts_open(argv, FTS_LOGICAL, NULL);
  if (fts == NULL)
    return;

  while ((ent = fts_read(fts)) != NULL) {
    size_t len;
    char *rel;

    if (ent->fts_info != FTS_F)
      continue;
    len = strlen(ent->fts_path);
    if (len < 4 || strcmp(ent->fts_path + len - 4, ".png") != 0)
      continue;

    rel = ent->fts_path + strlen(dir);
    while (*rel == '/')
      rel++;

    if (nnames == cap) {
      cap = cap ? cap * 2 : 64;
      names = realloc(names, cap * sizeof(*names));
      if (names == NULL) {
	fprintf(stderr, "sprite-test: out of memory\n");
	exit(1);
      }
    }
    names[nnames++] = strdup(rel);
  }
  fts_close(fts);

  if (nnames > 1)
    qsort(names, nnames, sizeof(*names), cmp_names);
}

static char *scale_text(void)
{
  static char buf[16];

  if (W_Scale == W_SCALE_AUTO)
    return "auto";
  snprintf(buf, sizeof(buf), "%d", W_Scale);
  return buf;
}

int spriteTestLoop(void)
{
  void *img = NULL;
  int index = 0;
  int ticks = 0;
  int failures = 0;
  int move = 0;
  int quit = 0;
  int width, height, nviews, hold;
  char line[256];
  W_Event ev;

  collect_names();
  if (nnames == 0) {
    fprintf(stderr, "sprite-test: no .png pixmaps found in %s\n", W_PixmapDir());
    return 1;
  }

  while (!quit) {
    while (W_SpNextEvent(&ev)) {
      if (ev.type == W_EV_CLOSED) {
	quit = 1;
      } else if (ev.type == W_EV_KEY) {
	switch (ev.key) {
	case 'q':
	case 'Q':
	  quit = 1;
	  break;
	case ' ':
	case 'n':
	case 'N':
	  move = 1;
	  break;
	case 'p':
	case 'P':
	  move = -1;
	  break;
	}
      }
    }
    if (quit)
      break;

    W_ImageInfo(img, &width, &height, &nviews);
    hold = nviews > HOLD_TICKS ? nviews : HOLD_TICKS;

    if (img == NULL || move != 0 || ticks >= hold) {
      /* Step to the next pixmap.  A pixmap that fails to load is skipped
       * on the following pass, so give up only after a full round. */
      if (img != NULL)
	W_DropImage(img);
      index = (index + (move != 0 ? move : 1) + nnames) % nnames;
      move = 0;
      ticks = 0;
      img = W_ReadImage(w, names[index]);
      if (img == NULL) {
	fprintf(stderr, "sprite-test: could not load %s\n", names[index]);
	if (++failures >= nnames) {
	  fprintf(stderr, "sprite-test: no pixmap could be loaded\n");
	  return 1;
	}
	continue;
      }
      failures = 0;
      W_ImageInfo(img, &width, &height, &nviews);
    }

    W_ClearWindow(w);
    W_DrawImage(TWINSIDE / 2 - width / 2, TWINSIDE / 2 - height / 2, img);
    W_AdvanceImageView(img);

    snprintf(line, sizeof(line), "%d/%d  %s  %dx%d  frame %d/%d",
	     index + 1, nnames, names[index], width, height,
	     ticks % (nviews > 0 ? nviews : 1) + 1, nviews > 0 ? nviews : 1);
    W_WriteText(w, 4, 4, W_White, line, strlen(line), W_RegularFont);

    snprintf(line, sizeof(line),
	     "scale requested %s   space/n next   p prev   q quit",
	     scale_text());
    W_WriteText(w, 4, 4 + W_Textheight, W_Grey, line, strlen(line), W_RegularFont);

    W_FlushWindow(w);
    W_Flush();
    usleep(FRAME_US);
    ticks++;
  }

  if (img != NULL)
    W_DropImage(img);
  return 0;
}
