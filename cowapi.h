/* cowapi.h    -- The COW Aplication interface */

/* Copyright (c) 1996   Kurt Siegl
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * The COW development team
 *
 * $Log: cowapi.h,v $
 * Revision 1.5  2001/04/28 04:03:56  quozl
 * change -U to also adopt a local port number for TCP mode.
 * 		-- Benjamin `Quisar' Lerman  <quisar@quisar.ambre.net>
 *
 * Revision 1.4  2000/05/19 14:24:52  jeffno
 * Improvements to playback.
 * - Can jump to any point in recording.
 * - Can lock on to cloaked players.
 * - Tactical/galactic repaint when paused.
 * - Can lock on to different players when recording paused.
 *
 * Revision 1.3  1999/08/20 18:32:45  siegl
 * WindowMaker Docking support
 *
 * Revision 1.2  1999/08/05 16:46:32  siegl
 * remove several defines (BRMH, RABBITEARS, NEWDASHBOARD2)
 *
 * Revision 1.1.1.1  1998/11/01 17:24:09  siegl
 * COW 3.0 initial revision
 * */

#ifndef _h_cowapi
#define _h_cowapi

/* COW mainloop - starts up a client window */

extern int cowmain(char *server, int port, char *name);

#ifdef RECORDGAME
/* COW mainloop - starts up a client window */
extern int pbmain(char *name);
#endif

/* Variables passing Optional Arguments to cowmain */

extern char   *deffile;
extern char   *recordFileName;
extern int     pb_create_index;
extern char   *logFileName;
extern char   *display_host;
extern int     passive;
extern int     checking;
extern char   *deffile;

#ifdef META
extern int     usemeta;
#endif

/* Global COW Variables which may be set and/or used outside */

/* Version Information */
extern char cflags[], arch[], cdate[], cbugs[], cowid[], cwho[];
extern struct timeval tv_ctime;

#ifndef PSEUDOSIZE
#define PSEUDOSIZE 16
#endif
extern char pseudo[PSEUDOSIZE];
extern char defpasswd[PSEUDOSIZE];
extern char login[PSEUDOSIZE];

extern int baseLocalPort; 
extern int log_packets;

extern int ghoststart;
extern int ghost_pno;
extern int debug;
extern int ignore_signals;

extern char   *title;

#ifndef WIN32
extern int takeNearest;
#endif

#ifdef GATEWAY
 extern int use_trekhopd;
 extern unsigned LONG  netaddr;
#endif

#ifdef RSA
extern int RSA_Client;
extern char key_name[];
extern char client_type[];
extern char client_arch[];
extern char client_creator[];
extern char client_comments[];
extern char client_key_date[];

#endif

#ifdef TOOLS
extern char *wwwlink;
extern char *upgradeURL;
extern char *releaseURL;
extern char *bugURL;
#endif

#ifdef WINDOWMAKER
  extern char **wm_argv;
  extern int wm_argc;
#endif

#endif /* _h_cow */





