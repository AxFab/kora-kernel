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
#include "cli.h"
#include "utils.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <kora/hmap.h>
#include <kora/splock.h>


static void do_help()
{
    printf("Usage:\n");
    for (int i = 0; __commands[i].name; ++i) {
        cli_cmd_t *cmd = &__commands[i];
        printf("  %-12s  %s\n", cmd->name, cmd->summary);
    }
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static int cli_read_string(const char **line, size_t *ptr)
{
    int i = 0;
    char buf[256];
    const char *str = *line;
    char stop = *str == '\'' || *str == '"'?*str:' ';
    if (*str == '\n' || *str == '\0')
        return 1;
    if (stop != ' ')
        str++;
    while (*str != '\0' && *str != '\n' && *str != stop) {
        if (*str == '\\') {
            str++;
            if (*str == 'n')
                buf[i] = '\n';
            else
                buf[i] = *str;
        } else
            buf[i] = *str;

        str++;
        i++;
    }
    if (stop != ' ' && *str != stop)
        return -1;

    buf[i] = 0;
    if (stop != ' ')
        str++;

    *line = str;
    *((char **)ptr) = strdup(buf);
    return 0;
}

static int cli_read_integer(const char **line, size_t *ptr)
{
    if ((*line)[0] == '\0')
        return 1;
    char *str;
    int value = strtol(*line, &str, 0);
    if (*line == str)
        return -1;
    *((int *)ptr) = value;
    return 0;
}

static int cli_read_inode(const char **line, size_t *ptr)
{
    return 1;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void cli_commands(FILE *fp, void *ctx, bool reecho)
{
    int i, n;
    size_t len = 1024;
    char line[1024];

    for (;;) {
        // Read per line
        char *ret = fgets(line, len, fp);
        if (ret == NULL)
            break;

        // Ignore blank and comments
        char *str = line;
        while (str[0] && isspace(str[0]))
            str++;
        if (str[0] == '#' || str[0] == '\0')
            continue;

        // Isolate first word as sub-routine
        if (reecho)
            printf("%s", str);
        char cmd[16];
        strncpy(cmd, str, 15);
        cmd[15] = '\0';
        char *space = strchr(cmd, ' ');
        char *newline = strchr(cmd, '\n');
        if (space)
            *space = '\0';
        if (newline)
            *newline = '\0';
        for (i = 0; cmd[i]; ++i)
            cmd[i] = toupper(cmd[i]);
        str += strlen(cmd);

        // Identify Command
        if (strcmp(cmd, "EXIT") == 0)
            break;
        if (strcmp(cmd, "HELP") == 0) {
            do_help();
            continue;
        }
        cli_cmd_t *routine = NULL;
        for (i = 0; __commands[i].name; ++i) {
            if (strcmp(cmd, __commands[i].name) == 0) {
                routine = &__commands[i];
                break;
            }
        }
        if (routine == NULL) {
            printf(" Unknown command: %s\n", cmd);
            continue;
        }

        // Parse args
        newline = strchr(str, '\n');
        if (newline)
            *newline = '\0';
        size_t args[ARGS_MAX] = { 0 };
        for (i = 0; i < ARGS_MAX; ++i) {
            while (str[0] && isspace(str[0]))
                str++;
            if (routine->params[i] == 0)
                break;
            else if (routine->params[i] == ARG_STR)
                n = cli_read_string(&str, &args[i]);
            else if (routine->params[i] == ARG_INT)
                n = cli_read_integer(&str, &args[i]);
            else if (routine->params[i] == ARG_INO)
                n = cli_read_inode(&str, &args[i]);
            else
                n = -1;
            if (n < 0 || (n != 0 && i < routine->min_arg)) {
                printf(" Bad parameter %d at %s\n", i + 1, str);
                routine = NULL;
                break;
            }
        }

        if (routine == NULL) {
            printf(" Command abort: %s\n", cmd);
            continue;
        }

        // Run command
        assert(irq_ready());
        int retn = routine->call(ctx, &args[0]);
        if (retn != 0)
            return -1;
        // getchar();
        assert(irq_ready());

        // printf(" Command '%s' (%x, %x, %x, %x, %x)\n", cmd, args[0], args[1], args[2], args[3], args[4]);
    }
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

hmap_t storage;

typedef struct cli_slot cli_slot_t;
struct cli_slot
{
    void *obj;
    int type;
};

void cli_store(char *name, void *obj, int type)
{
    cli_slot_t *slot = malloc(sizeof(cli_slot_t));
    slot->obj = obj;
    slot->type = type;
    hmp_put(&storage, name, strlen(name), slot);
}

void *cli_fetch(char *name, int type)
{
    cli_slot_t *slot = hmp_get(&storage, name, strlen(name));
    if (slot == NULL || (type != 0 && slot->type != type))
        return NULL;
    return slot->obj;
}

void *cli_remove(char *name, int type)
{
    cli_slot_t *slot = hmp_get(&storage, name, strlen(name));
    hmp_remove(&storage, name, strlen(name));
    if (slot == NULL)
        return NULL;
    void *obj = slot->obj;
    free(slot);
    return obj;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

splock_t log_lock = INIT_SPLOCK;
char log_buf[1024];

int write(int, const char *, unsigned);

int cli_warn(const char *msg, ...)
{
    splock_lock(&log_lock);
    va_list ap;
    va_start(ap, msg);
    strcpy(log_buf, "\033[33m");
    int lg = 5 + vsnprintf(&log_buf[5], 1024 - 5, msg, ap);
    if (log_buf[lg - 1] == '\n')
        lg--;
    strcpy(&log_buf[lg], "\033[0m\n");
    lg += 5;
    va_end(ap);
    write(1, log_buf, lg);
    splock_unlock(&log_lock);
    return 0;
}

int cli_error(const char *msg, ...)
{
    splock_lock(&log_lock);
    va_list ap;
    va_start(ap, msg);
    strcpy(log_buf, "\033[31m");
    int lg = 5 + vsnprintf(&log_buf[5], 1024 - 5, msg, ap);
    if (log_buf[lg - 1] == '\n')
        lg--;
    strcpy(&log_buf[lg], "\033[0m\n");
    lg += 5;
    va_end(ap);
    write(1, log_buf, lg);
    splock_unlock(&log_lock);
    return -1;
}


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

opt_t options[] = {
    OPTION('i', NULL, "Force usage of standard input"),
    // OPTION('a', "all", "Print all information"),
    END_OPTION("Display system information.")
};

char *usages[] = {
    "OPTION",
    NULL,
};

static void cli_parse(void *ptr, unsigned opt, char *arg)
{
    cli_cfg_t *cfg = ptr;
    if (opt == 'i')
        cfg->is_interactive = true;
}

static int cli_exec(void *ptr, char *path)
{
    cli_cfg_t *cfg = ptr;
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
        return -1;
    cli_commands(fp, cfg->context, true);
    printf("\n");
    fclose(fp);
    return 0;
}

int cli_main(int argc, char **argv, void *context, void(*initialize)(), void(*teardown)())
{
    cli_cfg_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    hmp_init(&storage, 16);
    cfg.context = context;
    int n = arg_parse(argc, argv, cli_parse, &cfg, options, usages);

    if (initialize)
        initialize();

    if (n != 0 && arg_names(argc, argv, cli_exec, &cfg, options) != 0)
        return -1;
    if (n == 0 || cfg.is_interactive)
        cli_commands(stdin, cfg.context, false);

    if (teardown)
        teardown();
    return 0;
}
