//=================================================================================================
// Think Tank 2
// Copyright (c) Slartibarty. All Rights Reserved.
//=================================================================================================

#include "../../core/core.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "imgui.h"

#define USE_STEAM_SKIN

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
