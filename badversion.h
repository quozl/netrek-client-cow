#define BADVERSION_SOCKET   0 /* CP_SOCKET version does not match, exiting */
#define BADVERSION_DENIED   1 /* access denied by netrekd */
#define BADVERSION_NOSLOT   2 /* no slot on queue */
#define BADVERSION_BANNED   3 /* banned */
#define BADVERSION_DOWN     4 /* game shutdown by server */
#define BADVERSION_SILENCE  5 /* daemon stalled */
#define BADVERSION_SELECT   6 /* internal error */

#define MAXBADVERSION 6

#ifdef __GNUC__
#define UNUSED __attribute__ ((unused))
#else
#define UNUSED
#endif

UNUSED static char *badversion_long_strings[] =
  {
    /* 0 */ "CP_SOCKET version does not match.\n"
            "You need a new version of the client code.",
    /* 1 */ "Access denied by server.",
    /* 2 */ "No free slots on server queue.",
    /* 3 */ "Banned from server.",
    /* 4 */ "Game shutdown by server.",
    /* 5 */ "Server daemon stalled, internal error.",
    /* 6 */ "Server reports internal error."
  };

UNUSED static char *badversion_short_strings[] =
  {
    /* 0 */ "Bad Version",
    /* 1 */ "Access Denied",
    /* 2 */ "No Free Slots",
    /* 3 */ "Banned",
    /* 4 */ "Game Shut",
    /* 5 */ "Internal Error",
    /* 6 */ "Internal Error"
  };
