/*
 * mkkey.c -- create the public, private, and modulo keys for
 * netrek RSA verification and write out a keycap file
 * and generate a rsa "black box" that contains the code to
 * do a rsa encryption with the private key.
 *
 * Compile with something like:
 * cc -O -o mkkey mkkey.c -lmp
 * (if you're using GNU MP then:
 * cc -O -o mkkey mkkey.c -lmp -lgmp
 * mkkey needs stuff in gmp as well.)
 *
 * mkkey is used for two things:
 *
 * 1. Generating new RSA keys, creating the two keycap files key_name
 * and key_name.secret. key_name.secret is the keycap with the additional
 * field sk, which holds the secret (private) key.
 *
 * 2. Reading in a secret keycap file (or old-style keys.h file) and
 * generating a rsa "black box".  The "black box" is a set of C
 * source files named basename.c, basename_0.c, ... basename_N.c.
 * Together these define a function called that looks like this:
 *
 * void rsa_black_box(unsigned char* out, unsigned char* in,
 *                    unsigned char* public, unsigned char* global)
 *
 * Each of these parameters is expected to be an array KEY_SIZE of elements,
 * except public and global which may be NULL.  The function will perform an
 * RSA encryption on in, writing the result to out.  If public (or global)
 * is non-NULL the public (or global) key will be copied to public (or
 * global).
 *
 * The obfuscation process used to generate the "black box" relies on
 * the fact that an encryption with a known private key can be expressed
 * as a sequence of operations (which I call X & Y, these correspond
 * to the two different steps of the standard expmod algorithm) on
 * two variables, the message being encrypted and the encryption result.
 * So instead of storing the private key, we represent it by the sequence
 * of X & Y's.
 *
 * To make the sequence harder to follow we generate arrays of messages
 * and results and store the "real" message/result in one of the slots.
 * X & Y's are semi-randomly performed on the elements of the array,
 * and the elements are shuffled every once in a while.  I think of this
 * as playing a shell game with the message/result.
 *
 * As a final defense, the X & Y's & shufflings (which are called swaps
 * in the code) are split into N separate files.
 *
 * This is a merger of genkey.c (author: Ray Jones) and mkrsaclient.c
 * plus too many features.
 *
 * NOTE: the generated rsa "black box" files do not need any of the
 * rsa_util{mp}.c, or rsa_clientutil.c files.  They *do* need to be
 * linked with GNU MP, and not say SunOS's MP.
 *
 * If you make any non-trivial enhancements or find any bugs I'd like
 * to hear about them.  In any case, you are free to do whatever you
 * want with this code...
 *
 * Sam Shen (sls@aero.org)
 *
 * $Log: mkkey.c,v $
 * Revision 1.1.1.1  1998/11/01 17:24:10  siegl
 * COW 3.0 initial revision
 * */

/* if you hack mkkey and release it change the following line in
 * some reasonable way, perhaps like this:
 * static char version[] = "[atm: July 4, 1993] based on [sls: June 7, 1993]";
 */

static char version[] = "[sls: Febuary 10, 1994] + atm random";

#include "config.h"

#ifdef WIN32
#include <gmp\mp.h>
#include <gmp\gmp.h>
#include <windows.h>
#define getpid() GetCurrentProcessId()
#define getuid() (int)GetModuleHandle(NULL)
#else
#include INC_MP
#ifdef __GNU_MP__
#include <gmp.h>
#endif
MPTYPEDEF
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>
#include INC_SYS_TIME
#include <ctype.h>
#include "defs.h"

/* extern LONG random(); */

/* extern char* malloc();*/
extern void free();
extern int getpid();

#ifdef KEY_SIZE
#define SIZE KEY_SIZE
#define HALF (KEY_SIZE/2)
#else
#define SIZE 32
#define HALF 16
#endif

#define DEFAULT_N_SHELLS         3
#define DEFAULT_SWAP_STEPS       2
#define DEFAULT_RSA_BOX_FILE     "rsa_box"
#define DEFAULT_N_FILES          5
#define DEFAULT_FILE_RATIO       0.8

/* fadden's World Famous random stuff */
int rand_state = 0;		/* which state we're using */
char rand_state1[256];		/* big honking state for random() */
char rand_state2[256];		/* even more big honking state for random() */

static MPTYPE zero;

#define check_positive(n) assert(mcmp(n, zero) >= 0)

static LONG random256()
{
    unsigned LONG tmp;

    rand_state = 1-rand_state;		/* alternate between generators */
    if (rand_state)			/* select the appropriate state */
	(void)setstate(rand_state1);
    else
	(void)setstate(rand_state2);
    tmp = RANDOM();
    return (tmp >> 24) ^ ((tmp >> 16)  & 0xff) ^ ((tmp >> 8) & 0xff)
	^ (tmp & 0xff);
}

