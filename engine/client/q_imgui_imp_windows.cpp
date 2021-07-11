//=================================================================================================
// Think Tank 2
// Copyright (c) Slartibarty. All Rights Reserved.
//=================================================================================================

#include "../renderer/gl_local.h"

#include "../../core/sys_includes.h"
#include "../../resources/resource.h"
#include <ShellScalingApi.h>
#include <VersionHelpers.h>

#include "GL/glew.h"
#include "GL/wglew.h"

#include "imgui.h"

extern cvar_t *r_swapinterval;
extern cvar_t *r_multisamples;

extern LRESULT CALLBACK ChildWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

#define USE_STEAM_SKIN

/*
===================================================================================================

	Platform interface

===================================================================================================
*/

namespace qImGui
{
	static constexpr auto QIMGUI_WINDOWCLASS = L"ImGui Platform";

	// per-viewport data
	struct viewportData_t
	{
		HDC dc;
		HGLRC glrc;
	};

	static struct winImp_t
	{
		HWND mainWindow;
		ImGuiMouseCursor lastcursor;
		double oldTime;
		bool wantMonitorUpdate;
	} pv;

	//=================================================================================================

	static float OSImp_GetDpiScaleForMonitor( void* monitor )
	{
		UINT xdpi = 96, ydpi = 96;
		if ( IsWindows8Point1OrGreater() )
		{
			GetDpiForMonitor( (HMONITOR)monitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi );
			assert( xdpi == ydpi );
			return xdpi / 96.0f;
		}
		else
		{
			const HDC dc = GetDC( nullptr );
			xdpi = GetDeviceCaps( dc, LOGPIXELSX );
			ydpi = GetDeviceCaps( dc, LOGPIXELSY );
			assert( xdpi == ydpi );
			ReleaseDC( nullptr, dc );
			return xdpi / 96.0f;
		}
	}

	static float OSImp_GetDpiScaleForHwnd( void* hwnd )
	{
		return OSImp_GetDpiScaleForMonitor( MonitorFromWindow( (HWND)hwnd, MONITOR_DEFAULTTONEAREST ) );
	}

	static DWORD OSImp_GetStyleFromFlags( ImGuiViewportFlags flags )
	{
		DWORD style = 0;

		if ( flags & ImGuiViewportFlags_NoDecoration ) {
			style |= WS_POPUP;
		} else {
			style |= WS_OVERLAPPEDWINDOW;
		}

		return style;
	}

	static DWORD OSImp_GetExStyleFromFlags( ImGuiViewportFlags flags )
	{
		DWORD exStyle = 0;

		if ( flags & ImGuiViewportFlags_NoTaskBarIcon ) {
			exStyle |= WS_EX_TOOLWINDOW;
		} else {
			exStyle |= WS_EX_APPWINDOW;
		}

		if ( flags & ImGuiViewportFlags_TopMost ) {
			exStyle |= WS_EX_TOPMOST;
		}

		return exStyle;
	}

	static void OSImp_CreateWindow( ImGuiViewport* viewport )
	{
		// Get style
		DWORD style = OSImp_GetStyleFromFlags( viewport->Flags );
		DWORD exStyle = OSImp_GetExStyleFromFlags( viewport->Flags );

		// Get parent window
		HWND parentWindow = nullptr;
		if ( viewport->ParentViewportId != 0 )
		{
			if ( ImGuiViewport* parentViewport = ImGui::FindViewportByID( viewport->ParentViewportId ) )
			{
				parentWindow = (HWND)parentViewport->PlatformHandle;
			}
		}

		RECT rect{ (LONG)viewport->Pos.x, (LONG)viewport->Pos.y, (LONG)( viewport->Pos.x + viewport->Size.x ), (LONG)( viewport->Pos.y + viewport->Size.y ) };
		AdjustWindowRectEx( &rect, style, FALSE, exStyle );

		// Create window
		HWND hWnd = CreateWindowExW(
			exStyle,
			QIMGUI_WINDOWCLASS,
			L"Untitled",
			style,
			rect.left,
			rect.top,
			rect.right - rect.left,
			rect.bottom - rect.top,
			parentWindow,
			nullptr,
			GetModuleHandleW( nullptr ),
			nullptr
		);

		viewportData_t *data = new viewportData_t;
		viewport->PlatformUserData = data;

		data->dc = GetDC( hWnd );
		assert( data->dc );

		data->glrc = reinterpret_cast<HGLRC>( GLimp_SetupContext( data->dc, true ) );

		viewport->PlatformHandle = hWnd;
		viewport->PlatformHandleRaw = hWnd;

		viewport->PlatformRequestResize = false;
	}

