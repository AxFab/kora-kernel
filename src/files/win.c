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
#include <kernel/device.h>
#include <kernel/files.h>
#include <kernel/task.h>
#include <kernel/input.h>
#include <string.h>
#include <errno.h>

typedef struct window window_t;
struct desktop {
    atomic_t win_no;
    llhead_t list;
    splock_t lock;
    window_t *focus;
    int key_mods;
};

desktop_t kDESK;
// Desktop related
surface_t *create_win();

struct window {
    surface_t *fb;
    pipe_t *mq;
    task_t *task;
    int status;
    llnode_t node;
};


// All
int win_fcntl(inode_t *ino, int cmd, void *params)
{
    errno = ENOSYS;
    return -1;
}

int win_close(inode_t *ino)
{
    assert(ino->type == FL_WIN);
    window_t *win = (window_t *)ino->info;
    ino->info = NULL;
    vds_destroy(win->fb);
    pipe_destroy(win->mq);
    kfree(win);
    return 0;
}

// Mapping
page_t win_fetch(inode_t *ino, off_t off)
{
    assert(ino->type == FL_WIN);
    window_t *win = (window_t *)ino->info;
    surface_t *fb = win->fb;
    return vds_fetch(fb, off);
}

void win_release(inode_t *ino, off_t off, page_t pg)
{
    assert(ino->type == FL_WIN);
    // window_t *win = (window_t *)ino->info;
    // surface_t *fb = win->fb;
    // vds_release(fb, off, pg);
}

// Fifo
int win_read(inode_t *ino, char *buf, size_t len, int flags)
{
    assert(ino->type == FL_WIN);
    window_t *win = (window_t *)ino->info;
    return pipe_read(win->mq, buf, len, flags);
}


int win_reset(inode_t *ino)
{
    assert(ino->type == FL_WIN);
    window_t *win = (window_t *)ino->info;
    return pipe_reset(win->mq);
}

// Framebuffer
void win_flip(inode_t *ino)
{

}

void win_resize(inode_t *ino, int width, int height)
{

}

void win_copy(inode_t *dst, inode_t *src, int x, int y, int lf, int tp, int rg, int bt)
{
    // vds_copy(dst, src, x, y, lf, tp, rg, bt);
}


ino_ops_t win_ops = {
    .fcntl = win_fcntl,
    .close = win_close,
    .fetch = win_fetch,
    .release = win_release,
    .read = win_read,
    .reset = win_reset,
    .flip = win_flip,
    .resize = win_resize,
    .copy = win_copy,
};


inode_t *window_open(desktop_t *desk, int width, int height, unsigned features, unsigned evmask)
{
    surface_t *fb = create_win(/*width, height*/);
    if (fb == NULL)
        return NULL;
    inode_t *ino = vfs_inode(atomic_xadd(&desk->win_no, 1), FL_WIN, NULL);

    window_t *win = kalloc(sizeof(window_t));
    win->fb = fb;
    win->mq = pipe_create();
    win->task = kCPU.running;
    win->status = 0;
    splock_lock(&kDESK.lock);
    ll_push_back(&kDESK.list, &win->node);
    splock_unlock(&kDESK.lock);

    ino->ops = &win_ops;
    ino->info = win;
    return ino;
}


void window_close(desktop_t *desk, inode_t *win)
{

}

void *window_map(mspace_t *mspace, inode_t *win)
{
    return NULL;
}

int window_set_features(inode_t *win, int features, int *args)
{
    return -1;
}

int window_get_features(inode_t *win, int features, int *args)
{
    return -1;
}







/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

extern const font_bmp_t font_6x10;
extern const font_bmp_t font_8x15;
extern const font_bmp_t font_7x13;
extern const font_bmp_t font_6x9;
extern const font_bmp_t font_8x8;


uint32_t colors_std[] = {
    0x000000, 0x800000, 0x008000, 0x808000, 0x000080, 0x800080, 0x008080, 0x808080, 0xFFFFFF,
    0xD0D0D0, 0xFF0000, 0x00FF00, 0xFFFF00, 0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF, 0xFFFFFF,
};

uint32_t colors_kora[] = {
    0x181818, 0xA61010, 0x10A610, 0xA66010, 0x1010A6, 0xA610A6, 0x10A6A6, 0xA6A6A6, 0xFFFFFF,
    0x323232, 0xD01010, 0x10D010, 0xD06010, 0x1010D0, 0xD010D0, 0x10D0D0, 0xD0D0D0, 0xFFFFFF,
};

