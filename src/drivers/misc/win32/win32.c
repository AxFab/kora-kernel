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
#include <kernel/device.h>
#include <kernel/input.h>
#include <string.h>
#include <windows.h>
#include <tchar.h>

int width = 1280;
int height = 720;
static TCHAR szWindowClass[] = _T("krn") ;
static TCHAR szTitle[] = _T("KoraOs") ;
static WNDCLASSEX wcex;
HINSTANCE appInstance;
HWND hwnd;
HDC hdc;
PAINTSTRUCT ps;
uint8_t *pixels;

void win_flip(inode_t *ino)
{
    RECT r;
    r.left = r.top = 0;
    r.right = width;
    r.bottom = height;
    InvalidateRect(hwnd, &r, FALSE);
}


void win_paint(inode_t *ino)
{
    HDC hdc = BeginPaint(hwnd, &ps);
    BITMAP bm;
    HBITMAP bmp = CreateBitmap(width, height, 1, 32, pixels) ;
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmOld = SelectObject(hdcMem, bmp);
    GetObject(bmp, sizeof(bm), &bm);
    BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdc, hbmOld);
    DeleteDC(hdcMem);
    EndPaint(hwnd, &ps);
    DeleteObject(bmp);
}

int win_ioctl(inode_t *ino, int cmd, void *params)
{
    return -1;
}

ino_ops_t win_ino_ops = {
    .flip = win_flip,
};

dev_ops_t win_dev_ops = {
    .ioctl = win_ioctl,
};


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void wmgr_input(inode_t *ino, int type, int param, pipe_t *pipe);

extern char __win32_to_ibm_keys[];

void win_events()
{
    int kdb_status = 0;
    hwnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, width + 16, height + 39, NULL, NULL, appInstance, NULL);
    if (!hwnd)
        return;
    hdc = GetDC(hwnd);
    for (;;) {
        MSG msg;
        if (! GetMessage(&msg, hwnd, 0, 0))
            continue;
        switch (msg.message) {
        case WM_KEYDOWN:
            if (msg.wParam == 17)
                kdb_status |= KDB_HOST;
            else if (msg.wParam == 16)
                kdb_status |= KDB_SHIFT;
            wmgr_input(NULL, EV_KEY_PRESS, (kdb_status << 16) | key_translate(msg.wParam), NULL);
            break;
        case WM_KEYUP:
            if (msg.wParam == 17)
                kdb_status &= ~KDB_HOST;
            else if (msg.wParam == 16)
                kdb_status &= ~KDB_SHIFT;
            wmgr_input(NULL, EV_KEY_RELEASE, (kdb_status << 16) | key_translate(msg.wParam), NULL);
            break;
        case WM_PAINT:
            win_paint(NULL);
            break;
        default:
            break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void win32_setup()
{
    appInstance = GetModuleHandle(NULL);
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = appInstance;
    wcex.hIcon = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_APPLICATION));
    wcex.hCursor = LoadIcon(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_APPLICATION));
    if (! RegisterClassEx(&wcex))
        return;

    DWORD myThreadId;
    HANDLE myHandle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE) win_events, NULL, 0, & myThreadId);

    framebuffer_t *fb = gfx_create(width, height, 4, NULL);
    pixels = (void *) fb->pixels;
    inode_t *ino = vfs_inode(1, FL_VDO, NULL);
    ino->info = fb;
    ino->ops = & win_ino_ops;
    ino->und.dev->model = "Fake VGA";
    ino->und.dev->devclass = "VGA";
    vfs_mkdev(ino, "fb0");
}

void win32_teardown()
{
    vfs_rmdev("fb0");
}

MODULE(win32, win32_setup, win32_teardown);


#include <kernel/input.h>

char __win32_to_ibm_keys[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0x0e, 0x0f, 0, 0, 0, 0, 0, 0,
    // 0x10
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0x20
    0x39, 0, 0, 0, 0, 0x4b, 0x48, 0x4d, 0x50, 0, 0, 0, 0, 0, 0, 0,
    // 0x30
    0x0b, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0, 0, 0, 0, 0, 0,
    // 0x40
    0, 0x1e, 0x30, 0x2e, 0, 0x12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0x50
    0, 0x10, 0x13, 0, 0x14, 0, 0, 0x11, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0x60
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0x70
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0x80
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0x90
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0xA0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0xB0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0d, 0, 0x0c, 0, 0,
    // 0xC0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0xD0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0xE0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0xF0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

int key_translate(int key)
{
    key &= 0xFF;
    return __win32_to_ibm_keys[key];
}