	static void OSImp_DestroyWindow( ImGuiViewport* viewport )
	{
		HWND window = reinterpret_cast<HWND>( viewport->PlatformHandle );
		if ( window == pv.mainWindow )
		{
			assert( viewport->PlatformUserData == nullptr );
			return;
		}
		if ( window )
		{
			if ( GetCapture() == window )
			{
				// Transfer capture so if we started dragging from a window that later disappears, we'll still receive the MOUSEUP event.
				ReleaseCapture();
				SetCapture( window );
			}
			viewportData_t *data = (viewportData_t *)viewport->PlatformUserData;
			assert( data );
			if ( data->glrc )
			{
				wglMakeCurrent( nullptr, nullptr );
				wglDeleteContext( data->glrc );
			}
			DestroyWindow( window );
		}
		delete viewport->PlatformUserData;
		viewport->PlatformUserData = nullptr;
		viewport->PlatformHandle = nullptr;
		viewport->PlatformHandleRaw = nullptr;
	}

	static void OSImp_ShowWindow( ImGuiViewport* viewport )
	{
		HWND window = reinterpret_cast<HWND>( viewport->PlatformHandle );
		assert( window != 0 );

		if ( viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing ) {
			ShowWindow( window, SW_SHOWNA );
		} else {
			ShowWindow( window, SW_SHOW );
		}
	}

	static void OSImp_UpdateWindow( ImGuiViewport* viewport )
	{
		HWND window = reinterpret_cast<HWND>( viewport->PlatformHandle );
		assert( window != 0 );

		// (Optional) Update Win32 style if it changed _after_ creation.
		// Generally they won't change unless configuration flags are changed, but advanced uses (such as manually rewriting viewport flags) make this useful.
		DWORD oldStyle = GetWindowLongW( window, GWL_STYLE );
		DWORD oldExStyle = GetWindowLongW( window, GWL_EXSTYLE );
		DWORD newStyle = OSImp_GetStyleFromFlags( viewport->Flags );
		DWORD newExStyle = OSImp_GetExStyleFromFlags( viewport->Flags );

		// Only reapply the flags that have been changed from our point of view (as other flags are being modified by Windows)
		if ( oldStyle != newStyle || oldExStyle != newExStyle )
		{
			// (Optional) Update TopMost state if it changed _after_ creation
			bool topMostChanged = ( oldExStyle & WS_EX_TOPMOST ) != ( newExStyle & WS_EX_TOPMOST );
			HWND insertAfter = topMostChanged ? ( ( viewport->Flags & ImGuiViewportFlags_TopMost ) ? HWND_TOPMOST : HWND_NOTOPMOST ) : 0;
			UINT swpFlag = topMostChanged ? 0 : SWP_NOZORDER;

			// Apply flags and position (since it is affected by flags)
			SetWindowLongW( window, GWL_STYLE, newStyle );
			SetWindowLongW( window, GWL_EXSTYLE, newExStyle );
			RECT rect{ (LONG)viewport->Pos.x, (LONG)viewport->Pos.y, (LONG)( viewport->Pos.x + viewport->Size.x ), (LONG)( viewport->Pos.y + viewport->Size.y ) };
			AdjustWindowRectEx( &rect, newStyle, FALSE, newExStyle ); // Client to Screen
			SetWindowPos( window, insertAfter, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, swpFlag | SWP_NOACTIVATE | SWP_FRAMECHANGED );
			ShowWindow( window, SW_SHOWNA ); // This is necessary when we alter the style
			viewport->PlatformRequestMove = true;
			viewport->PlatformRequestResize = true;
		}
	}

	static ImVec2 OSImp_GetWindowPos( ImGuiViewport* viewport )
	{
		HWND window = reinterpret_cast<HWND>( viewport->PlatformHandle );
		assert( window != 0 );

		POINT pos{};
		ClientToScreen( window, &pos );
		return ImVec2( (float)pos.x, (float)pos.y );
	}

	static void OSImp_SetWindowPos( ImGuiViewport* viewport, ImVec2 pos )
	{
		HWND window = reinterpret_cast<HWND>( viewport->PlatformHandle );
		assert( window != 0 );

		RECT rect{ (LONG)pos.x, (LONG)pos.y, (LONG)pos.x, (LONG)pos.y };
		AdjustWindowRectEx( &rect, GetWindowLongW( window, GWL_STYLE ), FALSE, GetWindowLongW( window, GWL_EXSTYLE ) );
		SetWindowPos( window, nullptr, rect.left, rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE );
	}

