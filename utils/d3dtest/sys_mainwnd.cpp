/*
===================================================================================================

	The main game window

	This is a client module!

===================================================================================================
*/

#include "cl_local.h"

#include "../../core/sys_includes.h"
#include "../../resources/resource.h"

extern LRESULT CALLBACK MainWindowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam );

#define W_CLASSNAME		L"YUP"
#define W_EXSTYLE		0
#define W_STYLE			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX

static struct windowVars_t
{
	HWND window;
	ATOM windowClass;
} s_wndVars;

StaticCvar wnd_width( "wnd_width", "1920", 0, nullptr, nullptr, "640", nullptr );
StaticCvar wnd_height( "wnd_height", "1080", 0, nullptr, nullptr, "640", nullptr );

void Sys_CreateMainWindow()
{
	HINSTANCE instance = GetModuleHandleW( nullptr );

	HICON icon = (HICON)LoadImageW( instance, MAKEINTRESOURCEW( IDI_ICON1 ), IMAGE_ICON, 0, 0, LR_SHARED );
	HCURSOR cursor = (HCURSOR)LoadImageW( nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED );

	const WNDCLASSEXW wc{
		.cbSize = sizeof( wc ),
		.style = 0,
		.lpfnWndProc = MainWindowProc,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = instance,
		.hIcon = icon,
		.hCursor = cursor,
		.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 ),
		.lpszMenuName = nullptr,
		.lpszClassName = W_CLASSNAME,
		.hIconSm = nullptr
	};
	ATOM windowClass = RegisterClassExW( &wc );

	const int width = wnd_width.GetInt();
	const int height = wnd_height.GetInt();

	RECT r{ 0, 0, width, height };
	AdjustWindowRectEx( &r, W_STYLE, FALSE, W_EXSTYLE );

	// not multi-monitor safe
	const int dispWidth = GetSystemMetrics( SM_CXSCREEN );
	const int dispHeight = GetSystemMetrics( SM_CYSCREEN );

	// centre window
	const int xPos = ( dispWidth / 2 ) - ( width / 2 );
	const int yPos = ( dispHeight / 2 ) - ( height / 2 );

	const platChar_t *windowTitle = FileSystem::ModInfo::GetWindowTitle();

	s_wndVars.window = CreateWindowExW(
		W_EXSTYLE, (LPCWSTR)(uintptr_t)windowClass, windowTitle, W_STYLE,
		xPos, yPos,
		r.right - r.left, r.bottom - r.top,
		nullptr, nullptr, instance, nullptr
	);

	s_wndVars.windowClass = windowClass;
}

void Sys_DestroyMainWindow()
{
	HINSTANCE instance = GetModuleHandleW( nullptr );

	DestroyWindow( s_wndVars.window );
	UnregisterClassW( (LPCWSTR)(uintptr_t)s_wndVars.windowClass, instance );
}

void Sys_ShowMainWindow()
{
	ShowWindow( s_wndVars.window, SW_SHOWDEFAULT );
}

void *Sys_GetMainWindow()
{
	return (void *)s_wndVars.window;
}
