// sys_win.c

#include "engine.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "sys.h"

//#define DEMO

unsigned		sys_msg_time;
unsigned		sys_frame_time;

int		g_argc;
char	**g_argv;

/*
===============================================================================

SYSTEM IO

===============================================================================
*/

[[noreturn]]
void Sys_Error (const char *error, ...)
{
	va_list		argptr;
	char		text[MAX_PRINT_MSG];

	CL_Shutdown ();
	Engine_Shutdown ();

	va_start (argptr, error);
	Q_vsprintf_s (text, error, argptr);
	va_end (argptr);
	fputs( text, stderr );

	// We won't have a window by now
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Engine Error", text, nullptr );

	exit (1);
}

[[noreturn]]
void Sys_Quit (void)
{
	CL_Shutdown();
	Engine_Shutdown ();

	exit (0);
}

//================================================================

/*
================
Sys_CopyProtect

================
*/
void Sys_CopyProtect (void)
{
}

//================================================================

/*
================
Sys_Init
================
*/
void Sys_Init (void)
{
}

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

	// grab frame time
	sys_frame_time = Sys_Milliseconds(); // FIXME: should this be at start?
}

/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData( void )
{
	return nullptr;
}

/*
==============================================================================

 WINDOWS CRAP

==============================================================================
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
========================================================================

GAME DLL

========================================================================
*/

static void *game_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame( void )
{
	if ( !game_library ) {
		Com_Error( ERR_FATAL, "Sys_UnloadGame called with NULL game_library\n" );
		return;
	}
	SDL_UnloadObject( game_library );
	game_library = nullptr;
}

typedef void *( *GetGameAPI_t ) ( void * );

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI( void *parms )
{
	char name[MAX_OSPATH];

	const char *gamename = "game" BLD_ARCHITECTURE ".so";

	if ( game_library ) {
		Com_Error( ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame" );
	}

	// now run through the search paths
	char *path = nullptr;
	while ( 1 )
	{
		path = FS_NextPath( path );
		if ( !path ) {
			return nullptr; // couldn't find one anywhere
		}
		Q_sprintf_s( name, "%s/%s", path, gamename );
		game_library = SDL_LoadObject( name );
		if ( game_library )
		{
			Com_DPrintf( "SDL_LoadObject (%s)\n", name );
			break;
		}
	}

	GetGameAPI_t GetGameAPI = (GetGameAPI_t)SDL_LoadFunction( game_library, "GetGameAPI" );
	if ( !GetGameAPI )
	{
		Sys_UnloadGame();
		return nullptr;
	}

	return GetGameAPI( parms );
}

//=======================================================================

/*
==================
WinMain

==================
*/
int main(int argc, char **argv)
{
	int time, oldtime, newtime;

	g_argc = argc; g_argv = argv;

	Time_Init();

	Engine_Init (argc, argv);
	oldtime = Sys_Milliseconds ();

    /* main window message loop */
	while (1)
	{
		do
		{
			newtime = Sys_Milliseconds ();
			time = newtime - oldtime;
		} while (time < 1);

		Engine_Frame (time);

		oldtime = newtime;
	}

	// never gets here
    return 0;
}
