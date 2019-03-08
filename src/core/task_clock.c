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
#include <kernel/task.h>
#include <kernel/syscalls.h>
#include <string.h>

int abs(int v);

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

void draw_circle(framebuffer_t *img, int x0, int y0, int radius, uint32_t clr)
{
    int x = radius - 1;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (radius << 1);

    while (x >= y) {
        PX(img, x0 + x, y0 + y) = clr;
        PX(img, x0 + y, y0 + x) = clr;
        PX(img, x0 - y, y0 + x) = clr;
        PX(img, x0 - x, y0 + y) = clr;
        PX(img, x0 - x, y0 - y) = clr;
        PX(img, x0 - y, y0 - x) = clr;
        PX(img, x0 + y, y0 - x) = clr;
        PX(img, x0 + x, y0 - y) = clr;

        if (err <= 0) {
            y++;
            err += dy;
            dy += 2;
        }

        if (err > 0) {
            x--;
            dx += 2;
            err += dx - (radius << 1);
        }
    }
}

void draw_line_low(framebuffer_t *img, int x0, int y0, int x1, int y1, uint32_t clr)
{
    int x;
    int dx = x1 - x0;
    int dy = y1 - y0;
    int yi = 1;
    if (dy < 0) {
        yi = -1;
        dy = -dy;
    }
    int D = 2 * dy - dx;
    int y = y0;

    for (x = x0; x < x1; ++x) {
        PX(img, x, y) = clr;
        if (D > 0) {
            y = y + yi;
            D = D - 2 * dx;
        }
        D = D + 2 * dy;
    }
}

void draw_line_high(framebuffer_t *img, int x0, int y0, int x1, int y1, uint32_t clr)
{
    int y;
    int dx = x1 - x0;
    int dy = y1 - y0;
    int xi = 1;
    if (dx < 0) {
        xi = -1;
        dx = -dx;
    }

    int D = 2 * dx - dy;
    int x = x0;

    for (y = y0; y < y1; ++y) {
        PX(img, x, y) = clr;
        if (D > 0) {
            x = x + xi;
            D = D - 2 * dy;
        }
        D = D + 2 * dx;
    }
}


void draw_line(framebuffer_t *img, int x0, int y0, int x1, int y1, uint32_t clr)
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

void main_clock()
{
    int k;
    inode_t *win = wmgr_create_window(NULL, 200, 200);

    framebuffer_t *img = ((window_t *)win->info)->frame;

    int sz = MIN(img->width, img->height) / 2;

    time_t last = 0;
    for (;;) {

        time_t now = KTIME_TO_SEC(kclock());
        if (now != last) {
            int secs = now % SEC_PER_DAY;
            int sec = (secs % SEC_PER_MIN);
            int min = secs % SEC_PER_HOUR / 60;
            int hour = secs / 3600;

            memset(img->pixels, 0xde, img->width * 4 * img->height);
            for (k = sz - 4; k > sz - 8; --k)
                draw_circle(img, sz, sz, k, 0x181818);
            // printf("Hour: %02d:%02d:%02d\n", hour, min, sec);
            float a = min * PI / 30.0f;
            draw_line(img, sz, sz, sz + sin(a) * sz * 0.75, sz - cos(a) * sz * 0.75, 0x181818);
            float h = hour * PI / 6.0f;
            draw_line(img, sz, sz, sz + sin(h) * sz * 0.45, sz - cos(h) * sz * 0.45, 0x181818);
            float s = sec * PI / 30.0f;
            draw_line(img, sz, sz, sz + sin(s) * sz * 0.85, sz - cos(s) * sz * 0.85, 0xa61818);
            for (k = 0; k < 5; ++k)
                draw_circle(img, sz, sz, k, 0x181818);

            win->ops->flip(win);
        }
        // export_image_bmp24(&img, "clock.bmp");
        sys_sleep(MSEC_TO_KTIME(500));
    }
}
