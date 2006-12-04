/* prototype camera support */

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <Imlib2.h>
#include "defs.h"

/* internal context */
static Display *disp = NULL;
static Visual *vis = NULL;
static Screen *scr = NULL;
static Colormap cm;
static Window rw, tw;
static int rx, ry;
static unsigned int rdx, rdy;
static int frame;
static int initialised = 0;

/* maximum file name size */
#define MAXFRAMEFILENAMESIZE 80

void camera_init(Display *arg_disp, Window arg_window) {
  if (initialised) return;
  int depth;
  disp = arg_disp;
  tw = arg_window;

  scr = ScreenOfDisplay(disp, DefaultScreen(disp));
  vis = DefaultVisual(disp, XScreenNumberOfScreen(scr));
  depth = DefaultDepth(disp, XScreenNumberOfScreen(scr));
  cm = DefaultColormap(disp, XScreenNumberOfScreen(scr));
  rw = RootWindow(disp, XScreenNumberOfScreen(scr));
  
  imlib_context_set_display(disp);
  imlib_context_set_visual(vis);
  imlib_context_set_colormap(cm);
  imlib_context_set_color_modifier(NULL);
  imlib_context_set_operation(IMLIB_OP_COPY);
  frame = 0;

  rx = 0;
  ry = 0;
  rdx = TWINSIDE;	/* restrict to tactical */
  rdy = TWINSIDE;
  imlib_context_set_drawable(tw);

  initialised++;
}

void camera_snap(Display *arg_disp, Window arg_window)
{
  Imlib_Image image;
  char *name;

  camera_init(arg_disp, arg_window);
  image = imlib_create_image_from_drawable(rw, rx, ry, rdx, rdy, 0);
  imlib_context_set_image(image);
  imlib_image_attach_data_value("quality", NULL, 15, NULL);
  name = malloc(MAXFRAMEFILENAMESIZE);
  snprintf(name, MAXFRAMEFILENAMESIZE-1, "netrek-%03d.png", frame++);
  imlib_save_image(name);
  imlib_free_image_and_decache();
  fprintf(stderr, "camera_snap: %s\n", name);
  free(name);
}
