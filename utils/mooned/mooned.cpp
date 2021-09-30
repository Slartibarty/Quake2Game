/*
===================================================================================================

	Main source file for MoonEd

===================================================================================================
*/

#include "mooned_local.h"

// Windows stuff
#ifdef _WIN32
#include <process.h>
#include "../../core/sys_includes.h"
#include <ShlObj.h>
#endif

// Local
#include "r_public.h"

#include "mainwindow.h"
#include "consolewindow.h"

// This module
#include "mooned.h"

#if 0
class MoonEdApplication : public QApplication
{
	Q_OBJECT

protected:
	bool event( QEvent *event ) override
	{
		switch ( event->type() )
		{
		case EV_UPDATECBUF:
			Cbuf_Execute();
			return;
		}

		return QApplication::event( event );
	}

};
#endif

CON_COMMAND( version, "MoonEd version info.", 0 )
{
	Com_Print( APP_NAME " version " APP_VERSION " by Slartibarty\n" );
}

CON_COMMAND( quit, "Exits MoonEd.", 0 )
{
	QApplication::quit();
}

CVAR_CALLBACK( StyleCallback )
{
	if ( var->GetString()[0] != '\0' )
	{
		QApplication::setStyle( var->GetString() );
	}
}

static StaticCvar qt_style( "qt_style", "", 0, "The Qt style to use.", StyleCallback );

static void MoonEd_Init( int argc, char **argv )
{
	Cbuf_Init();
	Cmd_Init();
	Cvar_Init();

	Cvar_AddEarlyCommands( argc, argv );

	// OVERSIGHT: filesystem doesn't get the luxury of using cvars from cfg files
	FileSystem::Init();

	// add config files
	Cbuf_AddText( "exec moonedconfig.cfg\n" );
	Cbuf_Execute();

	// command line priority
	Cvar_AddEarlyCommands( argc, argv );

	// add + commands, will be fired after qt init
	Cbuf_AddLateCommands( argc, argv );
}

static void MoonEd_Shutdown()
{
	FileSystem::Shutdown();
	Cvar_Shutdown();
	Cmd_Shutdown();
	Cbuf_Shutdown();
}

static void MoonEd_FusionDark()
{
	// modify palette to dark
	QPalette darkPalette = QApplication::palette();

	darkPalette.setColor( QPalette::Window, QColor( 53, 53, 53 ) );
	darkPalette.setColor( QPalette::WindowText, Qt::white );
	darkPalette.setColor( QPalette::Disabled, QPalette::WindowText, QColor( 127, 127, 127 ) );
	darkPalette.setColor( QPalette::Base, QColor( 42, 42, 42 ) );
	darkPalette.setColor( QPalette::AlternateBase, QColor( 66, 66, 66 ) );
	darkPalette.setColor( QPalette::ToolTipBase, Qt::white );
	darkPalette.setColor( QPalette::ToolTipText, Qt::white );
	darkPalette.setColor( QPalette::Text, Qt::white );
	darkPalette.setColor( QPalette::Disabled, QPalette::Text, QColor( 127, 127, 127 ) );
	darkPalette.setColor( QPalette::Dark, QColor( 35, 35, 35 ) );
	darkPalette.setColor( QPalette::Shadow, QColor( 20, 20, 20 ) );
	darkPalette.setColor( QPalette::Button, QColor( 53, 53, 53 ) );
	darkPalette.setColor( QPalette::ButtonText, Qt::white );
	darkPalette.setColor( QPalette::Disabled, QPalette::ButtonText, QColor( 127, 127, 127 ) );
	darkPalette.setColor( QPalette::BrightText, Qt::red );
	darkPalette.setColor( QPalette::Link, QColor( 42, 130, 218 ) );
	darkPalette.setColor( QPalette::Highlight, QColor( 42, 130, 218 ) );
	darkPalette.setColor( QPalette::Disabled, QPalette::Highlight, QColor( 80, 80, 80 ) );
	darkPalette.setColor( QPalette::HighlightedText, Qt::white );
	darkPalette.setColor( QPalette::Disabled, QPalette::HighlightedText, QColor( 127, 127, 127 ) );

	QApplication::setPalette( darkPalette );
}

MainWindow *GetMainWnd()
{
	return g_pMainWindow;
}

ConsoleWindow *GetConsoleWnd()
{
	return g_pConsoleWindow;
}

int main( int argc, char **argv )
{
#ifdef _WIN32
	// Make sure the CRT thinks we're a GUI app, this makes CRT asserts use a message box
	// rather than printing to stderr
	_set_app_type( _crt_gui_app );

#ifdef Q_MEM_DEBUG
	int dbgFlags = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
	dbgFlags |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag( dbgFlags );
#endif
#endif

	MoonEd_Init( argc, argv );

	Cvar_CallCallback( &qt_style );
	QApplication::setOrganizationName( APP_COMPANYNAME );
	QApplication::setApplicationName( APP_NAME );
	QApplication::setApplicationVersion( APP_VERSION );

	// Add resource search paths
	// Unfortunately the first search path with the folder gets priority for all
	// No real biggie right now
	QDir::addSearchPath( "tools", FileSystem::RelativePathToAbsolutePath( "tools" ) );
	QDir::addSearchPath( "toolsimages", FileSystem::RelativePathToAbsolutePath( "tools/images" ) );

	QApplication app( argc, argv );

	//MoonEd_FusionDark();

	QApplication::setWindowIcon( QIcon( "toolsimages:/mooned/mooned.ico" ) );

	g_pMainWindow = new MainWindow;
	g_pConsoleWindow = new ConsoleWindow;

	g_pMainWindow->show();
	g_pConsoleWindow->show();

	// FIXME: GOTTA FIND A WAY INTO QT'S EVENT LOOP!
	Cbuf_Execute();

	int result = QApplication::exec();

	// TODO: THERE IS A QT FUNCTION FOR ONEXIT, DO THIS THERE!!!
	delete g_pConsoleWindow;
	delete g_pMainWindow;

	R_Shutdown();

	MoonEd_Shutdown();

	return result;
}

/*
=======================================
	Log implementations
=======================================
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

	exit( EXIT_FAILURE );
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

	Con_Print( newMsg );

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
