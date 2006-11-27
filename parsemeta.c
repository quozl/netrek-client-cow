/* meta.c
 * 
 * $Log: parsemeta.c,v $
 * Revision 1.13  2006/11/27 00:40:31  quozl
 * fix segfault from comment length
 *
 * Revision 1.12  2006/11/27 00:36:55  quozl
 * fix segfault from comment length
 *
 * Revision 1.11  2006/04/14 10:35:37  quozl
 * fix sscanf compiler warning
 *
 * Revision 1.10  2006/04/14 10:29:40  quozl
 * fix segfault
 *
 * Revision 1.9  2006/02/22 22:55:22  quozl
 * fix ReadMetasRecv regression
 *
 * Revision 1.8  2006/01/27 09:57:27  quozl
 * *** empty log message ***
 *
 * Revision 1.7  2001/07/24 05:00:21  quozl
 * fix metaserver window delays
 *
 * Revision 1.6  1999/08/02 07:21:48  carlos
 *
 * Fixed a minor bug in the UDP menu display where it wouldn't highlight
 * the selection.
 *
 * --Carlos V.
 *
 * Revision 1.5  1999/03/25 20:56:26  siegl
 * CygWin32 autoconfig fixes
 *
 * Revision 1.4  1999/03/07 10:29:21  siegl
 * History log formating
 *
 *
 * - Nick Trown         May 1993    Original Version. 
 * - Andy McFadden      May 1993 ?  Connect to Metaserver. 
 * - BDyess (Paradise)  ???         Bug Fixes.  Add server type field. 
 * - Michael Kellen     Jan 1995    Don't list Paradise Servers. List empty servers. 
 * - James Soutter      Jan 1995    Big Parsemeta changes.  
 *       Included some Paradise Code.  Added Known Servers
 *       Option.  Added option for metaStatusLevel.  Bug Fixes.
 * - Jonathan Shekter Aug 1995 --  changed to read into buffer in all cases,
 *       use findfile() interface for cachefiles.
 * - James Cameron, Carlos Villalpando Mar 1999 --  Added UDP metaserver
 *     queries
 */

#undef DEBUG

#include "config.h"
#include "copyright.h"

#ifdef META
#include INC_LIMITS
#include INC_FCNTL
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include INC_SYS_SELECT
#include INC_STRINGS
#include <sys/socket.h>
#include INC_NETINET_IN
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"


#ifdef WIN32 /* socket garbage in case the client is not running on NT */
#define read(f,b,l) recv(f,b,l,0)
#define write(f,b,l) send(f,b,l,0)
#define close(s) closesocket(s)
#endif


/* Constants */

#define BUF     6144
#define LINE    80              /* Width of a meta-server line           */
#define MAXMETABYTES 2048	/* maximum metaserver UDP packet size    */
static int msock = -1;          /* the socket to talk to the metaservers */
static int sent = 0;            /* number of solicitations sent          */
static int seen = 0;            /* number of replies seen                */
static int verbose = 0;         /* whether to talk a lot about it all    */
static int type;		/* type of connection requested          */

/* Local Types */

struct servers {
  char    address[LINE];	/* host name or ip address of server	*/
  int     port;
  int     age;			/* age in seconds as received		*/
  time_t  when;			/* date time this record received	*/
  int     refresh;		/* data needs redisplaying		*/
  int     lifetime;		/* remaining cache life of entry        */
  int     players;
  int     status;
  char    typeflag;
  char    comment[LINE];
};

struct servers *serverlist = NULL;	/* The record for each server.  */
static int num_servers = 0;		/* The number of servers.       */
int metaHeight = 0;			/* The number of list lines.	*/
char *metaWindowName;			/* The window's name.           */
int statusLevel;


/* The status strings:  The order of the strings up until statusNull is
 * important because the meta-client will display all the strings up to a
 * particular point.
 * 
 * The strings after statusNull are internal status types and are formatted
 * separatly from the other strings.
 * 
 * The string corresponding to "statusNull" is assigned to thoes servers which
 * have "statusNobody" or earlier strings in old, cached, meta-server data. */

char   *statusStrings[] =
{"OPEN:", "Wait queue:", "Nobody", "Timed out", "No connection",
 "Active", "CANNOT CONNECT", "DEFAULT SERVER"};

enum statusTypes {
  statusOpen = 0, statusWait, statusNobody, statusTout, statusNoConnect,
  statusNull, statusCantConnect, statusDefault
};

const int defaultStatLevel = statusTout;


