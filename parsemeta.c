#undef DEBUG

#include "config.h"
#include "copyright.h"

#ifdef META
#include INC_LIMITS
#include INC_FCNTL
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
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
#include "version.h"
#include "patchlevel.h"

#include "badversion.h"
#include "defaults.h"
#include "newwin.h"

#include "parsemeta.h"

#ifdef WIN32 /* socket garbage in case the client is not running on NT */
#define read(f,b,l) recv(f,b,l,0)
#define write(f,b,l) send(f,b,l,0)
#define close(s) closesocket(s)
#endif

/* Constants */

#define BUF     6144
#define LINE    100             /* Width of a meta-server line           */
#define MAXMETABYTES 2048       /* maximum metaserver UDP packet size    */
static int msock = -1;          /* the socket to talk to the metaservers */
static int sent = 0;            /* number of solicitations sent          */
static int seen = 0;            /* number of replies seen                */
static int verbose = 0;         /* whether to talk a lot about it all    */
static time_t last;             /* time of last refresh                  */

/* Local Types */

struct servers {
  char    address[LINE];        /* host name or ip address of server    */
  int     port;
  int     age;                  /* age in seconds as received           */
  time_t  when;                 /* date time this record received       */
  int     refresh;              /* data needs redisplaying              */
  int     lifetime;             /* remaining cache life of entry        */
  int     players;
  int     status;
  char    typeflag;
  char    comment[LINE];
  pid_t   pid;                  /* our last known child playing here    */
  int     exitstatus;           /* exit status of last known child here */
  int     observer;             /* set if child is an observer          */
};

struct servers *serverlist = NULL;      /* The record for each server.  */
static int num_servers = 0;             /* The number of servers.       */
static int chosen = -1;                 /* Arrow key chosen server.     */
static int metaHeight = 0;              /* The number of list lines.    */
static char *metaWindowName;            /* The window's name.           */
static int statusLevel;
static W_Window metaWin, metaList, metaHelpWin = NULL;
void *logo;

/* button offsets from end of list */
#define B_ADD 4
#define B_REFRESH 3
#define B_HELP 2
#define B_QUIT 1

#define N_TITLES 1
#define N_BUTTONS 4
#define N_GAP 1
#define N_OVERHEAD N_TITLES+N_BUTTONS+N_GAP

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

static const int defaultStatLevel = statusTout;

/* Functions */
extern void terminate(int error);

char *metahelp_message[] =
  {
    "Netrek Server List - Help",
    "",
    "This is a list of Netrek servers, from the Metaserver, or on your",
    "local network.  You can do this:",
    "",
    "  -  click on a server to join the game there,",
    "",
    "  -  middle-click on a server to immediately join as guest,",
    "",
    "  -  right-click on a server to observe the game there,",
    "",
    "  -  click refresh to reload the list of servers,",
    "",
    "-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --",
    "",
    "Tips for new Netrek players:",
    "",
    "  1.  when you enter the game, press the number 4 to start moving,",
    "  other numbers are slower or faster,",
    "",
    "  2.  you are the white ship in the centre of the left-hand screen,",
    "",
    "  3.  use right-click on the mouse to steer toward a point in space,",
    "  you will find it easier to turn at lower speeds,",
    "",
    "  4.  use left-click on the mouse to fire a torpedo, they travel over",
    "  time so you have to point ahead of your target, they hurt enemy",
    "  ships but usually pass right through your team,",
    "",
    "  5.  use middle-click on your mouse to fire a phaser, they are",
    "  instant so point at your target, but only work well close up,",
    "",
    "  6.  the aim of the game is to capture planets, killing enemies is",
    "  only a means to an end, and dying is a good thing, because you get a",
    "  new ship,",
    "",
    "  7.  but if someone kills you, they can begin to capture planets, so",
    "  it is best to not die in vain,",
    "",
    "  8.  people in your team will try to communicate so that you can all",
    "  do things together, because when you cooperate better than the other",
    "  team, you win.",
    "",
    "  9.  use the 'h' key to see a list of commands for your ship.",
    "",
    "Observing is a good way to learn.  When you join as an observer, you",
    "won't see much until you use 'l' (lower-case L) to lock on to a",
    "player.  Choose a good player and watch what they do.",
    "",
    "For more information on Netrek, visit http://netrek.org/beginner",
    NULL
  };

