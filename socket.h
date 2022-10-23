#include <sys/select.h>
/* socket.c */
// void dummy(void);
// void print_sp_s_torp(char *sbuf, int type);
// int resetForce(void);
// int checkForce(void);
void connectToServer(int port);
// int set_tcp_opts(int s);
// int set_udp_opts(int s);
void callServer(int port, char *server);
extern int serverDead;
void socketPauseNoUser(void);
void socketPause(void);
int readFromServer(fd_set *readfds);
// int dotimers(void);
int getvpsize(char *bufptr);
// int doRead(int asock);
// void handleTorp(struct torp_spacket *packet);
// void handleTorpInfo(struct torp_info_spacket *packet);
// void handleStatus(struct status_spacket *packet);
// void handleSelf(struct you_spacket *packet);
// void handlePlayer(struct player_spacket *packet);
// void handleWarning(struct warning_spacket *packet);
void sendShortPacket(char type, char state);
void sendServerPacket(void *packet);
// void handlePlanet(struct planet_spacket *packet);
// void handlePhaser(struct phaser_spacket *packet);
void handleMessage(struct mesg_spacket *packet);
// void handleQueue(struct queue_spacket *packet);
void sendTeamReq(int team, int ship);
// void handlePickok(struct pickok_spacket *packet);
void sendLoginReq(char *name, char *pass, char *login, char query);
// void handleLogin(struct login_spacket *packet);
void sendTractorReq(char state, char pnum);
void sendRepressReq(char state, char pnum);
void sendDetMineReq(short int torp);
// void handlePlasmaInfo(struct plasma_info_spacket *packet);
// void handlePlasma(struct plasma_spacket *packet);
// void handleFlags(struct flags_spacket *packet);
// void handleKills(struct kills_spacket *packet);
// void handlePStatus(struct pstatus_spacket *packet);
// void handleMotd(struct motd_spacket *packet);
void sendMessage(char *mes, int group, int indiv);
// void handleMask(struct mask_spacket *packet);
void sendOptionsPacket(void);
// void handleBadVersion(struct badversion_spacket *packet);
int gwrite(int fd, char *buf, int bytes);
// void handleHostile(struct hostile_spacket *packet);
// void handlePlyrLogin(struct plyr_login_spacket *packet, int sock);
// void handleStats(struct stats_spacket *packet);
// void handlePlyrInfo(struct plyr_info_spacket *packet);
void sendUpdatePacket(LONG speed);
// void handlePlanetLoc(struct planet_loc_spacket *packet);
// void handleReserved(struct reserved_spacket *packet, int sock);
// void handleShipCap(struct ship_cap_spacket *packet);
void sendUdpReq(int req);
// void handleUdpReply(struct udp_reply_spacket *packet);
int closeUdpConn(void);
void printUdpInfo(void);
// void handleSequence(struct sequence_spacket *packet);
// void Log_Packet(char type, int act_size);
// void Log_OPacket(int type, int size);
void Dump_Packet_Log_Info(void);
// void print_packet(char *packet, int size);
// void print_opacket(char *packet, int size);
// char *strcpyp_return(register char *s1, register char *s2, register int length);
void become(struct player *pl);