/* Functions */
extern void terminate(int error);

static int open_port(char *host, int port, int verbose)
/* The connection to the metaserver is by Andy McFadden. This calls the
 * metaserver and parses the output into something useful */
{
  struct sockaddr_in addr;
  struct hostent *hp;
  int     sock;

  /* Connect to the metaserver */
  /* get numeric form */
  if ((addr.sin_addr.s_addr = inet_addr(host)) == -1) {
    if ((hp = gethostbyname(host)) == NULL) {
      if (verbose) fprintf(stderr, "unknown host '%s'\n", host);
      return (-1);
    } else {
      addr.sin_addr.s_addr = *(LONG *) hp->h_addr;
    }
  }
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    if (verbose) perror("socket");
    return (-1);
  }
  if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    if (verbose) perror("connect");
    close(sock);
    return (-1);
  }
  return (sock);
}


static void parseInput(char *in, FILE * out)
/* Read the information we need from the meta-server. */
{
  char    line[LINE + 1], *numstr, *point, **statStr;
  struct servers *slist;
  int     rtn, max_servers;
  int     count;

#ifdef DEBUG
   printf("In parseInput\n");
#endif   
   
  /* Create some space to hold the entries that will be read.  More space can
   * be added later */

  serverlist = (struct servers *) malloc(sizeof(struct servers) * 10);

  max_servers = 10;
  num_servers = 0;


  /* Add the default server */

  if (serverName) {
    strcpy(serverlist[num_servers].address, serverName);
    serverlist[num_servers].port = xtrekPort;
    serverlist[num_servers].status = statusDefault;
    serverlist[num_servers].players = 0;
    serverlist[num_servers].typeflag = ' ';
    num_servers++;
  }


  while (1) {
    /* Read a line */
    point = line;
    count = LINE + 1;
    
    do {
      if (!(--count)) {
	fputs("Warning: Line from meta server was too long!!!\n", stderr);
	++point;                           /* Pretend we read a '\n' */
	break;
      }
      
      rtn = *in++;
      if (!rtn)                              /* use a zero to mark end of buffer */
	return;
      
      *(point++) = rtn;
    }
    while (rtn != EOF && rtn != '\n');
    
    *(--point) = '\0';
    
    if (out != NULL) {                           /* Save a copy of the stuff
                                                  * we read */
      fputs(line, out);
      putc('\n', out);
    }


      /* Find somewhere to put the information that is just about to be
       * parsed */

    if (num_servers >= max_servers) {
      max_servers += 5;
      size_t size = sizeof(struct servers) * max_servers;
      serverlist = (struct servers *) realloc(serverlist, size);
    }
    
    slist = serverlist + num_servers;



    /* Is this a line we want? */

    if (sscanf(line, "-h %s -p %d %d %*d",
	       slist->address, &(slist->port),
	       &(slist->age)) != 3) {
      continue;
    }
    
    /* Find the status of the server, eg "Not responding". */
    
    for (statStr = statusStrings + statusLevel
           ; statStr >= statusStrings
           ; --statStr) {
      if ((numstr = strstr(line, *statStr)) != NULL) {
	(slist->status) = statStr - statusStrings;
	(slist->players) = 0;
	sscanf(numstr, "%*[^0123456789] %d", &(slist->players));
	break;
      }
    }
    
    if (statStr < statusStrings)               /* No status was allocated */
      continue;
    
    
    /* Read the flags */
    
    slist->typeflag = *(point - 1);
    
    
    /* Don't list Paradise Servers  */
    
    if (slist->typeflag != 'P') {
      
#ifdef DEBUG 
      printf("HOST:%-30s PORT:%-6d %12s %-5d %d %c\n",
	     serverlist[num_servers].address,
	     serverlist[num_servers].port,
	     statusStrings[serverlist[num_servers].status],
	     serverlist[num_servers].players,
	     serverlist[num_servers].typeflag);
#endif
      
      ++num_servers;
    }
  }
}

