
#include <kernel/vfs.h>
// #include <kernel/device.h>
#include <kora/mcrs.h>

#include <fcntl.h>
#undef snprintf
#include <stdio.h>
#include <stdlib.h>

int read_cmds(FILE *fp);

inode_t *getinode(const char *path)
{
    return path == NULL ? vfs_open_inode(kSYS.dev_ino) : vfs_search(kSYS.dev_ino, kSYS.dev_ino, path, NULL);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

void do_ls(const char *path)
{
    inode_t *ino;
    inode_t *dir = getinode(path);
    char name[256];
    char tmp[16];
    if (dir == NULL) {
        printf("No such directory\n");
        return;
    } else if (!VFS_ISDIR(dir)) {
        printf("This file isn't a directory\n");
        return;
    }

    void *ctx = vfs_opendir(dir, NULL);
    if (!ctx) {
        printf("Error during directory reading\n");
        return;
    }

    while ((ino = vfs_readdir(dir, name, ctx)) != NULL) {
        printf("    %s (%s)\n", name, vfs_inokey(ino, tmp));
        vfs_close_inode(ino);
    }

    vfs_closedir(dir, ctx);
    vfs_close_inode(dir);
}

void do_stat(const char *path)
{
    char tmp[64];
    inode_t *ino = getinode(path);
    if (ino == NULL) {
        printf("No such entry\n");
        return;
    }

    int blk = 0;
    if (ino->length && ino->dev->block)
        blk = ALIGN_UP(ino->length, ino->dev->block) / ino->dev->block;
    printf("      Size: %7d \tBlocks: %7d \tIO Block: %d \t%s\n", ino->length, blk, ino->dev->block, "");
    printf("     Inode: %7d \tDevice: %7d \tLinks: %d \n", ino->dev->no, ino->no, 0);
#ifdef _WIN32
    time_t t;
    t = USEC_TO_SEC(ino->atime);
    ctime_s(tmp, 64, &t);
    printf("    Access: %s", tmp);
    t = USEC_TO_SEC(ino->mtime);
    ctime_s(tmp, 64, &t);
    printf("    Modify: %s", tmp);
    t = USEC_TO_SEC(ino->ctime);
    ctime_s(tmp, 64, &t);
    printf("    Change: %s", tmp);
    printf("     Birth: %s", ino->btime == 0 ? "-\n" : tmp);
#endif
}

void do_mount(const char *path)
{
    char *rp;
    char *dev = strtok_r(path, ";", &rp);
    char *fs = strtok_r(NULL, ";", &rp);
    char *name = strtok_r(NULL, ";", &rp);
    vfs_mount(dev, fs, name, NULL);
}

void do_hd(const char *arg)
{
    char *rp;
    char *path = strtok_r(arg, ";", &rp);
    int lba = strtol(strtok_r(NULL, ";", &rp), NULL, 10);

    inode_t *ino = getinode(path);
    int len = 512;
    char *buf = malloc(len);
    vfs_read(ino, buf, len, len * lba, 0);
    kdump(buf, len);
    free(buf);
}

void do_exec(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        printf("Unable to find the script\n");
        return;
    }
    read_cmds(fp);
    fclose(fp);
}


ino_ops_t dsk_ino_ops = {
    .read = NULL,
};

dev_ops_t dsk_dev_ops = {
    .ioctl = NULL,
};

int imgdk_open(const char *path, const char *name);