static void make_help()
{
  int i, l, h, w;

  /* calculate width and height required */
  h = (sizeof(metahelp_message)/sizeof(char *));
  w = 0;
  for (i=0; i<h; i++) {
    char *line = metahelp_message[i];
    if (line == NULL) break;
    l = strlen(line);
    if (l > w) w = l;
  }
  metaHelpWin = W_MakeWindow("Netrek Server List - Help", 500, 500,
                             w * W_Textwidth + 40, h * W_Textheight + 40,
                             0, 2, foreColor);
}

static void expo_help()
{
  int i, h;
  h = (sizeof(metahelp_message)/sizeof(char *));
  for (i=0; i<h; i++) {
    char *line = metahelp_message[i];
    if (line == NULL) break;
    W_WriteText(metaHelpWin, 20, i * W_Textheight + 20, textColor, line, -1,
                W_RegularFont);
  }
}

static void show_help()
{
  W_MapWindow(metaHelpWin);
}

static void hide_help()
{
  W_UnmapWindow(metaHelpWin);
}

static void toggle_help()
{
  if (W_IsMapped(metaHelpWin)) {
    hide_help();
  } else {
    show_help();
  }
}

static int ReadMetasSend()
{
  char *metaservers;            /* our copy of the metaserver host names */
  char *token;                  /* current metaserver host name          */
  struct sockaddr_in address;   /* the address of the metaservers        */
  static char *req;             /* the request packet for the metaserver */
  static int reqlen;            /* the length of the request packet      */

  last = time(NULL);

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
    req = malloc(80);
    snprintf(req, 80, "?version=netrek-client-cow-%s.%d",
             mvers, PATCHLEVEL);
    reqlen = strlen(req);
  }

  /* send request to a multicast metaserver on local area network */
  address.sin_family = AF_INET;
  address.sin_port = htons(metaport);
  address.sin_addr.s_addr = inet_addr("224.0.0.1");
  if (verbose) fprintf(stderr,
                       "requesting player list from nearby servers on %s\n",
                       inet_ntoa(address.sin_addr));

  if (sendto(msock, req, reqlen, 0, (struct sockaddr *)&address,
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
          "cannot resolve host %s, check for DNS problems?\n",
          token);
      } else {
        int i;

        /* resolution worked, send a query to every metaserver listed */
        for(i=0;;i++) {

          /* check for end of list of addresses */
          if (hp->h_addr_list[i] == NULL) break;
          address.sin_addr.s_addr = *(long *) hp->h_addr_list[i];
          if (verbose) fprintf(stderr,
                "requesting player list from metaserver %s at %s\n",
                token, inet_ntoa(address.sin_addr));
          if (sendto(msock, req, reqlen, 0, (struct sockaddr *)&address,
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
                           "requesting player list from metaserver %s\n",
                           inet_ntoa(address.sin_addr));
      if (sendto(msock, req, reqlen, 0, (struct sockaddr *)&address,
                 sizeof(address)) < 0) {
        perror("ReadMetasSend: sendto");
      } else {
        sent++;
      }
    }

    /* look for next host name in list */
    token = strtok(NULL,",");
  } /* while (token != NULL) */

  metaWindowName = "Netrek Server List";
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
    size = sizeof(struct servers) * (servers + num_servers);
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
                       "metaserver at %s responded with %d server%s\n",
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

    host = strtok(NULL,",");            /* hostname */
    if (host == NULL) continue;

    p = strtok(NULL,",");               /* port */
    if (p == NULL) continue;
    port = atoi(p);

    p = strtok(NULL,",");               /* status */
    if (p == NULL) continue;
    status = atoi(p);

    /* ignore servers based on status */
    if (status > statusLevel)
      throwaway++;
    /* the sp->why_dead workaround */
    if (status == 6)
      if ((status - 3) <= statusLevel)
        throwaway--;

    p = strtok(NULL,",");               /* age (of data in seconds) */
    if (p == NULL) continue;
    age = atoi(p);

    p = strtok(NULL,",");               /* players */
    if (p == NULL) continue;
    players = atoi(p);

    p = strtok(NULL,",");               /* queue size */
    if (p == NULL) continue;
    queue = atoi(p);

    p = strtok(NULL,"\n");              /* server type */
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
    case SS_NOCONN:                     /* no connection */
    case SS_WORKING:            /* metaserver should not return this */
    case SS_INIT:                       /* nor this */
    default:
      sp->status = statusNoConnect;
      sp->players = 0;
      break;
    }
    sp->typeflag = type;
    strcpy(sp->comment, "");
    sp->pid = -1;
    sp->exitstatus = 0;
    sp->observer = 0;
  }
}

