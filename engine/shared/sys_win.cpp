// sys_win.c

#include "engine.h"

#include <process.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "../client/winquake.h"	// Hack?

#include "sys.h"

//#define DEMO

qboolean		ActiveApp, Minimized;

static HANDLE	hinput, houtput;

unsigned		sys_msg_time;
unsigned		sys_frame_time;

int		g_argc;
char	**g_argv;

/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_OutputDebugString( const char *msg )
{
	if ( IsDebuggerPresent() )
	{
		OutputDebugStringA( msg );
	}
}

[[noreturn]]
void Sys_Error( const char *msg )
{
	Sys_OutputDebugString( msg ); // Mirror to the debugger

	// We won't have a window by now
	MessageBoxA( nullptr, msg, "Engine Error", MB_OK | MB_ICONERROR | MB_TOPMOST );

	Sys_Quit( EXIT_FAILURE );
}

[[noreturn]]
void Sys_Quit( int code )
{
	if ( dedicated && dedicated->value )
		FreeConsole();

	// shut down QHOST hooks if necessary
	DeinitConProc();

	exit( code );
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
	if (dedicated->value)
	{
		if (!AllocConsole ())
			Com_FatalErrorf("Couldn't create dedicated server console");
		hinput = GetStdHandle (STD_INPUT_HANDLE);
		houtput = GetStdHandle (STD_OUTPUT_HANDLE);
	
		// let QHOST hook in
		InitConProc (g_argc, g_argv);
	}
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
	INPUT_RECORD recs;
	DWORD numread, numevents, dummy;
	int ch;

	if (!dedicated || !dedicated->value)
		return NULL;

	for ( ;; )
	{
		if (!GetNumberOfConsoleInputEvents (hinput, &numevents))
			Com_FatalErrorf("Error getting # of console events");

		if (numevents == 0)
			break;

		if (!ReadConsoleInput(hinput, &recs, 1, &numread))
			Com_FatalErrorf("Error reading console input");

		if (numread != 1)
			Com_FatalErrorf("Couldn't read console input");

		if (recs.EventType == KEY_EVENT)
		{
			if (!recs.Event.KeyEvent.bKeyDown)
			{
				ch = recs.Event.KeyEvent.uChar.AsciiChar;

				switch (ch)
				{
					case '\r':
						WriteFile(houtput, "\r\n", 2, &dummy, NULL);	

						if (console_textlen)
						{
							console_text[console_textlen] = 0;
							console_textlen = 0;
							return console_text;
						}
						break;

					case '\b':
						if (console_textlen)
						{
							console_textlen--;
							WriteFile(houtput, "\b \b", 3, &dummy, NULL);	
						}
						break;

					default:
						if (ch >= ' ')
						{
							if (console_textlen < sizeof(console_text)-2)
							{
								WriteFile(houtput, &ch, 1, &dummy, NULL);	
								console_text[console_textlen] = ch;
								console_textlen++;
							}
						}

						break;

				}
			}
		}
	}

	return NULL;
}


/*
================
Sys_ConsoleOutput

Print text to the dedicated console
================
*/
void Sys_ConsoleOutput (const char *string)
{
	DWORD	dummy;
	char	text[256];

	if (!dedicated || !dedicated->value)
		return;

	if (console_textlen)
	{
		text[0] = '\r';
		memset(&text[1], ' ', console_textlen);
		text[console_textlen+1] = '\r';
		text[console_textlen+2] = 0;
		WriteFile(houtput, text, console_textlen+2, &dummy, NULL);
	}

	WriteFile(houtput, string, strlen(string), &dummy, NULL);

	if (console_textlen)
		WriteFile(houtput, console_text, console_textlen, &dummy, NULL);
}


/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents( void )
{
	MSG msg;

	while ( PeekMessageW( &msg, NULL, 0, 0, PM_NOREMOVE ) )
	{
		if ( !GetMessageW( &msg, NULL, 0, 0 ) ) {
			Com_Quit( EXIT_SUCCESS );
		}
		sys_msg_time = msg.time;
		TranslateMessage( &msg );
		DispatchMessageW( &msg );
	}

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
	char *data = nullptr;

	if ( OpenClipboard( nullptr ) )
	{
		HANDLE clipData = GetClipboardData( CF_TEXT );

		if ( clipData )
		{
			char *clipText = (char *)GlobalLock( clipData );

			if ( clipText )
			{
				data = Z_CopyString( clipText );
				GlobalUnlock( clipData );
			}
		}

		CloseClipboard();
	}

	return data;
}

/*
===============================================================================

	WINDOWS CRAP

===============================================================================
*/

