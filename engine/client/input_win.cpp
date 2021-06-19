//=================================================================================================
// Windows mouse code
//=================================================================================================

#include "cl_local.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include "winquake.h"

#include "imgui.h"
#include "q_imgui_imp.h"

HWND cl_hwnd;

extern unsigned sys_frame_time;

// TODO: this shouldn't really be in a namespace
namespace input
{
	static bool in_appactive;
	static bool mouseactive;		// false when not in focus
	static RECT window_rect;
	static LONG last_xpos, last_ypos;

	static const uint8 scantokey[128]
	{
		//  0           1       2       3       4       5       6       7 
		//  8           9       A       B       C       D       E       F 
			0  ,    27,     '1',    '2',    '3',    '4',    '5',    '6',
			'7',    '8',    '9',    '0',    '-',    '=',    K_BACKSPACE, 9, // 0 
			'q',    'w',    'e',    'r',    't',    'y',    'u',    'i',
			'o',    'p',    '[',    ']',    13 ,    K_CTRL,'a',  's',      // 1 
			'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';',
			'\'' ,    '`',    K_SHIFT,'\\',  'z',    'x',    'c',    'v',      // 2 
			'b',    'n',    'm',    ',',    '.',    '/',    K_SHIFT,'*',
			K_ALT,' ',   0  ,    K_F1, K_F2, K_F3, K_F4, K_F5,   // 3 
			K_F6, K_F7, K_F8, K_F9, K_F10,  K_PAUSE,    0  , K_HOME,
			K_UPARROW,K_PGUP,K_KP_MINUS,K_LEFTARROW,K_KP_5,K_RIGHTARROW, K_KP_PLUS,K_END, //4 
			K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             0,              K_F11,
			K_F12,0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 5
			0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,
			0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 6 
			0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,
			0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0         // 7 
	};

	//-------------------------------------------------------------------------------------------------
	// Map from windows to quake keynums
	// TODO: This doesn't properly cover all keys
	// It also chokes on localised keys (My UK keyboard prints US characters)
	// Perhaps using the key scancode isn't the best idea
	// Really need to research how Microsoft considers the old keyinputs "legacy", do they really mean it?
	// Could also just go back to using the old regular window messages instead of raw input
	//-------------------------------------------------------------------------------------------------
	static int MapKey( int scancode )
	{
		if ( scancode > 127 ) {
			return 0;
		}

		return scantokey[scancode];
#if 0
		if ( isE0 )
		{
			switch ( result )
			{
			case K_HOME:
				return K_KP_HOME;
			case K_UPARROW:
				return K_KP_UPARROW;
			case K_PGUP:
				return K_KP_PGUP;
			case K_LEFTARROW:
				return K_KP_LEFTARROW;
			case K_RIGHTARROW:
				return K_KP_RIGHTARROW;
			case K_END:
				return K_KP_END;
			case K_DOWNARROW:
				return K_KP_DOWNARROW;
			case K_PGDN:
				return K_KP_PGDN;
			case K_INS:
				return K_KP_INS;
			case K_DEL:
				return K_KP_DEL;
			default:
				return result;
			}
		}
		else
		{
			switch ( result )
			{
			case 0x0D:
				return K_KP_ENTER;
			case 0x2F:
				return K_KP_SLASH;
			case 0xAF:
				return K_KP_PLUS;
			default:
				return result;
			}
		}
#endif
	}

	//-------------------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------------------
	static void AppActivate( bool fActive, bool minimized )
	{
		g_minimized = minimized;

		Key_ClearStates();

		// we don't want to act like we're active if we're minimized
		if ( fActive && !minimized ) {
			g_activeApp = true;
		} else {
			g_activeApp = false;
		}

		// minimize/restore mouse-capture on demand
		input::Activate( g_activeApp );
		CDAudio_Activate( g_activeApp );
		S_Activate( g_activeApp );
	}

	static void HandleKeyboardInput_ImGui( RAWKEYBOARD &raw )
	{
		assert( raw.VKey < 256 );

		ImGuiIO &io = ImGui::GetIO();

		io.KeysDown[raw.VKey] = !( raw.Flags & RI_KEY_BREAK );
	}