static void version_s(struct sockaddr_in *address)
{
  char *p;
  time_t now = time(NULL);

  /* use return address on packet as host address for this server,
  since it isn't practical for the server to know it's own address; as
  is the case with multihomed machines */
  char *host = inet_ntoa(address->sin_addr);

  if (verbose) fprintf(stderr, "server at %s responded\n", host);

  p = strtok(NULL,","); /* server type */
  if (p == NULL) return;
  char type = p[0];

  /* ignore paradise servers */
  if (type == 'P') return;

  p = strtok(NULL,",");         /* comment */
  if (p == NULL) return;
  char *comment = strdup(p);
  if (strlen(comment) > LINE) comment[LINE] = '\0';

  p = strtok(NULL,",");         /* number of ports */
  if (p == NULL) return;
  // int ports = atoi(p);               /* not currently used */

  // TODO: accept more than one port reply

  p = strtok(NULL,",");         /* port */
  if (p == NULL) return;
  int port = atoi(p);

  p = strtok(NULL,",");         /* players */
  if (p == NULL) return;
  int players = atoi(p);

  p = strtok(NULL,",");         /* queue size */
  if (p == NULL) return;
  // int queue = atoi(p);               /* not currently used */

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
  if (type == 'u' && players == 0) {
    sp->status = statusNobody;
  } else {
    sp->status = statusOpen;
  }
  sp->typeflag = type;
  strncpy(sp->comment, comment, LINE);
  sp->pid = -1;
  sp->exitstatus = 0;
  sp->observer = 0;
  free(comment);
}

static int ReadMetasRecv(int x)
{
  struct sockaddr_in address;   /* the address of the metaservers        */
  socklen_t length;             /* length of the address                 */
  int bytes;                    /* number of bytes received from meta'   */
  fd_set readfds;               /* the file descriptor set for select()  */
  struct timeval timeout;       /* timeout for select() call             */
  char packet[MAXMETABYTES];    /* buffer for packet returned by meta'   */
  char *p;

  /* now await and process replies */

  FD_ZERO(&readfds);
  if (msock >= 0) FD_SET(msock, &readfds);
  timeout.tv_sec = 10;
  timeout.tv_usec = 0;

  if (x != -1) FD_SET(x, &readfds);
  if (select(FD_SETSIZE, &readfds, NULL, NULL, &timeout) < 0) {
    if (errno == EINTR) return 0;
    perror("ReadMetasRecv: select");
    return 0;
  }

  /* if x activity, return immediately */
  if (x != -1 && FD_ISSET(x, &readfds)) return 0;
  if (msock < 0) return 0;

  /* if the wait timed out, then we give up */
  if (!FD_ISSET(msock, &readfds)) return 0;

  /* so we have data back from a metaserver or server */
  length = sizeof(address);
  bytes = recvfrom(msock, packet, MAXMETABYTES, 0,
                   (struct sockaddr *)&address, &length );
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
  if (p == NULL) return 0;

  switch (p[0]) {
  case 'r': version_r(&address); seen++; break;
  case 's': version_s(&address); seen++; break;
  }

  return 1;
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
      for(j=i;j<num_servers-1;j++)
        {
          memcpy(&serverlist[j],&serverlist[j+1],sizeof(struct servers));
        }

      /* adjust the current entry pointer, limit, and resize the memory */
      i--;
      num_servers--;
      serverlist =
        (struct servers *) realloc(serverlist,
                                   sizeof(struct servers) * (num_servers));
    }
}