	static ImVec2 OSImp_GetWindowSize( ImGuiViewport* viewport )
	{
		HWND window = reinterpret_cast<HWND>( viewport->PlatformHandle );
		assert( window != 0 );

		RECT rect;
		GetClientRect( window, &rect );
		return ImVec2( float( rect.right - rect.left ), float( rect.bottom - rect.top ) );
	}

	static void OSImp_SetWindowSize( ImGuiViewport* viewport, ImVec2 size )
	{
		HWND window = reinterpret_cast<HWND>( viewport->PlatformHandle );
		assert( window != 0 );

		RECT rect = { 0, 0, (LONG)size.x, (LONG)size.y };
		AdjustWindowRectEx( &rect, GetWindowLongW( window, GWL_STYLE ), FALSE, GetWindowLongW( window, GWL_EXSTYLE ) ); // Client to Screen
		SetWindowPos( window, nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE );
	}

	static void OSImp_SetWindowFocus( ImGuiViewport* viewport )
	{
		HWND window = reinterpret_cast<HWND>( viewport->PlatformHandle );
		assert( window != 0 );

		BringWindowToTop( window );
		SetForegroundWindow( window );
		SetFocus( window );
	}

	static bool OSImp_GetWindowFocus( ImGuiViewport* viewport )
	{
		HWND window = reinterpret_cast<HWND>( viewport->PlatformHandle );
		assert( window != 0 );

		return GetForegroundWindow() == window;
	}

	static bool OSImp_GetWindowMinimized( ImGuiViewport* viewport )
	{
		HWND window = reinterpret_cast<HWND>( viewport->PlatformHandle );
		assert( window != 0 );

		return IsIconic( window ) != FALSE;
	}

	static void OSImp_SetWindowTitle( ImGuiViewport* viewport, const char* title )
	{
		// Slart: On Windows 10 SetWindowTextA accepts UTF-8 with our app config
		HWND window = reinterpret_cast<HWND>( viewport->PlatformHandle );
		assert( window != 0 );
#if 0
		// ::SetWindowTextA() doesn't properly handle UTF-8 so we explicitely convert our string.
		int n = ::MultiByteToWideChar( CP_UTF8, 0, title, -1, NULL, 0 );
		ImVector<wchar_t> title_w;
		title_w.resize( n );
		::MultiByteToWideChar( CP_UTF8, 0, title, -1, title_w.Data, n );
		::SetWindowTextW( vd->Hwnd, title_w.Data );
#else
		SetWindowTextA( window, title );
#endif
	}

	static void OSImp_SetWindowAlpha( ImGuiViewport* viewport, float alpha )
	{
		HWND window = reinterpret_cast<HWND>( viewport->PlatformHandle );
		assert( window != 0 );

		assert( alpha >= 0.0f && alpha <= 1.0f );
		if ( alpha < 1.0f )
		{
			DWORD style = GetWindowLongW( window, GWL_EXSTYLE ) | WS_EX_LAYERED;
			SetWindowLongW( window, GWL_EXSTYLE, style );
			SetLayeredWindowAttributes( window, 0, static_cast<BYTE>( 255.0f * alpha ), LWA_ALPHA );
		}
		else
		{
			DWORD style = GetWindowLongW( window, GWL_EXSTYLE ) & ~WS_EX_LAYERED;
			SetWindowLongW( window, GWL_EXSTYLE, style );
		}
	}

	static float OSImp_GetWindowDpiScale( ImGuiViewport* viewport )
	{
		HWND window = reinterpret_cast<HWND>( viewport->PlatformHandle );
		assert( window != 0 );

		return OSImp_GetDpiScaleForHwnd( window );
	}

	// FIXME-DPI: Testing DPI related ideas
	static void OSImp_OnChangedViewport( [[maybe_unused]] ImGuiViewport* viewport )
	{
#if 0
		ImGuiStyle default_style;
		//default_style.WindowPadding = ImVec2(0, 0);
		//default_style.WindowBorderSize = 0.0f;
		//default_style.ItemSpacing.y = 3.0f;
		//default_style.FramePadding = ImVec2(0, 0);
		default_style.ScaleAllSizes( viewport->DpiScale );
		ImGuiStyle& style = ImGui::GetStyle();
		style = default_style;
#endif
	}