	static void HandleKeyboardInput( RAWKEYBOARD &raw )
	{
		Key_Event( MapKey( raw.MakeCode ), !( raw.Flags & RI_KEY_BREAK ), sys_frame_time );
	}

	static void HandleMouseInput_ImGui( RAWMOUSE &raw )
	{
		ImGuiIO &io = ImGui::GetIO();

		// Always process BUTTONS!
		switch ( raw.usButtonFlags )
		{
		case RI_MOUSE_LEFT_BUTTON_DOWN:
			io.MouseDown[0] = true;
			break;
		case RI_MOUSE_LEFT_BUTTON_UP:
			io.MouseDown[0] = false;
			break;
		case RI_MOUSE_RIGHT_BUTTON_DOWN:
			io.MouseDown[1] = true;
			break;
		case RI_MOUSE_RIGHT_BUTTON_UP:
			io.MouseDown[1] = false;
			break;
		case RI_MOUSE_MIDDLE_BUTTON_DOWN:
			io.MouseDown[2] = true;
			break;
		case RI_MOUSE_MIDDLE_BUTTON_UP:
			io.MouseDown[2] = false;
			break;
		case RI_MOUSE_WHEEL:
			io.MouseWheel += (float)(short)raw.usButtonData / (float)WHEEL_DELTA;
			break;
		case RI_MOUSE_HWHEEL:
			io.MouseWheelH += (float)(short)raw.usButtonData / (float)WHEEL_DELTA;
			break;
		}
	}
	
	static void HandleMouseInput( RAWMOUSE &raw )
	{
		// Always process BUTTONS!
		switch ( raw.usButtonFlags )
		{
		case RI_MOUSE_LEFT_BUTTON_DOWN:
			Key_Event( K_MOUSE1, true, sys_frame_time );
			break;
		case RI_MOUSE_LEFT_BUTTON_UP:
			Key_Event( K_MOUSE1, false, sys_frame_time );
			break;
		case RI_MOUSE_RIGHT_BUTTON_DOWN:
			Key_Event( K_MOUSE2, true, sys_frame_time );
			break;
		case RI_MOUSE_RIGHT_BUTTON_UP:
			Key_Event( K_MOUSE2, false, sys_frame_time );
			break;
		case RI_MOUSE_MIDDLE_BUTTON_DOWN:
			Key_Event( K_MOUSE3, true, sys_frame_time );
			break;
		case RI_MOUSE_MIDDLE_BUTTON_UP:
			Key_Event( K_MOUSE3, false, sys_frame_time );
			break;
		case RI_MOUSE_WHEEL:
			Key_Event( ( (short)raw.usButtonData > 0 ) ? K_MWHEELUP : K_MWHEELDOWN, true, sys_frame_time );
			Key_Event( ( (short)raw.usButtonData > 0 ) ? K_MWHEELUP : K_MWHEELDOWN, false, sys_frame_time );
			break;
		}

		if ( mouseactive == false ) {
			return;
		}

		float xpos = static_cast<float>( raw.lLastX + last_xpos );
		float ypos = static_cast<float>( raw.lLastY + last_ypos );
		last_xpos = raw.lLastX;
		last_ypos = raw.lLastY;

		xpos *= m_yaw->GetFloat() * sensitivity->GetFloat();
		ypos *= m_pitch->GetFloat() * sensitivity->GetFloat();

#if 0
		// force the mouse to the center, so there's room to move
		if ( mx || my )
			SetCursorPos( window_center_x, window_center_y );
#endif

		cl.viewangles[YAW] -= xpos;
		cl.viewangles[PITCH] += ypos;
	}

	//-------------------------------------------------------------------------------------------------
	// Called when the window gains focus
	//-------------------------------------------------------------------------------------------------
	static void ActivateMouse()
	{
		if ( mouseactive == true ) {
			return;
		}
		mouseactive = true;

		SetCapture( cl_hwnd );
		ClipCursor( &window_rect );
		while ( ShowCursor( FALSE ) >= 0 )
			;
	}

	//-------------------------------------------------------------------------------------------------
	// Called when the window loses focus
	//-------------------------------------------------------------------------------------------------
	static void DeactivateMouse()
	{
		if ( mouseactive == false ) {
			return;
		}
		mouseactive = false;

		ClipCursor( nullptr );
		ReleaseCapture();
		while ( ShowCursor( TRUE ) < 0 )
			;
	}