static int ReadMetasSend()
{
  char *metaservers;		/* our copy of the metaserver host names */
  char *token;			/* current metaserver host name          */
  struct sockaddr_in address;	/* the address of the metaservers	 */
  int length;			/* length of the address		 */
 
  /* host names of metaservers, default in data.c, comma delimited */ 
  if ((getdefault("metaserver")) != NULL)
    metaserver = getdefault("metaserver");

  /* port number of metaservers, unlikely to change, not a list */
  metaport = intDefault("metaport", metaport);

  /* whether to report everything that happens */
  verbose = booleanDefault("metaverbose", verbose);

  /* create the socket */
  if (msock < 0) {
    msock = socket(AF_INET, SOCK_DGRAM, 0);
    if (msock < 0) { perror("ReadMetasSend: socket"); return 0; }
    
    /* bind the socket to any address */
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_family      = AF_INET;
    address.sin_port        = 0;
    if (bind(msock,(struct sockaddr *)&address, sizeof(address)) < 0) {
      perror("ReadMetasSend: bind");
      close(msock);
      return 0;
    }
  }

  /* send request to a multicast metaserver on local area network */
  address.sin_family = AF_INET;
  address.sin_port = htons(metaport);
  address.sin_addr.s_addr = inet_addr("224.0.0.1");
  if (verbose) fprintf(stderr, 
		       "Requesting player list from nearby servers on %s\n",
		       inet_ntoa(address.sin_addr));
  if (sendto(msock, "?", 1, 0, (struct sockaddr *)&address,
	     sizeof(address)) < 0) {
    perror("ReadMetasSend: sendto");
  } else {
    sent++;
  }

  /* try each of the comma delimited metaserver host names */
  metaservers = strdup(metaserver);
  token = strtok(metaservers,",");

  while (token != NULL) {
    /* compose the address structure */
    address.sin_family = AF_INET;
    address.sin_port = htons(metaport);

    /* skip any blanks */
    while (*token == ' ') token++;

    /* attempt numeric translation first */
    if ((address.sin_addr.s_addr = inet_addr(token)) == -1) {
      struct hostent *hp;
        
      /* then translation by name */
      if ((hp = gethostbyname(token)) == NULL) {
        /* if it didn't work, return failure and warning */
        fprintf(stderr,
	  "Cannot resolve host %s, check for DNS problems?\n",
	  token);
      } else {
        int i;

        /* resolution worked, send a query to every metaserver listed */
        for(i=0;;i++) {

	  /* check for end of list of addresses */
	  if (hp->h_addr_list[i] == NULL) break;
	  address.sin_addr.s_addr = *(long *) hp->h_addr_list[i];
	  if (verbose) fprintf(stderr,
		"Requesting player list from metaserver %s at %s\n",
		token, inet_ntoa(address.sin_addr));
	  if (sendto(msock, "?", 1, 0, (struct sockaddr *)&address,
		sizeof(address)) < 0) {
	    perror("ReadMetasSend: sendto");
	  } else {
	    sent++;
	  }
        }
      }
    } else {
      /* call to inet_addr() worked, host name is in IP address form */
      if (verbose) fprintf(stderr, 
			   "Requesting player list from metaserver %s\n",
			   inet_ntoa(address.sin_addr));
      if (sendto(msock, "?", 1, 0, (struct sockaddr *)&address,
	sizeof(address)) < 0) {
        perror("ReadMetasSend: sendto");
      } else {
        sent++;
      }
    }

    /* look for next host name in list */
    token = strtok(NULL,",");
  } /* while (token != NULL) */
  return sent;
}

/* allocate or extend the server list */
static void grow(int servers)
{
  int size;
  if (serverlist == NULL) {
    size = sizeof(struct servers) * servers;
    serverlist = (struct servers *) malloc(size);
  } else {
    size = sizeof(struct servers) * ( servers + num_servers );
    serverlist = (struct servers *) realloc(serverlist, size);
  }
}

static struct servers *server_find(char *address, int port)
{
  int j;

  for(j=0;j<num_servers;j++) {
    struct servers *sp = serverlist + j;
    if ((!strcmp(sp->address, address)) && (sp->port == port)) {
      return sp;
    }
  }
  return NULL;
}

