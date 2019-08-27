#ifndef __KERNEL_RESX_WIN_H
#define __KERNEL_RESX_WIN_H 1

#include <stddef.h>


int window_get_features(inode_t *win, int features, int *args);
int window_poll_push(inode_t *win, event_t *event);
int window_push_event(inode_t *win, event_t *event);
int window_set_features(inode_t *win, int features, int *args);
void *window_map(mspace_t *mspace, inode_t *win);
inode_t *window_create(desktop_t *desk, int width, int height, int flags);
inode_t *wmgr_create_window(desktop_t *desk, int width, int height);
void window_destroy(desktop_t *desk, inode_t *win);
void wmgr_input(inode_t *ino, int type, int param, pipe_t *pipe);

inode_t *tty_inode(tty_t *tty);
tty_t *tty_create(int count);
void tty_input(tty_t *tty, int unicode);
void tty_resize(tty_t *tty, int width, int height);
void tty_window(tty_t *tty, inode_t *ino, const font_bmp_t *font);
int tty_puts(tty_t *tty, const char *buf);
int tty_write(tty_t *tty, const char *buf, int len);


#endif  /* __KERNEL_RESX_WIN_H */
