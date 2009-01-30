/* reserved.c */
void makeReservedPacket(struct reserved_spacket *packet);
void encryptReservedPacket(struct reserved_spacket *spacket, struct reserved_cpacket *cpacket, char *server, int pno);
