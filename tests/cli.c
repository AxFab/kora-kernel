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
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <kora/hmap.h>
#include <kora/splock.h>
#include <errno.h>

static int read_errno(const char *str)
{
#ifdef EPERM
    if (strcmp(str, "EPERM") == 0)  return EPERM;
#endif
#ifdef ENOENT
    if (strcmp(str, "ENOENT") == 0)  return ENOENT;
#endif
#ifdef ESRCH
    if (strcmp(str, "ESRCH") == 0)  return ESRCH;
#endif
#ifdef EINTR
    if (strcmp(str, "EINTR") == 0)  return EINTR;
#endif
#ifdef EIO
    if (strcmp(str, "EIO") == 0)  return EIO;
#endif
#ifdef ENXIO
    if (strcmp(str, "ENXIO") == 0)  return ENXIO;
#endif
#ifdef E2BIG
    if (strcmp(str, "E2BIG") == 0)  return E2BIG;
#endif
#ifdef ENOEXEC
    if (strcmp(str, "ENOEXEC") == 0)  return ENOEXEC;
#endif
#ifdef EBADF
    if (strcmp(str, "EBADF") == 0)  return EBADF;
#endif
#ifdef ECHILD
    if (strcmp(str, "ECHILD") == 0)  return ECHILD;
#endif
#ifdef EAGAIN
    if (strcmp(str, "EAGAIN") == 0)  return EAGAIN;
#endif
#ifdef ENOMEM
    if (strcmp(str, "ENOMEM") == 0)  return ENOMEM;
#endif
#ifdef EACCES
    if (strcmp(str, "EACCES") == 0)  return EACCES;
#endif
#ifdef EFAULT
    if (strcmp(str, "EFAULT") == 0)  return EFAULT;
#endif
#ifdef ENOTBLK
    if (strcmp(str, "ENOTBLK") == 0)  return ENOTBLK;
#endif
#ifdef EBUSY
    if (strcmp(str, "EBUSY") == 0)  return EBUSY;
#endif
#ifdef EEXIST
    if (strcmp(str, "EEXIST") == 0)  return EEXIST;
#endif
#ifdef EXDEV
    if (strcmp(str, "EXDEV") == 0)  return EXDEV;
#endif
#ifdef ENODEV
    if (strcmp(str, "ENODEV") == 0)  return ENODEV;
#endif
#ifdef ENOTDIR
    if (strcmp(str, "ENOTDIR") == 0)  return ENOTDIR;
#endif
#ifdef EISDIR
    if (strcmp(str, "EISDIR") == 0)  return EISDIR;
#endif
#ifdef EINVAL
    if (strcmp(str, "EINVAL") == 0)  return EINVAL;
#endif
#ifdef ENFILE
    if (strcmp(str, "ENFILE") == 0)  return ENFILE;
#endif
#ifdef EMFILE
    if (strcmp(str, "EMFILE") == 0)  return EMFILE;
#endif
#ifdef ENOTTY
    if (strcmp(str, "ENOTTY") == 0)  return ENOTTY;
#endif
#ifdef ETXTBSY
    if (strcmp(str, "ETXTBSY") == 0)  return ETXTBSY;
#endif
#ifdef EFBIG
    if (strcmp(str, "EFBIG") == 0)  return EFBIG;
#endif
#ifdef ENOSPC
    if (strcmp(str, "ENOSPC") == 0)  return ENOSPC;
#endif
#ifdef ESPIPE
    if (strcmp(str, "ESPIPE") == 0)  return ESPIPE;
#endif
#ifdef EROFS
    if (strcmp(str, "EROFS") == 0)  return EROFS;
#endif
#ifdef EMLINK
    if (strcmp(str, "EMLINK") == 0)  return EMLINK;
#endif
#ifdef EPIPE
    if (strcmp(str, "EPIPE") == 0)  return EPIPE;
#endif
#ifdef EDOM
    if (strcmp(str, "EDOM") == 0)  return EDOM;
#endif
#ifdef ERANGE
    if (strcmp(str, "ERANGE") == 0)  return ERANGE;
#endif
#ifdef EDEADLK
    if (strcmp(str, "EDEADLK") == 0)  return EDEADLK;
#endif
#ifdef ENAMETOOLONG
    if (strcmp(str, "ENAMETOOLONG") == 0)  return ENAMETOOLONG;
#endif
#ifdef ENOLCK
    if (strcmp(str, "ENOLCK") == 0)  return ENOLCK;
#endif
#ifdef ENOSYS
    if (strcmp(str, "ENOSYS") == 0)  return ENOSYS;
#endif
#ifdef ENOTEMPTY
    if (strcmp(str, "ENOTEMPTY") == 0)  return ENOTEMPTY;
#endif
#ifdef ELOOP
    if (strcmp(str, "ELOOP") == 0)  return ELOOP;
#endif
#ifdef EWOULDBLOCK
    if (strcmp(str, "EWOULDBLOCK") == 0)  return EWOULDBLOCK;
#endif
#ifdef ENOMSG
    if (strcmp(str, "ENOMSG") == 0)  return ENOMSG;
#endif
#ifdef EIDRM
    if (strcmp(str, "EIDRM") == 0)  return EIDRM;
#endif
#ifdef ECHRNG
    if (strcmp(str, "ECHRNG") == 0)  return ECHRNG;
#endif
#ifdef EL2NSYNC
    if (strcmp(str, "EL2NSYNC") == 0)  return EL2NSYNC;
#endif
#ifdef EL3HLT
    if (strcmp(str, "EL3HLT") == 0)  return EL3HLT;
#endif
#ifdef EL3RST
    if (strcmp(str, "EL3RST") == 0)  return EL3RST;
#endif
#ifdef ELNRNG
    if (strcmp(str, "ELNRNG") == 0)  return ELNRNG;
#endif
#ifdef EUNATCH
    if (strcmp(str, "EUNATCH") == 0)  return EUNATCH;
#endif
#ifdef ENOCSI
    if (strcmp(str, "ENOCSI") == 0)  return ENOCSI;
#endif
#ifdef EL2HLT
    if (strcmp(str, "EL2HLT") == 0)  return EL2HLT;
#endif
#ifdef EBADE
    if (strcmp(str, "EBADE") == 0)  return EBADE;
#endif
#ifdef EBADR
    if (strcmp(str, "EBADR") == 0)  return EBADR;
#endif
#ifdef EXFULL
    if (strcmp(str, "EXFULL") == 0)  return EXFULL;
#endif
#ifdef ENOANO
    if (strcmp(str, "ENOANO") == 0)  return ENOANO;
#endif
#ifdef EBADRQC
    if (strcmp(str, "EBADRQC") == 0)  return EBADRQC;
#endif
#ifdef EBADSLT
    if (strcmp(str, "EBADSLT") == 0)  return EBADSLT;
#endif
#ifdef EDEADLOCK
    if (strcmp(str, "EDEADLOCK") == 0)  return EDEADLOCK;
#endif
#ifdef EBFONT
    if (strcmp(str, "EBFONT") == 0)  return EBFONT;
#endif
#ifdef ENOSTR
    if (strcmp(str, "ENOSTR") == 0)  return ENOSTR;
#endif
#ifdef ENODATA
    if (strcmp(str, "ENODATA") == 0)  return ENODATA;
#endif
#ifdef ETIME
    if (strcmp(str, "ETIME") == 0)  return ETIME;
#endif
#ifdef ENOSR
    if (strcmp(str, "ENOSR") == 0)  return ENOSR;
#endif
#ifdef ENONET
    if (strcmp(str, "ENONET") == 0)  return ENONET;
#endif
#ifdef ENOPKG
    if (strcmp(str, "ENOPKG") == 0)  return ENOPKG;
#endif
#ifdef EREMOTE
    if (strcmp(str, "EREMOTE") == 0)  return EREMOTE;
#endif
#ifdef ENOLINK
    if (strcmp(str, "ENOLINK") == 0)  return ENOLINK;
#endif
#ifdef EADV
    if (strcmp(str, "EADV") == 0)  return EADV;
#endif
#ifdef ESRMNT
    if (strcmp(str, "ESRMNT") == 0)  return ESRMNT;
#endif
#ifdef ECOMM
    if (strcmp(str, "ECOMM") == 0)  return ECOMM;
#endif
#ifdef EPROTO
    if (strcmp(str, "EPROTO") == 0)  return EPROTO;
#endif
#ifdef EMULTIHOP
    if (strcmp(str, "EMULTIHOP") == 0)  return EMULTIHOP;
#endif
#ifdef EDOTDOT
    if (strcmp(str, "EDOTDOT") == 0)  return EDOTDOT;
#endif
#ifdef EBADMSG
    if (strcmp(str, "EBADMSG") == 0)  return EBADMSG;
#endif
#ifdef EOVERFLOW
    if (strcmp(str, "EOVERFLOW") == 0)  return EOVERFLOW;
#endif
#ifdef ENOTUNIQ
    if (strcmp(str, "ENOTUNIQ") == 0)  return ENOTUNIQ;
#endif
#ifdef EBADFD
    if (strcmp(str, "EBADFD") == 0)  return EBADFD;
#endif
#ifdef EREMCHG
    if (strcmp(str, "EREMCHG") == 0)  return EREMCHG;
#endif
#ifdef ELIBACC
    if (strcmp(str, "ELIBACC") == 0)  return ELIBACC;
#endif
#ifdef ELIBBAD
    if (strcmp(str, "ELIBBAD") == 0)  return ELIBBAD;
#endif
#ifdef ELIBSCN
    if (strcmp(str, "ELIBSCN") == 0)  return ELIBSCN;
#endif
#ifdef ELIBMAX
    if (strcmp(str, "ELIBMAX") == 0)  return ELIBMAX;
#endif
#ifdef ELIBEXEC
    if (strcmp(str, "ELIBEXEC") == 0)  return ELIBEXEC;
#endif
#ifdef EILSEQ
    if (strcmp(str, "EILSEQ") == 0)  return EILSEQ;
#endif
#ifdef ERESTART
    if (strcmp(str, "ERESTART") == 0)  return ERESTART;
#endif
#ifdef ESTRPIPE
    if (strcmp(str, "ESTRPIPE") == 0)  return ESTRPIPE;
#endif
#ifdef EUSERS
    if (strcmp(str, "EUSERS") == 0)  return EUSERS;
#endif
#ifdef ENOTSOCK
    if (strcmp(str, "ENOTSOCK") == 0)  return ENOTSOCK;
#endif
#ifdef EDESTADDRREQ
    if (strcmp(str, "EDESTADDRREQ") == 0)  return EDESTADDRREQ;
#endif
#ifdef EMSGSIZE
    if (strcmp(str, "EMSGSIZE") == 0)  return EMSGSIZE;
#endif
#ifdef EPROTOTYPE
    if (strcmp(str, "EPROTOTYPE") == 0)  return EPROTOTYPE;
#endif
#ifdef ENOPROTOOPT
    if (strcmp(str, "ENOPROTOOPT") == 0)  return ENOPROTOOPT;
#endif
#ifdef EPROTONOSUPPORT
    if (strcmp(str, "EPROTONOSUPPORT") == 0)  return EPROTONOSUPPORT;
#endif
#ifdef ESOCKTNOSUPPORT
    if (strcmp(str, "ESOCKTNOSUPPORT") == 0)  return ESOCKTNOSUPPORT;
#endif
#ifdef EOPNOTSUPP
    if (strcmp(str, "EOPNOTSUPP") == 0)  return EOPNOTSUPP;
#endif
#ifdef EPFNOSUPPORT
    if (strcmp(str, "EPFNOSUPPORT") == 0)  return EPFNOSUPPORT;
#endif
#ifdef EAFNOSUPPORT
    if (strcmp(str, "EAFNOSUPPORT") == 0)  return EAFNOSUPPORT;
#endif
#ifdef EADDRINUSE
    if (strcmp(str, "EADDRINUSE") == 0)  return EADDRINUSE;
#endif
#ifdef EADDRNOTAVAIL
    if (strcmp(str, "EADDRNOTAVAIL") == 0)  return EADDRNOTAVAIL;
#endif
#ifdef ENETDOWN
    if (strcmp(str, "ENETDOWN") == 0)  return ENETDOWN;
#endif
#ifdef ENETUNREACH
    if (strcmp(str, "ENETUNREACH") == 0)  return ENETUNREACH;
#endif
#ifdef ENETRESET
    if (strcmp(str, "ENETRESET") == 0)  return ENETRESET;
#endif
#ifdef ECONNABORTED
    if (strcmp(str, "ECONNABORTED") == 0)  return ECONNABORTED;
#endif
#ifdef ECONNRESET
    if (strcmp(str, "ECONNRESET") == 0)  return ECONNRESET;
#endif
#ifdef ENOBUFS
    if (strcmp(str, "ENOBUFS") == 0)  return ENOBUFS;
#endif
#ifdef EISCONN
    if (strcmp(str, "EISCONN") == 0)  return EISCONN;
#endif
#ifdef ENOTCONN
    if (strcmp(str, "ENOTCONN") == 0)  return ENOTCONN;
#endif
#ifdef ESHUTDOWN
    if (strcmp(str, "ESHUTDOWN") == 0)  return ESHUTDOWN;
#endif
#ifdef ETOOMANYREFS
    if (strcmp(str, "ETOOMANYREFS") == 0)  return ETOOMANYREFS;
#endif
#ifdef ETIMEDOUT
    if (strcmp(str, "ETIMEDOUT") == 0)  return ETIMEDOUT;
#endif
#ifdef ECONNREFUSED
    if (strcmp(str, "ECONNREFUSED") == 0)  return ECONNREFUSED;
#endif
#ifdef EHOSTDOWN
    if (strcmp(str, "EHOSTDOWN") == 0)  return EHOSTDOWN;
#endif
#ifdef EHOSTUNREACH
    if (strcmp(str, "EHOSTUNREACH") == 0)  return EHOSTUNREACH;
#endif
#ifdef EALREADY
    if (strcmp(str, "EALREADY") == 0)  return EALREADY;
#endif
#ifdef EINPROGRESS
    if (strcmp(str, "EINPROGRESS") == 0)  return EINPROGRESS;
#endif
#ifdef ESTALE
    if (strcmp(str, "ESTALE") == 0)  return ESTALE;
#endif
#ifdef EUCLEAN
    if (strcmp(str, "EUCLEAN") == 0)  return EUCLEAN;
#endif
#ifdef ENOTNAM
    if (strcmp(str, "ENOTNAM") == 0)  return ENOTNAM;
#endif
#ifdef ENAVAIL
    if (strcmp(str, "ENAVAIL") == 0)  return ENAVAIL;
#endif
#ifdef EISNAM
    if (strcmp(str, "EISNAM") == 0)  return EISNAM;
#endif
#ifdef EREMOTEIO
    if (strcmp(str, "EREMOTEIO") == 0)  return EREMOTEIO;
#endif
#ifdef EDQUOT
    if (strcmp(str, "EDQUOT") == 0)  return EDQUOT;
#endif
#ifdef ENOMEDIUM
    if (strcmp(str, "ENOMEDIUM") == 0)  return ENOMEDIUM;
#endif
#ifdef EMEDIUMTYPE
    if (strcmp(str, "EMEDIUMTYPE") == 0)  return EMEDIUMTYPE;
#endif
#ifdef ECANCELED
    if (strcmp(str, "ECANCELED") == 0)  return ECANCELED;
#endif
#ifdef ENOKEY
    if (strcmp(str, "ENOKEY") == 0)  return ENOKEY;
#endif
#ifdef EKEYEXPIRED
    if (strcmp(str, "EKEYEXPIRED") == 0)  return EKEYEXPIRED;
#endif
#ifdef EKEYREVOKED
    if (strcmp(str, "EKEYREVOKED") == 0)  return EKEYREVOKED;
#endif
#ifdef EKEYREJECTED
    if (strcmp(str, "EKEYREJECTED") == 0)  return EKEYREJECTED;
#endif

#ifdef EOWNERDEAD
    if (strcmp(str, "EOWNERDEAD") == 0)  return EOWNERDEAD;
#endif
#ifdef ENOTRECOVERABLE
    if (strcmp(str, "ENOTRECOVERABLE") == 0)  return ENOTRECOVERABLE;
#endif
    return -1;
}


