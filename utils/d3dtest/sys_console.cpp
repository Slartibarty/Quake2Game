/*
===================================================================================================

	Horrid external console

	This is a common module!

===================================================================================================
*/

#include "engine.h"

#include <string>

#include "../../core/sys_includes.h"
#include <windowsx.h>
#include <CommCtrl.h>
#include "../../resources/resource.h"

#define MAX_COMMAND_LENGTH	256
#define MAX_BUFFER_LENGTH	32768

#define MAIN_CLASSNAME	L"SysCon"
#define MAIN_NAME		L"Engine 3 SysConsole"
#define MAIN_EXSTYLE	0
#define MAIN_STYLE		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX
#define MAIN_WIDTH		500
#define MAIN_HEIGHT		640

#define BUFFER_ID		100
#define INPUT_ID		101
#define BUTTON_ID		102

struct sysConsole_t
{
	HWND mainWindow;
	HWND scrollBufferWindow;
	HWND inputLineWindow;
	HWND submitButton;

	HFONT niceFont; // Usually Segoe UI
	HFONT monoFont; // Consolas

	WNDPROC editControlWndProc;

	std::wstring earlyText; // Storage for text printed before the window is open
} sysCon;

static LRESULT CALLBACK ConsoleWindowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	return DefWindowProcW( wnd, msg, wParam, lParam );
}

static LRESULT CALLBACK InputLineWndProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	wchar_t windowText[MAX_COMMAND_LENGTH];
	char commandText[MAX_COMMAND_LENGTH * 2];

	switch ( msg )
	{
	case WM_CHAR:
		if ( wParam == 13 )
		{
			int windowTextLength = GetWindowTextW( wnd, windowText, countof( windowText ) );
			Sys_UTF16toUTF8( windowText, windowTextLength + 1, commandText, sizeof( commandText ) );
			
			// Clear
			SetWindowTextW( wnd, L"" );

			Cbuf_AddText( commandText );
			Cbuf_AddText( "\n" );

			Com_Printf( "] %s\n", commandText );

			return 0;
		}
	}

	return CallWindowProcW( sysCon.editControlWndProc, wnd, msg, wParam, lParam );
}

static void SetupFonts()
{
	NONCLIENTMETRICSW metrics;
	metrics.cbSize = sizeof( NONCLIENTMETRICS );
	SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, sizeof( NONCLIENTMETRICS ), &metrics, 0 );

	sysCon.niceFont = CreateFontIndirectW( &metrics.lfMessageFont );

	sysCon.monoFont = CreateFontW( metrics.lfMessageFont.lfHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, L"Consolas" );
}

static void DestroyFonts()
{
	DeleteObject( sysCon.monoFont );
	DeleteObject( sysCon.niceFont );
}

