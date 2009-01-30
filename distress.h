int makedistress(struct distress *dist, char *cry, char *pm);
void Dist2Mesg(struct distress *dist, char *buf);
void HandleGenDistr(char *message, unsigned char from, unsigned char to, struct distress *dist);