static void version_r(struct sockaddr_in *address) {
  char *p;
  int servers, i;
  time_t now = time(NULL);

  /* number of servers */
  p = strtok(NULL,"\n");
  if (p == NULL) return;
  servers = atoi(p);

  /* sanity check on number of servers */
  if (servers > 2048) return;
  if (servers < 0) return;

  if (verbose) fprintf(stderr,
		       "Metaserver at %s responded with %d server%s\n",
		       inet_ntoa(address->sin_addr),
		       servers,
		       servers == 1 ? "" : "s" );

  if (servers == 0) return;

  /* for each server listed by this metaserver packet */
  for(i=0;i<servers;i++) {
    struct servers *sp = NULL;
    char *host, type;
    int port, status, age, players, queue, throwaway;
      
    throwaway = 0;

    host = strtok(NULL,",");		/* hostname */
    if (host == NULL) continue;

    p = strtok(NULL,",");		/* port */
    if (p == NULL) continue;
    port = atoi(p);

    p = strtok(NULL,",");		/* status */
    if (p == NULL) continue;
    status = atoi(p);

    /* ignore servers based on status */
    if (status > statusLevel)
      throwaway++;
    /* the sp->why_dead workaround */
    if (status == 6)
      if ((status - 3) <= statusLevel) 
	throwaway--;

    p = strtok(NULL,",");		/* age (of data in seconds) */
    if (p == NULL) continue;
    age = atoi(p);

    p = strtok(NULL,",");		/* players */
    if (p == NULL) continue;
    players = atoi(p);

    p = strtok(NULL,",");		/* queue size */
    if (p == NULL) continue;
    queue = atoi(p);

    p = strtok(NULL,"\n");		/* server type */
    if (p == NULL) continue;
    type = p[0];

    /* ignore paradise servers */
    if (type == 'P') throwaway++;

    /* if it's to be thrown away, do not add this server, skip to next */
    if (throwaway) continue;

    /* find in current server list? */
    sp = server_find(host, port);

    /* if it was found, check age */
    if (sp != NULL) {
      if ((now-age) < (sp->when-sp->age)) {
	sp->age = now - (sp->when-sp->age);
	sp->when = now;
	sp->refresh = 1;
	sp->lifetime = 20;
	continue;
      } else {
	sp->age = age;
	sp->when = now;
	sp->lifetime = 20;
      }
    } else {
      /* not found, store it at the end of the list */
      grow(1);
      sp = serverlist + num_servers;
      num_servers++;
      strncpy(sp->address,host,LINE);
      sp->port = port;
      sp->age = age;
      sp->when = now;
      sp->lifetime = 4;
    }
    sp->refresh = 1;

    /* from meta.h of metaserver code */
#define SS_WORKING 0
#define SS_QUEUE 1
#define SS_OPEN 2
#define SS_EMPTY 3
#define SS_NOCONN 4
#define SS_INIT 5
    /* not a real metaserver number, but overcomes a limitation of dropping text */
    /* description of sp->why_dead */
#define SS_TOUT 6
    switch (status) {
    case SS_QUEUE:
      sp->status = statusWait;
      sp->players = queue;
      break;
    case SS_OPEN:
      sp->status = statusOpen;
      sp->players = players;
      break;
    case SS_EMPTY:
      sp->status = statusNobody;
      sp->players = 0;
      break;
    case SS_TOUT:
      sp->status = statusTout;
      sp->players = 0;
      break;
    case SS_NOCONN:			/* no connection */
    case SS_WORKING:		/* metaserver should not return this */
    case SS_INIT:			/* nor this */
    default:
      sp->status = statusNoConnect;
      sp->players = 0;
      break;
    }
    sp->typeflag = type;
    strcpy(sp->comment, "");
  }
}

static void version_s(struct sockaddr_in *address)
{
  char *p;
  time_t now = time(NULL);
  int throwaway = 0;

  /* use return address on packet as host address for this server,
  since it isn't practical for the server to know it's own address; as
  is the case with multihomed machines */
  char *host = inet_ntoa(address->sin_addr);

  if (verbose) fprintf(stderr, "Server at %s responded\n", host);

  p = strtok(NULL,",");	/* server type */
  if (p == NULL) return;
  char type = p[0];
  
  /* ignore paradise servers */
  if (type == 'P') return;
  
  p = strtok(NULL,",");		/* comment */
  if (p == NULL) return;
  char *comment = strndup(p, LINE-1);

  p = strtok(NULL,",");		/* number of ports */
  if (p == NULL) return;
  int ports = atoi(p);

  // TODO: accept more than one port reply
  
  p = strtok(NULL,",");		/* port */
  if (p == NULL) return;
  int port = atoi(p);
  
  p = strtok(NULL,",");		/* players */
  if (p == NULL) return;
  int players = atoi(p);

  p = strtok(NULL,",");		/* queue size */
  if (p == NULL) return;
  int queue = atoi(p);

  /* find in current server list? */
  struct servers *sp = server_find(host, port);

  /* if it was not found, add it to the end of the list */
  if (sp == NULL) {
    grow(1);
    sp = serverlist + num_servers;
    num_servers++;
  }

  /* add or update the entry */
  strncpy(sp->address, host, LINE);
  sp->port = port;
  sp->age = 0;
  sp->when = now;
  sp->refresh = 1;
  sp->lifetime = 20;
  sp->players = players;
  sp->status = statusOpen;
  sp->typeflag = type;
  strncpy(sp->comment, comment, LINE);
  free(comment);
}

