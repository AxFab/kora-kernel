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

#define BUF_SZ 512

opt_t options[] = {
    OPTION('A', "show-all", "Equivalent to options -vET"),
    OPTION('b', "number-nonblank", "Number non blank lines"),
    OPTION('e', NULL, "Equivalent to options -vE"),
    OPTION('E', "show-ends", "Display a $ character at the end of each line"),
    OPTION('n', "number", "Number all lines"),
    OPTION('s', "squeeze-blank", "Delete repeated empty lines"),
    OPTION('t', NULL, "Equivalent to options -vT"),
    OPTION('T', "show-tabs", "Display tab character as ^I"),
    OPTION('v', "show-non-printing", "Use ^ and M- notations for control characters, excepted for EOL and TAB"),
    END_OPTION("Concatenate FILEs or standard input into standard output.")
};

char *usages[] = {
    "OPTION... FILE...",
    NULL,
};

char *notations[] = {
    "?","?","?","?","?","?","?","?",
    "","^I","$\n","?","?","M-","?","?",
    "?","?","?","?","?","?","?","?",
    "?","?","?","?","?","?","?","?",
};
char notations_lg[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 2, 2, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
};

int main(int argc, char **argv)
{
    int i;
    int oflg = 0 ; // O_RDONLY;
    int numbers = 0;
    int blanks = 0;
    int squeeze = 0;
    int row = 0;
    char last = '\n';

    for (i = 1; i < argc; ++i) {
        if (argv[i][0] != '-' || argv[i][0] == '\0')
            continue;
        char *arg = argv[i][1] == '-' ? arg_long(&argv[i][2], options) : &argv[i][1];
        for (; *arg; ++arg) {
            switch (*arg) {
            case 'A' :
                blanks |= 7;
                break ;
            case 'b' :
                numbers ++;
                break;
            case 'e' :
                blanks |= 5;
                break ;
            case 'E' :
                blanks |= 4;
                break ;
            case 'n' :
                numbers = 2;
                break;
            case 's' :
                squeeze = 1;
                break ;
            case 't' :
                blanks |= 3;
                break ;
            case 'T' :
                blanks |= 2;
                break ;
            case 'v' :
                blanks |= 1;
                break ;
            case OPT_HELP :
                arg_usage(argv[0], options, usages) ;
                return 0;
            case OPT_VERS :
                arg_version(argv[0]);
                return 0;
            default:
                fprintf(stderr, "Option -%c non recognized.\n" HELP, *arg, argv[0]);
                return 1;
            }
        }
    }

    char buf[BUF_SZ] ;
    for (i = 1; i < argc; ++i) {
        if (argv[i][0] == '-' && argv[i][1] != '\0')
            continue;
        char *path = argv[i];
        int fd = strcmp(path, "-") ? open(path, oflg) : 1;
        if (fd == -1) {
            fprintf(stderr, "Unable to open file %s\n", path);
            return 1;
        }

        for (;;) {
            int lg = read(fd, buf, BUF_SZ);
            if (lg == 0)
                break ;
            if (lg < 0)
                return 1;
            if (numbers + blanks + squeeze == 0) {

                if (write(1, buf, lg) <= 0)
                    return 1;
            } else {
                int st = 0;
                while (st < lg) {
                    if (last == '\n' && (numbers > 2 || (numbers && buf[st] != '\n')))
                        dprintf(1, "%6d  ", ++row);
                    if (squeeze && last == '\n') {
                        // TODO - Erase until only one blank line!
                    }

                    int wr = st;
                    while ((buf[wr] < 0 || buf[wr] >= 0x20) && wr < lg)
                        wr++;
                    if (st != wr) {
                        if (write(1, & buf[st], wr - st) <= 0)
                            return 1;
                    }

                    if (wr == lg) {
                        //
                        break;
                    }

                    switch (buf[wr]) {
                    case '\n' :
                        if (blanks & 4)
                            write(1, notations[(int)buf[wr]], notations_lg[(int)buf[wr]]);
                        else
                            write(1, &buf[wr], 1);
                        break;
                    case '\t' :
                        if (blanks & 2)
                            write(1, notations[(int)buf[wr]], notations_lg[(int)buf[wr]]);
                        else
                            write(1, &buf[wr], 1);
                        break;
                    default:
                        if (blanks & 1)
                            write(1, notations[(int)buf[wr]], notations_lg[(int)buf[wr]]);
                        else
                            write(1, &buf[wr], 1);
                        break;
                    }
                    last = buf[wr];
                    st = wr + 1;
                }
            }
        }
    }
    return 0;
}
