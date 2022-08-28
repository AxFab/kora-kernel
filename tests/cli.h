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
#ifndef _SRC_CLI_H
#define _SRC_CLI_H 1

#include <stdio.h>
#include <stdbool.h>

#define ARGS_MAX 5

typedef struct cli_cmd cli_cmd_t;
typedef struct cli_cfg cli_cfg_t;

struct cli_cmd
{
    const char *name;
    const char *summary;
    char params[ARGS_MAX];
    int (*call)(void *ctx, size_t *param);
    int min_arg;
};

struct cli_cfg
{
    void *context;
    bool is_interactive;
};

#define CLI_END_OF_COMMANDS { NULL, NULL, { 0, 0, 0, 0, 0 }, NULL, 0 }

#define ARG_STR 1
#define ARG_INT 2

extern cli_cmd_t __commands[];

int cli_commands(FILE *fp, void *ctx, bool reecho);
int cli_main(int argc, char **argv, void *context, void(*initialize)(), void(*teardown)());

size_t cli_read_size(char *str);

void cli_store(char *name, void *obj, int type);
void *cli_fetch(char *name, int type);
void *cli_remove(char *name, int type);

int cli_msg(int ret, const char *clr, const char *msg, ...);

#define cli_info(...) cli_msg(0, "\033[34m", __VA_ARGS__)
#define cli_warn(...) cli_msg(0, "\033[33m", __VA_ARGS__)
#define cli_error(...) cli_msg(-1, "\033[31m", __VA_ARGS__)

#endif  /* _SRC_CLI_H */