	// shouldn't be here

	static void OSImp_RenderWindow( ImGuiViewport* viewport, void* renderArg )
	{
		viewportData_t *data = (viewportData_t *)viewport->PlatformUserData;
		assert( data );

		BOOL blah = wglMakeCurrent( data->dc, data->glrc );
		assert( blah );
	}

	static void OSImp_SwapBuffers( ImGuiViewport* viewport, void* renderArg )
	{
		viewportData_t *data = (viewportData_t *)viewport->PlatformUserData;
		assert( data );

		BOOL blah = wglMakeCurrent( data->dc, data->glrc );
		assert( blah );
		blah = SwapBuffers( data->dc );
		assert( blah );
	}

#if 0
	static LRESULT CALLBACK OSImp_WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{
		if ( MainWndProc( hWnd, msg, wParam, lParam ) != 0 ) {
			return 1; // TODO: ???
		}

		if ( ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle( (void*)hWnd ) )
		{
			switch ( msg )
			{
			case WM_CLOSE:
				viewport->PlatformRequestClose = true;
				return 0;
			case WM_MOVE:
				viewport->PlatformRequestMove = true;
				return 0;
			case WM_SIZE:
				viewport->PlatformRequestResize = true;
				return 0;
			case WM_MOUSEACTIVATE:
				if ( viewport->Flags & ImGuiViewportFlags_NoFocusOnClick ) {
					return MA_NOACTIVATE;
				}
				break;
			case WM_NCHITTEST:
				// Let mouse pass-through the window. This will allow the backend to set io.MouseHoveredViewport properly (which is OPTIONAL).
				// The ImGuiViewportFlags_NoInputs flag is set while dragging a viewport, as want to detect the window behind the one we are dragging.
				// If you cannot easily access those viewport flags from your windowing/event code: you may manually synchronize its state e.g. in
				// your main loop after calling UpdatePlatformWindows(). Iterate all viewports/platform windows and pass the flag to your windowing system.
				if ( viewport->Flags & ImGuiViewportFlags_NoInputs ) {
					return HTTRANSPARENT;
				}
				break;
			}
		}

		return DefWindowProcW( hWnd, msg, wParam, lParam );
	}
#endif

	//=================================================================================================

	static BOOL CALLBACK OSImp_UpdateMonitors_EnumFunc( HMONITOR monitor, HDC, LPRECT, LPARAM )
	{
		MONITORINFO info;
		info.cbSize = sizeof( MONITORINFO );
		if ( !GetMonitorInfoW( monitor, &info ) ) {
			return TRUE;
		}
		ImGuiPlatformMonitor imgui_monitor;
		imgui_monitor.MainPos = ImVec2( (float)info.rcMonitor.left, (float)info.rcMonitor.top );
		imgui_monitor.MainSize = ImVec2( (float)( info.rcMonitor.right - info.rcMonitor.left ), (float)( info.rcMonitor.bottom - info.rcMonitor.top ) );
		imgui_monitor.WorkPos = ImVec2( (float)info.rcWork.left, (float)info.rcWork.top );
		imgui_monitor.WorkSize = ImVec2( (float)( info.rcWork.right - info.rcWork.left ), (float)( info.rcWork.bottom - info.rcWork.top ) );
		imgui_monitor.DpiScale = OSImp_GetDpiScaleForMonitor( monitor );
		ImGuiPlatformIO& io = ImGui::GetPlatformIO();
		if ( info.dwFlags & MONITORINFOF_PRIMARY ) {
			io.Monitors.push_front( imgui_monitor );
		} else {
			io.Monitors.push_back( imgui_monitor );
		}
		return TRUE;
	}

	static void OSImp_UpdateMonitors()
	{
		ImGui::GetPlatformIO().Monitors.resize( 0 );
		EnumDisplayMonitors( nullptr, nullptr, OSImp_UpdateMonitors_EnumFunc, 0 );
		pv.wantMonitorUpdate = false;
	}

