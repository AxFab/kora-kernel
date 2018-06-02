#ifndef _WMGR_H
#define _WMGR_H 1

#include <kernel/core.h>
#include <kora/llist.h>
#include <kora/mcrs.h>
#include <kora/splock.h>

#define S_IFVDS 0x9000

typedef struct desktop desktop_t;
typedef struct window window_t;
typedef struct surface surface_t;
typedef struct user user_t;
typedef struct acl acl_t;
typedef struct inode inode_t;
typedef struct task task_t;


#define SCREEN_HZ 25 // 25Hz -> 40ms

typedef struct rect rect_t;
typedef struct vds_opt vds_opt_t;

struct surface {
	int width;
	int height;
	int pitch;
	int depth;
	uint8_t *pixels;
	uint8_t *backup;
	vds_opt_t *ops;
};
struct desktop {
	int handle_counter;
	user_t *user;
	acl_t *acl;
	inode_t *screen;
	llhead_t windows;
	llhead_t invals;
	splock_t lock;
};
struct window {
	inode_t *ino;
	int features;
	int events;
	task_t *thread;
	llnode_t node;
	int x, y;
};
struct rect {
	int lf, rg, tp, bt;
	llnode_t node;
	window_t *win;
	window_t *top;
};
struct vds_opt {
	void (*resize)(surface_t *srf, int width, int height);
	void(*flip)(surface_t *srf);
};

inode_t *vfs_inode(int no, int mode, acl_t *acl, size_t size);
int vfs_mkdev(CSTR name, void *dev, inode_t *ino);
inode_t *vfs_opendev(CSTR name);

void wmgr_start();
desktop_t *wmgr_create_desktop(user_t *user);
inode_t *wmgr_create_window(desktop_t *desktop, task_t *task, int width, int height, int features, int events);
void wmgr_inval(inode_t *ino, int x, int y, int w, int h);

void vds_init(inode_t *ino, size_t size);
inode_t *vds_device(CSTR name, int no, vds_opt_t ops);
void vds_resize(inode_t *ino, int *width, int *height);
void vds_blit(inode_t *dest, inode_t *src, int x, int y, int lf, int rg, int tp, int bt);
void vds_paint(inode_t *ino, int lf, int rg, int tp, int bt, uint32_t color);
void vds_flip(inode_t *ino);
void vds_fill(inode_t *ino, uint32_t color);

void export_bmp(const char* name, surface_t *win);

#endif  /* _WMGR_H */

// -=-=

#include "wmgr.h"

#define PACK(d) __pragma(pack(push,1)) d __pragma(pack(pop))

typedef struct BMP_header BMP_header_t;

PACK(struct BMP_header {
	uint16_t magic;
	uint32_t length;
	uint32_t reserved;
	uint32_t offset;
	uint32_t size;
	uint32_t width;
	uint32_t height;
	uint16_t planes;
	uint16_t bits;
	uint32_t compress;
	uint32_t image_size;
	uint32_t resol_x;
	uint32_t resol_y;
	uint32_t color_map;
	uint32_t color_count;
});

static void write_bmpdata24(FILE *fp, surface_t *win)
{
	for (int i = 0; i < win->height; ++i)
	    fwrite(&win->pixel[(win->height - i - 1) * win->pitch], win->pitch, 1, fp);
}

static void write_bmpdata24_from32(FILE *fp, surface_t *win)
{
	for (int i = 0; i < win->height; ++i) {
		for (int j = 0; j < win->width; ++j) {
			uint8_t *px = &win->pixels[(win->height - i - 1) * win->pitch + j * win->depth];
			uint32_t color = (*((uint32_t*)px)) & 0xFFFFFF;
			fwrite(&color, 3, 1, fp);
		}
		int pad = 4 - (win->width & 3) & 3;
		uin32_t z = 0;
		if (pad > 0)
		    fwrite(&z, pad, 1, fp);
	}
}

void export_bmp(const char* name, surface_t *win)
{
	BMP_header_t header;
	memset(&header, 0, sizeof(header));
	header.magic = 0x4D42;
	header.length = sizeof(header) + win->height * ALIGN_UP(win->width * 3);
	header.offset = sizeof(header);
	header.size = 40;
	header.width = win->width;
	header.height = win->height;
	header.planes = 1;
	header.image_size = win->height * ALIGN_UP(win->width, 4) * 3;
	header.bits = 24; // win->depth * 8
	header.resol_x = resol_y = 96 * 10000 / 254;
	FILE *fp = fopen(name, "w");
	fwrite(&header, sizeof(header), 1, fp);
	if (win->depth == 3)
	    write_bmpdata24(fp, win);
	else
	    write_bmpdata24_from32(fp, win);
	fclose(fp);
}

// -=-=

