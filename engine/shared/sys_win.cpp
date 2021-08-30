// sys_win.c

#include "engine.h"

#include <process.h>

#include <vector>
#include <string>

#include "../../core/sys_includes.h"
#include <ShlObj.h>
#include <VersionHelpers.h>
#include "../client/winquake.h"			// Hack?

#ifdef Q_USE_STEAM
#include "steam/steam_api.h"
#endif

#include "sys.h"

bool			g_activeApp, g_minimized;

static HANDLE	hinput, houtput;

uint			sys_frameTime;
int				curtime;

/*
===================================================================================================

	System I/O

===================================================================================================
*/

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

//=================================================================================================

// Called by Com_Init
void Sys_Init( int argc, char **argv )
{
	if ( dedicated && dedicated->GetBool() )
	{
		if ( !AllocConsole() ) {
			Com_FatalError( "Couldn't create dedicated server console\n" );
		}
		hinput = GetStdHandle( STD_INPUT_HANDLE );
		houtput = GetStdHandle( STD_OUTPUT_HANDLE );

		// let QHOST hook in
		InitConProc( argc, argv );
	}
}

// MSDN says OutputDebugStringW converts the unicode string to the system codepage
// So just directly use the A version
void Sys_OutputDebugString( const char *msg )
{
#ifndef Q_RETAIL
	if ( IsDebuggerPresent() )
	{
		OutputDebugStringA( msg );
	}
#endif
}

[[noreturn]]
void Sys_Quit( int code )
{
	// Shut down very low level stuff
	Sys_SecretShutdown();

	exit( code );
}

[[noreturn]]
void Sys_Error( const platChar_t *mainInstruction, const char *msg )
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

	Sys_Quit( EXIT_FAILURE );
}

//=================================================================================================

/*
================
Sys_CopyProtect
================
*/
void Sys_CopyProtect()
{

}

//=================================================================================================

// Called by Com_Shutdown
void Sys_Shutdown()
{
	if ( dedicated && dedicated->GetBool() )
	{
		FreeConsole();

		// shut down QHOST hooks if necessary
		DeinitConProc();
	}
}

static char	console_text[256];
static int	console_textlen;

