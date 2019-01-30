#include <kora/stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

PACK(struct bmp_header {
    uint16_t signature;
    uint32_t file_size;
    uint32_t reserved;
    uint32_t file_offset;
    uint32_t header_size;
    uint32_t width;
    uint32_t height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    uint32_t x_pixels_per_meter;
    uint32_t y_pixels_per_meter;
    uint32_t colors;
    uint32_t important_colors;
    uint32_t red_channel_bitmask;
    uint32_t green_channel_bitmask;
    uint32_t blue_channel_bitmask;
    uint32_t alpha_channel_bitmask;
});

typedef struct image image_t;

struct image {
    int width, height;
    uint32_t *pixels;
};


int export_image_bmp24(image_t *img, const char *name)
{
    int i, j;
    size_t off = 0x7a;
    struct bmp_header header;
    header.signature = 0x4D42;
    header.file_size = off + img->width * img->height * 3;
    header.reserved = 0;
    header.file_offset = off;
    header.header_size = 0x6c; // off - sizeof(struct bmp_header);
    header.width = img->width;
    header.height = img->height;
    header.planes = 1;
    header.bits_per_pixel = 24;
    header.compression = 0;
    header.image_size = img->width * img->height * 3;
    header.x_pixels_per_meter = 720090 / 254;
    header.y_pixels_per_meter = 720090 / 254;
    header.colors = 0;
    header.important_colors = 0;
    header.red_channel_bitmask = 0x73524742;
    header.green_channel_bitmask = 0;
    header.blue_channel_bitmask = 0;
    header.alpha_channel_bitmask = 0;

    FILE *fp = fopen(name, "w");
    if (fp == NULL)
        return -1;
    fwrite(&header, 1, sizeof(header), fp);
    fseek(fp, header.file_offset, SEEK_SET);
    for (i = 0; i < img->height; ++i) {
        int k = (img->height - i) * img->width;
        for (j = 0; j < img->width; ++j) {
            fwrite(&img->pixels[k + j], 3, 1, fp);
        }
    }

    fclose(fp);
    return 0;
}

#define PX(i,x,y)  (i)->pixels[(y)*(i)->width+(x)]

void draw_circle(image_t *img, int x0, int y0, int radius, uint32_t clr)
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

void draw_line_low(image_t *img, int x0, int y0, int x1, int y1, uint32_t clr)
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

void draw_line_high(image_t *img, int x0, int y0, int x1, int y1, uint32_t clr)
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


void draw_line(image_t *img, int x0, int y0, int x1, int y1, uint32_t clr)
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

#define PI 3.141279

#define  SEC_PER_DAY (24*60*60)
#define  SEC_PER_HOUR (60*60)
#define  SEC_PER_MIN (60)

int main(int argc, char **argv)
{
    int k;
    image_t img;
    img.width = img.height = 512;
    img.pixels = calloc(img.width * 4, img.height);

    // TODO -- Create framebuffer

    int sz = 256; // Smallest side / 2
    for (;;) {
        memset(img.pixels, 0xde, img.width * 4 * img.height);

        time_t now = time(NULL);
        int secs = now % SEC_PER_DAY;
        int sec = (secs % SEC_PER_MIN);
        int min = secs % SEC_PER_HOUR / 60;
        int hour = secs / 3600;

        for (k = sz - 4; k > sz - 8; --k)
            draw_circle(&img, sz, sz, k, 0x181818);
        // printf("Hour: %02d:%02d:%02d\n", hour, min, sec);
        float a = min * PI / 30.0f;
        draw_line(&img, sz, sz, sz + sin(a) * sz*0.75, sz - cos(a) * sz*0.75, 0x181818);
        float h = hour * PI / 6.0f;
        draw_line(&img, sz, sz, sz + sin(h) * sz*0.45, sz - cos(h) * sz*0.45, 0x181818);
        float s = sec * PI / 30.0f;
        draw_line(&img, sz + sin(s) * sz*-0.15, sz - cos(s) * sz*-0.15, sz + sin(s) * sz*0.85, sz - cos(s) * sz*0.85, 0xa61818);
        for (k = 0; k < 5; ++k)
            draw_circle(&img, sz, sz, k, 0x181818);

        // TODO -- Flip framebuffer

        // export_image_bmp24(&img, "clock.bmp");
        sleep(1);
    }
    return 0;
}

