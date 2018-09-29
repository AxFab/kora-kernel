/*
 *      This file is part of the KoraOS project.
 *  Copyright (C) 2018  <Fabien Bavent>
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
// #include <cairo.h>
// #include <cairo-xlib.h>


// void seat_fb0(int width, int height, int format, int pitch, void* mmio);

// void *create_surface(int width, int height)
// {
//     Display *dsp;
//     Drawable da;
//     int screen;
//     cairo_surface_t *sfc;

//     if ((dsp = XOpenDisplay(NULL)) == NULL) {
//         kprintf(-1, "Unable to open X11 display\n");
//         return NULL;
//     }
//     screen = DefaultScreen(dsp);
//     da = XCreateSimpleWindow(dsp, DefaultRootWindow(dsp), 0, 0, width, height, 0, 0, 0);
//     XSelectInput(dsp, da,
//         // ButtonMotionMask |  ButtonPressMask | ButtonReleaseMask | // Mouse
//         // KeyPressMask | KeyReleaseMask | KeymapStateMask | // Keyboard
//         // EnterWindowMask | LeaveWindowMask | PointerMotionMask |
//         // // Button1MotionMask | Button2MotionMask | Button3MotionMask |
//         // // Button4MotionMask | Button5MotionMask |
//         // VisibilityChangeMask | StructureNotifyMask | ResizeRedirectMask |
//         // SubstructureNotifyMask | SubstructureRedirectMask | FocusChangeMask |
//         // PropertyChangeMask | ColormapChangeMask |
//         ExposureMask);
//     XMapWindow(dsp, da);

//     sfc = cairo_xlib_surface_create(dsp, da, DefaultVisual(dsp, screen), width, height);
//     cairo_xlib_surface_set_size(sfc, width, height);
//     void *data = cairo_image_surface_get_data(sfc);
//     cairo_format_t frmt = cairo_image_surface_get_format(sfc);
//     // cairo_public unsigned char *
//     // cairo_image_surface_get_data (cairo_surface_t *surface);
//     // cairo_public cairo_format_t
//     // cairo_image_surface_get_format (cairo_surface_t *surface);

//     // seat_fb0(width, height, 0, width, data);
//     return sfc;
// }