tty_t *tty_syslog = NULL;

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define FRAME_SZ 13

int x = 0;
int y = 0;
int width, height;

tty_t *slog;

surface_t *create_win()
{
    surface_t *win;
    if (width > height) {
        win = vds_create(width / 2 - 2, height - 2 - FRAME_SZ, 4);
        win->x = x + 1;
        win->y = y + 1 + FRAME_SZ;
        x += width / 2;
        width /= 2;
    } else {
        win = vds_create(width - 2, height / 2 - 2 - FRAME_SZ, 4);
        win->x = x + 1;
        win->y = y + 1 + FRAME_SZ;
        y += height / 2;
        height /= 2;
    }
    return win;
}

#define TTY_WRITE(t,s)  do { tty_write(t,s,strlen(s)); kprintf(-1, s); } while(0)
#include <kernel/dlib.h>
#include <kernel/syscalls.h>


void krn_ls(tty_t *tty, inode_t *dir)
{
    inode_t *ino;
    char buf[256];
    char name[256];
    void *dir_ctx = vfs_opendir(dir, NULL);
    // kprintf(-1, "Root readdir:");
    while ((ino = vfs_readdir(dir, name, dir_ctx)) != NULL) {
        snprintf(buf, 256, "  /%s%s   ", name, ino->type == FL_DIR ? "/" : "");
        TTY_WRITE(tty, buf);
        vfs_close(ino);
    }
    TTY_WRITE(tty, "\n");
    vfs_closedir(dir, dir_ctx);
}

void krn_cat(tty_t *tty, const char *path)
{
    char *buf = kalloc(512);
    int fd = sys_open(-1, path, 0);
    if (fd == -1) {
        snprintf(buf, 512, "Unable to open %s \n", path);
        TTY_WRITE(tty, buf);
    } else {
        // kprintf(-1, "Content of '%s' (%x)\n", path, lg);
        for (;;) {
            int lg = sys_read(fd, buf, 512);
            if (lg <= 0)
                break;
            buf[lg] = '\0';
            tty_write(tty, buf, lg);
            // TTY_WRITE(tty, buf);
            // kprintf(-1, "%s\n", buf);
        }
        sys_close(fd);
    }
    kfree(buf);
}


// proc_t *dlib_process(resx_fs_t *fs, mspace_t *mspace);
// int dlib_openexec(proc_t *proc, const char *execname);

int main_clock();

void exec_task()
{
    inode_t *win = window_open(&kDESK, 0, 0, 0, 0);
    tty_t *tty = tty_create(128);
    tty_window(tty, ((window_t *)win->info)->fb, &font_8x15);

    task_create(main_clock, NULL, "Clock");

    inode_t *root = resx_fs_root(kCPU.running->resx_fs);
    inode_t *ino = vfs_search(root, root, "bin/basename", NULL);

    if (ino != NULL)
        TTY_WRITE(tty, "Found basename!!\n");

    mspace_t *mspace = mspace_create();
    mmu_context(mspace);
    kCPU.running->usmem = mspace;
    proc_t *proc = dlib_process(kCPU.running->resx_fs, mspace);
    int ret = dlib_openexec(proc, "bin/basename");
    if (ret == 0)
        TTY_WRITE(tty, "Proc opened!!\n");
    else
        TTY_WRITE(tty, "Proc error!!\n");

    ret = dlib_map_all(proc);
    if (ret == 0)
        TTY_WRITE(tty, "Proc mapped!!\n");
    else
        TTY_WRITE(tty, "Proc mapping error!!\n");

    void* start = dlib_exec_entry(proc);
    void* stack = mspace_map(mspace, 0, _Mib_, NULL, 0, VMA_STACK_RW);
    stack = ADDR_OFF(stack, _Mib_ - sizeof(size_t));
    kprintf(-1, "basename: start:%p, stack:%p\n", start, stack);
    mspace_display(mspace);

    cpu_usermode(start, stack);

    for (;;)
        sys_sleep(SEC_TO_KTIME(1));
}

// =-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-=


typedef struct image image_t;

struct image {
    int width, height;
    uint32_t *pixels;
};


#define PI 3.141279

#define  SEC_PER_DAY (24*60*60)
#define  SEC_PER_HOUR (60*60)
#define  SEC_PER_MIN (60)

