/*
===================================================================================================

	This file contains ALL Win32 specific stuff having to do with the
	OpenGL refresh.  When a port is being made the following functions
	must be implemented by the port:

	GLimp_EndFrame
	GLimp_Init
	GLimp_Shutdown
	GLimp_SwitchFullscreen

===================================================================================================
*/

#include "gl_local.h"
#include "../../resources/resource.h"

#include "imgui.h"

struct glwstate_t
{
	HINSTANCE	hInstance;

	HDC			hDC;
	HWND		hWnd;
	HGLRC		hGLRC;
};

static glwstate_t s_glwState;

/*
===================================================================================================

	Dummy window

===================================================================================================
*/

struct DummyVars
{
	HWND	hWnd;
	HGLRC	hGLRC;
};

static constexpr auto	DummyClassname = L"GRUG";

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

/*
===================================================================================================

	Client window

===================================================================================================
*/

static constexpr auto	WINDOW_CLASS_NAME = L"Q2GAME";
static constexpr DWORD	WINDOW_STYLE = ( WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX );

static void GLimp_SetWindowSize( int width, int height )
{
	DWORD dwStyle = (DWORD)GetWindowLongW( s_glwState.hWnd, GWL_STYLE );
	DWORD dwExStyle = (DWORD)GetWindowLongW( s_glwState.hWnd, GWL_EXSTYLE );

	RECT r{ 0, 0, width, height };
	AdjustWindowRectEx( &r, dwStyle, false, dwExStyle );

	int newWidth = r.right - r.left;
	int newHeight = r.bottom - r.top;

	SetWindowPos( s_glwState.hWnd, NULL, 0, 0, newWidth, newHeight, SWP_NOZORDER | SWP_NOMOVE );

	glViewport( 0, 0, width, height );
	VID_NewWindow( width, height );
	vid.width = width;
	vid.height = height;
}

static void GLimp_PerformCDS( int width, int height, bool fullscreen, bool alertWindow )
{
	// SLARTTODO: DENY FULLSCREEN!
	fullscreen = false;

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

		Com_Printf( "...attempting fullscreen\n" "...calling CDS: " );

		if ( ChangeDisplaySettingsW( &dm, CDS_FULLSCREEN ) == DISP_CHANGE_SUCCESSFUL )
		{
			glState.fullscreen = true;

			Com_Printf( "ok\n" );

			if ( alertWindow )
			{
				GLimp_SetWindowSize( width, height );
			}
		}
		else
		{
			Com_Printf( " failed\n" );

			Com_Printf( "...setting windowed mode\n" );

			ChangeDisplaySettingsW( 0, 0 );
		}
	}
	else
	{
		Com_Printf( "...setting windowed mode\n" );

		ChangeDisplaySettingsW( 0, 0 );

		glState.fullscreen = false;

		if ( alertWindow )
		{
			GLimp_SetWindowSize( width, height );
		}
	}
}

static void GLimp_PopulateLegacyPFD( PIXELFORMATDESCRIPTOR &pfd )
{
	pfd =
	{
		sizeof( PIXELFORMATDESCRIPTOR ),
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
}

void *GLimp_SetupContext( void *localContext, bool shareWithMain )
{
	PIXELFORMATDESCRIPTOR pfd;
	BOOL result;
	int pixelformat;

	HDC hDC = reinterpret_cast<HDC>( localContext );

	// Intel HD Graphics 3000 chips (my laptop) don't support this function
	if ( WGLEW_ARB_pixel_format )
	{
		const int multiSamples = Clamp( r_multisamples->GetInt(), 0, 8 );

		const int attriblist[]
		{
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_SAMPLES_ARB, multiSamples,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB, 32,
			WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,
			0
		};

		UINT numformats;

		result = wglChoosePixelFormatARB( hDC, attriblist, NULL, 1, &pixelformat, &numformats );
		assert( result );

		result = DescribePixelFormat( hDC, pixelformat, sizeof( pfd ), &pfd );
		assert( result != 0 );
	}
	else
	{
		GLimp_PopulateLegacyPFD( pfd );
		pixelformat = ChoosePixelFormat( hDC, &pfd );
	}

	result = SetPixelFormat( hDC, pixelformat, &pfd );
	assert( result );

	HGLRC hGLRC;

	// Intel HD Graphics 3000 chips (my laptop) don't support this function
	if ( WGLEW_ARB_create_context )
	{
		const int contextAttribs[]
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 3,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
			0, // End
		};

		hGLRC = wglCreateContextAttribsARB( hDC, NULL, contextAttribs );
		assert( hGLRC );
	}
	else
	{
		hGLRC = wglCreateContext( hDC );
		assert( hGLRC );
	}

	result = wglMakeCurrent( hDC, hGLRC );
	assert( result );

	if ( shareWithMain )
	{
		wglShareLists( s_glwState.hGLRC, hGLRC );
	}

	if ( WGLEW_EXT_swap_control )
	{
		wglSwapIntervalEXT( r_swapinterval->GetInt() );
	}

	return reinterpret_cast<void *>( hGLRC );
}