static int ReadMetasRecv(int x)
{
  struct sockaddr_in address;	/* the address of the metaservers	 */
  socklen_t length;		/* length of the address		 */
  int bytes;			/* number of bytes received from meta'   */
  fd_set readfds;		/* the file descriptor set for select()	 */
  struct timeval timeout;	/* timeout for select() call		 */
  char packet[MAXMETABYTES];	/* buffer for packet returned by meta'	 */
  time_t now;			/* current time for age calculations     */
  int isawsomething = 0;        /* have I seen a response at all?        */ 

  /* now await and process replies */
  while(1) {
    char *p;

    FD_ZERO(&readfds);
    if (msock >= 0) FD_SET(msock, &readfds);
    timeout.tv_sec=4;
    timeout.tv_usec=0;

    if (x != -1) FD_SET(x, &readfds);
    if (select(FD_SETSIZE, &readfds, NULL, NULL,
	       (x != -1) ? NULL : &timeout) < 0) {
      perror("ReadMetasRecv: select");
      return 0;
    }

    /* if x activity, return immediately */
    if (x != -1 && FD_ISSET(x, &readfds)) return 0;
    if (msock < 0) return 0;

    /* if the wait timed out, then we give up */
    if (!FD_ISSET(msock, &readfds)) {
      if(isawsomething)
        return 1;          /* I do have new metaserver data */
      else
        return 0;          /* I don't have metaserver data at all */
    }

    /* so we have data back from a metaserver or server */
    isawsomething++;
    length = sizeof(address);
    bytes = recvfrom(msock, packet, MAXMETABYTES, 0, (struct sockaddr *)&address,
	&length );
    if (bytes < 0) {
      perror("ReadMetasRecv: recvfrom");
      return 0;
    }

    /* terminate the packet received */
    packet[bytes] = 0;
#ifdef DEBUG
    fprintf(stderr, "%s", packet);
#endif /* DEBUG */

    /* process the packet, updating our server list */

    /* version identifier */
    p = strtok(packet,",");
    if (p == NULL) continue;

    switch (p[0]) {
    case 'r': version_r(&address); seen++; break;
    case 's': version_s(&address); seen++; break;
    }

    /* finished processing the packet */

    /* if this is the first call, return on first reply, for sizing list */
    if (x == -1) return 1;

    /* if we have seen the same number of replies to what we sent, end */
    if (sent == seen) return 1;
  }
}

static void SaveMetasCache()
{
  FILE *cache;
  char cacheFileName[PATH_MAX];
  char tmpFileName[PATH_MAX];
  char *cacheName;
  int len;

  cacheName = getdefault("metaUDPCache");
  /* overwrite existing file if possible */
  if (cacheName && !findfile(cacheName, cacheFileName))
   strcpy(cacheFileName, cacheName);

  if (cacheName)
    {
      len = strlen(cacheFileName);
      strcpy(tmpFileName, cacheFileName);

      /* create a temporary file with roughly the same name */
      if ((cacheFileName[len - 1] == 'T') || (cacheFileName[len - 1] == 't'))
        tmpFileName[len-1] = 'R';
      else
        tmpFileName[len-1] = 'T';
      
      cache = fopen(tmpFileName, "w");

      if (cache == NULL)
        {
          fprintf(stderr,
                   "Cannot create a metaUDPCache temporary file `%s'\n",
              tmpFileName);
          fprintf(stderr, "Meta-server read will not be cached.\n");
        }
    }
  else
    {
      cache = NULL;
    }


  if (cache != NULL)
    {

      fwrite(&statusLevel, sizeof(statusLevel), 1, cache);
      fwrite(&num_servers, sizeof(num_servers), 1, cache);
      fwrite(serverlist, sizeof(struct servers), num_servers, cache);

      fclose(cache);

#ifdef WIN32
      /* Can't rename file to existing name under NT */
#ifdef _MSC_VER
      _unlink(cacheName);
#else
      unlink(cacheName);
#endif
#endif    
      if (rename(tmpFileName, cacheName) == -1)
	perror("Could not rename new cache file");
    }

}

