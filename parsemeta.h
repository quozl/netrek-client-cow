#ifndef meta_h_header
#define meta_h_header
#ifdef META

/*
   meta.h
*/


/* Function Definitions */

void parsemeta();

void metawindow(void);
/*
 *  Show the meta server menu window
 */

void metainput(void);
/*  
 *  Wait for actions in the meta-server window.
 *
 *  This is really the meta-server window's own little input() function.
 *  It is needed so we don't have to use all the bull in the main
 *  input(). Plus to use it I'd have to call mapAll() first and the client
 *  would read in the default server and then call it up before I can
 *  select a server.
 */


#endif /* defined META */ 
#endif /* defined meta_h_header */
