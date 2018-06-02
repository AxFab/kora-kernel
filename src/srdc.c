// -----
#define CLOCKS_PER_SEC 1000000

// the function returns an approximation of processor time used by the program (in us)
clock_t clock(void);

#if defined(__POSIX1)  ||
void tzset(void);
extern char *tzname[2];
#if defined(__SVID)
extern long timezone; // seconds west of UTC
extern int daylight;
#endif
#endif

char *asctime(const struct tm *tm);
char *ctime(const time_t *timep);
struct tm *gmtime(const time_t *timep);
struct tm *localtime(const time_t *timep);

char *asctime_r(const struct tm *tm, char *buf);
char *ctime_r(const time_t *timep, char *buf);
struct tm *gmtime_r(const time_t *timep, struct tm *result);
struct tm *localtime_r(const time_t *timep, struct tm *result);
// ctime() = asctime(localtime())
// time_t is UTC

time_t mktime(struct tm *tm);


// =------------
struct lconv localconv(void);
char *setlocale(int categ, const char *local);
// lang[_region][.encoding][@modifier]
// en_UK.UTF-8
// -------------------

char *getenv(const char *);
int putenv(char *);

// -------------------

int mblen(const char *s, size_t n) 
{
	if (*s > 0) 
        return 1;
    int m;
	if ((unsigned)*s >= 0xFE)
	    return -1;
	else if ((unsigned)*s >= 0xFC)
	    m = 6;
	else if ((unsigned)*s >= 0xF8)
	    m = 5;
	else if ((unsigned)*s >= 0xF0)
	    m = 4;
	else if ((unsigned)*s >= 0xE0)
	    m = 3;
	else if ((unsigned)*s >= 0xC0)
	    m = 2;
	
	if (m > n)
	    return -1;
	int i = m;
	while (i-- > 1) {
		if ((unsigned)s[i] < 0x80 || (unsigned)s[i] >= 0xC0)
		    return -1;
	}
	return m;
}

// -------------------

remove();
rename();

size_t strftime(char *buf, size_t len, const char *format, const struct tm *tm);

#define P_tmpdir "/tmp/"
#define P_tmpnam_pfx P_tmpdir "tmp."
#define L_tmpnam 20
#define L_tmpnam_pfx 9

char *tmpnam(char *s)
{
	if (s == NULL) {
		s = s_tmpnam;
	}
	strcpy(s, P_tmpdir "tmp.");
	for (i = L_tmpnam_pfx; i < L_tmpnam - 1; ++i)
	    s[i] = ""[rand() % 36];
	s[L_tmpnam - 1] = '\0';
	return s;
}

FILE *tmpfile()
{
	char buf[L_tmpnam];
	FILE *fp;
	int retry = 3;
	while (retry-- > 0) {
		fp = fopen(tmpnam(buf), "w+");
		if (fp)
		    return fp;
	}
	return NULL;
}

void system(const char *cmd)
{
	const char *bin = getenv("SHELL");
	const char *args[] = {
		"-c", cmd
	};
	pid_t pid = execv(bin, 2, args);
	waitpid(pid, &status, W_EXITED);
}
