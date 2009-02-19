/* input.c */
unsigned char getctrlkey(unsigned char **s);
void initkeymap(void);
void detsetallow(int _dummy);
void input(void);
int process_event(void);
void mkeyaction(W_Event *data);
void buttonaction(W_Event *data);
int getcourse(W_Window ww, int x, int y);
void lockPlanetOrBase(W_Window ww, int x, int y);
void macro_on(void);
void doMacro(W_Event *data);
void Key109(void);
