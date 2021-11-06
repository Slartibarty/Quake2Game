
#include "../../core/core.h"

#include "../../framework/cvarsystem.h"

#include "../../core/sys_includes.h"
#include "../../resources/resource.h"

// Local
#include "input.h"

// This
#include "window.h"

//=================================================================================================

#define W_CLASSNAME		L"EEB"
#define W_TITLE			L"A New Microwave Burger Series Emerges | Ashens"
#define W_EXSTYLE		0
#define W_STYLE			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX

static struct windowVars_t
{
	HWND window;
	ATOM windowClass;
} s_wndVars;

StaticCvar wnd_width( "wnd_width", "1920", 0, nullptr, nullptr, "640", nullptr );
StaticCvar wnd_height( "wnd_height", "1080", 0, nullptr, nullptr, "640", nullptr );

void CreateMainWindow( int width, int height )
{
	HINSTANCE instance = GetModuleHandleW( nullptr );

	HICON icon = (HCURSOR)LoadImageW( instance, MAKEINTRESOURCEW( IDI_ICON1 ), IMAGE_ICON, 0, 0, LR_SHARED );
	HCURSOR cursor = (HCURSOR)LoadImageW( nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED );

	const WNDCLASSEXW wc{
		.cbSize = sizeof( wc ),
		.style = 0,
		.lpfnWndProc = D3DTestWindowProc,
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

	RECT r{ 0, 0, width, height };
	AdjustWindowRectEx( &r, W_STYLE, FALSE, W_EXSTYLE );

	// not multi-monitor safe
	int dispWidth = GetSystemMetrics( SM_CXSCREEN );
	int dispHeight = GetSystemMetrics( SM_CYSCREEN );

	// centre window
	int xPos = ( dispWidth / 2 ) - ( width / 2 );
	int yPos = ( dispHeight / 2 ) - ( height / 2 );

	s_wndVars.window = CreateWindowExW(
		W_EXSTYLE, (LPCWSTR)(uintptr_t)windowClass, W_TITLE, W_STYLE,
		xPos, yPos,
		r.right - r.left, r.bottom - r.top,
		nullptr, nullptr, instance, nullptr
	);

	s_wndVars.windowClass = windowClass;
}

void DestroyMainWindow()
{
	HINSTANCE instance = GetModuleHandleW( nullptr );

	DestroyWindow( s_wndVars.window );
	UnregisterClassW( (LPCWSTR)(uintptr_t)s_wndVars.windowClass, instance );
}

void ShowMainWindow()
{
	ShowWindow( s_wndVars.window, SW_SHOWDEFAULT );
}

void *GetMainWindow()
{
	return (void *)s_wndVars.window;
}
