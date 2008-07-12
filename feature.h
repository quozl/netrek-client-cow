/* feature.c */
void checkFeature(struct feature_cpacket *packet);
void sendFeature(char *name, char feature_type, int value, char arg1, char arg2);
void reportFeatures(void);
void handleFeature(struct feature_cpacket *packet);
