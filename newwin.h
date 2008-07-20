/* newwin.c */
pid_t newwin_fork();
void newwin(char *hostmon, char *progname);
void showMotd(W_Window motdwin, int atline);
void mapAll(void);
int entrywindow(int *team, int *s_type);
void newMotdLine(char *line);
