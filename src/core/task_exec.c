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
#include <kernel/core.h>
#include <kernel/files.h>
#include <kernel/vfs.h>
#include <kernel/dlib.h>
#include <kernel/task.h>
#include <kernel/syscalls.h>
#include <string.h>

extern const font_bmp_t font_8x15;


void krn_ls(tty_t *tty, inode_t *dir)
{
    inode_t *ino;
    char buf[256];
    char name[256];
    void *dir_ctx = vfs_opendir(dir, NULL);
    // kprintf(-1, "Root readdir:");
    while ((ino = vfs_readdir(dir, name, dir_ctx)) != NULL) {
        snprintf(buf, 256, "  /%s%s   ", name, ino->type == FL_DIR ? "/" : "");
        tty_puts(tty, buf);
        vfs_close(ino);
    }
    tty_puts(tty, "\n");
    vfs_closedir(dir, dir_ctx);
}

void krn_cat(tty_t *tty, const char *path)
{
    char *buf = kalloc(512);
    int fd = sys_open(-1, path, 0);
    if (fd == -1) {
        snprintf(buf, 512, "Unable to open %s \n", path);
        tty_puts(tty, buf);
    } else {
        // kprintf(-1, "Content of '%s' (%x)\n", path, lg);
        for (;;) {
            int lg = sys_read(fd, buf, 512);
            if (lg <= 0)
                break;
            buf[lg] = '\0';
            tty_write(tty, buf, lg);
            // tty_puts(tty, buf);
            // kprintf(-1, "%s\n", buf);
        }
        sys_close(fd);
    }
    kfree(buf);
}


void fake_shell_task()
{
    const char *txt_filename = "boot/grub/grub.cfg";
    // const char *txt_filename = "BOOT/GRUB/MENU.LST";

    tty_t *tty = tty_create(128);
    inode_t *win = wmgr_create_window(NULL, 384, 240);
    tty_window(tty, win, &font_8x15);

    // void* pixels = kmap(2 * _Mib_, ino, 0, VMA_FILE_RW | VMA_RESOLVE);
    // MAP NODE / USE IT TO DRAW ON SURFACE
    sys_sleep(SEC_TO_KTIME(3));

    inode_t *root = resx_fs_pwd(kCPU.running->resx_fs);
    if (root == NULL)
        sys_exit(-1);
    tty_puts(tty, "shell> ls\n");
    krn_ls(tty, root);
    tty_puts(tty, "shell> cat ");
    tty_puts(tty, txt_filename);
    tty_puts(tty, "\n");
    krn_cat(tty, txt_filename);
    tty_puts(tty, "shell> ... \n");

    char v = 0;
    for (;; ++v) {
        sys_sleep(SEC_TO_KTIME(10));
        // memset(pixels, v, 32 * PAGE_SIZE);
    }
}




void exec_task(const char **exec_args)
{
    int i, lg;
    tty_t *tty = tty_create(128);
    inode_t *win = wmgr_create_window(NULL, 180, 120);
    tty_window(tty, win, &font_8x15);

    mspace_t *mspace = mspace_create();
    mmu_context(mspace);
    kCPU.running->usmem = mspace;
    proc_t *proc = dlib_process(kCPU.running->resx_fs, mspace);
    if (proc->root == NULL) {
        tty_puts(tty, "No root! ");
        sys_exit(-1);
    }
    kCPU.running->proc = proc;
    int ret = dlib_openexec(proc, exec_args[0]);
    if (ret == 0)
        tty_puts(tty, "Proc opened!!\n");
    else
        tty_puts(tty, "Proc error!!\n");

    ret = dlib_map_all(proc);
    if (ret == 0)
        tty_puts(tty, "Proc mapped!!\n");
    else
        tty_puts(tty, "Proc mapping error!!\n");

    inode_t * std_tty = tty_inode(tty);
    stream_t *std_in = resx_set(kCPU.running->resx, std_tty);
    stream_t *std_out = resx_set(kCPU.running->resx, std_tty);
    stream_t *std_err = resx_set(kCPU.running->resx, std_tty);

    void *start = dlib_exec_entry(proc);
    void *stack = mspace_map(mspace, 0, _Mib_, NULL, 0, VMA_STACK_RW);
    stack = ADDR_OFF(stack, _Mib_ - sizeof(size_t));
    kprintf(-1, "%s: start:%p, stack:%p\n", exec_args[0], start, stack);
    mspace_display(mspace);

    int argc = 3;
    char **argv = ADDR_PUSH(stack, argc * sizeof(char *));
    for (i = 0; i < argc; ++i) {
        lg = strlen(exec_args[i]) + 1;
        argv[i] = ADDR_PUSH(stack, ALIGN_UP(lg, 4));
        strcpy(argv[i], exec_args[i]);
        kprintf(-1, "Set arg.%d: '%s'\n", i, argv[i]);
    }

    size_t *args = ADDR_PUSH(stack, 4 * sizeof(char *));
    args[1] = argc;
    args[2] = (size_t)argv;
    args[3] = 0;

    kprintf(-1, "%s: start:%p, stack:%p\n", exec_args[0], start, stack);
    cpu_usermode(start, stack);
}

