#ifndef __KERNEL_RESX_GFX_H
#define __KERNEL_RESX_GFX_H 1

#include <stddef.h>


framebuffer_t *gfx_create(int width, int height, int depth, void *pixels);
void gfx_clear(framebuffer_t *fb, uint32_t color);
void gfx_copy(framebuffer_t *dest, framebuffer_t *src, int x, int y, int w, int h);
void gfx_copy_blend(framebuffer_t *dest, framebuffer_t *src, int x, int y, int w, int h);
void gfx_flip(framebuffer_t *fb);
void gfx_rect(framebuffer_t *fb, int x, int y, int w, int h, uint32_t color);
void gfx_resize(framebuffer_t *fb, int w, int h, void *pixels);
void gfx_slide(framebuffer_t *sfc, int height, uint32_t color);

void font_paint(framebuffer_t *fb, const font_bmp_t *data, uint32_t unicode, uint32_t *color, int x, int y);


#endif  /* __KERNEL_RESX_GFX_H */
