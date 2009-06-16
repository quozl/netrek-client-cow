/* newwin.c */
pid_t newwin_fork();
void newwinmeta(char *hostmon, char *progname);
void newwin(char *hostmon, char *progname);
void showMotd(W_Window motdwin, int atline);
void mapAll(void);
void entrywindow(int *team, int *s_type);
void newMotdLine(char *line);
void motd_refresh(void);
