/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2021  <Fabien Bavent>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   - - - - - - - - - - - - - - -
 */
#ifndef __UTILS_H
#define __UTILS_H 1

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#  include <unistd.h>
#endif

void *memrchr(const void *s, int c, size_t n);
int tty_rows(void);
int tty_cols(void);

typedef void(*parsa_t)(void *, unsigned char);

typedef struct opt {
    unsigned char sh;
    char resv;
    char *lgopt;
    char *desc;
} opt_t;

#define VERSION "1.0.0"
#define COPYRIGHT "Copyright 2015-2020 KoraOS\nLicense AGPL <http://gnu.org/licenses/agpl.html>"
#define HELP "Enter the command \"%s --help\" for more information\n"

#define OPT_HELP (0xff)
#define OPT_VERS (0xfe)
#define OPTION(s,l,d) {s,0,l,d}
#define END_OPTION(d) \
    OPTION(OPT_HELP, "help", "Display help and leave the program"), \
    OPTION(OPT_VERS, "version", "Display version information and leave the program"), \
    {0,0,"",d}

#ifdef __GNUC__
#  include <sys/ioctl.h>
#  include <stdio.h>
#elif defined _WIN32
#  include <Windows.h>
#endif


int tty_cols(void)
{
    int cols = 80;
    // int lines = 24;
#ifdef TIOCGSIZE
    struct ttysize ts;
    ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
    cols = ts.ts_cols;
    // lines = ts.ts_lines;
#elif defined(TIOCGWINSZ)
    struct winsize ts;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
    cols = ts.ws_col;
    // lines = ts.ws_row;
#elif defined _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi) == TRUE)
        cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
#endif /* TIOCGSIZE */
    return cols;
}

int tty_rows(void)
{
    // int cols = 80;
    int lines = 24;
#ifdef TIOCGSIZE
    struct ttysize ts;
    ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
    // cols = ts.ts_cols;
    lines = ts.ts_lines;
#elif defined(TIOCGWINSZ)
    struct winsize ts;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
    // cols = ts.ws_col;
    lines = ts.ws_row;
#elif defined _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi) == TRUE)
        lines = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#endif /* TIOCGSIZE */
    return lines;
}

static inline unsigned char *arg_long(char *arg, opt_t *opts)
{
    for (; opts->sh; opts++) {
        if (opts->lgopt && strcmp(opts->lgopt, arg) == 0)
            return &opts->sh;
    }
    return (unsigned char *)"";
}

static inline void arg_usage(char *program, opt_t *opts, char **usages)
{
    opt_t *brief = opts;
    for (; brief->sh; brief++);
    printf("usage:  %s %s\n", program, *usages);
    while (*(++usages))
        printf("   or:  %s %s\n", program, *usages);

    printf("\n%s\n", brief->desc);

    if (opts->sh)
        printf("\nwith options:\n");
    for (; opts->sh; opts++) {
        if (opts->lgopt && opts->sh > 0x20 && opts->sh < 0x7F)
            printf("  -%c  --%-16s   ", opts->sh, opts->lgopt);
        else if (opts->sh > 0x20 && opts->sh < 0x7F)
            printf("  -%c    %-16s   ", opts->sh, "");
        else
            printf("      --%-16s   ", opts->lgopt);

        char *msg = opts->desc;
        int sz = tty_cols() - 30;
        int lg = strlen(msg);
        int ld = opts->lgopt != NULL ? strlen(opts->lgopt) : 0;
        do {
            int mx = sz;
            if (msg == opts->desc && ld > 16)
                mx -= ld - 16;
            if (mx > lg) {
                printf("%s\n", msg);
                break;
            }
            char *l = memrchr(msg, ' ', mx);
            if (l != NULL)
                mx = l - msg;
            fwrite(msg, mx, 1, stdout);
            msg += mx;
            lg -= mx;
            if (lg != 0)
                printf("\n                           ");
        } while (lg > 0);
    }
}

static inline void arg_version(char *program)
{
    if (strrchr(program, '/'))
        program = strrchr(program, '/') + 1;
    if (strrchr(program, '\\'))
        program = strrchr(program, '\\') + 1;
    printf("%s (Kora system) %s\n%s\n", program, VERSION, COPYRIGHT);
}

static inline int arg_parse(int argc, char **argv, parsa_t func, void *params, opt_t *opts)
{
    int o, n = 0;
    for (o = 1; o < argc; ++o) {
        if (argv[o][0] != '-') {
            n++;
            continue;
        }

        unsigned char *opt = (unsigned char *)&argv[o][1];
        if (*opt == '-')
            opt = arg_long(&argv[o][2], opts);
        for (; *opt; ++opt)
            func(params, *opt);
    }
    return n;
}

#endif /* __UTILS_H */
