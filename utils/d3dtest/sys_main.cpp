/*
===================================================================================================

	Where Engine 3 begins

	This is a common module!

===================================================================================================
*/

#include "engine.h"

#include <process.h>

#include "../../core/sys_includes.h"
#include <ShlObj.h>

/*
===================================================================================================
	Logging Implementations
===================================================================================================
*/

[[noreturn]]
static void Sys_Error( const platChar_t *mainInstruction, const char *msg )
{
	// Trim the newline
	char newMsg[MAX_PRINT_MSG];
	Q_strcpy_s( newMsg, msg );
	char *lastNewline = newMsg + strlen( newMsg ) - 1;
	if ( *lastNewline == '\n' )
	{
		*lastNewline = '\0';
	}

	// Use old msg, we need the newline
	Sys_OutputDebugString( msg );

	wchar_t reason[MAX_PRINT_MSG];
	Sys_UTF8ToUTF16( newMsg, Q_strlen( newMsg ) + 1, reason, countof( reason ) );

	const platChar_t *windowTitle = FileSystem::ModInfo::GetWindowTitle();

	TaskDialog(
		nullptr,
		nullptr,
		windowTitle,
		mainInstruction,
		reason,
		TDCBF_OK_BUTTON,
		TD_ERROR_ICON,
		nullptr );

	if ( IsDebuggerPresent() )
	{
		__debugbreak();
	}

	Sys_Quit( EXIT_FAILURE );
}

/*
========================
Com_Print

Both client and server can use this, and it will output
to the apropriate place.
========================
*/

static void CopyAndStripColorCodes( char *dest, strlen_t destSize, const char *src )
{
	const char *last;
	int c;

	last = dest + destSize - 1;
	while ( ( dest < last ) && ( c = *src ) != 0 ) {
		if ( src[0] == C_COLOR_ESCAPE && src + 1 != last && IsColorIndex( src[1] ) ) {
			src++;
		}
		else {
			*dest++ = c;
		}
		src++;
	}
	*dest = '\0';
}

void Com_Print( const char *msg )
{
	char newMsg[MAX_PRINT_MSG];

	// create a copy of the msg for places that don't want the colour codes
	CopyAndStripColorCodes( newMsg, sizeof( newMsg ), msg );

	SysConsole_Print( newMsg );

	Sys_OutputDebugString( newMsg );
}

void Com_Printf( _Printf_format_string_ const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_Print( msg );
}

/*
========================
Com_DPrint

A Com_Print that only shows up if the "developer" cvar is set
========================
*/

void Com_DPrint( const char *msg )
{
	Com_Printf( S_COLOR_YELLOW "%s", msg );
}

void Com_DPrintf( _Printf_format_string_ const char *fmt, ... )
{
	va_list argptr;
	char msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_Printf( S_COLOR_YELLOW "%s", msg );
}

/*
========================
Com_Error

The peaceful option
Equivalent to an old Com_Error( ERR_DROP )
========================
*/

[[noreturn]]
void Com_Error( const char *msg )
{
	Com_FatalError( msg );
}

[[noreturn]]
void Com_Errorf( _Printf_format_string_ const char *fmt, ... )
{
	va_list argptr;
	char msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_FatalError( msg );
}

/*
========================
Com_FatalError

The nuclear option
Equivalent to an old Com_Error( ERR_FATAL )
Kills the server, kills the client, shuts the engine down and quits the program
========================
*/

[[noreturn]]
void Com_FatalError( const char *msg )
{
	Sys_Error( PLATTEXT( "Engine Error" ), msg );
}

[[noreturn]]
void Com_FatalErrorf( _Printf_format_string_ const char *fmt, ... )
{
	va_list argptr;
	char msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_FatalError( msg );
}

//=================================================================================================

static void InitSteamStuff()
{
#ifdef Q_USE_STEAM
	if ( SteamAPI_RestartAppIfNecessary( Q_STEAM_APPID ) )
	{
		exit( EXIT_FAILURE );
	}

	if ( !SteamAPI_Init() )
	{
		TaskDialog(
			nullptr,
			nullptr,
			TEXT( ENGINE_VERSION ),
			L"Failed to initialise Steam",
			L"SteamAPI initialisation failed. Is Steam running?",
			TDCBF_OK_BUTTON,
			TD_ERROR_ICON,
			nullptr );

		exit( EXIT_FAILURE );
	}
#endif
}

static void ShutdownSteamStuff()
{
#ifdef Q_USE_STEAM
	SteamAPI_Shutdown();
#endif
}

// Called in main()
static void Sys_SecretInit()
{
	InitSteamStuff();

	CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );
}

// Called in Sys_Quit
static void Sys_SecretShutdown()
{
	CoUninitialize();

	// If Steam needed COM it'd call it itself
	ShutdownSteamStuff();
}

[[noreturn]]
void Sys_Quit( int code )
{
	// Shut down very low level stuff
	Sys_SecretShutdown();

	exit( code );
}

//=================================================================================================

int main( int argc, char **argv )
{
	// Make sure the CRT thinks we're a GUI app, this makes CRT asserts use a message box
	// rather than printing to stderr
	_set_app_type( _crt_gui_app );
#ifdef Q_MEM_DEBUG
	_CrtSetDbgFlag( _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) | _CRTDBG_LEAK_CHECK_DF );
#endif

	// Init very low level stuff
	Sys_SecretInit();

	Time_Init();

	Com_Init( argc, argv );

	double deltaTime, oldTime, newTime;
	MSG msg;

	oldTime = Time_FloatMilliseconds();

	// Show the main window just before we start doing things
	extern void Sys_ShowMainWindow(); // LOL hack
	Sys_ShowMainWindow();

	// Main loop
	while ( 1 )
	{
		// Process all queued messages
		while ( PeekMessageW( &msg, nullptr, 0, 0, PM_REMOVE ) )
		{
			if ( msg.message == WM_QUIT )
			{
				// Perform a graceful quit
				Com_Quit( EXIT_SUCCESS );
			}
 			TranslateMessage( &msg );
			DispatchMessageW( &msg );
		}

		newTime = Time_FloatMilliseconds();
		deltaTime = newTime - oldTime;

		Com_Frame( static_cast<float>( MS2SEC( deltaTime ) ) );

		oldTime = newTime;
	}
	
	// Unreachable
	return 0;
}
