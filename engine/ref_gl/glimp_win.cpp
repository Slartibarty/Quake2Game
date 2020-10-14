//-------------------------------------------------------------------------------------------------
//
// glimp_win.c
//
// This file contains ALL Win32 specific stuff having to do with the
// OpenGL refresh.  When a port is being made the following functions
// must be implemented by the port:
//
// GLimp_EndFrame
// GLimp_Init
// GLimp_Shutdown
// GLimp_SwitchFullscreen
//
//-------------------------------------------------------------------------------------------------

#include "gl_local.h"
#include <Windows.h>
#include "../res/resource.h"

struct glwstate_t
{
	HINSTANCE	hInstance;

	HDC			hDC;
	HWND		hWnd;
	HGLRC		hGLRC;
};

static glwstate_t s_glwState;

//-------------------------------------------------------------------------------------------------
//
// Dummy window
//
//-------------------------------------------------------------------------------------------------

struct DummyVars
{
	HWND	hWnd;
	HGLRC	hGLRC;
};

static constexpr auto	DummyClassname = L"GRUG";

// Create a dummy window
//
static void GLimp_CreateDummyWindow( DummyVars &dvars )
{
	BOOL result;

	// Create our dummy window class
	const WNDCLASSEXW wcex
	{
		sizeof( wcex ),				// Size
		CS_OWNDC,					// Style
		DefWindowProcW,				// Wndproc
		0,							// ClsExtra
		0,							// WndExtra
		s_glwState.hInstance,		// hInstance
		NULL,						// Icon
		NULL,						// Cursor
		(HBRUSH)( COLOR_WINDOW + 1 ),	// Background colour
		NULL,						// Menu
		DummyClassname,				// Class name
		NULL						// Small icon
	};
	RegisterClassExW( &wcex );

	dvars.hWnd = CreateWindowExW(
		0,
		DummyClassname,
		L"",
		0,
		0, 0, 800, 600,
		NULL,
		NULL,
		s_glwState.hInstance,
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
	HDC hDC = GetDC( dvars.hWnd );
	assert( hDC );

	int pixelformat = ChoosePixelFormat( hDC, &pfd );
	assert( pixelformat != 0 );

	result = SetPixelFormat( hDC, pixelformat, &pfd );
	assert( result );

	dvars.hGLRC = wglCreateContext( hDC );
	assert( dvars.hGLRC );

	result = wglMakeCurrent( hDC, dvars.hGLRC );
	assert( result );
}

// Destroy a dummy window
//
static void GLimp_DestroyDummyWindow( DummyVars &dvars )
{
	BOOL result;

	result = wglDeleteContext( dvars.hGLRC );
	assert( result );
	result = DestroyWindow( dvars.hWnd );
	assert( result );
	result = UnregisterClassW( DummyClassname, s_glwState.hInstance );
	assert( result );
}

//-------------------------------------------------------------------------------------------------
//
// Client window
//
//-------------------------------------------------------------------------------------------------

static constexpr auto	WINDOW_TITLE = L"Quake 2 - OpenGL";
static constexpr auto	WINDOW_CLASS_NAME = L"Q2GAME";
static constexpr DWORD	WINDOW_STYLE = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU);