static void *memcpy32(void *dest, void *src, size_t lg)
{
	assert(IS_ALIGN(lg, 4));
	assert(IS_ALIGN(dest, 4));
	assert(IS_ALIGN(src, 4));
	register uint32_t *a = (uint32_t*)src;
	register uint32_t *b = (uint32_t*)dest;
	while (lg > 16) {
		b[0] = a[0];
		b[1] = a[1];
		b[2] = a[2];
		b[3] = a[3];
		lg -= 16;
		a += 4;
		b += 4;
	}
	while (lg > 0) {
		b[0] = a[0];
		lg -= 4;
		a++;
		b++;
	}
	return dest;
}

static void *memset32(void *dest, uint32_t val, size_t lg)
{
	assert(IS_ALIGN(lg, 4));
	assert(IS_ALIGN(dest, 4));
	register uint32_t *a = (uint32_t*)dest;
	while (lg > 16) {
		a[0] = val;
		a[1] = val;
		a[2] = val;
		a[3] = val;
		lg -= 16;
		a += 4;
	}
	while (lg > 0) {
		a[0] = val;
		lg -= 4;
		a++;
	}
	return dest;
}

void vds_init(inode_t *ino)
{
	surface_t *srf = (surface_t*)kalloc(sizeof(surface_t));
	ino->object = srf;
}

inode_t *vds_device(CSTR name, int no, vds_ops_t *ops)
{
	inode_t *ino = vfs_inode(no, S_IFVDS | 0740, NULL, 0);
	surface_t *srf = (surface_t*)ino->object;
	srf->ops = ops;
	vfs_mkdev(name, NULL, ino);
	return ino;
}

void vds_resize(inode_t *ino, int *width, int *height)
{
	surface_t *srf = (surface_t*)ino->object;
	if (srf->ops) {
		srf->ops->resize(srf, *width, *height);
		*width = srf->width;
		*height = srf->height;
		return;
	}
	if (srf->pixels) {
		kfree(srf->pixels);
		kfree(srf->backup);
	}
	srf->width = *width;
	srf->height = *height;
	srf->pitch = ALIGN_UP(*width, 4) * 4;
	srf->pixels = (uint8_t*)kalloc(srf->height * srf->pitch);
	srf->backup = (uint8_t*)kalloc(srf->height * srf->pitch);
    srf->depth = 4;
}

void vds_flip(inode_t *ino)
{
	surface_t *srf = (surface_t*)ino->object;
	if (srf->ops) {
		srf->ops->flip(srf);
		return;
	}

    memcpy32(srf->backup, srf->pixels, srf->pitch * srf->height);
}

void vds_blit(inode_t *dest, inode_t *src, int x, int y, int lf, int rg, int tp, int bt)
{
	surface_t *sa = (surface_t*)src->object;
	surface_t *sb = (surface_t*)dedt->object;
	for (int i = tp; i < bt; ++i) {
		int ka = (i - y) * sa->pitch;
		int kb = i * sb->pitch;
		uint8_t *pxa = &sa->backup[ka + (lf - x) * sa->depth];
		uint8_t *pxb = &sb->pixels[kb + lf * sb->depth];
		memcpy32(pxb, pxa, (rg - lf) * sb->depth);
		/*
		for (int j = lf; j < rg; ++j) {
		}
		*/
	}
}

void vds_paint(inode_t *ino, int lf, int rg, int tp, int bt, uint32_t color)
{
	surface_t *srf = (surface_t*)ino->object;
	for (int i = tp; i < bt; ++i) {
		int k = i * srf->pitch;
		uint8_t *px = &sb->pixels[kb + lf * sb->depth];
		memset32(px, color, (rg - lf) * sb->depth);
		/*
		for (int j = lf; j < rg; ++j) {
		}
		*/
	}
}

void vds_fill(inode_t *ino, uint32_t color)
{
	surface_t *srf = (surface_t*)ino->object;
	vds_paint(ino, 0, srf->width, 0, srf->height);
}

void vds_shadow(inode_t *ino, int x, int y, int w, int h)
{

}

// -=-=

void rect_intersect(rect_t *r, rect_t *a, rect_t *b)
{
	r->lf = MAX(a->lf, b->lf);
	r->rg = MIN(a->rg, b->rg);
	r->tp = MAX(a->tp, b->tp);
	r->bt = MIN(a->bt, b->bt);
}

void rect_push(llhead_t *queue, int lf, int rg, int tp, int bt, window_t *win, window_t *top)
{
	rect_t *i = (rect_t*)kalloc(sizeof(rect_t));
	i->lf = lf;
	i->rg = rg;
	i->tp = tp;
	i->bt = bt;
	i->win = win
	i->top = top;
	ll_enqueue(queue, &i->node);
}

void rect_requeue_sub(llhead_t *queue, rect_t *a, rect_t *b, window_t *win, window_t *top)
{
	if (a->tp < b->tp)
	    rect_push(queue, a->lf, a->rg, a->tp, b->tp, win, top);
	if (a->lf < b->lf)
	    rect_push(queue, a->lf, b->lf, b->tp, b->bt, win, top);
	if (a->rg > b->rg)
	    rect_push(queue, b->rg, a->rg, b->tp, b->bt, win, top);
	if (a->bt > b->bt)
	    rect_push(queue, a->lf, a->rg, b->bt, a->bt, win, top);
}