static void LoadMetasCache()
{
  FILE *cache;
  char *cacheName;
  char cacheFileName[PATH_MAX];
  int  i;

  cacheName = getdefault("metaUDPCache");

  if(!cacheName)
    {
      num_servers = 0;
      return;
    }

  findfile(cacheName, cacheFileName);

  cache = fopen(cacheFileName, "r");
  if (cache == NULL) 
    { 
      num_servers = 0; 
      return; 
    }
 
  /* ignore the cache if user changed statusLevel */
  fread(&i, sizeof(i), 1, cache);
  if (i != statusLevel)
    {
      num_servers = 0;
      fclose(cache);
      return; 
    }
 

  /* read the server list into memory from the file */
  fread(&num_servers, sizeof(num_servers), 1, cache);
  serverlist = (struct servers *) malloc(sizeof(struct servers)*num_servers);
  fread(serverlist, sizeof(struct servers), num_servers, cache);
  fclose(cache);
  
  /* hunt and kill old server lines from cache */
  for(i=0;i<num_servers;i++)
    {
      int j;

      /* mark each server as needing to be refreshed */
      serverlist[i].refresh = 1;

      /* skip the deletion below if the entry was received recently */
      if (serverlist[i].lifetime-- > 0) continue;

      /* delete this entry by moving the ones above down */
      for(j=i;j<num_servers-1;i++)
	{
	  memcpy(&serverlist[j],&serverlist[j+1],sizeof(struct servers));
	}
      
      /* adjust the current entry pointer, limit, and resize the memory */
      i--;
      num_servers--;
      serverlist = 
	(struct servers *) realloc(serverlist, 
				   sizeof(struct servers) * ( num_servers ));
    }
}

static int ReadFromMeta()
/* Read from the meta-server.  Return TRUE on success and FALSE on failure. */
{
  FILE   *in, *out;
  char   *cacheName;
  char    cacheFileName[PATH_MAX];
  char    tmpFileName[PATH_MAX];
  char   *sockbuf, *buf;
  int     bufleft = BUF - 1;
  int     len;
  int     sock;

  if ((getdefault("metaserver")) != NULL)
    metaserver = getdefault("metaserver");

  metaport = intDefault("metaport", metaport);

  if ((sock = open_port(metaserver, metaport, 1)) <= 0)
    {
      fprintf(stderr, "Cannot connect to MetaServer (%s , %d)\n",
              metaserver, metaport);
      return 0;
    }

  /* Allocate a buffer and read until full */
  buf = sockbuf = (char *)malloc(BUF);
  while (bufleft > 0 && (len = read(sock, buf, bufleft)) > 0)
    {
      bufleft-=len;
      buf += len;
#ifdef DEBUG    
      printf("Read %d bytes from Metaserver\n", len);
#endif
    }
  close (sock);
  *buf = 0;                   /* End of buffer marker */

  if (len<0)
    {
      perror ("read");
      free(sockbuf);
      return 0;
    }

  cacheName = getdefault("metaCache");
  if (cacheName && !findfile(cacheName, cacheFileName))
   strcpy(cacheFileName, cacheName);        /* overwrite existing file if possible */

  if (cacheName)
    {
      len = strlen(cacheFileName);
      strcpy(tmpFileName, cacheFileName);

      /* Create a temporary file with roughly the same name */
      
      if ((cacheFileName[len - 1] == 'T') || (cacheFileName[len - 1] == 't'))
        tmpFileName[len-1] = 'R';
      else
        tmpFileName[len-1] = 'T';
      
      out = fopen(tmpFileName, "w");

      if (out == NULL)
        {
          fprintf(stderr,
                   "Cannot write to the metaCache temporary file `%s'.\n",
              tmpFileName);
          fprintf(stderr, "Meta-server read will not be cached.\n");
        }
    }
  else
    {
      out = NULL;
    }

  parseInput(sockbuf, out);

  if (out != NULL)
  {
    fclose(out);

#ifdef WIN32
   /* Can't rename file to existing name under NT */
#ifdef _MSC_VER
   _unlink(cacheName);
#else
   unlink(cacheName);
#endif
#endif    
    if (rename(tmpFileName, cacheName) == -1)
      perror("Could not write to cache file");
   }

  free(sockbuf);
  metaWindowName = "MetaServer List";

  return 1;
}