char *Sys_ConsoleInput()
{
	INPUT_RECORD recs;
	DWORD numread, numevents, dummy;
	int ch;

	if (!dedicated || !dedicated->GetBool())
		return NULL;

	for ( ;; )
	{
		if (!GetNumberOfConsoleInputEvents (hinput, &numevents))
			Com_FatalErrorf("Error getting # of console events\n");

		if (numevents == 0)
			break;

		if (!ReadConsoleInput(hinput, &recs, 1, &numread))
			Com_FatalErrorf("Error reading console input\n");

		if (numread != 1)
			Com_FatalErrorf("Couldn't read console input\n");

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

// Print text to the dedicated console
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

char *Sys_GetClipboardData()
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

// TODO: Remove?
void Sys_AppActivate()
{
	ShowWindow( cl_hwnd, SW_RESTORE );
	SetForegroundWindow( cl_hwnd );
}

void Sys_FileOpenDialog(
	std::string &filename,
	const platChar_t *title,
	const filterSpec_t *supportedTypes,
	const uint numTypes,
	const uint defaultIndex )
{
	char filenameBuffer[LONG_MAX_PATH];

	IFileOpenDialog *pDialog;
	HRESULT hr = CoCreateInstance(
		CLSID_FileOpenDialog,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS( &pDialog ) );

	const COMDLG_FILTERSPEC *filterSpec = reinterpret_cast<const COMDLG_FILTERSPEC *>( supportedTypes );

	if ( SUCCEEDED( hr ) )
	{
		FILEOPENDIALOGOPTIONS flags;
		hr = pDialog->GetOptions( &flags ); assert( SUCCEEDED( hr ) );
		flags |= FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR;
		hr = pDialog->SetOptions( flags ); assert( SUCCEEDED( hr ) );
		hr = pDialog->SetFileTypes( numTypes, filterSpec ); assert( SUCCEEDED( hr ) );

		hr = pDialog->SetTitle( title ); assert( SUCCEEDED( hr ) );

		hr = pDialog->SetFileTypeIndex( defaultIndex ); assert( SUCCEEDED( hr ) );
		//hr = pDialog->SetDefaultExtension( L"dds" ); assert( SUCCEEDED( hr ) );

		hr = pDialog->Show( cl_hwnd );
		if ( SUCCEEDED( hr ) )
		{
			IShellItem *pItem;
			hr = pDialog->GetResult( &pItem );
			if ( SUCCEEDED( hr ) )
			{
				LPWSTR pName;
				pItem->GetDisplayName( SIGDN_FILESYSPATH, &pName );
				Sys_UTF16toUTF8( pName, static_cast<int>( wcslen( pName ) + 1 ), filenameBuffer, sizeof( filenameBuffer ) );
				CoTaskMemFree( pName );
				Str_FixSlashes( filenameBuffer );
				filename.assign( filenameBuffer );

				pItem->Release();
			}
		}
		pDialog->Release();
	}
}

void Sys_FileOpenDialogMultiple(
	std::vector<std::string> &filenames,
	const platChar_t *title,
	const filterSpec_t *supportedTypes,
	const uint numTypes,
	const uint defaultIndex )
{
	char filenameBuffer[LONG_MAX_PATH];

	IFileOpenDialog *pDialog;
	HRESULT hr = CoCreateInstance(
		CLSID_FileOpenDialog,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS( &pDialog ) );

	const COMDLG_FILTERSPEC *filterSpec = reinterpret_cast<const COMDLG_FILTERSPEC *>( supportedTypes );

	if ( SUCCEEDED( hr ) )
	{
		FILEOPENDIALOGOPTIONS flags;
		hr = pDialog->GetOptions( &flags ); assert( SUCCEEDED( hr ) );
		flags |= FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR | FOS_ALLOWMULTISELECT;
		hr = pDialog->SetOptions( flags ); assert( SUCCEEDED( hr ) );
		hr = pDialog->SetFileTypes( numTypes, filterSpec ); assert( SUCCEEDED( hr ) );

		hr = pDialog->SetTitle( title ); assert( SUCCEEDED( hr ) );

		hr = pDialog->SetFileTypeIndex( defaultIndex ); assert( SUCCEEDED( hr ) );
		//hr = pDialog->SetDefaultExtension( L"dds" ); assert( SUCCEEDED( hr ) );

		hr = pDialog->Show( cl_hwnd );
		if ( SUCCEEDED( hr ) )
		{
			IShellItemArray *pArray;
			hr = pDialog->GetResults( &pArray );
			if ( SUCCEEDED( hr ) )
			{
				DWORD numItems;
				pArray->GetCount( &numItems );
				filenames.reserve( numItems );
				for ( DWORD i = 0; i < numItems; ++i )
				{
					IShellItem *pItem;
					pArray->GetItemAt( i, &pItem );
					LPWSTR pName;
					pItem->GetDisplayName( SIGDN_FILESYSPATH, &pName );
					Sys_UTF16toUTF8( pName, static_cast<int>( wcslen( pName ) + 1 ), filenameBuffer, sizeof( filenameBuffer ) );
					CoTaskMemFree( pName );
					Str_FixSlashes( filenameBuffer );
					filenames.push_back( filenameBuffer );
				}
				pArray->Release();
			}
		}
		pDialog->Release();
	}
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
		Com_FatalError( "FreeLibrary failed for game library\n" );
	}
	game_library = nullptr;
}

void Sys_UnloadCGame()
{
	if ( !FreeLibrary( cgame_library ) ) {
		Com_FatalError( "FreeLibrary failed for cgame library\n" );
	}
	cgame_library = nullptr;
}

typedef void *( *GetAPI_t ) ( void * );

static void *Sys_GetAPI( void *parms, HINSTANCE &instance, const char *gamename, const char *procname )
{
	if ( instance ) {
		Com_FatalError( "Sys_GetAPI without Sys_Unload(C)Game\n" );
	}

	char fullPath[MAX_OSPATH];
	if ( !FileSystem::FindPhysicalFile( gamename, fullPath, sizeof( fullPath ) ) )
	{
		return nullptr;
	}

	instance = LoadLibraryA( fullPath );
	if ( !instance )
	{
		return nullptr;
	}

	GetAPI_t GetGameAPI = (GetAPI_t)GetProcAddress( instance, procname );
	if ( !GetGameAPI )
	{
		FreeLibrary( instance );
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
	MSG msg;
	int frameTime, oldTime, newTime;

	// Make sure the CRT thinks we're a GUI app, this makes CRT asserts use a message box
	// rather than printing to stderr
	_set_app_type( _crt_gui_app );

	// no abort/retry/fail errors
	// Slart: wtf is this?
	//SetErrorMode( SEM_FAILCRITICALERRORS );

#ifdef Q_MEM_DEBUG
	int dbgFlags = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
	dbgFlags |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag( dbgFlags );
#endif

	if ( !IsWindows7SP1OrGreater() )
	{
		// Just deny anyone running win7sp0 or prior from running the game
		TaskDialog(
			nullptr,
			nullptr,
			TEXT( ENGINE_VERSION ),
			L"Incompatible System",
			L"You must be running Windows 7 SP1 or newer to play this game",
			TDCBF_OK_BUTTON,
			TD_ERROR_ICON,
			nullptr );

		exit( EXIT_FAILURE );
	}

	// Init very low level stuff
	Sys_SecretInit();

	Time_Init();

	Sys_InitVidModes();

	Com_Init( argc, argv );

	if ( GetACP() == CP_UTF8 ) {
		Com_DPrint( "Using Windows UTF-8 codepage\n" );
	}

	assert( dedicated );

	oldTime = Sys_Milliseconds();

	// main loop
	while ( 1 )
	{
		// if at a full screen console, don't update unless needed
		if ( !g_activeApp || dedicated->GetBool() )
		{
			Sleep( 1 );
		}

		// process all queued messages
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
			newTime = Sys_Milliseconds();
			frameTime = newTime - oldTime;
		} while ( frameTime < 1 );

		// set legacy
		sys_frameTime = newTime;
		curtime = newTime;

		Com_Frame( frameTime );

		oldTime = newTime;
	}

	// unreachable
}
