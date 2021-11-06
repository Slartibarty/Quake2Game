
#include "../../core/core.h"

#include "../../framework/filesystem.h"
#include "../../framework/cmdsystem.h"
#include "../../framework/cvarsystem.h"

#include "../../core/sys_includes.h"
#include <process.h>

// Local
#include "renderer/d3d_public.h"
#include "window.h"
#include "input.h"

//=================================================================================================

extern float sys_frameTime;

static void Com_Init( int argc, char **argv )
{
	Time_Init();

	Cbuf_Init();
	Cmd_Init();
	Cvar_Init();

	Cvar_AddEarlyCommands( argc, argv );

	FileSystem::Init();

	Cbuf_AddLateCommands( argc, argv );

	CreateMainWindow( wnd_width.GetInt(), wnd_height.GetInt() );
	Input_Init();

	Renderer::Init();
}

static void Com_Shutdown()
{
	Renderer::Shutdown();

	DestroyMainWindow();

	FileSystem::Shutdown();
	Cvar_Shutdown();
	Cmd_Shutdown();
	Cbuf_Shutdown();
}

static void Com_Quit( int code )
{
	Com_Shutdown();

	exit( code );
}

static void Com_Frame( double deltaTime )
{
	Input_Frame();

	Renderer::Frame();
}

int main( int argc, char **argv )
{
#ifdef _WIN32
	// Make sure the CRT thinks we're a GUI app, this makes CRT asserts use a message box
	// rather than printing to stderr
	_set_app_type( _crt_gui_app );
#ifdef Q_MEM_DEBUG
	_CrtSetDbgFlag( _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) | _CRTDBG_LEAK_CHECK_DF );
#endif
#endif

	Com_Init( argc, argv );

	double deltaTime, oldTime, newTime;
	MSG msg;

	oldTime = Time_FloatMilliseconds();

	// Show the main window just before we start doing things
	ShowMainWindow();

	// Main loop
	while ( 1 )
	{
		// Process all queued messages
		while ( PeekMessageW( &msg, nullptr, 0, 0, PM_REMOVE ) )
		{
			if ( msg.message == WM_QUIT )
			{
				Com_Quit( EXIT_SUCCESS );
			}
			TranslateMessage( &msg );
			DispatchMessageW( &msg );
		}

		newTime = Time_FloatMilliseconds();
		deltaTime = newTime - oldTime;

		sys_frameTime = (float)MS2SEC( deltaTime );

		Com_Frame( MS2SEC( deltaTime ) );

		oldTime = newTime;
	}
	
	// unreachable
	return 0;
}
