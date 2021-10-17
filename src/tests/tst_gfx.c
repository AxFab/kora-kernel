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
#include <kernel/core.h>
#include <kernel/files.h>
#include <threads.h>
#include <assert.h>
#include <string.h>
#include "check.h"

void futex_init();



framebuffer_t *gfx_create(int width, int height, int depth, void *pixels);
void gfx_destroy(framebuffer_t *fb, void *pixels);
void gfx_resize(framebuffer_t *fb, int w, int h, void *pixels);
void gfx_rect(framebuffer_t *fb, int x, int y, int w, int h, uint32_t color);
void gfx_shadow(framebuffer_t *fb, int x, int y, int r, uint32_t color);
void gfx_clear(framebuffer_t *fb, uint32_t color);
void gfx_slide(framebuffer_t *fb, int height, uint32_t color);
void gfx_copy(framebuffer_t *dest, framebuffer_t *src, int x, int y, int w, int h);
void gfx_copy_blend(framebuffer_t *dest, framebuffer_t *src, int x, int y, int w, int h);


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

TEST_CASE(tst_gfx_01)
{
    framebuffer_t *fb1 = gfx_create(120, 120, 4, NULL);
    framebuffer_t *fb2 = gfx_create(120, 120, 4, NULL);

    gfx_clear(fb1, 0x181818);
    gfx_rect(fb1, 0, 0, 120, 20, 0xa61010);
    gfx_copy(fb2, fb1, 0, 0, 120, 120);
    gfx_slide(fb2, 40, 0x1010a6);

    gfx_destroy(fb1, NULL);
    gfx_destroy(fb2, NULL);
}

TEST_CASE(tst_gfx_02)
{
    framebuffer_t *fb1 = gfx_create(120, 120, 4, NULL);
    framebuffer_t *fb2 = gfx_create(120, 120, 3, NULL);

    gfx_clear(fb1, 0x181818);
    gfx_rect(fb1, 0, 0, 120, 20, 0xa61010);
    gfx_copy(fb2, fb1, 0, 0, 120, 120);

    gfx_clear(fb2, 0x181818);
    gfx_rect(fb2, 0, 0, 120, 20, 0xa61010);
    gfx_copy(fb1, fb2, 0, 0, 120, 120);

    gfx_destroy(fb1, NULL);
    gfx_destroy(fb2, NULL);
}

TEST_CASE(tst_gfx_03)
{
    framebuffer_t *fb1 = gfx_create(120, 120, 4, NULL);
    framebuffer_t *fb2 = gfx_create(60, 40, 4, NULL);

    gfx_resize(fb1, 120, 80, NULL);
    gfx_shadow(fb1, 40, 40, 25, 0xFF10a610);

    gfx_clear(fb2, 0x181818);
    gfx_rect(fb2, 0, 0, 120, 20, 0xa61010);

    gfx_copy_blend(fb2, fb1, 0, 0, 40, 80);

    gfx_destroy(fb1, NULL);
    gfx_destroy(fb2, NULL);
}


jmp_buf __tcase_jump;
SRunner runner;

int main()
{
    kSYS.cpus = calloc(sizeof(struct kCpu), 0x500);
    // futex_init();

    suite_create("Gfx");
    tcase_create(tst_gfx_01);
    tcase_create(tst_gfx_02);
    tcase_create(tst_gfx_03);

    free(kSYS.cpus);
    return summarize();
}
