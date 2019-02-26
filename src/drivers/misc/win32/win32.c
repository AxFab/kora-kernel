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
	InvalidateRect (hwnd, &r, FALSE);
} 


void win_paint(inode_t *ino) 
{
	HDC = BeginPaint (hwnd, &ps);
	BITMAP bm;
	HBITMAP bmp = CreateBitmap(width, height, 1, 32, pixels) ;
	HDC hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hbmOld = SelectObject(hdcMem, bmp);
	GetObject(bmp, sizeof(bm), &bm);
	BitBlt(hdc, 0, 0, width, height, 0, 0, SRCCOPY);
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
	.flip = win_flip, 
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
		case WN_KEYDOWN:
		    if (msg.wParam == 17)
                kdb_status |= KDB_HOST;
            else if (msg.wParam == 16)
                kdb_status |= KDB_SHIFT;
            wmgr_input(NULL, EV_KEY_PRESS, (kdb_status << 16) | key_translate(msg.wParam), NULL);
		    break;
		case WN_KEYUP:
		    if (msg.wParam == 17)
                kdb_status &= ~KDB_HOST;
            else if (msg.wParam == 16)
                kdb_status &= ~KDB_SHIFT;
            wmgr_input(NULL, EV_KEY_RELEASE, (kdb_status << 16) | key_translate(msg.wParam), NULL);
		    break;
		case WN_PAINT:
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
	wcex.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	if (! RegisterClassEx(&wcex)) 
	    return;
	
	DWORD myThreadId;
	HANDLE myHandle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE) win_events, NULL, 0, & myThreadId);
	
	framebuffer_t *fb = gfx_create(width, height, 4, NULL);
	pixels = (void*) fb->pixels;
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
	
};

int key_translate(int key)
{
	key &= 0x7F;
	return __win32_to_ibm_keys[key];
}