/*
=================
Sys_AppActivate
=================
*/
void Sys_AppActivate (void)
{
	ShowWindow ( cl_hwnd, SW_RESTORE);
	SetForegroundWindow ( cl_hwnd );
}

/*
========================================================================

	GAME DLL

========================================================================
*/

static HINSTANCE game_library;
static HINSTANCE cgame_library;

void Sys_UnloadGame()
{
	if ( !FreeLibrary( game_library ) ) {
		Com_FatalErrorf("FreeLibrary failed for game library" );
	}
	game_library = nullptr;
}

void Sys_UnloadCGame()
{
	if ( !FreeLibrary( cgame_library ) ) {
		Com_FatalErrorf("FreeLibrary failed for cgame library" );
	}
	cgame_library = nullptr;
}

typedef void *( *GetAPI_t ) ( void * );

static void *Sys_GetAPI( void *parms, HINSTANCE &instance, const char *gamename, const char *procname )
{
	char name[MAX_OSPATH];

	if ( instance ) {
		Com_FatalErrorf("Sys_GetAPI without Sys_Unload(C)Game" );
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
		instance = LoadLibraryA( name );
		if ( instance )
		{
			Com_DPrintf( "LoadLibrary (%s)\n", name );
			break;
		}
	}

	GetAPI_t GetGameAPI = (GetAPI_t)GetProcAddress( instance, procname );
	if ( !GetGameAPI )
	{
		Sys_UnloadGame();
		return nullptr;
	}

	return GetGameAPI( parms );
}

void *Sys_GetGameAPI( void *parms )
{
	return Sys_GetAPI( parms, game_library, "game.dll", "GetGameAPI" );
}

void *Sys_GetCGameAPI( void *parms )
{
	return Sys_GetAPI( parms, cgame_library, "cgame.dll", "GetCGameAPI" );
}

//=======================================================================

#if 0
static char whatevertest[]{ "Hello this is a very long string, whatever, blah blah, hey my name is jeff I am trypfjs nd ksd wdsld 566 ((8***7&&@@~~}}{{ ??>><< dsjdjosojd jsdj snncnk knsdnddks dksnkndiqhiqhishiqsh iqr857835 7811 $$$ ffmfmc cmc vnvnv ... /// ;;; '''' #### [[[]]] p---+++++**---++++, how much longer can i make this huh im just testing my rambling capabilities it's completely random what I type it could be anything at all i could start talking about half-life alpha stuff it's just a constnat stream of mind isn't that really cool huh? what are you doing dude, it's not funny I'm very amusing or not i dont know im not you i am the funneist person who ever lived just liek joker (2019) this is athreat and i am very dangerous I own 15 sword and a burrito taco machine, beat that bub." };

void Cmd_Testwhatever()
{
	int varthing;
	char test01[256]{ "Hello World!" };

	double start, end;

	start = Time_FloatMicroseconds();
	varthing = Q_strcasecmp( whatevertest, whatevertest );
	end = Time_FloatMicroseconds();
	Com_Printf( "Q_strcasecmp Took %f microseconds\n", end - start );
	start = Time_FloatMicroseconds();
	varthing = _stricmp( whatevertest, whatevertest );
	end = Time_FloatMicroseconds();
	Com_Printf( "_stricmp Took %f microseconds\n", end - start );
}
#endif

/*
==================
WinMain

==================
*/
HINSTANCE	g_hInstance;

int main(int argc, char **argv)
{
	int time, oldtime, newtime;

	// Make sure the CRT thinks we're a GUI app, this makes CRT asserts use a message box
	// rather than printing to stderr
	_set_app_type( _crt_gui_app );

	g_hInstance = GetModuleHandleW(NULL);

	g_argc = argc; g_argv = argv;

	// no abort/retry/fail errors
	// Slart: Was SetErrorMode
	SetThreadErrorMode(SEM_FAILCRITICALERRORS, NULL);

	Time_Init();

	Engine_Init (argc, argv);
	oldtime = Sys_Milliseconds ();

	if ( GetACP() == CP_UTF8 )
	{
		Com_DPrintf( "Using Windows UTF-8 codepage\n" );
	}

	/* main window message loop */
	while (1)
	{
		// if at a full screen console, don't update unless needed
		if (Minimized || (dedicated && dedicated->value) )
		{
			Sleep (1);
		}

		do
		{
			newtime = Sys_Milliseconds ();
			time = newtime - oldtime;
		} while (time < 1);

		Engine_Frame (time);

		oldtime = newtime;
	}

	// never gets here
	return TRUE;
}
