#ifndef VMSUTILS_H
#define VMSUTILS_H

typedef long fd_set;
typedef int uid_t;

void exit();

#define FD_ZERO(fdl)         ((*fdl)=0L)
#define FD_SET(fd, fdl)     ((*fdl) |= (1<<(fd)))
#define FD_ISSET(fd, fdl)   ((*fdl) & (1<<(fd)))

#endif VMSUTILS_H
