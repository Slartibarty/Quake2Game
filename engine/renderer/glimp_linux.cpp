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

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

struct glwstate_t
{
	SDL_Window *		hWnd;
	SDL_GLContext		hGLRC;
};

static glwstate_t s_glwState;

/*
===================================================================================================

	Client window

===================================================================================================
*/

static constexpr auto		WINDOW_TITLE = "JaffaQuake";
static constexpr uint32		WINDOW_STYLE = ( SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN );

static void GLimp_SetWindowSize( int width, int height )
{
	SDL_SetWindowSize(s_glwState.hWnd, width, height);

	glViewport( 0, 0, width, height );
	VID_NewWindow( width, height );
	vid.width = width;
	vid.height = height;
}

static void GLimp_PerformCDS( int width, int height, bool fullscreen, bool alertWindow )
{
	// SLARTTODO: DENY FULLSCREEN!
	fullscreen = false;

	Com_Printf( "...setting windowed mode\n" );

	glState.fullscreen = false;

	if ( alertWindow )
	{
		GLimp_SetWindowSize( width, height );
	}
}

static void GLimp_CreateWindow( int width, int height, bool fullscreen )
{
	//SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	//SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	//SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

	s_glwState.hWnd = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, WINDOW_STYLE);
	if (!s_glwState.hWnd) {
		Com_FatalError("Failed to create SDL2 window");
	}

	s_glwState.hGLRC = SDL_GL_CreateContext(s_glwState.hWnd);
	if (!s_glwState.hGLRC) {
		Com_FatalErrorf("Failed to create GL context: %s", SDL_GetError());
	}

	SDL_GL_SetSwapInterval( r_swapinterval->GetInt32() );

	// let the sound and input subsystems know about the new window
	VID_NewWindow( width, height );
	vid.width = width;
	vid.height = height;

	if ( fullscreen )
	{
		// Perform our CDS now
		GLimp_PerformCDS( width, height, true, false );
	}

	SDL_ShowWindow( s_glwState.hWnd );
}

static void GLimp_DestroyWindow()
{
	if ( s_glwState.hGLRC )
	{
		SDL_GL_MakeCurrent( nullptr, nullptr );
		SDL_GL_DeleteContext( s_glwState.hGLRC );
		s_glwState.hGLRC = nullptr;
	}
	if ( s_glwState.hWnd )
	{
		SDL_DestroyWindow( s_glwState.hWnd );
		s_glwState.hWnd = nullptr;
	}
}

/*
===================================================================================================

	Miscellaneous

===================================================================================================
*/

void GLimp_SetGamma( byte *red, byte *green, byte *blue )
{

}

void GLimp_RestoreGamma( void )
{

}

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
	int width, height;
	if ( !Sys_GetVidModeInfo( width, height, r_mode->GetInt32() ) )
	{
		Com_Printf( "GLimp_Init() - invalid mode\n" );
		return false;
	}

	GLimp_CreateWindow( width, height, r_fullscreen->GetBool() );

	// initialize our OpenGL dynamic bindings
	glewExperimental = GL_TRUE;
	if ( glewInit() != GLEW_OK )
	{
		Com_Printf( "GLimp_Init() - could not load OpenGL bindings\n" );
		return false;
	}

	r_mode->ClearModified();
	r_fullscreen->ClearModified();

	return true;
}

void GLimp_Shutdown()
{
	GLimp_DestroyWindow();

	if ( glState.fullscreen == true )
	{
		glState.fullscreen = false;
	}
}

void GLimp_BeginFrame()
{
	// If our swap interval has changed, then change it!
	if ( r_swapinterval->IsModified() )
	{
		r_swapinterval->ClearModified();

		SDL_GL_SetSwapInterval( r_swapinterval->GetInt32() );
	}
}

void GLimp_EndFrame()
{
	SDL_GL_SwapWindow( s_glwState.hWnd );
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
