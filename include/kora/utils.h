/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2015-2019  <Fabien Bavent>
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
#ifndef __KORA_UTILS_H
#define __KORA_UTILS_H 1

#include <string.h>
#include <kora/fd.h>
#include <kora/format.h>
#include <kora/std.h>

typedef struct opt {
    char sh;
    char resv;
    char *lgopt;
    char *desc;
} opt_t;

#define VERSION "1.0.0"
#define COPYRIGHT "Copyright 2015-2018 KoraOS\nLicense AGPL <http://gnu.org/licenses/agpl.html>"
#define HELP "Enter the command \"%s --help\" for more information\n"

#define OPT_HELP (-1)
#define OPT_VERS (-2)
#define OPTION(s,l,d) {s,0,l,d}
#define END_OPTION(d) \
    OPTION(OPT_HELP, "help", "Display help and leave the program"), \
    OPTION(OPT_VERS, "version", "Display version information and leave the program"), \
    {0,0,"",d}

static inline char *arg_long(char *arg, opt_t *opts)
{
    for (; opts->sh; opts++) {
        if (opts->lgopt && strcmp(opts->lgopt, arg) == 0)
            return &opts->sh;
    }
    return "";
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
        if (opts->lgopt && opts->sh > 0)
            printf("  -%c  --%-16s \t%s\n", opts->sh, opts->lgopt, opts->desc);
        else if (opts->sh > 0)
            printf("  -%c    %-16s \t%s\n", opts->sh, "", opts->desc);
        else
            printf("      --%-16s \t%s\n", opts->lgopt, opts->desc);
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

#endif /* __KORA_UTILS_H */
