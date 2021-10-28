/*
===================================================================================================

	OpenGL Dummy Window

===================================================================================================
*/

#ifdef _WIN32

#include <cassert>

#include "../../core/sys_includes.h"

#include "gl_dummywindow.h"

#define DUMMY_WINDOW_CLASSNAME L"JimmyJams"

void WGLimp_CreateDummyWindow( DummyVars &dvars )
{
	BOOL result;

	HINSTANCE hInstance = GetModuleHandleW( nullptr );

	// Create our dummy window class
	const WNDCLASSEXW wcex
	{
		sizeof( wcex ),				// Size
		CS_OWNDC,					// Style
		DefWindowProcW,				// Wndproc
		0,							// ClsExtra
		0,							// WndExtra
		hInstance,					// hInstance
		NULL,						// Icon
		NULL,						// Cursor
		(HBRUSH)( COLOR_WINDOW + 1 ),	// Background colour
		NULL,						// Menu
		DUMMY_WINDOW_CLASSNAME,		// Class name
		NULL						// Small icon
	};
	RegisterClassExW( &wcex );

	dvars.hWnd = CreateWindowExW(
		0,
		DUMMY_WINDOW_CLASSNAME,
		L"",
		0,
		0, 0, 800, 600,
		NULL,
		NULL,
		hInstance,
		NULL
	);
	assert( dvars.hWnd );

	// Create our dummy PFD
	const PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof( pfd ),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    // Flags
		PFD_TYPE_RGBA,		// The kind of framebuffer. RGBA or palette
		32,					// Colordepth of the framebuffer
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,					// Number of bits for the depthbuffer
		8,					// Number of bits for the stencilbuffer
		0,					// Number of Aux buffers in the framebuffer
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	// Get this window's DC, we don't need to release it afterwards due to
	// CS_OWNDC
	HDC hDC = GetDC( (HWND)dvars.hWnd );
	assert( hDC );

	int pixelformat = ChoosePixelFormat( hDC, &pfd );
	assert( pixelformat != 0 );

	result = SetPixelFormat( hDC, pixelformat, &pfd );
	assert( result );

	dvars.hGLRC = wglCreateContext( hDC );
	assert( dvars.hGLRC );

	result = wglMakeCurrent( hDC, (HGLRC)dvars.hGLRC );
	assert( result );
}

void WGLimp_DestroyDummyWindow( DummyVars &dvars )
{
	BOOL result;

	result = wglDeleteContext( (HGLRC)dvars.hGLRC );
	assert( result );
	result = DestroyWindow( (HWND)dvars.hWnd );
	assert( result );
	result = UnregisterClassW( DUMMY_WINDOW_CLASSNAME, GetModuleHandleW( nullptr ) );
	assert( result );
}

#endif