#define PX(i,x,y)  ((uint32_t*)(i)->pixels)[(y)*(i)->width+(x)]

void draw_circle(surface_t *img, int x0, int y0, int radius, uint32_t clr)
{
    int x = radius-1;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (radius << 1);

    while (x >= y)
    {
        PX(img, x0 + x, y0 + y) = clr;
        PX(img, x0 + y, y0 + x) = clr;
        PX(img, x0 - y, y0 + x) = clr;
        PX(img, x0 - x, y0 + y) = clr;
        PX(img, x0 - x, y0 - y) = clr;
        PX(img, x0 - y, y0 - x) = clr;
        PX(img, x0 + y, y0 - x) = clr;
        PX(img, x0 + x, y0 - y) = clr;

        if (err <= 0)
        {
            y++;
            err += dy;
            dy += 2;
        }

        if (err > 0)
        {
            x--;
            dx += 2;
            err += dx - (radius << 1);
        }
    }
}

void draw_line_low(surface_t *img, int x0, int y0, int x1, int y1, uint32_t clr)
{
    int x;
    int dx = x1 - x0;
    int dy = y1 - y0;
    int yi = 1;
    if (dy < 0) {
        yi = -1;
        dy = -dy;
    }
    int D = 2*dy - dx;
    int y = y0;

    for (x = x0; x < x1; ++x) {
        PX(img, x,y) = clr;
        if (D > 0) {
           y = y + yi;
           D = D - 2*dx;
        }
        D = D + 2*dy;
    }
}

void draw_line_high(surface_t *img, int x0, int y0, int x1, int y1, uint32_t clr)
{
    int y;
    int dx = x1 - x0;
    int dy = y1 - y0;
    int xi = 1;
    if (dx < 0) {
        xi = -1;
        dx = -dx;
    }

    int D = 2*dx - dy;
    int x = x0;

    for (y = y0; y < y1; ++y) {
        PX(img, x,y) = clr;
        if (D > 0) {
            x = x + xi;
            D = D - 2*dy;
        }
        D = D + 2*dx;
    }
}


void draw_line(surface_t *img, int x0, int y0, int x1, int y1, uint32_t clr)
{
    if (abs(y1 - y0) < abs(x1 - x0)) {
        if (x0 > x1)
            draw_line_low(img, x1, y1, x0, y0, clr);
        else
            draw_line_low(img, x0, y0, x1, y1, clr);
    } else {
        if (y0 > y1)
            draw_line_high(img, x1, y1, x0, y0, clr);
        else
            draw_line_high(img, x0, y0, x1, y1, clr);
    }
}


float step_values[] = {
    0.00000, 0.10472, 0.20944, 0.31416, 0.41888, 0.52360, 0.62832, 0.73304, 0.83776, 0.94248, 1.04720, 1.15192, 1.25664, 1.36136, 1.46608, 1.57080, 1.67552, 1.78024, 1.88496, 1.98968, 2.09440, 2.19911, 2.30383, 2.40855, 2.51327, 2.61799, 2.72271, 2.82743, 2.93215, 3.03687, 3.14159, 3.24631, 3.35103, 3.45575, 3.56047, 3.66519, 3.76991, 3.87463, 3.97935, 4.08407, 4.18879, 4.29351, 4.39823, 4.50295, 4.60767, 4.71239, 4.81711, 4.92183, 5.02655, 5.13127, 5.23599, 5.34071, 5.44543, 5.55015, 5.65487, 5.75959, 5.86431, 5.96903, 6.07375, 6.17847, 6.28319, 7,
};

float sin_values[] = {
    0.00000, 0.10453, 0.20791, 0.30902, 0.40674, 0.50000, 0.58779, 0.66913, 0.74314, 0.80902, 0.86603, 0.91355, 0.95106, 0.97815, 0.99452, 1.00000, 0.99452, 0.97815, 0.95106, 0.91355, 0.86603, 0.80902, 0.74314, 0.66913, 0.58779, 0.50000, 0.40674, 0.30902, 0.20791, 0.10453, 0.00000, -0.10453, -0.20791, -0.30902, -0.40674, -0.50000, -0.58779, -0.66913, -0.74314, -0.80902, -0.86603, -0.91355, -0.95106, -0.97815, -0.99452, -1.00000, -0.99452, -0.97815, -0.95106, -0.91355, -0.86603, -0.80902, -0.74314, -0.66913, -0.58779, -0.50000, -0.40674, -0.30902, -0.20791, -0.10453, 0.00000, 0.00000,
};