//-------------------------------------------------------------------------------------------------
// Resizes a window without destroying it
//-------------------------------------------------------------------------------------------------
static void GLimp_SetWindowSize( int width, int height )
{
	DWORD dwStyle = (DWORD)GetWindowLongPtrW( s_glwState.hWnd, GWL_STYLE );
	DWORD dwExStyle = (DWORD)GetWindowLongPtrW( s_glwState.hWnd, GWL_EXSTYLE );

	RECT r = { 0, 0, width, height };
	AdjustWindowRectEx( &r, dwStyle, false, dwExStyle );

	SetWindowPos( s_glwState.hWnd, NULL, r.right - r.left, r.bottom - r.top, 0, 0, SWP_NOZORDER | SWP_NOMOVE );

	ri.Vid_NewWindow( width, height );
	vid.width = width;
	vid.height = height;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void GLimp_PerformCDS( int width, int height, qboolean fullscreen, qboolean alertWindow )
{
	// do a CDS if needed
	if ( fullscreen )
	{
		DEVMODEW dm
		{
			.dmSize = sizeof( dm ),
			.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT,
			.dmPelsWidth = (DWORD)width,
			.dmPelsHeight = (DWORD)height
		};

		ri.Con_Printf( PRINT_ALL, "...attempting fullscreen\n" "...calling CDS: " );

		if ( ChangeDisplaySettingsW( &dm, CDS_FULLSCREEN ) == DISP_CHANGE_SUCCESSFUL )
		{
			gl_state.fullscreen = true;

			ri.Con_Printf( PRINT_ALL, "ok\n" );

			if ( alertWindow )
			{
				GLimp_SetWindowSize( width, height );
			}
		}
		else
		{
			ri.Con_Printf( PRINT_ALL, " failed\n" );

			ri.Con_Printf( PRINT_ALL, "...setting windowed mode\n" );

			ChangeDisplaySettingsW( 0, 0 );
		}
	}
	else
	{
		ri.Con_Printf( PRINT_ALL, "...setting windowed mode\n" );

		ChangeDisplaySettingsW( 0, 0 );

		gl_state.fullscreen = false;

		if ( alertWindow )
		{
			GLimp_SetWindowSize( width, height );
		}
	}
}

//-------------------------------------------------------------------------------------------------
// Fill a PFD struct
//-------------------------------------------------------------------------------------------------
static void PopulateLegacyPFD(PIXELFORMATDESCRIPTOR &pfd)
{
	const PIXELFORMATDESCRIPTOR newpfd
	{
		sizeof(PIXELFORMATDESCRIPTOR),
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

	pfd = newpfd;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void GLimp_CreateWindow( WNDPROC wndproc, int width, int height, qboolean fullscreen )
{
	const WNDCLASSEXW wcex
	{
		sizeof( wcex ),			// Structure size
		CS_OWNDC,				// Class style
		wndproc,				// WndProc
		0,						// Extra storage
		0,						// Extra storage
		s_glwState.hInstance,	// Module
		(HICON)LoadImageW( s_glwState.hInstance, MAKEINTRESOURCEW( IDI_ICON1 ), IMAGE_ICON, 0, 0, LR_SHARED ),		// Icon
		(HCURSOR)LoadImageW( NULL, MAKEINTRESOURCEW( 32512 ), IMAGE_CURSOR, 0, 0, LR_SHARED ),	// Cursor to use
		(HBRUSH)( COLOR_WINDOW + 1 ),		// Background colour
		NULL,					// Menu
		WINDOW_CLASS_NAME,		// Classname
		NULL					// Small icon
	};
	RegisterClassExW( &wcex );

	DWORD	dwStyle, dwExStyle;
	int		xPos, yPos;

	if ( fullscreen )
	{
		dwStyle = WS_POPUP;
		dwExStyle = WS_EX_TOPMOST;

		xPos = 0;
		yPos = 0;
	}
	else
	{
		dwStyle = WINDOW_STYLE;
		dwExStyle = 0;

		cvar_t *vid_xpos = ri.Cvar_Get( "vid_xpos", "0", 0 );
		cvar_t *vid_ypos = ri.Cvar_Get( "vid_ypos", "0", 0 );
		xPos = (int)vid_xpos->value;
		yPos = (int)vid_ypos->value;
	}

	RECT r{ 0, 0, width, height };
	AdjustWindowRectEx( &r, dwStyle, false, dwExStyle );

	s_glwState.hWnd = CreateWindowExW(
		dwExStyle,				// Ex-Style
		WINDOW_CLASS_NAME,		// Window class
		WINDOW_TITLE,			// Window title
		dwStyle,				// Window style
		xPos,					// X pos
		yPos,					// Y pos (Calls ShowWindow if WS_VISIBLE is set)
		r.right - r.left,		// Width
		r.bottom - r.top,		// Height
		NULL,					// Parent window
		NULL,					// Menu to use
		s_glwState.hInstance,	// Module instance
		NULL					// Additional params
	);

	BOOL	result;
	PIXELFORMATDESCRIPTOR pfd;
	int		pixelformat;

	s_glwState.hDC = GetDC( s_glwState.hWnd );
	assert( s_glwState.hDC );

	// Intel HD Graphics 3000 chips (my laptop) don't support this function
	if (WGLEW_ARB_pixel_format)
	{
		const int attriblist[]
		{
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB, 32,
			WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,
			0
		};

		UINT numformats;

		result = wglChoosePixelFormatARB(s_glwState.hDC, attriblist, NULL, 1, &pixelformat, &numformats);
		assert(result);

		result = DescribePixelFormat(s_glwState.hDC, pixelformat, sizeof(pfd), &pfd);
		assert(result != 0);
	}
	else
	{
		PopulateLegacyPFD(pfd);
		pixelformat = ChoosePixelFormat(s_glwState.hDC, &pfd);
	}

	result = SetPixelFormat( s_glwState.hDC, pixelformat, &pfd );
	assert( result );

	// Intel HD Graphics 3000 chips (my laptop) don't support this function
	if (WGLEW_ARB_create_context)
	{
		const int contextAttribs[]
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 3,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
			0, // End
		};

		s_glwState.hGLRC = wglCreateContextAttribsARB(s_glwState.hDC, NULL, contextAttribs);
		assert(s_glwState.hGLRC);
	}
	else
	{
		s_glwState.hGLRC = wglCreateContext(s_glwState.hDC);
		assert(s_glwState.hGLRC);
	}

	result = wglMakeCurrent( s_glwState.hDC, s_glwState.hGLRC );
	assert( result );

	if (WGLEW_EXT_swap_control)
	{
		wglSwapIntervalEXT((int)gl_swapinterval->value);
	}

	// let the sound and input subsystems know about the new window
	ri.Vid_NewWindow( width, height );
	vid.width = width;
	vid.height = height;

	if ( fullscreen )
	{
		// Perform our CDS now
		GLimp_PerformCDS( width, height, fullscreen, false );
	}

	ShowWindow( s_glwState.hWnd, SW_SHOW );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void GLimp_DestroyWindow( void )
{
	if ( s_glwState.hGLRC )
	{
		wglMakeCurrent( NULL, NULL );
		wglDeleteContext( s_glwState.hGLRC );
		s_glwState.hGLRC = NULL;
	}
	if ( s_glwState.hWnd )
	{
		DestroyWindow( s_glwState.hWnd );
		UnregisterClassW( WINDOW_CLASS_NAME, s_glwState.hInstance );
		s_glwState.hWnd = NULL;
	}
}

//-------------------------------------------------------------------------------------------------
//
// Misc
//
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
rserr_t GLimp_SetMode( int *pWidth, int *pHeight, int mode, qboolean fullscreen )
{
	int width, height;

	ri.Con_Printf( PRINT_ALL, "Initializing OpenGL display\n" );

	ri.Con_Printf( PRINT_ALL, "...setting mode %d:", mode );

	if ( !ri.Vid_GetModeInfo( &width, &height, mode ) )
	{
		ri.Con_Printf( PRINT_ALL, " invalid mode\n" );
		*pWidth = 0; *pHeight = 0;
		return rserr_invalid_mode;
	}

	ri.Con_Printf( PRINT_ALL, " %d %d %s\n", width, height, fullscreen ? "W" : "FS" );

	GLimp_PerformCDS( width, height, fullscreen, true );

	*pWidth = width;
	*pHeight = height;

	return rserr_ok;
}

//-------------------------------------------------------------------------------------------------
// This routine does all OS specific shutdown procedures for the OpenGL
// subsystem.  Under OpenGL this means NULLing out the current DC and
// HGLRC, deleting the rendering context, and releasing the DC acquired
// for the window.  The state structure is also nulled out.
//-------------------------------------------------------------------------------------------------
void GLimp_Shutdown( void )
{
	GLimp_DestroyWindow();

	if ( gl_state.fullscreen == true )
	{
		ChangeDisplaySettingsW( 0, 0 );
		gl_state.fullscreen = false;
	}
}

//-------------------------------------------------------------------------------------------------
// This routine is responsible for initializing the OS specific portions
// of OpenGL.  Under Win32 this means dealing with the pixelformats and
// doing the wgl interface stuff.
//-------------------------------------------------------------------------------------------------
qboolean GLimp_Init( void *hinstance, void *wndproc )
{
	s_glwState.hInstance = (HINSTANCE)hinstance;

	DummyVars dvars;
	GLimp_CreateDummyWindow( dvars );

	// initialize our OpenGL dynamic bindings
	glewExperimental = GL_TRUE;
	if ( glewInit() != GLEW_OK && wglewInit() != GLEW_OK )
	{
		ri.Con_Printf( PRINT_ALL, "ref_gl::GLimp_Init() - could not load OpenGL bindings\n" );
		return false;
	}

	GLimp_DestroyDummyWindow( dvars );

	int width, height;
	if ( !ri.Vid_GetModeInfo( &width, &height, (int)gl_mode->value ) )
	{
		ri.Con_Printf( PRINT_ALL, "ref_gl::GLimp_Init() - invalid mode\n" );
		return false;
	}

	GLimp_CreateWindow( (WNDPROC)wndproc, width, height, (qboolean)vid_fullscreen->value );

	return true;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void GLimp_BeginFrame( void )
{
	// If our swap interval has changed, then change it!
	if (gl_swapinterval->modified)
	{
		gl_swapinterval->modified = false;

		if (WGLEW_EXT_swap_control)
		{
			wglSwapIntervalEXT((int)gl_swapinterval->value);
		}
	}
}

//-------------------------------------------------------------------------------------------------
// Responsible for doing a swapbuffers and possibly for other stuff
// as yet to be determined.  Probably better not to make this a GLimp
// function and instead do a call to GLimp_SwapBuffers.
//-------------------------------------------------------------------------------------------------
void GLimp_EndFrame( void )
{
	SwapBuffers( s_glwState.hDC );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void GLimp_AppActivate( qboolean active )
{
	if ( active )
	{
		SetForegroundWindow( s_glwState.hWnd );
		ShowWindow( s_glwState.hWnd, SW_RESTORE );
	}
	else
	{
		if ( vid_fullscreen->value )
			ShowWindow( s_glwState.hWnd, SW_MINIMIZE );
	}
}