static int ReadFromCache()
/* Read from the cache.  Return TRUE on success and FALSE on failure. */
{
  FILE   *in;
  char   *cacheName;
  struct  servers *slist;
  char   *sockbuf, *buf;
  int     bufleft = BUF - 1;
  int     len;
  char    cacheFileName[PATH_MAX];
  
  cacheName = getdefault("metaCache");

  if (!cacheName)
    {
      fprintf(stderr,
              "You must define the .xtrekrc variable `metaCache' in\n");
      fprintf(stderr,
              "order to use the `show known servers' option.\n");
      return 0;
    }
  else
     if (!findfile(cacheName, cacheFileName) || !(in = fopen(cacheFileName, "r")) )
       {
         fprintf(stderr,
                 "The metaCache file `%s' is empty or not accessable.\n",
                 cacheName);
         return 0;
       }


  /* Allocate a buffer and read until full. Why do we
     go through this silly stuff? Because sockets are
     not file handles on all verions of Windows */
  buf = sockbuf = (char *)malloc(BUF);

  while (bufleft >0 && ((len = fread(buf, 1, bufleft, in)) > 0))
    {
    bufleft-=len;
    buf += len;
#ifdef DEBUG    
    printf("Read %d bytes from Metaserver cache file\n", len);
#endif
    }
  *buf = 0;                   /* End of buffer marker */
  fclose (in);

  if (len<0)
    {
    perror ("fread");
    free(sockbuf);
    return 0;
    }

  /* Show all servers known to have been reachable */

  if (statusLevel <= statusNobody)
    statusLevel = statusNobody;

  parseInput(sockbuf, NULL);


  /* Don't promise games from old data */
  for (slist = serverlist + num_servers - 1
       ; slist >= serverlist
       ; --slist)
    {
      if (slist->status <= statusNobody)
        slist->status = statusNull;
    }

  free(sockbuf);
  metaWindowName = "Known Servers";

  return 1;
}


void    parsemeta(int metaType)
/* Read and Parse the metaserver information, either from the metaservers
 * by UDP (1), from a single metaserver by TCP (3), or from the cache (2).
 * 
 * NOTE: This function sets the variable "num_servers" which is
 * used in newwin() to set the height of the meta-server window.
 */
{
  statusLevel = intDefault("metaStatusLevel", defaultStatLevel);

  if (statusLevel < 0)
    statusLevel = 0;
  else if (statusLevel >= statusNull)
    statusLevel = statusNull - 1;

  type = metaType;
  switch (type)
    {
      case 1:
        ReadMetasSend();
	LoadMetasCache();
	if (num_servers == 0) ReadMetasRecv(-1);
	if (num_servers != 0) {
	  metaHeight = num_servers + 5;
	} else {
	  printf("Warning: no response from metaservers, "
		 "are you firewalled?\n"
		 "         (no reply to probe on UDP port %d)\n", metaport);
	  metaHeight = num_servers + 10;
	}
        return;
	break;
      case 2:
	if (ReadFromCache() || ReadFromMeta()) 
	  {
           /* add 2 for header and quit button */
	    metaHeight = num_servers + 2;
	    return;
	  }
	terminate(0);
	break;
      case 3:
	if (ReadFromMeta() || ReadFromCache()) 
          {
           /* add 2 for header and quit button */
            metaHeight = num_servers + 2;
            return;
          }
	terminate(0);
	break;
    }
}


static void metarefresh(int i, W_Color color)
/* Refresh line i in the list */
{
  char    buf[LINE + 1];
  struct servers *sp;

  /* can't say a thing if line is beyond server list */
  if (i >= num_servers) {
    /* but we can at least blank the line shown */
    if (i < metaHeight-3) W_WriteText(metaWin, 0, i+1, color, "", 0, 0);
    return;
  }

  sp = serverlist + i;

  snprintf(buf, 56, "%-40s %14s ",
          strlen(sp->comment) > 0 ? sp->comment : sp->address,
          statusStrings[sp->status]);

  if (sp->status <= statusNull)
    {
      if (sp->status == statusOpen || sp->status == statusWait)
        {
          /* Don't print the number of players if nobody is playing */
          sprintf(buf + strlen(buf), "%-5d  ", sp->players);
        }
      else
        {
          strcat(buf, "       ");
        }

      switch (sp->typeflag)
        {
        case 'P':
          strcat(buf, "Paradise");
          break;
        case 'B':
          strcat(buf, "Bronco  ");
          break;
        case 'C':
          strcat(buf, "Chaos   ");
          break;
        case 'I':
          strcat(buf, "INL     ");
          break;
        case 'S':
          strcat(buf, "Sturgeon");
          break;
        case 'H':
          strcat(buf, "Hockey  ");
          break;
        case 'F':
          strcat(buf, "Dogfight");
          break;
        default:
          strcat(buf, "Unknown ");
          break;
        }
      
      if (type == 1)
	{
	  int age = sp->age;
	  char *units;

	  if (age > 86400)
	    {
	      age = age / 86400;
	      units = "d";
	    }
	  else if (age > 3600)
	    {
	      age = age / 3600;
	      units = "h";
	    }
	  else if (age > 90)
	    {
	      age = age / 60;
	      units = "m";
	    }
	  else
	    {
              units = "s";
	    }
	  sprintf(buf + strlen(buf), " %4d%s", age, units);
	}
    }

  W_WriteText(metaWin, 0, i+1, color, buf, strlen(buf), 0);
  sp->refresh = 0;
}