void SysConsole_Init()
{
	HINSTANCE instance = GetModuleHandleW( nullptr );

	// Create the window

	HICON icon = (HICON)LoadImageW( instance, MAKEINTRESOURCEW( IDI_ICON1 ), IMAGE_ICON, 0, 0, LR_SHARED );
	HCURSOR cursor = (HCURSOR)LoadImageW( nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED );

	const WNDCLASSEXW wc{
		.cbSize = sizeof( wc ),
		.style = 0,
		.lpfnWndProc = ConsoleWindowProc,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = instance,
		.hIcon = icon,
		.hCursor = cursor,
		.hbrBackground = (HBRUSH)( COLOR_MENU + 1 ),
		.lpszMenuName = nullptr,
		.lpszClassName = MAIN_CLASSNAME,
		.hIconSm = nullptr
	};
	ATOM windowClass = RegisterClassExW( &wc );

	RECT r{ 0, 0, MAIN_WIDTH, MAIN_HEIGHT };
	AdjustWindowRectEx( &r, MAIN_STYLE, FALSE, MAIN_EXSTYLE );

	sysCon.mainWindow = CreateWindowExW( MAIN_EXSTYLE, (LPCWSTR)(uintptr_t)windowClass, MAIN_NAME, MAIN_STYLE,
		CW_USEDEFAULT, 0, r.right - r.left, r.bottom - r.top, nullptr, nullptr, instance, nullptr );

	SetupFonts();

	// Create the text buffer

	constexpr int margin = 7;

	constexpr int inputHeight = 23; // 24
	constexpr int buttonWidth = 75;
	constexpr int buttonHeight = 23;
	constexpr int inputYAdd = 0; // amount to add to input y

	int x = margin;
	int y = margin;
	int w = MAIN_WIDTH - ( margin * 2 );
	int h = ( MAIN_HEIGHT - ( margin * 2 ) ) - buttonHeight - margin;

	sysCon.scrollBufferWindow = CreateWindowExW( WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE, WC_EDITW, nullptr,
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
		ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
		x, y, w, h, sysCon.mainWindow, (HMENU)BUFFER_ID, instance, nullptr );

	SendMessageW( sysCon.scrollBufferWindow, EM_SETEXTENDEDSTYLE, 0, EC_ENDOFLINE_LF );
	SendMessageW( sysCon.scrollBufferWindow, EM_SETLIMITTEXT, MAX_BUFFER_LENGTH, 0 );
	SendMessageW( sysCon.scrollBufferWindow, WM_SETFONT, (WPARAM)sysCon.monoFont, MAKELPARAM( FALSE, 0 ) );

	SetWindowTextW( sysCon.scrollBufferWindow, sysCon.earlyText.c_str() );
	sysCon.earlyText.clear();
	sysCon.earlyText.shrink_to_fit();

	// Create the input line

	x = margin;
	y = h + ( margin * 2 ) + inputYAdd;

	w = w - buttonWidth - margin;
	h = inputHeight;

	sysCon.inputLineWindow = CreateWindowExW( WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE, WC_EDITW, nullptr,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP |
		ES_LEFT | ES_AUTOHSCROLL,
		x, y, w, h, sysCon.mainWindow, (HMENU)INPUT_ID, instance, nullptr );

	SendMessageW( sysCon.inputLineWindow, EM_SETEXTENDEDSTYLE, 0, EC_ENDOFLINE_LF );
	SendMessageW( sysCon.inputLineWindow, EM_SETLIMITTEXT, MAX_COMMAND_LENGTH, 0 );
	SendMessageW( sysCon.inputLineWindow, WM_SETFONT, (WPARAM)sysCon.monoFont, MAKELPARAM( FALSE, 0 ) );

	sysCon.editControlWndProc = (WNDPROC)SetWindowLongPtrW( sysCon.inputLineWindow, GWLP_WNDPROC, (LONG_PTR)InputLineWndProc );

	// Create the submit button

	x = x + w + margin;
	y = y - inputYAdd;
	w = buttonWidth;
	h = buttonHeight;
	
	sysCon.submitButton = CreateWindowExW( WS_EX_NOPARENTNOTIFY, WC_BUTTONW, L"Submit",
		WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP |
		BS_PUSHBUTTON | BS_TEXT,
		x, y, w, h, sysCon.mainWindow, (HMENU)BUTTON_ID, instance, nullptr );

	SendMessageW( sysCon.submitButton, WM_SETFONT, (WPARAM)sysCon.niceFont, MAKELPARAM( TRUE, 0 ) );

	ShowWindow( sysCon.mainWindow, SW_SHOWDEFAULT );
	UpdateWindow( sysCon.mainWindow );
}

void SysConsole_Shutdown()
{
	HINSTANCE instance = GetModuleHandleW( nullptr );

	DestroyWindow( sysCon.submitButton );
	DestroyWindow( sysCon.inputLineWindow );
	DestroyWindow( sysCon.scrollBufferWindow );

	DestroyFonts();

	DestroyWindow( sysCon.mainWindow );
	UnregisterClassW( MAIN_CLASSNAME, GetModuleHandleW( nullptr ) );
}

void SysConsole_Print( const char *msg )
{
	wchar_t wideMsg[MAX_PRINT_MSG];
	Sys_UTF8ToUTF16( msg, Q_strlen( msg ) + 1, wideMsg, countof( wideMsg ) );

	if ( sysCon.scrollBufferWindow )
	{
		int length = GetWindowTextLengthW( sysCon.scrollBufferWindow );
		if ( !( length >= MAX_BUFFER_LENGTH - 1 ) )
		{
			SendMessageW( sysCon.scrollBufferWindow, EM_SETSEL, length, length );
			SendMessageW( sysCon.scrollBufferWindow, EM_REPLACESEL, 0, (LPARAM)wideMsg );
		}
		// welp?
	}
	else
	{
		sysCon.earlyText.append( wideMsg );
	}
}
