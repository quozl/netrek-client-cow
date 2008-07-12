/* ping.c */
void handlePing(struct ping_spacket *packet);
int startPing(void);
int stopPing(void);
int sendServerPingResponse(int number);
int calc_lag(void);