static void do_help()
{
    printf("Usage:\n");
    for (int i = 0; __commands[i].name; ++i) {
        cli_cmd_t *cmd = &__commands[i];
        printf("  %-12s  %s\n", cmd->name, cmd->summary);
    }
}

int do_include(char *str, void *ctx)
{
    int i;
    while (isblank(*str)) str++;
    for (i = 0; str[i] && !isspace(str[i]); ++i);
    str[i] = '\0';
    const char *path = strdup(str);
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
        return cli_error("Unable to open file %s [%d - %s]\n", str, errno, strerror(errno));
    printf("\033[36mScript from %s\033[0m\n", path);
    free(path);
    int ret = cli_commands(fp, ctx, true);
    printf("\n");
    fclose(fp);
    return ret;
}

static int do_cli(void *ctx)
{
    int ret = cli_commands(stdin, ctx, false);
    return ret;
}


static int do_error(char *str, int *perr)
{
    while (str[0] && isspace(str[0]))
        str++;
    if (str[0] == 'O') {
        if (str[1] == 'N') {
            str[2] = '\0';
            *perr = -1;
        } else if (str[1] == 'F' && str[2] == 'F') {
            str[3] = '\0';
            *perr = 0;
        } else
            return -1;
    } else if (str[0] == 'E') {
        int i = 0;
        for (; isalnum(str[i]); ++i)
            str[i] = toupper(str[i]);
        str[i] = '\0';
        int eno = read_errno(str);
        if (eno < 0)
            return -1;
        *perr = eno;
    } else
        return -1;

    printf("Set error: %s\n", str);
    return 0;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

static int cli_read_string(const char **line, size_t *ptr)
{
    int i = 0;
    char buf[1024];
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
        if (i > 1024)
            return -1;
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

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

int cli_commands(FILE *fp, void *ctx, bool reecho)
{
    int i, n;
    size_t len = 1024;
    char line[1024];
    int error_handling = 0;

    for (int row = 1; ; row++) {
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
        } else if (strcmp(cmd, "INCLUDE") == 0) {
            do_include(str, ctx);
            continue;
        } else if (strcmp(cmd, "CLI") == 0 && reecho) {
            do_cli(ctx);
            continue;
        } else if (strcmp(cmd, "ERROR") == 0) {
            if (do_error(str, &error_handling) != 0)
                return -1;
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
            printf(" Unknown command '%s' at %d\n", cmd, row);
            return -1;
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
            else
                n = -1; // 1 is not present ; -1 is bad
            if (n < 0 || (n != 0 && i < routine->min_arg)) {
                printf(" Bad parameter %d at %s\n", i + 1, str);
                routine = NULL;
                break;
            }
        }

        if (routine == NULL) {
            printf(" Command abort: %s\n", cmd);
            return -1;
        }

        // Run command
        assert(irq_ready());
        int retn = routine->call(ctx, &args[0]);
        for (i = 0; i < ARGS_MAX; ++i) {
            if (routine->params[i] == ARG_STR && args[i])
                free(args[i]);
        }
        if (retn != 0 && (error_handling < 0 || (error_handling > 0 && error_handling != errno))) {
            printf("Error with %s (%d) at row %d\n", cmd, errno, row);
            return -1;
        } else if (retn == 0 && error_handling > 0 && error_handling != errno) {
            printf("Expected an error with %s (%d) at row %d\n", cmd, errno, row);
            return -1;
        }

        assert(irq_ready());
        // printf(" Command '%s' (%x, %x, %x, %x, %x)\n", cmd, args[0], args[1], args[2], args[3], args[4]);
    }
    return 0;
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


int cli_msg(int ret, const char *clr, const char *msg, ...)
{
    splock_lock(&log_lock);
    va_list ap;
    va_start(ap, msg);
    strcpy(log_buf, clr);
    int lg = 5 + vsnprintf(&log_buf[5], 1024 - 5, msg, ap);
    if (log_buf[lg - 1] == '\n')
        lg--;
    strcpy(&log_buf[lg], "\033[0m\n");
    lg += 5;
    va_end(ap);
    write(1, log_buf, lg);
    splock_unlock(&log_lock);
    return ret;
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
    return do_include(path, cfg->context);
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

    hmp_destroy(&storage);
    printf("Ending program %d: alloc, %d map\n", kalloc_count(), kmap_count());
    return 0;
}
