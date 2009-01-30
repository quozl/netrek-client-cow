/* short.c */
void sendThreshold(short unsigned int v);
void handleVTorp(unsigned char *sbuf);
void handleSelfShort(struct youshort_spacket *packet);
void handleSelfShip(struct youss_spacket *packet);
void handleVPlayer(unsigned char *sbuf);
void handleSMessage(struct mesg_s_spacket *packet);
void handleShortReply(struct shortreply_spacket *packet);
void handleVTorpInfo(unsigned char *sbuf);
void handleVPlanet(unsigned char *sbuf);
void resetWeaponInfo(void);
void sendShortReq(char state);
void handleSWarning(struct warning_s_spacket *packet);
void add_whydead(char *s, int m);
void handleVKills(unsigned char *sbuf);
void handleVPhaser(unsigned char *sbuf);
void handle_s_Stats(struct stats_s_spacket *packet);
void new_flags(unsigned int data, int which);