static void GLimp_CreateWindow( WNDPROC wndproc, int width, int height, bool fullscreen )
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
		(HCURSOR)LoadImageW( NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED ),	// Cursor to use
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

		// not multi-monitor safe
		int dispWidth = GetSystemMetrics( SM_CXSCREEN );
		int dispHeight = GetSystemMetrics( SM_CYSCREEN );

		// centre window
		xPos = ( dispWidth / 2 ) - ( width / 2 );
		yPos = ( dispHeight / 2 ) - ( height / 2 );
	}

	RECT r{ 0, 0, width, height };
	AdjustWindowRectEx( &r, dwStyle, false, dwExStyle );

	const platChar_t *windowTitle = FileSystem::ModInfo::GetWindowTitle();

	s_glwState.hWnd = CreateWindowExW(
		dwExStyle,				// Ex-Style
		WINDOW_CLASS_NAME,		// Window class
		windowTitle,			// Window title
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

	s_glwState.hDC = GetDC( s_glwState.hWnd );
	assert( s_glwState.hDC );

	s_glwState.hGLRC = reinterpret_cast<HGLRC>( GLimp_SetupContext( s_glwState.hDC, false ) );

	// let the sound and input subsystems know about the new window
	VID_NewWindow( width, height );
	vid.width = width;
	vid.height = height;

	if ( fullscreen )
	{
		// Perform our CDS now
		GLimp_PerformCDS( width, height, true, false );
	}

	ShowWindow( s_glwState.hWnd, SW_SHOW );
}

static void GLimp_DestroyWindow()
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
		s_glwState.hDC = NULL;
	}
}

/*
===================================================================================================

	Miscellaneous

===================================================================================================
*/

bool GLimp_SetMode( int &width, int &height, int mode, bool fullscreen )
{
	Com_Printf( "Setting mode %d:", mode );

	if ( !Sys_GetVidModeInfo( width, height, mode ) )
	{
		Com_Printf( " invalid mode\n" );
		width = 0;
		height = 0;
		return false;
	}

	Com_Printf( " %dx%d %s\n", width, height, ( fullscreen ? "FS" : "W" ) );

	GLimp_PerformCDS( width, height, fullscreen, true );

	glViewport( 0, 0, width, height );

	return true;
}

bool GLimp_Init()
{
	s_glwState.hInstance = GetModuleHandleW( nullptr );

	DummyVars dvars;
	GLimp_CreateDummyWindow( dvars );

	// initialize our OpenGL dynamic bindings
	glewExperimental = GL_TRUE;
	if ( glewInit() != GLEW_OK || wglewInit() != GLEW_OK )
	{
		Com_Printf( "GLimp_Init() - could not load OpenGL bindings\n" );
		return false;
	}

	GLimp_DestroyDummyWindow( dvars );

	int width, height;
	if ( !Sys_GetVidModeInfo( width, height, r_mode->GetInt() ) )
	{
		Com_Printf( "GLimp_Init() - invalid mode\n" );
		return false;
	}

	extern LRESULT CALLBACK MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	GLimp_CreateWindow( MainWndProc, width, height, r_fullscreen->GetBool() );

	r_mode->ClearModified();
	r_fullscreen->ClearModified();

	return true;
}

void GLimp_Shutdown()
{
	GLimp_DestroyWindow();

	if ( glState.fullscreen == true )
	{
		ChangeDisplaySettingsW( 0, 0 );
		glState.fullscreen = false;
	}
}

void GLimp_BeginFrame()
{
	// If our swap interval has changed, then change it!
	if ( r_swapinterval->IsModified() )
	{
		r_swapinterval->ClearModified();

		if ( WGLEW_EXT_swap_control )
		{
			wglSwapIntervalEXT( r_swapinterval->GetInt() );
		}
	}
}

void GLimp_EndFrame()
{
	if ( ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		wglMakeCurrent( s_glwState.hDC, s_glwState.hGLRC );
	}

	SwapBuffers( s_glwState.hDC );
}

// helper for client code (phasing out cl_hwnd)
void *R_GetWindowHandle()
{
	return reinterpret_cast<void *>( s_glwState.hWnd );
}

void GLimp_AppActivate( bool active )
{
#if 0
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
#endif
}