void rand_raw(str, num)
     unsigned char *str;
     int num;
{
    int i;

    for (i = 0; i < num - 1; i++)
	str[i] = random256();

    /* force it to be num digits long */
    str[i] = 0;
    while (! str[i])
	str[i] = random256();
    i++;

    for (; i < SIZE; i++)
	str[i] = 0;
}

void raw_to_num(out, in)
     MPTYPE out;
     unsigned char *in;
{
    int i;
    MPTYPE temp;
    MPTYPE twofiftysix;
    MPTYPE thisval;

    assignItom(temp,0);
    mult(temp, temp, out); /* IckyIcky how do you zero in libmp? */
    assignItom(twofiftysix,256);

    for (i = 0; i < SIZE; i++) {
	mult(temp, twofiftysix, temp);
        assignItom(thisval,in[SIZE - i - 1]);
	madd(temp, thisval, temp);
	mfree(thisval);
    }
    madd(temp, out, out); /* There is no copy function! */
    mfree(temp);
    mfree(twofiftysix);
}
 
void num_to_raw(out, in)  /* Destroys in */
     unsigned char *out;
     MPTYPE in;
{
    int i;
    short temp;

    for (i = 0; i < SIZE; i++)
    {
        if (!mcmp (in, zero))
          temp = 0;
        else
	  sdiv(in, 256, in, &temp);
        out[i] = temp & 0xFF;
    }
}

#ifdef __GNU_MP__
#define is_prime(n) mpz_probab_prime_p(n, 50)
#else
int is_prime(n)
     MPTYPE n;
{
    int i;
    short j;
    MPTYPE temp1;
    MPTYPE temp2;

    assignItom(temp2,0);

    /* 25 is quite a lot, actually */
    for (i = 0; i < 25; i++) {
	/* 
	 * We should choose a number that is between 0 and n, but this
	 * works well enough.
	 */
	j = (short) RANDOM();
	if (j < 0) j = (-j);
	assignItom(temp1,j);
	mp_pow(temp1, n, n, temp2);
	if (mcmp(temp1, temp2)) {
	    mfree(temp1);
	    mfree(temp2);
	    return (0);
	}
	mfree(temp1);
    }
    mfree(temp2);
    return (1);
}
#endif

static void mmod(a, b, c)
     MPTYPE a, b, c;
{
    MPTYPE quotient;
    assignItom(quotient,0);
    mdiv(a, b, quotient, c);
    mfree(quotient);
}

#ifdef __GNU_MP__
/*
 * GNU MP (and possibly other implementations of mp) does not have
 * invert so this was transliterated from the equivalent routine in
 * PGP 2.2
 *
 * rjones had an my_invert here but it didn't seem to work
 */

#define move(src, dest) madd(src, zero, dest)

#define iplus1  ( i==2 ? 0 : i+1 )      /* used by Euclid algorithms */
#define iminus1 ( i==0 ? 2 : i-1 )      /* used by Euclid algorithms */

static void invert(a, n, x)
     MPTYPE x, a, n;
{
    MPTYPE g[3];
    MPTYPE v[3];
    MPTYPE y;
    MPTYPE temp;
    MPTYPE temp2;
    int i;
    assignItom(y,0);
    assignItom(temp,0);
    assignItom(temp2,0);
    for (i = 0; i < 3; i++)
	assignItom(g[i],0);
    move(n, g[0]); move(a, g[1]);
    assignItom(v[0], 0); assignItom(v[1], 1); assignItom(v[2], 0);
    i = 1;
    while (mcmp(g[i], zero)) {
	check_positive(g[iminus1]);
	check_positive(g[i]);
	mdiv(g[iminus1], g[i], y, g[iplus1]);
	mult(y, v[i], temp);
	move(v[iminus1], v[iplus1]);
	msub(v[iplus1], temp, temp2);
	move(temp2, v[iplus1]);
	i = iplus1;
    }
    move(v[iminus1], x);
    while (mcmp(x, zero) < 0) {
	madd(x, n, temp2);
	move(temp2, x);
    }
    for (i = 0; i < 3; i++) {
	mfree(v[i]);
	mfree(g[i]);
    }
    mfree(y);
    mfree(temp);
    mfree(temp2);
}
#endif

/* verify_key checks to make sure a RSA key actually works
 */
