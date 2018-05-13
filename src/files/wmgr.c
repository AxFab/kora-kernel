#include <kernel/core.h>
#include <kernel/files.h>
#include <kernel/input.h>
#include <kora/mcrs.h>

#define DISPLAY_HZ 45

struct desktop {
    display_t *display;
    uint32_t bg_color;
    int mouse_x, mouse_y;
    llhead_t windows;
    splock_t lock;
    int width, height;
};

struct display {
    splock_t lock;
    llhead_t screens;
};


display_t *current_display = NULL;
desktop_t *current_desktop = NULL;


surface_t *wmgr_surface(desktop_t *desktop, int width, int height, int depth)
{
    surface_t *surface = (surface_t*)kalloc(sizeof(surface_t));
    surface->width = width;
    surface->height = height;
    surface->depth = 4;
    surface->pitch = width * 4;
    return surface;
}


void wmgr_register_screen(surface_t *screen)
{
    if (current_display == NULL) {
        current_display = (display_t*)kalloc(sizeof(display_t));
        splock_init(&current_display->lock);
    }

    splock_lock(&current_display->lock);
    ll_append(&current_display->screens, &screen->node);
    splock_unlock(&current_display->lock);
}


surface_t *wmgr_window(desktop_t *desktop, int width, int height)
{
    splock_lock(&desktop->lock);
    surface_t *surface = (surface_t*)kalloc(sizeof(surface_t));
    surface->width = width;
    surface->height = height;
    surface->depth = 4;
    surface->pitch = width * 4;
    surface->pixels = kmap(surface->pitch * height, NULL, 0, VMA_ANON_RW | VMA_RESOLVE);
    surface->backup = kmap(surface->pitch * height, NULL, 0, VMA_ANON_RW | VMA_RESOLVE);
    ll_append(&desktop->windows, &surface->node);
    splock_unlock(&desktop->lock);
    return surface;
}

void wmgr_event(event_t *ev)
{
    switch (ev->type) {
    case EV_MOUSE_MOTION:
        current_desktop->mouse_x += (int)ev->param1;
        current_desktop->mouse_y += (int)ev->param2;
        current_desktop->mouse_x = MAX(0, MIN(current_desktop->width, current_desktop->mouse_x));
        current_desktop->mouse_y = MAX(0, MIN(current_desktop->height, current_desktop->mouse_y));
        break;
    }
}


void wmgr_tasket(desktop_t *desktop)
{
    // task_priority(kCPU.running, KRN_RT, MICROSEC_PER_SEC / DISPLAY_HZ);
    for (; ; ) {
        advent_wait(NULL, NULL, 1000000 / DISPLAY_HZ); // 1 sec
        kprintf(KLOG_MSG, "Paint\n");
        // display_t *display = desktop->display;
        // splock_lock(&display->lock);
        // surface_t *screen;
        // for ll_each(&display->screens, screen, surface_t, node) {
        //     vds_mouse(screen, desktop->mouse_x, desktop->mouse_y);
        //     vds_flip(screen);
        //     vds_fill(screen, desktop->bg_color);
        // }

        // splock_lock(&desktop->lock);
        // for ll_each(&display->screens, screen, surface_t, node) {

        //     surface_t *win;
        //     for ll_each(&desktop->windows, win, surface_t, node) {
        //         vds_copy(screen, win, 10, 10);
        //     }
        // }

        // splock_unlock(&desktop->lock);
        // splock_unlock(&display->lock);
    }
}



desktop_t *wmgr_desktop()
{
    desktop_t *desktop = (desktop_t *)kalloc(sizeof(desktop_t));
    desktop->bg_color = 0x4d80b3;
    desktop->display = current_display;
    surface_t *screen = ll_first(&current_display->screens, surface_t, node);
    if (screen != NULL) {
        desktop->mouse_x = screen->width / 2;
        desktop->mouse_y = screen->height / 2;
        desktop->width = screen->width;
        desktop->height = screen->height;
    }
    current_desktop = desktop;
    kernel_tasklet(wmgr_tasket, (long)desktop, "Desktop");
    return desktop;
}


