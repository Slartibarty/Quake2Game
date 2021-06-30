// Windows specific variables

#pragma once

#ifndef _WIN32
#error winquake.h should not be included on this platform!
#endif

extern HWND		cl_hwnd;
extern bool		g_activeApp, g_minimized;