void verify_key(raw_public, raw_private, raw_global)
     unsigned char* raw_public, *raw_private, *raw_global;
{
    MPTYPE orig_message;
    MPTYPE message;
    MPTYPE crypt_text;
    MPTYPE public;
    MPTYPE private;
    MPTYPE global;
    int failures = 0;
    int i;
    unsigned char temp[SIZE];

    assignItom(orig_message,0);
    assignItom(message,0);
    assignItom(crypt_text,0);
    assignItom(public,0);
    assignItom(private,0);
    assignItom(global,0);
    
    printf("Testing key");
    fflush(stdout);
    raw_to_num(public, raw_public);
    raw_to_num(private, raw_private);
    raw_to_num(global, raw_global);
    for (i = 0; i < 50; i++) {
	rand_raw(temp, SIZE - 1);
	raw_to_num(orig_message, temp);
	mp_pow(orig_message, private, global, crypt_text);
	mp_pow(crypt_text, public, global, message);
	if (mcmp(message, orig_message) != 0) {
	    putchar('!');
	    failures++;
	} else {
	    putchar('.');
	}
	fflush(stdout);
    }
    if (failures == 0)
	printf("key seems o.k.\n");
    else {
	printf("\nSorry, key is bogus.\n");
	exit(1);
    }
    mfree(orig_message); mfree(message); mfree(crypt_text);
    mfree(public); mfree(private); mfree(global);
}

/* kgetkeyname, kgetstr and kgetkey are stolen from Ted Hadley's
 * rsa_keycomp.c
 */

#define SEP                     ':'
#define GLOBAL_KEY_FIELD        "gk="
#define PUBLIC_KEY_FIELD        "pk="
#define SECRET_KEY_FIELD        "sk="
#define CLIENT_TYPE_FIELD       "ct="
#define CREATOR_FIELD           "cr="
#define CREATED_FIELD           "cd="
#define ARCH_TYPE_FIELD         "ar="
#define COMMENTS_FIELD          "cm="
#ifndef KEY_SIZE
#define KEY_SIZE                SIZE
#endif

/*
 * Extract the key name descriptor from an entry buffer
 * "Key Name:"
 */

int
kgetkeyname(src, dest)
   char *src;
   char *dest;
{
   int  l = strchr(src,SEP)-src;
   strncpy(dest, src, l);
   dest[l] = 0;
   return 1;
}

/*
 * Place contents of specified field entry into destination string.
 * "pk=Text Text Text:"
 */

int
kgetstr(src, field, dest)
   char *src, *field, *dest;
{
   char *s = (char*) strstr(src, field);
   int  l;
   if(!s)
      return 0;
   
   s += strlen(field);
   l = strchr(s,SEP)-s;
   strncpy(dest, s, l);
   dest[l] = 0;
   return 1;
}

/*
 * Place contents of specified binary field entry into destination string.
 * "pk=Text Text Text:"
 */

int
kgetkey(src, field, dest)
   
   char                 *src, 
                        *field;
   unsigned char        *dest;
{
   char                 unencoded_dest[KEY_SIZE*2+1],
                        uc[3];
   register char        *s;
   unsigned int         c;

   if(!kgetstr(src, field, unencoded_dest))
      return 0;

   uc[2] = '\0';
   /* convert encoded binary to binary */
   s = unencoded_dest;
   while(*s){
      uc[0] = *s++;
      uc[1] = *s++;
      sscanf(uc, "%x", &c);
      *dest++ = (unsigned char)c;
  }
   return 1;
}

int scan_array_element(buffer, xp, np)
     char* buffer;
     int* xp;
     int* np;
{
    char* bp = buffer;
    while (*bp && isspace(*bp))
	bp++;
    if (*bp == '0' && *(bp+1) == 'x') {
	if (sscanf(bp+2, "%x", xp) != 1)
	    return 0;
	bp += 2;
    } else if (sscanf(bp, "%d", xp) != 1)
	return 0;
    while (*bp && isxdigit(*bp))
	bp++;
    while (*bp && *bp != ',')
	bp++;
    bp++;
    *np = bp - buffer;
    return 1;
}

/* look through the text in buffer for name, and read in len comma
 * separated ints into data
 */
void get_array(buffer, name, data, len)
     char* buffer;
     char* name;
     unsigned char* data;
     int len;
{
    int i, n, x;
    char* start;
    start = (char*) strstr(buffer, name);
    if (start == NULL) goto err;
    while (*start && *start != '{')
	start++;
    if (!*start) goto err;
    start++;
    for (i = 0; i < len; i++) {
	if (scan_array_element(start, &x, &n) != 1)
	    goto err;
	data[i] = x;
	start += n;
    }
    return;
 err:
    fprintf(stderr, "Ooops! Couldn't get array %s in keys.h file.\n",
	    name);
    exit(1);
}

static char* header = "\
/*\n\
 * DO NOT EDIT THIS FILE!!! GENERATED AUTOMATICALLY!!!\n\
 */\n\
#include \"config.h\"\n\
#include <stdio.h>\n\
#include <gmp.h>\n\
\n\
void rsa_black_box(out, in, public, global)\n\
unsigned char* out;\n\
unsigned char* in;\n\
unsigned char* public;\n\
unsigned char* global;\n\
{\n\
    MP_INT m_msg;\n\
    MP_INT m_result;\n\
    MP_INT m_public;\n\
    MP_INT m_global;\n\
    MP_INT m_tmp;\n\
    int i;\n\
    mpz_init(&m_msg);\n\
    mpz_init(&m_result);\n\
    mpz_init(&m_public);\n\
    mpz_init(&m_global);\n\
    mpz_init(&m_tmp);\n\
    for (i = 0; i < %d; i++) {\n\
        mpz_mul_2exp(&m_msg, &m_msg, 8);\n\
	mpz_add_ui(&m_msg, &m_msg, in[%d-1-i]);\n\
    }\n\
";