	static void OSImp_InitPlatformInterface()
	{
		HINSTANCE hInstance = GetModuleHandleW( nullptr );

		const WNDCLASSEXW wcex
		{
			sizeof( wcex ),			// Structure size
			CS_OWNDC,				// Class style
			ChildWndProc,			// WndProc
			0,						// Extra storage
			0,						// Extra storage
			hInstance,				// Module
			(HICON)LoadImageW( hInstance, MAKEINTRESOURCEW( IDI_ICON1 ), IMAGE_ICON, 0, 0, LR_SHARED ),		// Icon
			(HCURSOR)LoadImageW( NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED ),							// Cursor to use
			(HBRUSH)( COLOR_WINDOW + 1 ),		// Background colour
			nullptr,				// Menu
			QIMGUI_WINDOWCLASS,		// Classname
			nullptr					// Small icon
		};
		RegisterClassExW( &wcex );

		OSImp_UpdateMonitors();

		ImGuiPlatformIO& platIO = ImGui::GetPlatformIO();
		platIO.Platform_CreateWindow = OSImp_CreateWindow;
		platIO.Platform_DestroyWindow = OSImp_DestroyWindow;
		platIO.Platform_ShowWindow = OSImp_ShowWindow;
		platIO.Platform_SetWindowPos = OSImp_SetWindowPos;
		platIO.Platform_GetWindowPos = OSImp_GetWindowPos;
		platIO.Platform_SetWindowSize = OSImp_SetWindowSize;
		platIO.Platform_GetWindowSize = OSImp_GetWindowSize;
		platIO.Platform_SetWindowFocus = OSImp_SetWindowFocus;
		platIO.Platform_GetWindowFocus = OSImp_GetWindowFocus;
		platIO.Platform_GetWindowMinimized = OSImp_GetWindowMinimized;
		platIO.Platform_SetWindowTitle = OSImp_SetWindowTitle;
		platIO.Platform_SetWindowAlpha = OSImp_SetWindowAlpha;
		platIO.Platform_UpdateWindow = OSImp_UpdateWindow;
		platIO.Platform_GetWindowDpiScale = OSImp_GetWindowDpiScale; // FIXME-DPI
		platIO.Platform_OnChangedViewport = OSImp_OnChangedViewport; // FIXME-DPI
#if HAS_WIN32_IME
		platIO.Platform_SetImeInputPos = OSImp_SetImeInputPos;
#endif

		platIO.Platform_RenderWindow = OSImp_RenderWindow;
		platIO.Platform_SwapBuffers = OSImp_SwapBuffers;

		// Renderer_CreateWindow
		// Renderer_DestroyWindow
		// Renderer_SetWindowSize
		// Renderer_RenderWindow
		// Renderer_SwapBuffers

		// Register main window handle (which is owned by the main application, not by us)
		// This is mostly for simplicity and consistency, so that our code (e.g. mouse handling etc.) can use same logic for main and secondary viewports.
		ImGuiViewport* mainViewport = ImGui::GetMainViewport();
		mainViewport->PlatformHandle = pv.mainWindow;
		mainViewport->PlatformHandleRaw = pv.mainWindow;
	}

	static void OSImp_ShutdownPlatformInterface()
	{
		UnregisterClassW( QIMGUI_WINDOWCLASS, GetModuleHandleW( nullptr ) );
	}

	//-------------------------------------------------------------------------------------------------
	// Windows renderer-agnostic implementation for ImGui, based on the example backend
	//-------------------------------------------------------------------------------------------------

	bool OSImp_Init( void *window )
	{
		pv.mainWindow = reinterpret_cast<HWND>( window );
		pv.lastcursor = ImGuiMouseCursor_COUNT;
		pv.wantMonitorUpdate = true;

		ImGuiIO &io = ImGui::GetIO();
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;			// We can honor GetMouseCursor() values (optional)
		io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;			// We can honor io.WantSetMousePos requests (optional, rarely used)
		io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;		// We can create multi-viewports on the Platform side (optional)
		io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;	// We can set io.MouseHoveredViewport correctly (optional, not easy)
		io.BackendPlatformName = "imgui_quake_imp_windows";

		ImGuiViewport *mainViewport = ImGui::GetMainViewport();
		mainViewport->PlatformHandle = window;
		mainViewport->PlatformHandleRaw = window;
		if ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable ) {
			OSImp_InitPlatformInterface();
		}

		// Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array that we will update during the application lifetime.
		io.KeyMap[ImGuiKey_Tab] = VK_TAB;
		io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
		io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
		io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
		io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
		io.KeyMap[ImGuiKey_Home] = VK_HOME;
		io.KeyMap[ImGuiKey_End] = VK_END;
		io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
		io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
		io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
		io.KeyMap[ImGuiKey_Space] = VK_SPACE;
		io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
		io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
		io.KeyMap[ImGuiKey_KeyPadEnter] = VK_RETURN;
		io.KeyMap[ImGuiKey_A] = 'A';
		io.KeyMap[ImGuiKey_C] = 'C';
		io.KeyMap[ImGuiKey_V] = 'V';
		io.KeyMap[ImGuiKey_X] = 'X';
		io.KeyMap[ImGuiKey_Y] = 'Y';
		io.KeyMap[ImGuiKey_Z] = 'Z';

#ifdef USE_STEAM_SKIN

		ImVec4 *colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4( 1.00f, 1.00f, 1.00f, 1.00f );
		colors[ImGuiCol_TextDisabled] = ImVec4( 0.50f, 0.50f, 0.50f, 1.00f );
		colors[ImGuiCol_WindowBg] = ImVec4( 0.29f, 0.34f, 0.26f, 1.00f );
		colors[ImGuiCol_ChildBg] = ImVec4( 0.29f, 0.34f, 0.26f, 1.00f );
		colors[ImGuiCol_PopupBg] = ImVec4( 0.24f, 0.27f, 0.20f, 1.00f );
		colors[ImGuiCol_Border] = ImVec4( 0.54f, 0.57f, 0.51f, 0.50f );
		colors[ImGuiCol_BorderShadow] = ImVec4( 0.14f, 0.16f, 0.11f, 0.52f );
		colors[ImGuiCol_FrameBg] = ImVec4( 0.24f, 0.27f, 0.20f, 1.00f );
		colors[ImGuiCol_FrameBgHovered] = ImVec4( 0.27f, 0.30f, 0.23f, 1.00f );
		colors[ImGuiCol_FrameBgActive] = ImVec4( 0.30f, 0.34f, 0.26f, 1.00f );
		colors[ImGuiCol_TitleBg] = ImVec4( 0.24f, 0.27f, 0.20f, 1.00f );
		colors[ImGuiCol_TitleBgActive] = ImVec4( 0.29f, 0.34f, 0.26f, 1.00f );
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4( 0.00f, 0.00f, 0.00f, 0.51f );
		colors[ImGuiCol_MenuBarBg] = ImVec4( 0.24f, 0.27f, 0.20f, 1.00f );
		colors[ImGuiCol_ScrollbarBg] = ImVec4( 0.35f, 0.42f, 0.31f, 1.00f );
		colors[ImGuiCol_ScrollbarGrab] = ImVec4( 0.28f, 0.32f, 0.24f, 1.00f );
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4( 0.25f, 0.30f, 0.22f, 1.00f );
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4( 0.23f, 0.27f, 0.21f, 1.00f );
		colors[ImGuiCol_CheckMark] = ImVec4( 0.59f, 0.54f, 0.18f, 1.00f );
		colors[ImGuiCol_SliderGrab] = ImVec4( 0.35f, 0.42f, 0.31f, 1.00f );
		colors[ImGuiCol_SliderGrabActive] = ImVec4( 0.54f, 0.57f, 0.51f, 0.50f );
		colors[ImGuiCol_Button] = ImVec4( 0.29f, 0.34f, 0.26f, 0.40f );
		colors[ImGuiCol_ButtonHovered] = ImVec4( 0.35f, 0.42f, 0.31f, 1.00f );
		colors[ImGuiCol_ButtonActive] = ImVec4( 0.54f, 0.57f, 0.51f, 0.50f );
		colors[ImGuiCol_Header] = ImVec4( 0.35f, 0.42f, 0.31f, 1.00f );
		colors[ImGuiCol_HeaderHovered] = ImVec4( 0.35f, 0.42f, 0.31f, 0.6f );
		colors[ImGuiCol_HeaderActive] = ImVec4( 0.54f, 0.57f, 0.51f, 0.50f );
		colors[ImGuiCol_Separator] = ImVec4( 0.14f, 0.16f, 0.11f, 1.00f );
		colors[ImGuiCol_SeparatorHovered] = ImVec4( 0.54f, 0.57f, 0.51f, 1.00f );
		colors[ImGuiCol_SeparatorActive] = ImVec4( 0.59f, 0.54f, 0.18f, 1.00f );
		colors[ImGuiCol_ResizeGrip] = ImVec4( 0.19f, 0.23f, 0.18f, 0.00f ); // grip invis
		colors[ImGuiCol_ResizeGripHovered] = ImVec4( 0.54f, 0.57f, 0.51f, 1.00f );
		colors[ImGuiCol_ResizeGripActive] = ImVec4( 0.59f, 0.54f, 0.18f, 1.00f );
		colors[ImGuiCol_Tab] = ImVec4( 0.35f, 0.42f, 0.31f, 1.00f );
		colors[ImGuiCol_TabHovered] = ImVec4( 0.54f, 0.57f, 0.51f, 0.78f );
		colors[ImGuiCol_TabActive] = ImVec4( 0.59f, 0.54f, 0.18f, 1.00f );
		colors[ImGuiCol_TabUnfocused] = ImVec4( 0.24f, 0.27f, 0.20f, 1.00f );
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4( 0.35f, 0.42f, 0.31f, 1.00f );
		colors[ImGuiCol_PlotLines] = ImVec4( 0.61f, 0.61f, 0.61f, 1.00f );
		colors[ImGuiCol_PlotLinesHovered] = ImVec4( 0.59f, 0.54f, 0.18f, 1.00f );
		colors[ImGuiCol_PlotHistogram] = ImVec4( 1.00f, 0.78f, 0.28f, 1.00f );
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4( 1.00f, 0.60f, 0.00f, 1.00f );
		colors[ImGuiCol_TextSelectedBg] = ImVec4( 0.59f, 0.54f, 0.18f, 1.00f );
		colors[ImGuiCol_DragDropTarget] = ImVec4( 0.73f, 0.67f, 0.24f, 1.00f );
		colors[ImGuiCol_NavHighlight] = ImVec4( 0.59f, 0.54f, 0.18f, 1.00f );
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4( 1.00f, 1.00f, 1.00f, 0.70f );
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4( 0.80f, 0.80f, 0.80f, 0.20f );
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4( 0.80f, 0.80f, 0.80f, 0.35f );

