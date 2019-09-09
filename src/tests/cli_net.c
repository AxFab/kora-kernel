
#include <kernel/net.h>
#include <kora/mcrs.h>

#undef snprintf
#include <stdio.h>
#include <stdlib.h>

int read_cmds(FILE* fp);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void do_ls(const char* path)
{
}

void do_stat(const char* path)
{
}

void do_init(const char* path)
{
}

#ifdef _WIN32
#include <Windows.h>

void do_fs(const char* path)
{
	HMODULE hm = LoadLibrary(path);
	if (hm == 0) {

		LPVOID lpMsgBuf;
		DWORD dw = GetLastError();

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)& lpMsgBuf,
			0, NULL);

		printf("Error: %s\n", lpMsgBuf);
		return;
	}
	kmod_t* mod = GetProcAddress(hm, "kmod_info_isofs");
	if (mod == NULL) {
		printf("No kernel info structure\n");
		return;
	}
	mod->setup();
}
#else
#include <dlfcn.h>
void do_fs(const char* path)
{
	void* ctx = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
	if (ctx == NULL) {
		printf("Error loading %s: %s\n", path, dlerror());
		return;
	}
	char buf[128];
	char *nm = strrchr(path, '/') + 1;
	char *dt = strchr(nm, '.');
	if (dt)
	    dt[0] = '\0';
	snprintf(buf, 128, "kmod_info_%s", nm);
	printf("Open %s.\n", buf);
	
	kmod_t* mod = dlsym(ctx, buf);
	if (mod == NULL) {
		printf("No kernel info structure\n");
		return;
	}
	mod->setup();
}
#endif

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

char* tokenize(const char* line, const char** sreg)
{
	const char* OPS = "<>|&^()";
	const char* OPS2 = "<>|&";
	const char* QUOTE = "'\"`";
	int lg, c1, c2, s = 0;
	const char* tok1 = *sreg;
	char* tok2;
	if (tok1 == NULL)
		tok1 = line;

	while (isblank(*tok1) || *tok1 == '\n')
		tok1++;

	if (*tok1 == '\0')
		return NULL;

	*sreg = tok1;
	c1 = *tok1;
	if (strchr(OPS, c1) != NULL) {
		tok1++;
		c2 = *tok1;
		if (c2 == c1 && strchr(OPS2, c1) != NULL)
			tok1++;
	}
	else if (strchr(QUOTE, c1) != NULL) {
		tok1++;
		c2 = *tok1;
		*sreg = tok1;
		while (c2 && c1 != c2) {
			c2 = tok1[1];
			tok1++;
		}
		s = 1;
		if (c2)
			tok1++;
	}
	else {
		while (strchr(OPS, *tok1) == NULL && *tok1 != '\0' && *tok1 != '\n' && (*tok1 < 0 || !isblank(*tok1)))
			tok1++;
	}

	lg = tok1 - (*sreg);
	if (lg == 0) {
		tok1++;
		lg = tok1 - (*sreg);
	}

	tok2 = (char*)malloc((lg + 1) * sizeof(char));
	memcpy(tok2, *sreg, lg * sizeof(char));
	tok2[lg - s] = '\0';
	*sreg = tok1;
	return tok2;
}

void do_help(const char* path);

#define _ROUTINE(n,d) { #n, d, do_ ## n }
struct {
	const char* name;
	const char* desc;
	void(* entry)(const char *name);
} routines[] = {
	_ROUTINE(help, "Display this help"),
	_ROUTINE(ls, "List entry of directory"),
	_ROUTINE(fs, "Register a new file system"),
	_ROUTINE(init, "Create a new node to virtual network"),
	{ NULL, NULL }
};

void do_help(const char* path)
{
	printf("Usage:\n");
	printf("    command [argument] [options...]\n");
	printf("\nwith commands:\n");

	int i = 0;
	while (routines[i].entry != NULL) {
		printf("    %-12s %s\n", routines[i].name, routines[i].desc);
		++i;
	}

}

int read_cmds(FILE *fp)
{
	char buf[512];
	for (;;) {
		printf("> ");
		char* rp = fgets(buf, 512, fp);
		if (rp == NULL) {
			printf("\n");
			break;
		}
		if (fp != stdin)
			printf("%s", buf);

		char* token;
		char* cmd = NULL;
		char* arg = NULL;
		bool run = true;
		while ((token = tokenize(buf, &rp)) != NULL) {
			if (cmd == NULL)
				cmd = token;
			else if (arg == NULL && token[0] != '-')
				arg = token;
			else {
				printf("Invalid token \n");
				run = false;
				break;
			}
		}

		if (cmd == NULL || !run)
			continue;

		if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "q") == 0)
			break;

		int i = 0;
		while (routines[i].entry != NULL) {
			if (strcmp(routines[i].name, cmd) == 0) {
				routines[i].entry(arg);
				run = false;
				free(cmd);
				if (arg != NULL)
					free(arg);
				break;
			}
			++i;
		}

		if (run)
			printf("Unknown command %s \n", cmd);
	}
	return 0;
}

int main(int argc, char **argv)
{
	kSYS.cpus = kalloc(sizeof(struct kCpu) * 2);
	for (int o = 1; o < argc; ++o) {
        if (argv[o][0] == '-')
            continue;
        FILE *fp = fopen(argv[o], "r");
        if (fp == NULL)
            continue;
        read_cmds(fp);
        fclose(fp);
    }
	return read_cmds(stdin);
}