float cos_values[] = {
    1.00000, 0.99452, 0.97815, 0.95106, 0.91355, 0.86603, 0.80902, 0.74314, 0.66913, 0.58779, 0.50000, 0.40674, 0.30902, 0.20791, 0.10453, 0.00000, -0.10453, -0.20791, -0.30902, -0.40674, -0.50000, -0.58779, -0.66913, -0.74314, -0.80902, -0.86603, -0.91355, -0.95106, -0.97815, -0.99452, -1.00000, -0.99452, -0.97815, -0.95106, -0.91355, -0.86603, -0.80902, -0.74314, -0.66913, -0.58779, -0.50000, -0.40674, -0.30902, -0.20791, -0.10453, 0.00000, 0.10453, 0.20791, 0.30902, 0.40674, 0.50000, 0.58779, 0.66913, 0.74314, 0.80902, 0.86603, 0.91355, 0.95106, 0.97815, 0.99452, 1.00000, 1.00000,
};


float sin(float a)
{
    int i = 0;
    while (a > 2.0f * PI)
        a -= 2.0f * PI;
    while (a > step_values[i])
        ++i;
    return sin_values[i];
}

float cos(float a)
{
    int i = 0;
    while (a > 2.0f * PI)
        a -= 2.0f * PI;
    while (a > step_values[i])
        ++i;
    return cos_values[i];
}

int main_clock()
{
    int k;
    inode_t *win = window_open(&kDESK, 0, 0, 0, 0);
    // image_t img;
    // img.width = img.height = 512;
    // img.pixels = calloc(img.width * 4, img.height);

    surface_t *img = ((window_t *)win->info)->fb;
    // TODO -- Create framebuffer

    int sz = MIN(img->width, img->height) / 2;
    for (;;) {
        memset(img->pixels, 0xde, img->width * 4 * img->height);

        time_t now = KTIME_TO_SEC(kclock());
        int secs = now % SEC_PER_DAY;
        int sec = (secs % SEC_PER_MIN);
        int min = secs % SEC_PER_HOUR / 60;
        int hour = secs / 3600;

        for (k = sz - 4; k > sz - 8; --k)
            draw_circle(img, sz, sz, k, 0x181818);
        // printf("Hour: %02d:%02d:%02d\n", hour, min, sec);
        float a = min * PI / 30.0f;
        draw_line(img, sz, sz, sz + sin(a) * sz*0.75, sz - cos(a) * sz*0.75, 0x181818);
        float h = hour * PI / 6.0f;
        draw_line(img, sz, sz, sz + sin(h) * sz*0.45, sz - cos(h) * sz*0.45, 0x181818);
        float s = sec * PI / 30.0f;
        draw_line(img, sz, sz, sz + sin(s) * sz*0.85, sz - cos(s) * sz*0.85, 0xa61818);
        for (k = 0; k < 5; ++k)
            draw_circle(img, sz, sz, k, 0x181818);

        // TODO -- Flip framebuffer

        // export_image_bmp24(&img, "clock.bmp");
        sys_sleep(SEC_TO_KTIME(1));
    }
    return 0;
}


void graphic_task()
{
    const char *txt_filename = "boot/grub/grub.cfg";
    // const char *txt_filename = "BOOT/GRUB/MENU.LST";


    inode_t *ino = window_open(&kDESK, 0, 0, 0, 0);
    tty_t *tty = tty_create(128);
    tty_window(tty, ((window_t *)ino->info)->fb, &font_8x15);
    // void* pixels = kmap(2 * _Mib_, ino, 0, VMA_FILE_RW | VMA_RESOLVE);
    // MAP NODE / USE IT TO DRAW ON SURFACE

    inode_t *root = resx_fs_pwd(kCPU.running->resx_fs);

    TTY_WRITE(tty, "shell> ls\n");
    krn_ls(tty, root);
    TTY_WRITE(tty, "shell> cat ");
    TTY_WRITE(tty, txt_filename);
    TTY_WRITE(tty, "\n");
    krn_cat(tty, txt_filename);
    TTY_WRITE(tty, "shell> ... \n");

    char v = 0;
    for (;; ++v) {
        sys_sleep(SEC_TO_KTIME(1));
        // memset(pixels, v, 32 * PAGE_SIZE);
    }
}


#define KMOD_CTRL  1
#define KMOD_ALT   2
#define KMOD_SHIFT 4

