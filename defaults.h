/* defaults.c */
int initDefaults(char *deffile);
char *getdefault(char *str);
int strcmpi(char *str1, char *str2);
int strncmpi(char *str1, char *str2, int max);
int booleanDefault(char *def, int preferred);
int intDefault(char *def, int preferred);
int findDefaults(char *deffile, char *file);
int resetdefaults(void);
int shipchange(int type);
int findfile(char *fname, char *found);
