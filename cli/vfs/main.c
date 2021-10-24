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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../utils.h"
#include <kora/mcrs.h>
#include <kernel/stdc.h>
#include <kernel/vfs.h>

#if !defined(_WIN32) // kernel/src/tests/check.h
# define _valloc(s)  valloc(s)
# define _vfree(p)  free(p)
#else
# define _valloc(s)  _aligned_malloc(s, PAGE_SIZE)
# define _vfree(p)  _aligned_free(p)
#endif

typedef struct cli_cmd cli_cmd_t;
struct cli_cmd {
    char name[16];
    char params[8];
    void (*call)(vfs_t* fs, size_t* param);
    int min_arg;
};

#define ARG_STR 1
#define ARG_INT 2
#define ARG_FLG 2
#define ARG_INO 20
#define ARGS_MAX 5


opt_t options[] = {
    OPTION('i', NULL, "Force usage of standard input"),
    // OPTION('a', "all", "Print all information"),
    END_OPTION("Display system information.")
};

char* usages[] = {
    "OPTION",
    NULL,
};

void do_help(vfs_t* fs, size_t* param);
void do_dump(vfs_t* vfs, size_t* param);
void do_ls(vfs_t* fs, size_t* param);
void do_stat(vfs_t* fs, size_t* param);
void do_cd(vfs_t* fs, size_t* param);
void do_chroot(vfs_t* fs, size_t* param);
void do_pwd(vfs_t* fs, size_t* param);
void do_open(vfs_t* fs, size_t* param);
void do_close(vfs_t* fs, size_t* param);
void do_look(vfs_t* fs, size_t* param);
void do_create(vfs_t* fs, size_t* param);
void do_unlink(vfs_t* fs, size_t* param);
void do_mkdir(vfs_t* fs, size_t* param);
void do_rmdir(vfs_t* fs, size_t* param);
void do_dd(vfs_t* fs, size_t* param);
void do_mount(vfs_t* fs, size_t* param);
void do_unmount(vfs_t* fs, size_t* param);
void do_extract(vfs_t* fs, size_t* param);
void do_checksum(vfs_t* vfs, size_t* param);
void do_img_open(vfs_t* fs, size_t* param);
void do_img_create(vfs_t* fs, size_t* param);
void do_img_copy(vfs_t* fs, size_t* param);
void do_img_remove(vfs_t* vfs, size_t* param);
void do_tar_mount(vfs_t* vfs, size_t* param);
void do_format(vfs_t* vfs, size_t* param);
void do_restart(vfs_t* vfs, size_t* param);

cli_cmd_t __commands[] = {
    { "HELP", { 0, 0, 0, 0, 0, 0, 0, 0, }, do_help, 1 },
    { "RESTART", { 0, 0, 0, 0, 0, 0, 0, 0, }, do_restart, 1 },
    { "DUMP", { 0, 0, 0, 0, 0, 0, 0, 0, }, do_dump, 1 },
    { "LS", { ARG_STR, ARG_INO, ARG_FLG, 0, 0, 0, 0, 0, }, do_ls, 1 },
    { "STAT", { ARG_STR, ARG_INO, ARG_FLG, 0, 0, 0, 0, 0, }, do_stat, 1 },
    { "CD", { ARG_STR, ARG_INO, ARG_FLG, 0, 0, 0, 0, 0, }, do_cd, 1 },
    { "CHROOT", { ARG_STR, ARG_INO, ARG_FLG, 0, 0, 0, 0, 0, }, do_chroot, 1 },
    { "PWD", { 0, 0, 0, 0, 0, 0, 0, 0, }, do_pwd, 0 },
    { "OPEN", { ARG_STR, ARG_INO, ARG_FLG, 0, 0, 0, 0, 0, }, do_open, 1 },
    { "CLOSE", { ARG_INO, ARG_FLG, 0, 0, 0, 0, 0, }, do_close, 1 },
    { "LOOK", { ARG_STR, ARG_INO, ARG_FLG, 0, 0, 0, 0, 0, }, do_look, 1 },

    { "CREAT", { ARG_STR, ARG_INO, ARG_FLG, 0, 0, 0, 0, 0, }, do_create, 1 },
    { "UNLINK", { ARG_STR, ARG_INO, ARG_FLG, 0, 0, 0, 0, 0, }, do_unlink, 1 },
    // LINK, RENAME, SYMLINK, READLINK ...
    { "MKDIR", { ARG_STR, ARG_INO, ARG_FLG, 0, 0, 0, 0, 0, }, do_mkdir, 1 },
    { "RMDIR", { ARG_STR, ARG_INO, ARG_FLG, 0, 0, 0, 0, 0, }, do_rmdir, 1 },

    { "DD", { ARG_STR, ARG_STR, ARG_INT, 0, 0, 0, 0, 0, }, do_dd, 3 },

    { "MOUNT", { ARG_STR, ARG_STR, ARG_STR, 0, 0, 0, 0, 0, }, do_mount, 3 },
    { "UNMOUNT", { ARG_STR, ARG_INO, ARG_FLG, 0, 0, 0, 0, 0, }, do_unmount, 1 },

    // CHMOD, CHOWN, UTIMES, TRUNCATE ...
    // OPEN DIR, READ DIR, CLOSE DIR ...
    // READ, WRITE ...
    // ACCESS ...
    // FDISK !?
    // IO <ino> <lba> [count] [flags] 
    // IOCTL <ino> <cmd> [..?]

    { "EXTRACT", { ARG_STR, ARG_STR, ARG_INO, ARG_FLG, 0, 0, 0, 0, }, do_extract, 1 },
    { "CKSUM", { ARG_STR, ARG_STR, ARG_INO, ARG_FLG, 0, 0, 0, 0, }, do_checksum, 1 },

    { "IMG_OPEN", { ARG_STR, ARG_STR, ARG_STR, 0, 0, 0, 0, 0, }, do_img_open, 2 },
    { "IMG_CREATE", { ARG_STR, ARG_STR, ARG_STR, 0, 0, 0, 0, 0, }, do_img_create, 2 },
    { "IMG_COPY", { ARG_STR, ARG_STR, ARG_INT, 0, 0, 0, 0, 0, }, do_img_copy, 2 },
    { "IMG_RM",  { ARG_STR, 0, 0, 0, 0, 0, 0, 0, }, do_img_remove, 1 },

    { "TAR", { ARG_STR, ARG_STR, 0, 0, 0, 0, 0, 0, }, do_tar_mount, 2 },

    { "FORMAT", { ARG_STR, ARG_STR, ARG_STR, 0, 0, 0, 0, 0, }, do_format, 2 },
};

