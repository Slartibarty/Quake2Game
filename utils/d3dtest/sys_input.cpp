/*
===================================================================================================

	Terrible Input

	This is a client module!

	Notes on coordinates:

	A completely zeroed camera is looking down positive X

	Origin:
		Positive X is moving forwards
		Negative X is moving backwards
		Positive Y is moving left
		Negative Y is moving right
		Positive Z is moving up
		Negative Z is moving down

	Angles:
		

===================================================================================================
*/

#include "cl_local.h"

#include "../../core/sys_includes.h"

static struct inputDefs_t
{
	RECT windowRect;
	bool mouseActive;
} input;

void SysInput_Init()
{
	// Register for raw input

	Com_Print( "Initialising Windows InputSystem...\n" );

	RAWINPUTDEVICE rid[2];

	HWND window = (HWND)Sys_GetMainWindow();

	// Mouse
	rid[0].usUsagePage = 0x01;
	rid[0].usUsage = 0x02;
	rid[0].dwFlags = 0;
	rid[0].hwndTarget = window;

	// Keyboard
	rid[1].usUsagePage = 0x01;
	rid[1].usUsage = 0x06;
	rid[1].dwFlags = 0;
	rid[1].hwndTarget = window;

	BOOL result = RegisterRawInputDevices( rid, 2, sizeof( *rid ) );
	if ( result == FALSE ) {
		Com_FatalError( "Failed to register for raw input!\n" );
	}
}

static void HandleKeyboardInput( RAWKEYBOARD &raw )
{
	// Translate the system key to a generic key
	bool down = !( raw.Flags & RI_KEY_BREAK );

	Input_KeyEvent( raw.VKey, down );
}

static void HandleMouseInput( RAWMOUSE &raw, HWND wnd )
{
	// Always process BUTTONS!
	switch ( raw.usButtonFlags )
	{
	case RI_MOUSE_LEFT_BUTTON_DOWN:
		//Key_Event( K_MOUSE1, true, sys_frameTime );
		break;
	case RI_MOUSE_LEFT_BUTTON_UP:
		//Key_Event( K_MOUSE1, false, sys_frameTime );
		break;
	case RI_MOUSE_RIGHT_BUTTON_DOWN:
		//Key_Event( K_MOUSE2, true, sys_frameTime );
		input.mouseActive = true;
		SetCapture( wnd );
		ClipCursor( &input.windowRect );
		while ( ShowCursor( FALSE ) >= 0 )
			;
		break;
	case RI_MOUSE_RIGHT_BUTTON_UP:
		//Key_Event( K_MOUSE2, false, sys_frameTime );
		input.mouseActive = false;
		ClipCursor( nullptr );
		ReleaseCapture();
		while ( ShowCursor( TRUE ) < 0 )
			;
		break;
	case RI_MOUSE_MIDDLE_BUTTON_DOWN:
		//Key_Event( K_MOUSE3, true, sys_frameTime );
		break;
	case RI_MOUSE_MIDDLE_BUTTON_UP:
		//Key_Event( K_MOUSE3, false, sys_frameTime );
		break;
	case RI_MOUSE_WHEEL:
		//Key_Event( ( (short)raw.usButtonData > 0 ) ? K_MWHEELUP : K_MWHEELDOWN, true, sys_frameTime );
		//Key_Event( ( (short)raw.usButtonData > 0 ) ? K_MWHEELUP : K_MWHEELDOWN, false, sys_frameTime );
		break;
	}

	if ( input.mouseActive == false ) {
		return;
	}

	Input_MouseEvent( raw.lLastX, raw.lLastY );
}

LRESULT CALLBACK MainWindowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
	case WM_ERASEBKGND:
		return 0;
	case WM_CLOSE:
		PostQuitMessage( EXIT_SUCCESS );
		return 0;
	case WM_INPUT:
	{
		UINT dwSize = sizeof( RAWINPUT );

		RAWINPUT raw;

		// Now read into the buffer
		GetRawInputData( (HRAWINPUT)lParam, RID_INPUT, &raw, &dwSize, sizeof( RAWINPUTHEADER ) );

		// if imgui is focused, we should not send keyboard input to the game
		switch ( raw.header.dwType )
		{
		case RIM_TYPEKEYBOARD:
			HandleKeyboardInput( raw.data.keyboard );
			break;
		case RIM_TYPEMOUSE:
			HandleMouseInput( raw.data.mouse, wnd );
			break;
		}
	}
	return 0;
	case WM_SIZE: [[fallthrough]];
	case WM_MOVE:
		GetClientRect( wnd, &input.windowRect );
		MapWindowPoints( wnd, nullptr, (LPPOINT)&input.windowRect, 2 );
		return 0;
	}

	return DefWindowProcW( wnd, msg, wParam, lParam );
}