static char* trailer = "\
    for (i = 0; i < %d; i++) {\n\
	mpz_divmod_ui(&m_result, &m_tmp, &m_result, 256);\n\
	*out++ = mpz_get_ui(&m_tmp);\n\
    }\n\
    mpz_clear(&m_msg);\n\
    mpz_clear(&m_result);\n\
    mpz_clear(&m_public);\n\
    mpz_clear(&m_global);\n\
    mpz_clear(&m_tmp);\n\
}\n\
";

void gen_key(fp, name, key, len)
     FILE* fp;
     char* name;
     unsigned char* key;
     int len;
{
    int i;
    unsigned LONG limb;
    for (i = len-4; i >= 0; i-= 4) {
	limb = (key[i+3] << 24) + (key[i+2] << 16) + (key[i+1] << 8) + key[i];
	fprintf(fp, "    mpz_mul_2exp(&m_%s, &m_%s, 32);\n", name, name);
	fprintf(fp, "    mpz_add_ui(&m_%s, &m_%s, (unsigned LONG) 0x%x);\n",
		name, name, limb);
    }
    fprintf(fp, "    if (%s != NULL) {\n", name);
    for (i = 0; i < len; i++)
	fprintf(fp, "        %s[%d] = %d;\n", name, i, key[i]);
    fprintf(fp, "    }\n");
}

static char* rsa_box_defs = "\
#define X(m, r, g) \\\n\
    mpz_mul(r, m, r);\\\n\
    mpz_mod(r, r, g)\n\
#define Y(m, r, g) \\\n\
    mpz_mul(m, m, m);\\\n\
    mpz_mod(m, m, g)\n\
#define SWAP(i, j) \\\n\
    do { \\\n\
        MP_INT tmp; \\\n\
        tmp = m[i]; m[i] = m[j]; m[j] = tmp; \\\n\
        tmp = r[i]; r[i] = r[j]; r[j] = tmp; \\\n\
    } while(0)\n\
";

static char* sequence_header = "\
{\n\
    MP_INT r[%d], m[%d], m_swap_tmp;\n\
    for (i = 0; i < %d; i++) {\n\
        mpz_init_set_ui(&r[i], 1); mpz_init(&m[i]);\n\
    }\n\
    mpz_set(&m[%d], &m_msg);\n\
#define g &m_global\n\
";

static char* sequence_trailer = "\
\n\
    mpz_set(&m_result, &r[%d]);\n\
    for (i = 0; i < %d; i++) {\n\
        mpz_clear(&r[i]); mpz_clear(&m[i]);\n\
    }\n\
}\n\
";

static char* per_box_header = "\
#include \"config.h\"\n\
#include <gmp.h>\n\
void rsa_partial_box_%d(m, r, g)\n\
MP_INT* m, * r;\n\
MP_INT* g;\n\
{\n\
";

static char* per_box_trailer = "\
}\n\
";

/*
 * Write out an obfuscated rsa computation.  This code is a little
 * ugly, forgive me...
 */