void do_help(vfs_t* fs, size_t* param)
{
    printf("Usage:\n");
    int i, n;
    for (i = 0, n = sizeof(__commands) / sizeof(cli_cmd_t); i < n; ++i) {
        cli_cmd_t* cmd = &__commands[i];
        printf("  %s\n", cmd->name);
    }

}

int readParamString(const char** line, size_t* ptr)
{
    int i = 0;
    char buf[256];
    const char* str = *line;
    char stop = *str == '\'' || *str  == '"' ? *str : ' ';
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
        }
        else 
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
    *((char**)ptr) = strdup(buf);
    return 0;
}

int readParamInteger(const char** line, size_t* ptr)
{
    if ((*line)[0] == '\0')
        return 1;
    char* str;
    int value = strtol(*line, &str, 0);
    if (*line == str)
        return -1;
    *((int*)ptr) = value;
    return 0;
}

int readParamFlags(const char** line, size_t* ptr)
{
    return 1;
}

int readParamInode(const char** line, size_t* ptr)
{
    return 1;
}


vfs_t *VFS;

void fat_setup();
void fat_teardown();
void isofs_setup();
void isofs_teardown();
void ext2_setup();
void ext2_teardown();

void intialize()
{
    VFS = vfs_init();

#ifdef _EMBEDED_FS
    fat_setup();
    isofs_setup();
    ext2_setup();
#endif
}

void teardown()
{
#ifdef _EMBEDED_FS
    fat_teardown();
    isofs_teardown();
    ext2_teardown();
#endif

    vfs_sweep(VFS);
}

void do_load(vfs_t* fs, size_t* param)
{

}

void do_restart(vfs_t* fs, size_t* param)
{
    teardown();
    // Check all is clean
    intialize();
}

void readCommands(FILE* fp, bool reecho)
{
    int i, n;
    size_t len = 1024;
    char* line = malloc(len);

    for (;;) {
        // Read per line
        char* ret = fgets(line, len, fp);
        if (ret == NULL)
            break;

        // Ignore blank and comments
        char* str = line;
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
        char* space = strchr(cmd, ' ');
        char* newline = strchr(cmd, '\n');
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
        cli_cmd_t* routine = NULL;
        for (i = 0, n = sizeof(__commands)/sizeof(cli_cmd_t); i < n; ++i) {
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
                n = readParamString(&str, &args[i]);
            else if (routine->params[i] == ARG_INT)
                n = readParamInteger(&str, &args[i]);
            else if (routine->params[i] == ARG_INT)
                n = readParamFlags(&str, &args[i]);
            else if (routine->params[i] == ARG_INO)
                n = readParamInode(&str, &args[i]);
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
        routine->call(VFS, &args[0]);
        assert(irq_ready());
        // printf(" Command '%s' (%x, %x, %x, %x, %x)\n", cmd, args[0], args[1], args[2], args[3], args[4]);
    }

    free(line);
}

bool cliInteractive = false;

int main(int argc, char** argv)
{
    // Read parameter
    const char* stream = NULL;
    int o, n = 0;
    for (o = 1; o < argc; ++o) {
        if (argv[o][0] != '-') {
            n++;
            continue;
        }

        unsigned char* opt = (unsigned char*)&argv[o][1];
        if (*opt == '-') {
            if (opt[1] == '\0')
                break;
            opt = arg_long(&argv[o][2], options);
        }
        for (; *opt; ++opt) {
            switch (*opt) {
            case 'i':
                cliInteractive = true;
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

    // Read inputs commands
    bool useAll = false;
    intialize();
    if (n == 0) {
        readCommands(stdin, false);
    }
    else {
        for (o = 1; o < argc; ++o) {
            if (!useAll && argv[o][0] == '-') {
                if (argv[o][1] == '-' && argv[o][2] == '\0')
                    useAll = true;
                continue;
            }
            FILE* fp = fopen(argv[o], "r");
            if (fp == NULL) {
                continue;
            }
            readCommands(fp, true);
            fclose(fp);
        }
        if (cliInteractive)
            readCommands(stdin, false);

    }
    teardown();

    return 0;
}