#define KEY_CAPS_LOCK  0x3A
#define KEY_NUM_LOCK  0x45
#define KEY_SCROLL_LOCK  0x46
#define KEY_SHIFT_L  0x2A
#define KEY_SHIFT_R  0x36
#define KEY_CTRL_L  0x1D
#define KEY_CTRL_R  0x61
#define KEY_ALT  0x38
#define KEY_ALTGR  0x64
#define KEY_HOST  0x5b
#define KEY_TAB  0x0f

void desktop_event(desktop_t *desk, event_t *event)
{
    splock_lock(&desk->lock);
    window_t *win = desk->focus;
    splock_unlock(&desk->lock);
    pipe_write(win->mq, (char *)&event, sizeof(event), 0);
}

static int input_event_kdbdown(desktop_t *desk, event_t *event)
{
    int key = event->param1 & 0xFFF;
    int mod = event->param1 >> 16;
    kprintf(-1, "KEY DW (%x - %x)\n", key, mod);

    if (key == KEY_CTRL_L || key == KEY_CTRL_R)
        desk->key_mods |= KMOD_CTRL;
    else if (key == KEY_ALT || key == KEY_ALTGR)
        desk->key_mods |= KMOD_ALT;
    else if (key == KEY_SHIFT_L || key == KEY_SHIFT_R)
        desk->key_mods |= KMOD_SHIFT;

    if (key == KEY_TAB) {
        splock_lock(&desk->lock);
        window_t *win = itemof(ll_pop_front(&desk->list), window_t, node);
        if (win != NULL) {
            desk->focus = win;
            ll_push_back(&desk->list, &win->node);
        }
        splock_unlock(&desk->lock);
    } else
        desktop_event(desk, event);
    return 0;
}

static int input_event_kdbup(desktop_t *desk, event_t *event)
{
    int key = event->param1 & 0xFFF;
    int mod = event->param1 >> 16;
    kprintf(-1, "KEY UP (%x - %x)\n", key, mod);

    if (key == KEY_CTRL_L || key == KEY_CTRL_R)
        desk->key_mods &= ~KMOD_CTRL;
    else if (key == KEY_ALT || key == KEY_ALTGR)
        desk->key_mods &= ~KMOD_ALT;
    else if (key == KEY_SHIFT_L || key == KEY_SHIFT_R)
        desk->key_mods &= ~KMOD_SHIFT;

    desktop_event(desk, event);
    return 0;
}

void input_event(inode_t *dev, int type, int param, pipe_t *pipe)
{
    event_t event;
    event.type = type;
    event.param1 = param;
    switch (type) {
    case 3:
        if (input_event_kdbdown(&kDESK, &event) == 0)
            return;
        break;
    case 4:
        if (input_event_kdbup(&kDESK, &event) == 0)
            return;
        break;
    }

    if (pipe != NULL)
        pipe_write(pipe, (char *)&event, sizeof(event), 0);
}


void desktop()
{
    inode_t *dev;
    for (;;) {
        dev = vfs_search_device("fb0");
        if (dev != NULL)
            break;
        sys_sleep(MSEC_TO_KTIME(50));
    }

    // We found the screen
    surface_t *screen = (surface_t *)dev->info;
    width = screen->width;
    height = screen->height;
    kprintf(-1, "Desktop %dx%d\n", width, height);

    // create window
    inode_t *ino1 = window_open(&kDESK, 0, 0, 0, 0);
    surface_t *win1 = ((window_t *)ino1->info)->fb;
    task_create(exec_task, NULL, "Example app");
    // task_create(graphic_task, NULL, "Example app");
    // task_create(graphic_task, NULL, "Example app");
    kDESK.focus = (window_t *)ino1->info;

    tty_window(slog, win1, &font_7x13);

    for (;;) {
        vds_fill(screen, 0x536d7d);
        tty_paint(slog);

        splock_lock(&kDESK.lock);
        window_t *win;
        for ll_each(&kDESK.list, win, window_t, node) {
            vds_rect(screen, win->fb->x, win->fb->y - FRAME_SZ,  win->fb->width,
                     FRAME_SZ, kDESK.focus == win ? 0x8b9b4e : 0xdddddd);
            vds_copy(screen, win->fb, win->fb->x, win->fb->y);
        }
        splock_unlock(&kDESK.lock);

        screen->flip(screen);
        sys_sleep(MSEC_TO_KTIME(50));
    }
}

