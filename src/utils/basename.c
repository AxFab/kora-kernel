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
#include <kora/utils.h>
#include <kora/fd.h>


opt_t options[] = {
    OPTION('a', "multiple", "Accept several operands as PATH"),
    OPTION('s', "suffix", "Use the next argument as SUFFIX, to remove"),
    OPTION('z', "zero", "Finish all line by NULL unstead of EOL"),
    END_OPTION("Print PATH without the parent directoies of the last component. "
    "If requested and find, SUFFIX can be removed from the end of the name.")
};

char *usages[] = {
    "NAME [SUFFIX]...",
    "OPTION... NAME...",
    NULL,
};

int main(int argc, char **argv)
{
    int multiple = 0;
    char *name = NULL;
    char *suffix = NULL;

    int o, n;
    char eol = '\n';
    for (o = 1; o < argc; ++o) {
        if (argv[o][0] != '-') {
            n++;
            continue;
        }

        char *opt = &argv[o][1];
        if (*opt == '-')
            opt = arg_long(&argv[o][2], options);
        for (; *opt; ++opt) {
            switch (*opt) {
            case 'a': // --multiple
                multiple = 1;
                break;
            case 's': // --suffix
                multiple = 1;
                suffix = argv[o + 1];
                argv[o + 1] = "-";
                break;
            case 'z': // --zero
                eol = '\0';
                break;
            case OPT_HELP: // --help
                arg_usage(argv[0], options, usages);
                return 0;
            case OPT_VERS: // --version
                arg_version(argv[0]);
                return 0;
            }
        }
    }

    if (n == 0) {
        fprintf(stderr, "%s: execpt an operand\n" HELP, argv[0], argv[0]);
        return -1;
    }

    for (o = 1; o < argc; ++o) {
        if (argv[o][0] == '-')
            continue;

        int lg = strlen(argv[o]);
        if (lg > 0 && argv[o][lg - 1] == '/')
            argv[o][lg - 1] = '\0';

        name = strrchr(argv[o], '/');
        if (name == NULL)
            name = argv[o];
        else
            name++;
        if (n >= 2 && multiple == 0) {
            suffix = argv[o + 1]; // TODO - Next operand
        }

        if (suffix != NULL) {
            int off = strlen(name) - strlen(suffix);
            if (strcmp(&name[off], suffix) == 0)
                name[off] = '\0';
        }

        printf("%s%c", name, eol);
        if (multiple == 0)
            break;
    }
    return 0;
}