bool rect_isnull(rect_t *r)
{
	return r->lf >= r->rg || r->tp >= r->bt;
}

void wmgr_inval(inode_t *ino, int x, int y, int w, int h)
{
	desktop_t *desktop = __desktop;
	splock_lock(&desktop->lock);
	if (ino == NULL) {
		rect_push(&desktop->invals, x, x + w, y, y + h, NULL, NULL);
	} else {
		window_t *win = NULL;
		for ll_each(&desktop->windows, win, window_t, node) {
			if (win->ino == ino)
			    break;
		}
		if (win != NULL) {
			vds_flip(ino);
			// TODO -- intersect, move clip
			rect_push(&desktop->invals, x, x + w, y, y + h, win, NULL);
		}
	}
	splock_unlock(&desktop->lock);
}

void wmgr_draw(desktop_t *desktop)
{
	if (desktop->invals.count_ <= 0)
	    return;
	// todo, copy queue and remove dobles
	splock_lock(&desktop->lock);
	llhead_t second = INIT_LLHEAD;
	for (;;) {
		if (desktop->invals.count_ == 0)
			break;
		rect_t *area = ll_dequeue(&desktop->invals, rect_t, node);
		window_t *win = area->win != NULL ? area->win: ll_last(&desktop->windows, window_t, node);
		bool draw = false;
		for (; win != NULL; win = ll_previous(&win->node, window_t, node)) {
			rect_t rect;
			rect.lf = win->x;
			rect.tp = win->y;
			rect.rg = win->x + ((surface_t*)win->ino->object)->width;
			rect.bt = win->y + ((surface_t*)win->ino->object)->height;
			rect_t *intr = (rect_t*)kalloc(sizeof(rect_t));
			rect_intersect(intr, area, &rect);
			if (rect_isnull(intr))
			    continue
			// todo - loops over screens
			if (area->win == NULL || area->win == win) {
				vds_blit(desktop->screen, win->ino, win->x, win->y, intr->lf, intr->rg, intr->tp, intr->bt);
				intr->win = win;
				ll_enqueue(&second, &intr->node);
				rect_requeue_sub(&desktop->invals, area, intr, area->win, win);
			} else {
			    rect_requeue_sub(&desktop->invals, area, intr, area->win, win);
			    free(intr);
			}
			free(area);
			draw = true;
			break;
		}
		if (!draw) {
			vds_paint(desktop->screen, area->lf, area->rg, area->tp, area->bt, 0x3D7E92);
			area->win = NULL;
			ll_enqueue(&second, &area->node);
		}
	}

	window_t *last = ll_last(&desktop->windows, window_t, node);
	vds_shadow();
	splock_unlock();
	// mouse
	vds_flip(desktip->screen);

	while (second.count > 0) {
		rect_t *area = ll_dequeue(&second, );
	    if (area->win)
	        vds_blit();
	    else
	        vds_paint();
	    free(area);
	}
}

void wmgr_loop()
{
	int period = 1000000 / DISPLAY_HZ;
	for (;;async_wait(NULL, NULL, period - time64() + last)) {
		last = time64();
		desktop_t *desktop = __desktop;
		if (desktop) {
		    wmgr_draw(desktop);
		} else {
			// Why !?
		}
	}
}

desktop_t *wmgr_open_desktop(CSTR name, user_t *user, inode_t *screen)
{
	int lg = strlen(name);
	// lock
	desktop_t *desktop = hmp_get(name, );
	if (desktop) {
		// unlock
		return desktop;
	}
	// upgrade rwlock!?
    desktop = (desktop_t*)kalloc(sizeof(desktop_t));
	desktop->user = user;
	hmp_put(__desks, name, strlen(name), desktop);
	desktop->screen = screen;
	// create desktop area!
	// unlock
    return desktop;
}

inode_t *create_window(desktop_t *desktop, task_t *task, int features, int events)
{
	window_t *win = (window_t*)kalloc(sizeof(window_t));
	win->ino = vfs_inode(desktop->handle, S_IFVDS | 0740, NULL, 0);
	vds_resize(win->ino, width, height);
    ll_push_back(&desktop->window, &win->node);
    desktop->sx += 20;
    desktop->sy += 20;
    if (desktop->sx + width > desktop->w)
        desktop->sx = 0;
    if (desktop->sy + height > desktop-h)
        desktop->sy = 0;
    win->x = desktop->sx;
    win->y = desktop->sy;
	return win->ino;
}

void wmgr_start()
{
	inode_t *screen = vfs_opendev("Scr0");
	if (screen == NULL) {
		kprintf(KLOG_ERR, "No display found, windows mamager task aborted\n");
		return;
	}

	inode_t *img = wmgr_loadimage("login_bg.png");
	// Load login screen
	wmgr_loop();
}
