//=================================================================================================
// Think Tank 2
// Copyright (c) Slartibarty. All Rights Reserved.
//=================================================================================================

#include "../../core/core.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "imgui.h"

//-------------------------------------------------------------------------------------------------
// Windows renderer-agnostic implementation for ImGui, based on the example backend
//-------------------------------------------------------------------------------------------------

namespace qImGui
{
	struct OurVars
	{
		HWND hWnd;
		ImGuiMouseCursor lastcursor;

		double oldTime;
	};

	static OurVars pv;

	bool OSImp_Init( void *window )
	{
		pv.hWnd = reinterpret_cast<HWND>( window );
		pv.lastcursor = ImGuiMouseCursor_COUNT;

		ImGuiIO &io = ImGui::GetIO();
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;			// We can honor GetMouseCursor() values (optional)
		io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;			// We can honor io.WantSetMousePos requests (optional, rarely used)
		io.BackendPlatformName = "imgui_quake_imp_windows";
		io.ImeWindowHandle = pv.hWnd;

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

		return true;
	}

	void OSImp_Shutdown()
	{

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
			SetCursor( nullptr );
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
			if ( ClientToScreen( pv.hWnd, &pos ) ) {
				SetCursorPos( pos.x, pos.y );
			}
		}

		// Set mouse position
		io.MousePos = ImVec2( -FLT_MAX, -FLT_MAX );
		POINT pos;
		if ( HWND active_window = GetForegroundWindow() ) {
			if ( active_window == pv.hWnd || IsChild( active_window, pv.hWnd ) ) {
				if ( GetCursorPos( &pos ) && ScreenToClient( pv.hWnd, &pos ) ) {
					io.MousePos = ImVec2( (float)pos.x, (float)pos.y );
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
		GetClientRect( pv.hWnd, &rect );
		io.DisplaySize = ImVec2( (float)( rect.right - rect.left ), (float)( rect.bottom - rect.top ) );

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
