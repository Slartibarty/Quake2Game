// winquake.h: Win32-specific Quake header file

#pragma once

#include <Windows.h>

extern HINSTANCE	g_hInstance;

extern HWND			cl_hwnd;
extern qboolean		ActiveApp, Minimized;

void IN_MouseEvent (int mstate);