	void Init()
	{
		// Register for raw input

		Com_Print( "Initialising Windows InputSystem...\n" );

		RAWINPUTDEVICE rid[2];

		// Mouse
		rid[0].usUsagePage = 0x01;
		rid[0].usUsage = 0x02;
		rid[0].dwFlags = 0;
		rid[0].hwndTarget = cl_hwnd;

		// Keyboard
		rid[1].usUsagePage = 0x01;
		rid[1].usUsage = 0x06;
		rid[1].dwFlags = 0;
		rid[1].hwndTarget = cl_hwnd;

		BOOL result = RegisterRawInputDevices( rid, 2, sizeof( *rid ) );
		if ( result == FALSE ) {
			Com_Print( "Failed to register for raw input!\n" );
		}
	}

	void Shutdown()
	{
	}

	void Commands()
	{
	}

	//-------------------------------------------------------------------------------------------------
	// Called every frame, even if not generating commands
	//-------------------------------------------------------------------------------------------------
	void Frame()
	{
		if ( !in_appactive )
		{
			DeactivateMouse();
			return;
		}

		if ( !cl.refresh_prepped
			|| cls.key_dest == key_console
			|| cls.key_dest == key_menu )
		{
			// Deactivate mouse if in a menu
			DeactivateMouse();
			return;
		}

		ActivateMouse();
	}

	void Activate( bool active )
	{
		in_appactive = active;
	}

	void SendQuitMessage()
	{
		static bool sent = false;

		// don't send twice, I have a feeling it's totally fine to call this twice anyway
		if ( !sent ) {
			PostQuitMessage( 0 );
			sent = true;
		}
	}

}

LRESULT CALLBACK MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	using namespace input;

	switch ( uMsg )
	{
	case WM_INPUT:
	{
		UINT dwSize = sizeof( RAWINPUT );

		RAWINPUT raw;

		// Now read into the buffer
		GetRawInputData( (HRAWINPUT)lParam, RID_INPUT, &raw, &dwSize, sizeof( RAWINPUTHEADER ) );

		ImGuiIO &io = ImGui::GetIO();

		// if imgui is focused, we should not send keyboard input to the game
		switch ( raw.header.dwType )
		{
		case RIM_TYPEKEYBOARD:
			HandleKeyboardInput_ImGui( raw.data.keyboard );
			if ( !io.WantCaptureKeyboard ) {
				HandleKeyboardInput( raw.data.keyboard );
			}
			break;
		case RIM_TYPEMOUSE:
			HandleMouseInput_ImGui( raw.data.mouse );
			if ( !io.WantCaptureMouse ) {
				HandleMouseInput( raw.data.mouse );
			}
			break;
		}
	}
	return 0;

	case WM_CHAR:
		// You can also use ToAscii()+GetKeyboardState() to retrieve characters.
		if ( wParam > 0 && wParam < 0x10000 ) {
			ImGuiIO &io = ImGui::GetIO();
			io.AddInputCharacterUTF16( (ImWchar16)wParam );
		}
		break;

	// for ImGui
	case WM_SETCURSOR:
		if ( LOWORD( lParam ) == HTCLIENT && qImGui::OSImp_UpdateMouseCursor() ) {
			return 1;
		}
		break;

	case WM_ACTIVATE:
	{
		WORD fActive = LOWORD( wParam );
		WORD fMinimized = HIWORD( wParam );

		AppActivate( fActive != WA_INACTIVE, fMinimized );

		// there used to be a check here for reflib_active
		// but since the window is handled by the ref
		// this was always true!
		R_AppActivate( fActive != WA_INACTIVE );
	}
	return 0;

	case WM_SIZE: [[fallthrough]];
	case WM_MOVE:
		GetWindowRect( hWnd, &window_rect );
		return 0;

	case WM_CREATE:
		cl_hwnd = hWnd;
		return 0;

		// DefWindowProc calls this after WM_CLOSE
	case WM_DESTROY:
		// let sound and input know about this?
		cl_hwnd = nullptr;
		PostQuitMessage( 0 );
		return 0;
	}

	return DefWindowProcW( hWnd, uMsg, wParam, lParam );
}
