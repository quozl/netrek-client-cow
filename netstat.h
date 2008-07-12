/* netstat.c */
void ns_init(int v);
void ns_record_update(int count);
void ns_do_stat(int v, int c);
double ns_get_tstat(void);
double ns_get_lstat(void);
int ns_get_nfailures(void);
char *ns_get_nfthresh_s(void);
void ns_set_nfthresh_s(char *s);
int ns_get_nfthresh(void);
void ns_set_nfthresh(int v);