		ImGuiStyle &style = ImGui::GetStyle();
		style.FrameBorderSize = 1.0f;
		style.WindowRounding = 0.0f;
		style.ChildRounding = 0.0f;
		style.FrameRounding = 0.0f;
		style.PopupRounding = 0.0f;
		style.ScrollbarRounding = 0.0f;
		style.GrabRounding = 0.0f;
		style.TabRounding = 0.0f;

#endif

		return true;
	}

	void OSImp_Shutdown()
	{
		if ( !pv.mainWindow ) {
			// never initialised
			return;
		}

		OSImp_ShutdownPlatformInterface();
	}

	void OSImp_SetWantMonitorUpdate()
	{
		if ( ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable ) {
			// we only need this if multiple viewports are enabled
			pv.wantMonitorUpdate = true;
		}
	}

	bool OSImp_UpdateMouseCursor()
	{
		ImGuiIO &io = ImGui::GetIO();
		if ( io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange ) {
			return false;
		}

		ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
		if ( imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor )
		{
			// Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
			while ( ShowCursor( FALSE ) >= 0 )
				;
		}
		else
		{
			// Show OS mouse cursor
			LPCWSTR win32_cursor = IDC_ARROW;
			switch ( imgui_cursor )
			{
			//case ImGuiMouseCursor_Arrow:		win32_cursor = IDC_ARROW; break;
			case ImGuiMouseCursor_TextInput:	win32_cursor = IDC_IBEAM; break;
			case ImGuiMouseCursor_ResizeAll:	win32_cursor = IDC_SIZEALL; break;
			case ImGuiMouseCursor_ResizeEW:		win32_cursor = IDC_SIZEWE; break;
			case ImGuiMouseCursor_ResizeNS:		win32_cursor = IDC_SIZENS; break;
			case ImGuiMouseCursor_ResizeNESW:	win32_cursor = IDC_SIZENESW; break;
			case ImGuiMouseCursor_ResizeNWSE:	win32_cursor = IDC_SIZENWSE; break;
			case ImGuiMouseCursor_Hand:			win32_cursor = IDC_HAND; break;
			case ImGuiMouseCursor_NotAllowed:	win32_cursor = IDC_NO; break;
			}
			SetCursor( LoadCursorW( nullptr, win32_cursor ) ); // This seems slow, why not cache the cursor handles?
		}

		return true;
	}

	static void OSImp_UpdateMousePosition()
	{
		ImGuiIO &io = ImGui::GetIO();

		// Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
		if ( io.WantSetMousePos )
		{
			POINT pos{ (LONG)io.MousePos.x, (LONG)io.MousePos.y };
			if ( !( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable ) ) {
				ClientToScreen( pv.mainWindow, &pos );
			}
			SetCursorPos( pos.x, pos.y );
		}

		io.MousePos = ImVec2( -FLT_MAX, -FLT_MAX );
		io.MouseHoveredViewport = 0;

		// Set mouse position
		POINT mouse_screen_pos;
		if ( !GetCursorPos( &mouse_screen_pos ) ) {
			return;
		}
		if ( HWND focused_hwnd = GetForegroundWindow() )
		{
			if ( IsChild( focused_hwnd, pv.mainWindow ) ) {
				focused_hwnd = pv.mainWindow;
			}
			if ( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
			{
				// Multi-viewport mode: mouse position in OS absolute coordinates (io.MousePos is (0,0) when the mouse is on the upper-left of the primary monitor)
				// This is the position you can get with GetCursorPos(). In theory adding viewport->Pos is also the reverse operation of doing ScreenToClient().
				if ( ImGui::FindViewportByPlatformHandle( (void*)focused_hwnd ) != nullptr ) {
					io.MousePos = ImVec2( (float)mouse_screen_pos.x, (float)mouse_screen_pos.y );
				}
			}
			else
			{
				// Single viewport mode: mouse position in client window coordinates (io.MousePos is (0,0) when the mouse is on the upper-left corner of the app window.)
				// This is the position you can get with GetCursorPos() + ScreenToClient() or from WM_MOUSEMOVE.
				if ( focused_hwnd == pv.mainWindow )
				{
					POINT mouse_client_pos = mouse_screen_pos;
					ScreenToClient( focused_hwnd, &mouse_client_pos );
					io.MousePos = ImVec2( (float)mouse_client_pos.x, (float)mouse_client_pos.y );
				}
			}
		}

		// (Optional) When using multiple viewports: set io.MouseHoveredViewport to the viewport the OS mouse cursor is hovering.
		// Important: this information is not easy to provide and many high-level windowing library won't be able to provide it correctly, because
		// - This is _ignoring_ viewports with the ImGuiViewportFlags_NoInputs flag (pass-through windows).
		// - This is _regardless_ of whether another viewport is focused or being dragged from.
		// If ImGuiBackendFlags_HasMouseHoveredViewport is not set by the backend, imgui will ignore this field and infer the information by relying on the
		// rectangles and last focused time of every viewports it knows about. It will be unaware of foreign windows that may be sitting between or over your windows.
		if ( HWND hovered_hwnd = WindowFromPoint( mouse_screen_pos ) )
		{
			if ( ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle( (void*)hovered_hwnd ) )
			{
				// FIXME: We still get our NoInputs window with WM_NCHITTEST/HTTRANSPARENT code when decorated?
				if ( ( viewport->Flags & ImGuiViewportFlags_NoInputs ) == 0 ) {
					io.MouseHoveredViewport = viewport->ID;
				}
			}
		}
	}

	void OSImp_NewFrame()
	{
		ImGuiIO &io = ImGui::GetIO();
		assert( io.Fonts->IsBuilt() ); // Make sure the renderer implementation's NewFrame was called before this one

		// Setup display size (every frame to accommodate for window resizing)
		RECT rect;
		GetClientRect( pv.mainWindow, &rect );
		io.DisplaySize = ImVec2( (float)( rect.right - rect.left ), (float)( rect.bottom - rect.top ) );
		if ( pv.wantMonitorUpdate ) {
			OSImp_UpdateMonitors();
		}

		// Setup time step
		double newTime = Time_FloatSeconds();
		io.DeltaTime = static_cast<float>( newTime - pv.oldTime );
		pv.oldTime = newTime;

		// Read keyboard modifiers inputs
		io.KeyCtrl = ( GetKeyState( VK_CONTROL ) & 0x8000 ) != 0;
		io.KeyShift = ( GetKeyState( VK_SHIFT ) & 0x8000 ) != 0;
		io.KeyAlt = ( GetKeyState( VK_MENU ) & 0x8000 ) != 0;
		io.KeySuper = false;
		// io.KeysDown[], io.MousePos, io.MouseDown[], io.MouseWheel: filled by the WndProc handler below.

		// Update OS mouse position
		OSImp_UpdateMousePosition();

		// Update OS mouse cursor with the cursor requested by imgui
		ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
		if ( pv.lastcursor != mouse_cursor )
		{
			pv.lastcursor = mouse_cursor;
			OSImp_UpdateMouseCursor();
		}
	}

}