void do_disk(const char *path)
{
    // vdi, vhd, vmdk, img, iso
    static int dsk_count = 0;
    char tmp[12];

#if 1
    snprintf(tmp, 12, "dsk%d", dsk_count + 1);
    if (imgdk_open(path, tmp) == 0)
        ++dsk_count;
    else
        printf("Unable to open the image disk.\n");
#else

    int fd = open(path, O_RDWR);
    if (fd == -1) {
        printf("Unable to open the image disk.\n");
        return;
    }

    inode_t *ino = vfs_inode(fd, FL_BLK, NULL);
    ino->dev->devclass = strdup("Image disk"); // Mandatory
    ino->length = lseek(fd, 0, SEEK_END); // Optional
    ino->ops = &dsk_ino_ops; // Mandatory
    ino->dev->ops = &dsk_dev_ops; // Mandatory
    if (strcmp(strrchr(path, '.'), ".img") == 0)
        ino->dev->block = 512; // Optional
    else if (strcmp(strrchr(path, '.'), ".iso") == 0)
        ino->dev->block = 2048; // Optional
    // O ino->dev->devname
    // O ino->dev->model
    ino->btime = cpu_clock(0);
    ino->ctime = ino->btime;
    ino->mtime = ino->btime;
    ino->atime = ino->btime;

    snprintf(tmp, 12, "dsk%d", ++dsk_count);
    vfs_mkdev(ino, tmp);
#endif

}


#ifdef _WIN32
#include <Windows.h>

void do_fs(const char *path)
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
    kmod_t *mod = GetProcAddress(hm, "kmod_info_isofs");
    if (mod == NULL) {
        printf("No kernel info structure\n");
        return;
    }
    mod->setup();
}
#else
#include <dlfcn.h>
void do_fs(const char *path)
{
    void *ctx = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
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

    kmod_t *mod = dlsym(ctx, buf);
    if (mod == NULL) {
        printf("No kernel info structure\n");
        return;
    }
    mod->setup();
}
#endif

void do_image_vhd(char *arg)
{
    char *rp;
    char *path = strtok_r(arg, ";", &rp);
    char *sfx;
    size_t len = strtoul(strtok_r(NULL, ";", &rp), &sfx, 10);
    switch (sfx[0] | 0x20) {
    case 'k':
        len *= _Kib_;
        break;
    case 'm':
        len *= _Mib_;
        break;
    case 'g':
        len *= _Gib_;
        break;
    }
    vhd_create_dyn(path, len);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

char *tokenize(const char *line, const char **sreg)
{
    const char *OPS = "<>|&^()";
    const char *OPS2 = "<>|&";
    const char *QUOTE = "'\"`";
    int lg, c1, c2, s = 0;
    const char *tok1 = *sreg;
    char *tok2;
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
    } else if (strchr(QUOTE, c1) != NULL) {
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
    } else {
        while (strchr(OPS, *tok1) == NULL && *tok1 != '\0' && *tok1 != '\n' && (*tok1 < 0 || !isblank(*tok1)))
            tok1++;
    }

    lg = tok1 - (*sreg);
    if (lg == 0) {
        tok1++;
        lg = tok1 - (*sreg);
    }

    tok2 = (char *)malloc((lg + 1) * sizeof(char));
    memcpy(tok2, *sreg, lg * sizeof(char));
    tok2[lg - s] = '\0';
    *sreg = tok1;
    return tok2;
}

void do_help(const char *path);

#define _ROUTINE(n,d) { #n, d, do_ ## n }
struct {
    const char *name;
    const char *desc;
    void(* entry)(const char *name);
} routines[] = {
    _ROUTINE(help, "Display this help"),
    _ROUTINE(ls, "List entry of directory"),
    _ROUTINE(fs, "Register a new file system"),
    _ROUTINE(mount, "Mount a device, arg is dev;fs;name"),
    _ROUTINE(stat, "Print information about an inode"),
    _ROUTINE(exec, "Run commands from file"),
    _ROUTINE(hd, "Dump a block of the file (arg is PATH:LBA"),
    _ROUTINE(disk, "Mount a virtual disk image as a block device"),
    _ROUTINE(image_vhd, "Create a VHD disk image"),
    { NULL, NULL }
};

void do_help(const char *path)
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
        char *rp = fgets(buf, 512, fp);
        if (rp == NULL) {
            printf("\n");
            break;
        }
        if (fp != stdin)
            printf("%s", buf);

        char *token;
        char *cmd = NULL;
        char *arg = NULL;
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
    vfs_init();
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