void gen_rsa_sequence(clientsfp, clients_basename, n_files,
                      raw_private, n_shells, swap_steps, file_ratio)
     FILE* clientsfp;
     unsigned char* raw_private;
     char* clients_basename;
     int n_files, n_shells, swap_steps;
     double file_ratio;
{
    FILE* fp;
    int i, j, real_j, tmp, done, bit, n_bits, file_no, in_fp;
    int n_bits_in_section, bit_in_section;
    char buf[200];
    char* op;
    double ratio;

    /* first, get a rough idea how many bits there are in raw_private */
    for (i = SIZE-1; i >= 0; --i) {
	if (raw_private[i] != 0)
	    break;
    }
    n_bits = i * 8;

    real_j = random256() % n_shells;
    fprintf(clientsfp, sequence_header, n_shells, n_shells, n_shells, real_j);
    fprintf(clientsfp, rsa_box_defs);
    bit = 0;
    file_no = 0;
    fp = NULL;
    bit_in_section = 0;
    n_bits_in_section = -1;
    in_fp = 0;
    while (1) {
	/* decide if we want to continue writing to wherever or to make
	 * a change
	 */
	done = 0;
    finish:
	if (done || (bit_in_section > n_bits_in_section)) {
	    if (in_fp) {
		fprintf(fp, per_box_trailer);
		fclose(fp);
		fprintf(clientsfp,
			"    rsa_partial_box_%d(m, r, &m_global);\n",
			file_no);
		file_no++;
		fp = clientsfp;
		in_fp = 0;
	    } else if (file_no < n_files) {
		sprintf(buf, "%s_%d.c", clients_basename, file_no);
		fp = fopen(buf, "w");
		if (fp == NULL) {
		    fprintf(stderr, "Couldn't open rsa box file \"%s\"!\n",
			    buf);
		    exit(1);
		}
		fprintf(fp, per_box_header, file_no);
		fprintf(fp, rsa_box_defs);
		in_fp = 1;
	    }
	    if (file_no == n_files)
		n_bits_in_section = n_bits - bit;
	    else {
		int bits_remaining;
		if (in_fp)
		    ratio = file_ratio;
		else
		    ratio = 1.0 - file_ratio;
		bits_remaining = n_bits - bit;
		n_bits_in_section =
		    (int) (ratio * (bits_remaining / (n_files - file_no)));
		/* have n_bits_in_section vary by 50% */
		n_bits_in_section = (5 * n_bits_in_section / 4 -
				     ((random256() % n_bits_in_section) / 2));
		printf("%d bits left, %d files left, %d bits in %s\n",
		       bits_remaining, n_files - file_no, n_bits_in_section,
		       in_fp ? buf : "main file");
	    }
	    bit_in_section = 0;
	}
	if (done)
	    break;
	
	/* check if we're done */
	done = 1;
	for (i = 0; i < SIZE; i++)
	    if (raw_private[i] != 0) {
		done = 0;
		break;
	    }
	if (done) goto finish;
	fprintf(fp, "    /* real_j is %d, bit %d is %d */\n", real_j,
		bit, raw_private[0] & 0x1);
	for (j = 0; j < n_shells; j++) {
	    if (j == real_j) {
		if (raw_private[0] & 0x1)
		    op = "X";
		else
		    op = 0;
	    } else if ((random256() % 2) == 0) {
		op = "X";
	    } else {
		op = "Y";
	    }
	    if (op != 0)
		fprintf(fp, "    %s(&m[%d], &r[%d], g);\n", op, j, j);
	}
	for (j = 0; j < n_shells; j++) {
	    if (j == real_j || ((random256() % 2) == 0)) {
		op = "Y";
	    } else {
		op = "X";
	    }
	    fprintf(fp, "    %s(&m[%d], &r[%d], g);\n", op, j, j);
	}
	if ((random256() % (swap_steps + 1)) == 0) {
	    for (j = 0; j < n_shells; j++) {
		tmp = random256() % (j + 1);
		if (real_j == tmp) {
		    real_j = j;
		} else if (real_j == j) {
		    real_j = tmp;
		}
		fprintf(fp, "    SWAP(%d, %d);\n", j, tmp);
	    }
	}
	for (i = 0; i < SIZE - 1; i++) {
	    raw_private[i] >>= 1;
	    if (raw_private[i+1] & 0x1)
		raw_private[i] |= 0x80;
	}
	raw_private[SIZE-1] >>= 1;
	bit++;
	bit_in_section++;
    }
    fprintf(fp, sequence_trailer, real_j, n_shells);
}

static char* allocbuf()
{
    char* buf = (char*) malloc(BUFSIZ);
    assert(buf != NULL);
    return buf;
}

static char* usage_lines[] = {
    "",
    "-v",
    "\tVerify the key only.",
    "",
    "-h keys.h-file [ key_name client_type architecture/OS creator comments ]",
    "\tRead key from old-style keys.h file using key_name ... to complete",
    "\tthe key (keys.h files don't have this information.)  If -v was used",
    "\tthen key_name etc are optional.",
    "",
    "-k keycap.secret-file",
    "\tRead key from keycap file with sk (secret key) field.",
    "",
    "-n n-shells",
    "\tSet the number of shells in the shell-game obfuscation.",
    "",
    "-s n-steps-to-swap",
    "\tSet the average number of steps between swaps.",
    "",
    "-nt",
    "\tSkip testing the key.",
    "",
    "-f n-files",
    "\tWrite out rsa computation into n-files separate files.",
    "",
    "-b client-sourcefile-basename",
    "\tUse client-sourcefile-basename as base name for rsa computation files.",
    "",
    "key_name client_type architecture/OS creator comments",
    "\tCreate a new key.  You may have to quote some of the args to make",
    "\tyour shell do the right thing.",
    "",
    NULL
};

static void usage()
{
    int i;
    for (i = 0; usage_lines[i]; i++) {
	printf("%s\n", usage_lines[i]);
    }
    exit(1);
}

