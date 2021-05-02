// sys_win.c

#include "engine.h"

#include <vector>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "sys.h"

qboolean		ActiveApp, Minimized;

unsigned		sys_frame_time;

int		g_argc;
char	**g_argv;

/*
===================================================================================================

	System I/O

===================================================================================================
*/

void Sys_OutputDebugString( const char *msg )
{
	fputs( msg, stderr );
}

[[noreturn]]
void Sys_Error( const char *msg )
{
	Sys_OutputDebugString( msg );

	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Engine Error", msg, nullptr );

	Sys_Quit( EXIT_FAILURE );
}

[[noreturn]]
void Sys_Quit( int code )
{
	exit( code );
}

//=================================================================================================

/*
================
Sys_CopyProtect

================
*/
void Sys_CopyProtect (void)
{

}

//=================================================================================================

/*
================
Sys_Init
================
*/
void Sys_Init (void)
{

}

static char	console_text[256];
static int	console_textlen;

/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void)
{
	return nullptr;
}


/*
================
Sys_ConsoleOutput

Print text to the dedicated console
================
*/
void Sys_ConsoleOutput (const char *string)
{

}


/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents( void )
{

}


/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData( void )
{
	char *data = nullptr;

	if ( SDL_HasClipboardText() )
	{
		// SlartTodoLinux: LEAK LEAK LEAK! must be freed using SDL_Free(data)!
		data = SDL_GetClipboardText();
	}

	return data;
}

/*
===================================================================================================

	Video modes. Don't move this to the renderer.

===================================================================================================
*/

struct vidMode_t
{
	int width, height;
};

std::vector<vidMode_t> s_vidModes;

static void Sys_InitVidModes()
{
	int numModes = SDL_GetNumDisplayModes(0);

	s_vidModes.reserve(numModes);

	// We only care about different widths and heights
	int lastWidth = 0, lastHeight = 0;
	SDL_DisplayMode mode{};

	for (int i = 0; i < numModes; ++i)
	{
		SDL_GetDisplayMode(0, i, &mode);

		if (lastWidth == mode.w && lastHeight == mode.h)
		{
			continue;
		}

		// add to list
		s_vidModes.push_back({mode.w, mode.h});

		lastWidth = mode.w;
		lastHeight = mode.h;
	}
}

// never called anywhere right now
// s_vidModes is global anyway, so it'll be freed at exit by the CRT
static void Sys_FreeVidModes()
{
	s_vidModes.clear();
	s_vidModes.shrink_to_fit();
}

int Sys_GetNumVidModes()
{
	return static_cast<int>( s_vidModes.size() );
}

bool Sys_GetVidModeInfo( int &width, int &height, int mode )
{
	if ( mode < 0 || mode >= Sys_GetNumVidModes() ) {
		return false;
	}

	width = s_vidModes[mode].width;
	height = s_vidModes[mode].height;

	return true;
}

/*
===================================================================================================

	Miscellaneous

===================================================================================================
*/

/*
=================
Sys_AppActivate
=================
*/
void Sys_AppActivate (void)
{

}

/*
===================================================================================================

	Game DLLs

===================================================================================================
*/

using libHandle_t = void *;

static libHandle_t game_library;
static libHandle_t cgame_library;

void Sys_UnloadGame()
{
	SDL_UnloadObject( game_library );

	game_library = nullptr;
}

void Sys_UnloadCGame()
{
	SDL_UnloadObject( cgame_library );

	game_library = nullptr;
}

typedef void *( *GetAPI_t ) ( void * );

static void *Sys_GetAPI( void *parms, libHandle_t &instance, const char *gamename, const char *procname )
{
	char name[MAX_OSPATH];

	if ( instance ) {
		Com_FatalError( "Sys_GetAPI without Sys_Unload(C)Game" );
	}

	// now run through the search paths
	const char *path = nullptr;
	while ( 1 )
	{
		path = FS_NextPath( path );
		if ( !path ) {
			return nullptr;		// couldn't find one anywhere
		}
		Q_sprintf_s( name, "%s/%s", path, gamename );
		instance = SDL_LoadObject( gamename );
		if ( instance )
		{
			Com_DPrintf( "LoadLibrary (%s)\n", name );
			break;
		}
	}

	GetAPI_t GetGameAPI = (GetAPI_t)SDL_LoadFunction( instance, procname );
	if ( !GetGameAPI )
	{
		Com_Printf("Failed to get game API: %s", SDL_GetError());
		Sys_UnloadGame();
		return nullptr;
	}

	return GetGameAPI( parms );
}

void *Sys_GetGameAPI( void *parms )
{
	return Sys_GetAPI( parms, game_library, "/home/josh/projects/moon/game/base/libgame.so", "GetGameAPI" );
}

void *Sys_GetCGameAPI( void *parms )
{
	return Sys_GetAPI( parms, cgame_library, "/home/josh/projects/moon/game/base/libcgame.so", "GetCGameAPI" );
}

//=================================================================================================

/*
========================
main
========================
*/
int main( int argc, char **argv )
{
	int time, oldtime, newtime;

	g_argc = argc; g_argv = argv;

	// TODO: tune this
	SDL_InitSubSystem(SDL_INIT_EVERYTHING);

	Time_Init();

	Sys_InitVidModes();

	Engine_Init( argc, argv );

	oldtime = Sys_Milliseconds();

	SDL_Event msg;

	// main loop
	while ( 1 )
	{
		// if at a full screen console, don't update unless needed
		if ( Minimized || ( dedicated && dedicated->value ) )
		{

		}

		// proces all queued messages
		while ( SDL_PollEvent( &msg ) )
		{
			// handoff to input system
		}

		do
		{
			newtime = Sys_Milliseconds();
			time = newtime - oldtime;
		} while ( time < 1 );

		sys_frame_time = newtime;

		Engine_Frame( time );

		oldtime = newtime;
	}

	// unreachable
	return 0;
}
