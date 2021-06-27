// sys_win.c

#include "engine.h"

#include <process.h>

#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#include "../client/winquake.h"	// Hack?

#include "sys.h"

bool			g_activeApp, g_minimized;

static HANDLE	hinput, houtput;

unsigned		sys_frame_time;
int				curtime;

int		g_argc;
char	**g_argv;

/*
===================================================================================================

	System I/O

===================================================================================================
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
	Sys_OutputDebugString( msg );

	/*TaskDialog(
		nullptr,
		nullptr,
		L"Engine Error",
		L"The game crashed!",
		L"Bruh",
		TDCBF_OK_BUTTON,
		TD_ERROR_ICON,
		nullptr );*/

	MessageBoxA( nullptr, msg, "Engine Error", MB_OK | MB_ICONERROR | MB_TOPMOST );

	Sys_Quit( EXIT_FAILURE );
}

[[noreturn]]
void Sys_Quit( int code )
{
	// SLARTHACK: FIXME!!!!
//	if ( dedicated && dedicated->value )
//		FreeConsole();

	// shut down QHOST hooks if necessary
	DeinitConProc();

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
	if (dedicated->GetBool())
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

	if (!dedicated || !dedicated->GetBool())
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

	if (!dedicated || !dedicated->GetBool())
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
				data = Mem_CopyString( clipText );
				GlobalUnlock( clipData );
			}
		}

		CloseClipboard();
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

static constexpr size_t DefaultNumModes = 64;

std::vector<vidMode_t> s_vidModes;

static int SortModes( const void *p1, const void *p2 )
{
	const vidMode_t *m1 = reinterpret_cast<const vidMode_t *>( p1 );
	const vidMode_t *m2 = reinterpret_cast<const vidMode_t *>( p2 );

	if ( m1->width > m2->width ) {
		return 1;
	}
	if ( m1->width < m2->width ) {
		return -1;
	}

	if ( m1->width == m2->width && m1->height > m2->height ) {
		return 1;
	}
	if ( m1->width == m2->width && m1->height < m2->height ) {
		return -1;
	}

	return 0;
}

static void Sys_InitVidModes()
{
	DEVMODEW dm;
	dm.dmSize = sizeof( dm );
	dm.dmDriverExtra = 0;

	DWORD lastWidth = 0, lastHeight = 0;

	int internalNum = 0;				// for EnumDisplaySettings

	s_vidModes.reserve( DefaultNumModes );

	while ( EnumDisplaySettingsW( nullptr, internalNum, &dm ) != FALSE )
	{
		if ( dm.dmPelsWidth == lastWidth && dm.dmPelsHeight == lastHeight )
		{
			++internalNum;
			continue;
		}

		lastWidth = dm.dmPelsWidth;
		lastHeight = dm.dmPelsHeight;

		vidMode_t &mode = s_vidModes.emplace_back();

		mode.width = static_cast<int>( dm.dmPelsWidth );
		mode.height = static_cast<int>( dm.dmPelsHeight );

		++internalNum;
	}

	qsort( s_vidModes.data(), s_vidModes.size(), sizeof( vidMode_t ), SortModes );
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
	ShowWindow ( cl_hwnd, SW_RESTORE);
	SetForegroundWindow ( cl_hwnd );
}

/*
===================================================================================================

	Game DLLs

===================================================================================================
*/

static HINSTANCE game_library;
static HINSTANCE cgame_library;

void Sys_UnloadGame()
{
	if ( !FreeLibrary( game_library ) ) {
		Com_FatalError( "FreeLibrary failed for game library" );
	}
	game_library = nullptr;
}

void Sys_UnloadCGame()
{
	if ( !FreeLibrary( cgame_library ) ) {
		Com_FatalError( "FreeLibrary failed for cgame library" );
	}
	cgame_library = nullptr;
}

typedef void *( *GetAPI_t ) ( void * );

static void *Sys_GetAPI( void *parms, HINSTANCE &instance, const char *gamename, const char *procname )
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
========================
main
========================
*/
int main( int argc, char **argv )
{
	int time, oldtime, newtime;

	// Make sure the CRT thinks we're a GUI app, this makes CRT asserts use a message box
	// rather than printing to stderr
	_set_app_type( _crt_gui_app );

#ifdef Q_MEM_DEBUG
	int dbgFlags = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
	dbgFlags |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag( dbgFlags );
#endif

	g_argc = argc; g_argv = argv;

	// no abort/retry/fail errors
	SetErrorMode( SEM_FAILCRITICALERRORS );

	Time_Init();

	double initStart = Time_FloatMilliseconds();

	Sys_InitVidModes();

	Engine_Init( argc, argv );

	if ( GetACP() == CP_UTF8 ) {
		Com_DPrint( "Using Windows UTF-8 codepage\n" );
	}

	oldtime = Sys_Milliseconds();

	MSG msg;

	// main loop
	while ( 1 )
	{
		// if at a full screen console, don't update unless needed
		if ( !g_activeApp || ( dedicated && dedicated->GetBool() ) )
		{
			Sleep( 1 );
		}

		// proces all queued messages
		while ( PeekMessageW( &msg, nullptr, 0, 0, PM_REMOVE ) )
		{
			if ( msg.message == WM_QUIT )
			{
				Com_Quit( EXIT_SUCCESS );
			}
			TranslateMessage( &msg );
			DispatchMessageW( &msg );
		}

		do
		{
			newtime = Sys_Milliseconds();
			time = newtime - oldtime;
		} while ( time < 1 );

		sys_frame_time = newtime;
		curtime = newtime;

		Engine_Frame( time );

		oldtime = newtime;
	}

	// unreachable
	return 0;
}