void    metawindow()
/* Show the meta server menu window */
{
  int     i;
  char *header;

  if (type == 1) {
    header = "Server ----------------------------------------- Status ------ Type ----- Age";
  } else {
    header = "Server ----------------------------------------- Status ------ Type";
  }
  W_WriteText(metaWin, 0, 0, W_Yellow, header, strlen(header), 0);

  for (i = 0; i < metaHeight; i++)
    metarefresh(i, textColor);

  /* Give the window the right name */
  W_RenameWindow(metaWin, metaWindowName);

  /* Add additional options */
  if (type == 1)
    W_WriteText(metaWin, 0, metaHeight-2, W_Yellow, "Refresh", 7, 0);
  W_WriteText(metaWin, 0, metaHeight-1, W_Yellow, "Quit", 4, 0);

  /* Map window */
  W_MapWindow(metaWin);
}


static void metadone(void)
/* Unmap the metaWindow */
{
  /* Unmap window */
  W_UnmapWindow(metaWin);
  if (type == 1) SaveMetasCache();
  free(serverlist);
}


void    metaaction(W_Event * data)
/* Recieve an action in the meta server window.  Check selection to see if
 * was valid.  If it was then we have a winner! */
{
  int     sock;
  char    buf[80];
  struct servers *slist;

#ifdef DEBUG
   printf("Got meta window action, y=%d\n", data->y);
#endif
  if ((data->y > 0) && (data->y <= num_servers))
    {
      slist = serverlist + data->y - 1;
      xtrekPort = slist->port;
      if (data->key==W_RBUTTON)  /* Guess at an observer port */
	{
          xtrekPort++;
          printf("Attempting to observe on port %d...\n",xtrekPort);
	  metarefresh(data->y - 1, W_Cyan);
	} else {
	  metarefresh(data->y - 1, W_Yellow);
	}
      W_Flush();
      serverName = strdup(slist->address);

      printf("Attempting to connect to %s on port %d...\n",serverName,xtrekPort);
      if ((sock = open_port(serverName, xtrekPort, 0)) <= 0)
        {
          fprintf(stderr,"Cannot connect to %s!\n",serverName);
          slist->status = statusCantConnect;
          metarefresh(data->y - 1, W_Red);
          W_Flush();
        }
      else
        {
	  metarefresh(data->y - 1, W_Green);
	  W_Flush();
          close(sock);
          sprintf(buf, "Netrek  @  %s", serverName);
          W_RenameWindow(baseWin, buf);
          metadone();
        }
    }
  else if (data->y == (metaHeight-2) && type == 1)
    {
      int i;

      W_WriteText(metaWin, 0, metaHeight-2, W_Red, "Asking for refresh from metaservers and nearby servers", 13, 0);
      W_Flush();
      ReadMetasSend();
    }
  else if (data->y == metaHeight-1)
    {
      metadone();
      terminate(0);
    }
}


void    metainput(void)
/* Wait for actions in the meta-server window.
 * 
 * This is really the meta-server window's own little input() function. It is
 * needed so we don't have to use all the bull in the main input(). Plus to
 * use it I'd have to call mapAll() first and the client would read in the
 * default server and then call it up before I can select a server. */
{
  W_Event data;

  statusLevel = intDefault("metaStatusLevel", defaultStatLevel);

  while (W_IsMapped(metaWin))
    {
      if (type == 1)
	{
	  do
            {
	      W_Flush();
	      if (ReadMetasRecv(W_Socket())) metawindow();
	    } while (!W_EventsPending());
	}
      W_NextEvent(&data);
      switch ((int) data.type)
        {
        case W_EV_KEY:
          if (data.Window == metaWin)
            metaaction(&data);
          break;
        case W_EV_BUTTON:
          if (data.Window == metaWin)
            metaaction(&data);
          break;
        case W_EV_EXPOSE:
          break;
        default:
          break;
        }
    }
}

#endif