int main(argc, argv)
     int argc;
     char* argv[];
{
    MPTYPE x, y, global, xyminus1, private, public;
    MPTYPE one, two;
    unsigned char temp[SIZE];
    unsigned char raw_global[SIZE], raw_private[SIZE], raw_public[SIZE];
    FILE* clientsfp;		/* clientsfp is the client source file */
    FILE* keycapfp;		/* keycapfp is the keycap file */
    FILE* skeycapfp;		/* skeycapfp is the keycap file with
				 * the secret key included
				 */
    int i, n, verify_only, n_shells, swap_steps, test_key, n_files;
    double file_ratio;
    char buf[200];
    time_t today;
    char* key_name, * client_type, * architecture, * creator, *class, *comments;
    char* created;
    char* keydoth, * keycap;
    char* clients_basename;

    /*
     * Initialize the random number generators.  Both get 256-byte pieces
     * of state (default is 128).  The second is partially seeded with
     * output from first.
     *
     * Total interesting bits in the seeds:
     *   time: 24 bits (assuming you can guess when it was generated)
     *   pid: 15 bits (most systems use a signed short)
     *   uid: 10 bits (most users will fall in this range)
     *   gid: ~3 bits (these are easy to guess)
     *
     * That's about 52 bits, perturbed in strange and unusual ways.  If this
     * ain't random, I'm lost.
     */
    /*srandom(time(0) ^ (getpid()<<16) ^ getuid());*/
    (void) initstate(time(0) ^ (getpid()<<16) ^ getgid(),
	rand_state1, 256);
    (void) initstate((getuid() ^ RANDOM()) | (time(0) % 65239) << 16,
	rand_state2, 256);
    rand_state = 0;

    printf("mkkey version %s\n", version);
    created = NULL;
    keydoth = NULL;
    keycap = NULL;
    verify_only = 0;
    n_shells = DEFAULT_N_SHELLS;
    swap_steps = DEFAULT_SWAP_STEPS;
    n_files = DEFAULT_N_FILES;
    clients_basename = DEFAULT_RSA_BOX_FILE;
    file_ratio = DEFAULT_FILE_RATIO;
    assignItom(zero,0);
    test_key = 1;
    n = 1;
    while (argc > n) {
	if (!strcmp(argv[n], "-h")) {
	    keydoth = argv[n+1];
	    n += 2;
	} else if (!strcmp(argv[n], "-k")) {
	    keycap = argv[n+1];
	    n += 2;
	} else if (!strcmp(argv[n], "-v")) {
	    verify_only = 1;
	    n++;
	} else if (!strcmp(argv[n], "-n")) {
	    n_shells = atoi(argv[n+1]);
	    n += 2;
	} else if (!strcmp(argv[n], "-s")) {
	    swap_steps = atoi(argv[n+1]);
	    n += 2;
	} else if (!strcmp(argv[n], "-nt")) {
	    test_key = 0;
	    n++;
	} else if (!strcmp(argv[n], "-b")) {
	    clients_basename = argv[n+1];
	    n += 2;
	} else if (!strcmp(argv[n], "-f")) {
	    n_files = atoi(argv[n+1]);
	    n += 2;
	} else if (!strcmp(argv[n], "-c")) {
	    n_files = RANDOM() % 10+1;
	    n++;
	} else if (!strcmp(argv[n], "-r")) {
	    sscanf(argv[n+1], "%lf", &file_ratio);
	    n += 2;
	} else
	    break;
    }
    if (keycap != NULL && keydoth != NULL) {
	fprintf(stderr, "%s: can't use both a keys.h file and keycap file\n",
		argv[0]);
	usage();
    }
    if (keycap == NULL && verify_only == 0 && (argc - n) !=5 && (argc - n) != 6) {
	usage();
    }
    if (n_shells < 1) {
	fprintf(stderr, "%s: number of shells must >= 1\n", argv[0]);
	usage();
    }
    if (swap_steps < 0) {
	fprintf(stderr,
		"%s: average number of steps between stops must be >= 0\n",
		argv[0]);
	usage();
    }
    if (n_files < 1) {
	fprintf(stderr, "%s: number of files must be >= 1\n", argv[0]);
	usage();
    }
    if (keycap == NULL) {
	key_name = argv[n];
	client_type = argv[n+1];
	architecture = argv[n+2];
	creator = argv[n+3];
	comments = argv[n+4];
	if (argv[n+5]) class = argv[n+5]; else class = "???";
    }
    
    printf("Source basename: \"%s\"\n", clients_basename);
    printf("Number of shells: %d\n", n_shells);
    printf("Number of steps between swaps: %d\n", swap_steps);
    printf("Number of files: %d\n", n_files);
    printf("Ratio of computation in files to main file: %lg\n", file_ratio);
    
    if (keydoth == NULL && keycap == NULL) {
	printf("Making new key, hold on....\n");
 
#ifdef __GNU_MP__
	printf("(using GNU MP)\n");
#else
	printf("(using the system's mp)\n");
#endif
	
	assignItom(one,1);
	assignItom(two,2);
	assignItom(x,0);
	assignItom(y,0);
	assignItom(global,0);
	assignItom(private,0);
	assignItom(public,0);
	assignItom(xyminus1,0);
	
	/*
	 * here we find x and y, two large primes
	 */
	rand_raw(temp, HALF);
	temp[0] |= 1; /* force odd */
	raw_to_num(x, temp);
	while (! is_prime(x))
	    madd(x, two, x);
	check_positive(x);
	assert(is_prime(x));
	
	rand_raw(temp, HALF);
	temp[0] |= 1; /* force odd */
	raw_to_num(y, temp);
	while (! is_prime(y))
	    madd(y, two, y);
	check_positive(y);
	assert(is_prime(y));
	
	/*
	 * the private key is a large prime (it should be the larger
	 * than x & y)
	 */
	rand_raw(temp, HALF + 1);
	temp[0] |= 1; /* force odd */
	raw_to_num(private, temp);
	while (! is_prime(private))
	    madd(private, two, private);
	check_positive(private);
	assert(is_prime(private));
	if (mcmp(x, private) > 0) {
#ifdef HAVE_GMP2_H
	  MPTYPE tmp; tmp[0] = x[0]; x[0] = private[0]; private[0] = tmp[0];
#else
	  MPTYPE tmp; tmp = x; x = private; private = tmp;
#endif
	}
	if (mcmp(y, private) > 0) {
#ifdef HAVE_GMP2_H
	  MPTYPE tmp; tmp[0] = x[0]; x[0] = private[0]; private[0] = tmp[0];
#else
	  MPTYPE tmp; tmp = y; y = private; private = tmp;
#endif
	}
	assert(mcmp(private, x) > 0);
	assert(mcmp(private, y) > 0);

	/*
	 * the modulus global is x * y
	 */
	mult(x, y, global);
	check_positive(global);

	/*
	 * the public key is such that
	 * (public * private) mod ((x - 1) * (y - 1)) == 1
	 */
	msub(x, one, x);
	msub(y, one, y);
	mult(x, y, xyminus1);
	assert(!is_prime(xyminus1));
	invert(private, xyminus1, public);
	check_positive(public);

	/* check to make sure the invert worked */
	{
	    MPTYPE ps;
	    MPTYPE m;
	    MPTYPE one;
	    assignItom(ps,0);
	    assignItom(one,1);
	    assignItom(m,0);
	    mult(private, public, ps);
	    mmod(ps, xyminus1, m);
	    if (mcmp(m, one) != 0) {
		printf("Ooops! invert failed!\n");
		exit(1);
	    }
	    mfree(ps); mfree(m); mfree(one);
	}

	/*
	 * convert to raw format
	 */
	num_to_raw(raw_global, global);
	num_to_raw(raw_public, public);
	num_to_raw(raw_private, private);

	/* cleanup */
	mfree(one);
	mfree(two);
	mfree(x);
	mfree(y);
	mfree(global);
	mfree(public);
	mfree(private);
	mfree(xyminus1);
    } else if (keydoth != NULL) {
	FILE* fp = fopen(keydoth, "r");
	char* buffer;
	struct stat statbuf;
	printf("Reading old key from keys.h file \"%s\"...\n", keydoth);
	if (fp == NULL) {
	    printf("%s: can't open keys.h file \"%s\"\n", argv[0],
		   keydoth);
	    perror("");
	    exit(1);
	}
	if (fstat(fileno(fp), &statbuf) < 0) {
	    perror("fstat");
	    exit(1);
	}
	buffer = (char*) malloc(statbuf.st_size);
	assert(buffer != NULL);
	fread(buffer, 1, statbuf.st_size, fp);
	fclose(fp);
	get_array(buffer, "key_global", raw_global, SIZE);
	get_array(buffer, "key_public", raw_public, SIZE);
	get_array(buffer, "key_private", raw_private, SIZE);
	free(buffer);
    } else {
	char* buffer;
	struct stat statbuf;
	FILE* fp = fopen(keycap, "r");
	printf("Reading key from keycap file \"%s\"...\n", keycap);
	if (fp == NULL) {
	    printf("%s: can't open keycap file \"%s\"\n", argv[0], keycap);
	    perror("");
	    exit(1);
	}
	if (fstat(fileno(fp), &statbuf) < 0) {
	    perror("fstat");
	    exit(1);
	}
	buffer = (char*) malloc(statbuf.st_size);
	assert(buffer != NULL);
	fread(buffer, 1, statbuf.st_size, fp);
	fclose(fp);
	key_name = allocbuf();
	client_type = allocbuf();
	architecture = allocbuf();
	creator = allocbuf();
	created = allocbuf();
	comments = allocbuf();
	if (!kgetkeyname(buffer, key_name) ||
	    !kgetstr(buffer, CLIENT_TYPE_FIELD, client_type) ||
	    !kgetstr(buffer, CREATOR_FIELD, creator) ||
	    !kgetstr(buffer, CREATED_FIELD, created) ||
	    !kgetstr(buffer, ARCH_TYPE_FIELD, architecture) ||
	    !kgetstr(buffer, COMMENTS_FIELD, comments) ||
	    !kgetkey(buffer, GLOBAL_KEY_FIELD, raw_global) ||
	    !kgetkey(buffer, PUBLIC_KEY_FIELD, raw_public) ||
	    !kgetkey(buffer, SECRET_KEY_FIELD, raw_private)) {
	    fprintf(stderr, "%s: can't parse keycap file \"%s\"\n", argv[0],
		    keycap);
	    exit(1);
	}
	free(buffer);
    }

    if (test_key)
	verify_key(raw_public, raw_private, raw_global);
    if (verify_only)
	exit(0);

    if (keycap == NULL) {
	keycapfp = fopen(key_name, "w");
	assert(keycapfp != NULL);
	strcpy(buf, key_name);
	strcat(buf, ".secret");
	skeycapfp = fopen(buf, "w");
	assert(skeycapfp != NULL);
    } else {
	keycapfp = NULL;
    }

    sprintf(buf, "%s.c", clients_basename);
    printf("Writing...\n");
    clientsfp = fopen(buf, "w");
    assert(clientsfp != NULL);

    /*
     * first write out some generic info
     */
    fprintf(clientsfp, "char key_name[] = \"%s\";\n", key_name);
    fprintf(clientsfp, "char client_type[] = \"%s\";\n", client_type);
    fprintf(clientsfp, "char client_arch[] = \"%s\";\n", architecture);
    fprintf(clientsfp, "char client_creator[] = \"%s\";\n", creator);
    fprintf(clientsfp, "char client_comments[] = \"%s\";\n", comments);
    if (created == NULL) {
	time(&today);
	strftime(buf, sizeof(buf), "%B %Y", gmtime(&today));
	created = buf;
    }
    fprintf(clientsfp, "char client_key_date[] = \"%s\";\n", created);
    if (keycapfp != NULL) {
	fprintf(keycapfp,
		"%s:ct=%s:cr=%s:\\\n   :cd=%s:ar=%s:cl=%s:\\\n   :cm=%s:\\\n",
		key_name, client_type, creator, buf, architecture, class, comments);
	fprintf(skeycapfp,
		"%s:ct=%s:cr=%s:\\\n   :cd=%s:ar=%s:cl=%s:\\\n   :cm=%s:\\\n",
		key_name, client_type, creator, buf, architecture, class, comments);
    }

    /*
     * write the client source header
     */
    fprintf(clientsfp, header, KEY_SIZE, KEY_SIZE);

    /*
     * write the global modulos
     */
    if (keycapfp != NULL) {
	fprintf(keycapfp, "   :gk=");
	fprintf(skeycapfp, "   :gk=");
	for (i = 0; i < SIZE; i++) {
	    fprintf(keycapfp, "%02x", (int) raw_global[i]);
	    fprintf(skeycapfp, "%02x", (int) raw_global[i]);
	}
	fprintf(keycapfp, ":\\\n");
	fprintf(skeycapfp, ":\\\n");
    }
    gen_key(clientsfp, "global", raw_global, SIZE);

    /*
     * write the public key
     */
    if (keycapfp != NULL) {
	fprintf(keycapfp, "   :pk=");
	fprintf(skeycapfp, "   :pk=");
	for (i = 0; i < SIZE; i++) {
	    fprintf(keycapfp, "%02x", (int) raw_public[i]);
	    fprintf(skeycapfp, "%02x", (int) raw_public[i]);
	}
	fprintf(keycapfp, ":\n");
	fprintf(skeycapfp, ":\\\n");
    }
    gen_key(clientsfp, "public", raw_public, SIZE);

    /*
     * write the private key
     */
    if (keycapfp != NULL) {
	fprintf(skeycapfp, "   :sk=");
	for (i = 0; i < SIZE; i++)
	    fprintf(skeycapfp, "%02x", (int) raw_private[i]);
	fprintf(skeycapfp, ":\n");
    }
    
    /*
     * write the sequence of computations that compute
     * (out ** private) mod global
     */
    gen_rsa_sequence(clientsfp, clients_basename, n_files,
		     raw_private, n_shells, swap_steps, file_ratio);

    /*
     * write the source trailer
     */
    fprintf(clientsfp, trailer, KEY_SIZE);

    putc('\n', stdout);

    /* cleanup */
    fclose(clientsfp);
    if (keycapfp != NULL) {
	fclose(keycapfp);
	fclose(skeycapfp);
    }

    exit(0);
}