void    parsemeta()
/* Read and Parse the metaserver information, either from the metaservers
 * by UDP (1), from a single metaserver by TCP (3), or from the cache (2).
 */
{
  statusLevel = intDefault("metaStatusLevel", defaultStatLevel);

  if (statusLevel < 0)
    statusLevel = 0;
  else if (statusLevel >= statusNull)
    statusLevel = statusNull - 1;

  ReadMetasSend();
  LoadMetasCache();
  metaHeight = 2 + N_OVERHEAD;
}


static void redraw(int i)
/* Redraw line i in the list */
{
  char    buf[LINE + 1];
  struct servers *sp;

  /* can't say a thing if line is beyond server list */
  if (i >= num_servers) {
    /* but we can at least blank the line shown */
    if (i < metaHeight-3) W_WriteText(metaList, 0, i+1, W_White, "", 0, 0);
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

  strcat(buf, "    ");
  if (sp->pid != -1) {
    strcat(buf, sp->observer ? "Observing" : "Playing" );
  } else {
    switch (sp->exitstatus) {
    case EXIT_FORK_FAILURE:
      strcat(buf, "Cannot Start");
      break;
    case EXIT_UNKNOWN:
    case EXIT_OK:
      break;
    case EXIT_CONNECT_FAILURE:
      strcat(buf, "Connect Fail");
      break;
    case EXIT_LOGIN_FAILURE:
      strcat(buf, "Login Fail");
      break;
    default:
      {
        int badversion = (sp->exitstatus - EXIT_BADVERSION_BASE);
        if (badversion >= 0 && badversion <= MAXBADVERSION) {
          strcat(buf, badversion_short_strings[badversion]);
        } else {
          strcat(buf, "Unknown Response");
        }
      }
      break;
    }
  }

  W_Color color = W_White;
  if (i == chosen) color = W_Green;
  if (sp->status == statusCantConnect) color = W_Red;
  if (sp->pid != -1) color = W_Cyan;

  W_WriteText(metaList, 0, i+1, color, buf, -1, 0);
  sp->refresh = 0;
}


static char add_buffer[LINE];
static int add_offset;

static void add_init()
{
  add_buffer[0] = '\0';
  add_offset = 0;
}

static void add_redraw()
{
  char buf[LINE + 1];

  snprintf(buf, LINE, "Add a server: %s_", add_buffer);
  W_WriteText(metaList, 0, metaHeight-B_ADD, W_Yellow, buf, -1, 0);
}

static void add_commit()
{
  struct servers *sp;

  grow(1);
  sp = serverlist + num_servers;
  num_servers++;
  strncpy(sp->address, add_buffer, LINE);
  sp->port = 2592;
  sp->age = 0;
  sp->when = time(NULL);
  sp->refresh = 1;
  sp->lifetime = 20;
  sp->players = 0;
  sp->status = statusNobody;
  sp->typeflag = 'U';
  strncpy(sp->comment, add_buffer, LINE);
  sp->pid = -1;
  sp->exitstatus = 0;
  sp->observer = 0;
  metawindow();
}

static int add_key(W_Event *data)
{
  if (data->key == '\r') {
    add_commit();
    add_init();
    add_redraw();
  } else if (data->key == 21) {
    add_init();
    add_redraw();
  } else if (data->key == 8 || data->key == '\177') {
    if (add_offset > 0) {
      add_buffer[add_offset-1] = '\0';
      add_offset--;
      add_redraw();
    }
  } else if (add_offset < (LINE-1)) {
    add_buffer[add_offset+1] = '\0';
    add_buffer[add_offset] = data->key;
    add_offset++;
    add_redraw();
  }
  return 0;
}

void    metawindow()
/* Show the meta server menu window */
{
  int i, height;
  char *header;
  static int lastHeight = 0;

  if (!metaWin) {
    height = 250 + metaHeight * (W_Textheight + 8) + 4 * (metaHeight - 1);
    metaWin = W_MakeWindow("Netrek Server List", 0, 0, 716, height, NULL, 2,
                           foreColor);
    W_SetBackgroundImage(metaWin, "Misc/map_back.png");
    logo = W_ReadImage(metaWin, "netrek-green-white-300px.png");
    metaList = W_MakeMenu("metalist", 50, 200, LINE, metaHeight, metaWin, 1);
    lastHeight = metaHeight;
    make_help();
  } else {
    if (metaHeight > lastHeight) {
      W_ReinitMenu(metaList, LINE, metaHeight);
      W_ResizeMenu(metaList, LINE, metaHeight);
      lastHeight = metaHeight;
    }
    // FIXME: handle metaList growing beyond metaWin
  }

  header = "Server                                           Status        Type      Age";
  W_WriteText(metaList, 0, 0, W_Cyan, header, -1, 0);

  for (i = 0; i < metaHeight; i++) redraw(i);

  /* Give the window the right name */
  W_RenameWindow(metaWin, metaWindowName);

  /* Add additional options */
  W_WriteText(metaList, 0, metaHeight-B_REFRESH, W_Yellow,
              "Refresh                                           (r)",
              -1, 0);
  add_redraw();
  W_WriteText(metaList, 0, metaHeight-B_HELP, W_Yellow,
                "Help & Tips                                       (h)",
              -1, 0);
  W_WriteText(metaList, 0, metaHeight-B_QUIT, W_Yellow,
                "Quit                                              (q)",
              -1, 0);

  /* Map window */
  W_MapWindow(metaList);
  W_MapWindow(metaWin);
}


static void metadone(void)
{
  W_UnmapWindow(metaList);
  W_DropImage(logo);
  W_UnmapWindow(metaWin);
  SaveMetasCache();
  free(serverlist);
}


static void refresh()
{
  W_WriteText(metaList, 0, metaHeight-B_REFRESH, W_Red,
              "Refresh (in progress)", -1, 0);
  W_NextScreenShot(metaWin, 0, 0);
  W_Flush();
  ReadMetasSend();
}


static void refresh_cyclic()
{
  struct servers *sp;
  int i, interval = 30;

  /* while we have a local player, chances are they want their network
  link for play, and their eyes are on the tactical, they don't need
  to know about other servers */
  for (i=0;i<num_servers;i++) {
    sp = serverlist + i;
    if (sp->pid != -1) {
      if (!sp->observer) return;
      interval = 90;
    }
  }

  /* don't do until sufficient time has elapsed */
  if ((time(NULL) - last) > interval) {
    W_NextScreenShot(metaWin, 0, 0);
    ReadMetasSend();
  }
}


static void choose(int way)
{
  int was;
  int lo = 0;
  int hi = num_servers - 1;

  if (hi < 0) {
    return;
  }

  was = chosen;
  if (chosen == -1) {
    chosen = lo;
    if (way > 0) chosen = lo;
    if (way < 0) chosen = hi;
  } else {
    if (way > 0) { chosen++; if (chosen > hi) chosen = hi; }
    if (way < 0) { chosen--; if (chosen < lo) chosen = lo; }
  }
  if (was != chosen) {
    if (was != -1) redraw(was);
    redraw(chosen);
  }
}


static int chose(int which, unsigned char key)
{
  struct servers *sp;
  pid_t pid;

  if (which != chosen) {
    int was;
    was = chosen;
    chosen = which;
    if (was != -1) redraw(was);
    redraw(chosen);
  }

  sp = serverlist + which;
  xtrekPort = sp->port;
  if (key == W_RBUTTON) { /* Guess at an observer port */
    xtrekPort++;
    fprintf(stderr,
            "you chose to observe on %s, guessing port %d\n",
            sp->address, xtrekPort);
  }
  serverName = strdup(sp->address);

  sp->pid = -1;
  sp->exitstatus = EXIT_UNKNOWN;
  fprintf(stderr, "you chose server %s port %d\n", serverName, xtrekPort);

  if ((pid = fork()) == 0) {
    char *args[16];
    int argc = 0;

    setpgid(0, 0);
    args[argc++] = program;
    args[argc++] = "--server";
    args[argc++] = serverName;
    if (xtrekPort != DEFAULT_PORT) {
      char port[32];
      sprintf(port, "%d", xtrekPort);
      args[argc++] = "--port";
      args[argc++] = strdup(port);
    }

    if (key == W_MBUTTON)
      args[argc++] = "--fast-guest";

    args[argc++] = NULL;

    execvp(program, args);
    perror("execvp");
    _exit(1);
  }

  /* we are the parent, did the fork fail? */
  if (pid < 0) {
    perror("fork");
    sp->exitstatus = EXIT_FORK_FAILURE;
  } else {
    sp->pid = pid;
    sp->observer = (key == W_RBUTTON);
  }

  redraw(which);
  W_Flush();
  return 0;
}


static int button(W_Event *data)
{
  if ((data->y > 0) && (data->y <= num_servers)) { /* click on server */
    return chose(data->y - 1, data->key);
  }
  if (data->y == (metaHeight-B_REFRESH)) { /* refresh */
    refresh();
  } else if (data->y == metaHeight-B_HELP) { /* help */
    toggle_help();
  } else if (data->y == metaHeight-B_QUIT) { /* quit */
    metadone();
    terminate(0);
  }
  return 0;
}


static int key(W_Event *data)
{
  if (data->y == (metaHeight-B_ADD)) return add_key(data);
  if (data->key == 113 || data->key == 196) { /* q or ^d */
    metadone();
    terminate(0);
  } else if (data->key == 114 || data->key == 210) { /* r or ^r */
    refresh();
  } else if (data->key == W_Key_Up) {
    choose(-1);
  } else if (data->key == W_Key_Down) {
    choose(1);
  } else if (data->key == '\r' || data->key == ' ') { /* enter or space */
    if (chosen != -1) return chose(chosen, W_LBUTTON);
  } else if (data->key == 'g') { /* g, for guest */
    if (chosen != -1) return chose(chosen, W_MBUTTON);
  } else if (data->key == 'o') { /* o, for observe */
    if (chosen != -1) return chose(chosen, W_RBUTTON);
  } else if (data->key == 'h') {
    toggle_help();
  } else {
    return button(data);
  }
  return 0;
}


static int metareap_needed = 0;

/*! @brief Check if a child process, a playing client, has terminated.
    @details Attempts a no hang wait on each active client process in
    the server list and clears the pid entry if a child has
    terminated.
    @return activity count, number of processes seen to have terminated. */
static int metareap(void)
{
  struct servers *sp;
  int i, status, activity;
  pid_t pid;

  metareap_needed = 0;
  activity = 0;
  for (i=0;i<num_servers;i++) {
    sp = serverlist + i;
    if (sp->pid != -1) {
      pid = waitpid(sp->pid, &status, WNOHANG);
      if (pid == sp->pid) {
        sp->pid = -1;
        if (WIFEXITED(status)) {
          activity++;
          sp->exitstatus = WEXITSTATUS(status);
        }
        redraw(i);
        W_Flush();
      }
    }
  }
  return activity;
}


/*! @brief child death signal handler
    @details does nothing much, but needs to exist to ensure that
    select(2) is given EINTR when a child terminates, otherwise we
    will not note a termination until after the select times out.
*/
void sigchld(int ignored)
{
  metareap_needed++;
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

  (void) SIGNAL(SIGCHLD, sigchld);
  while (W_IsMapped(metaWin)) {
    while (1) {
      W_Flush();
      if (W_EventsPending()) break;
      if (ReadMetasRecv(W_Socket()) || metareap_needed) {
        metareap();
        metaHeight = num_servers + N_OVERHEAD;
        metawindow();
        W_Flush();
      }
      refresh_cyclic();
    }
    W_NextEvent(&data);
    switch ((int) data.type) {
    case W_EV_KEY:
      if (data.Window == metaList || data.Window == metaWin)
        if (key(&data)) return;
      if (data.Window == metaHelpWin) hide_help();
      break;
    case W_EV_BUTTON:
      if (data.Window == metaList)
        if (button(&data)) return;
      if (data.Window == metaHelpWin) hide_help();
      if (data.Window == metaWin && data.y < 200)
        W_NextScreenShot(metaWin, 0, 0);
      break;
    case W_EV_EXPOSE:
      if (data.Window == metaHelpWin) expo_help();
      if (data.Window == metaWin) {
        W_DrawScreenShot(metaWin, 0, 0);
        W_DrawImage(200, 9, logo);
      }
      break;
    case W_EV_CLOSED:
      if (data.Window == metaWin) {
        fprintf(stderr, "you quit, by closing the server list window\n");
        terminate(0);
      }
      break;
    default:
      break;
    }
  }
}

#endif
