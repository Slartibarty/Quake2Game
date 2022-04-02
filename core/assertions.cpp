/*
===================================================================================================

	Assertion Handler

	The user has to press a button to submit to g_assertDisables, so thread safety is no concern

===================================================================================================
*/

#include "core.h"

#include <vector>

#ifdef _WIN32
#include "sys_includes.h"
#include "assert_dialog.h"
#endif

#include "assertions.h"

#define MAX_ASSERT_STRING_LENGTH 256

struct dialogInitInfo_t
{
	const char *message;
	const char *expression;
	const char *file;
	uint32 line;
};

struct assertDisable_t
{
	const char *file;
	uint32 line;
};

std::vector<assertDisable_t> g_assertDisables;

static bool g_bIgnoreAllAsserts;

//-------------------------------------------------------------------------------------------------

#ifdef _WIN32

static INT_PTR CALLBACK AssertDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	static thread_local dialogInitInfo_t *s_pInfo;

	wchar_t wideMessage[MAX_ASSERT_STRING_LENGTH];
	wchar_t wideExpression[MAX_ASSERT_STRING_LENGTH];
	wchar_t wideFile[MAX_PATH];

	switch( uMsg )
	{
		case WM_INITDIALOG:
		{
			s_pInfo = reinterpret_cast<dialogInitInfo_t *>( lParam );

			Sys_UTF8ToUTF16( s_pInfo->message,    static_cast<strlen_t>( strlen( s_pInfo->message ) ) + 1,    wideMessage,    MAX_ASSERT_STRING_LENGTH );
			Sys_UTF8ToUTF16( s_pInfo->expression, static_cast<strlen_t>( strlen( s_pInfo->expression ) ) + 1, wideExpression, MAX_ASSERT_STRING_LENGTH );
			Sys_UTF8ToUTF16( s_pInfo->file,       static_cast<strlen_t>( strlen( s_pInfo->file ) ) + 1,       wideFile,       MAX_PATH );

			SetDlgItemTextW( hDlg, IDC_MESSAGE, wideMessage );
			SetDlgItemTextW( hDlg, IDC_EXPRESSION, wideExpression );
			SetDlgItemTextW( hDlg, IDC_FILE, wideFile );
			SetDlgItemInt( hDlg, IDC_LINE, s_pInfo->line, false );
		
			// Centre the dialog on screen
			RECT rcDlg, rcDesktop;
			GetWindowRect( hDlg, &rcDlg );
			GetWindowRect( GetDesktopWindow(), &rcDesktop );
			SetWindowPos(
				hDlg,
				HWND_TOP,
				((rcDesktop.right-rcDesktop.left) - (rcDlg.right-rcDlg.left)) / 2,
				((rcDesktop.bottom-rcDesktop.top) - (rcDlg.bottom-rcDlg.top)) / 2,
				0,
				0,
				SWP_NOSIZE );

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch( LOWORD( wParam ) )
			{
				case IDC_IGNORE_ONCE:
				{
					EndDialog( hDlg, FALSE );
					return TRUE;
				}

				case IDC_IGNORE_ALWAYS:
				{
					g_assertDisables.push_back( { s_pInfo->file, s_pInfo->line } );

					EndDialog( hDlg, FALSE );
					return TRUE;
				}

				case IDC_IGNORE_ALL:
				{
					g_bIgnoreAllAsserts = true;

					EndDialog( hDlg, FALSE );
					return TRUE;
				}

				case IDC_BREAK:
				{
					EndDialog( hDlg, TRUE );
					return TRUE;
				}
			}

			case WM_KEYDOWN:
			{
				if ( wParam == VK_ESCAPE )
				{
					EndDialog( hDlg, FALSE );
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

#endif

void AssertionFailed( const char *message, const char *expression, const char *file, uint32 line )
{
	// Are we ignoring all asserts?
	if ( g_bIgnoreAllAsserts )
	{
		return;
	}

	// Are we ignoring *this* assert?
	for ( const assertDisable_t &disable : g_assertDisables )
	{
		if ( Q_strcmp( disable.file, file ) == 0 && disable.line == line )
		{
			return;
		}
	}

	// If we have no message, set it to a default now
	// expression and file are guaranteed to have a value
	if ( !message ) { message = "N/A"; }

#ifdef _WIN32
	// Trim the filename down to "code", this obviously only works if the code is in a folder called "code"
	const char *trimmedFile = Q_strstr( file, "code" );
	if ( trimmedFile )
	{
		trimmedFile += 5; // length of "code\\"
	}
	else
	{
		trimmedFile = file;
	}

	dialogInitInfo_t info;
	info.message = message;
	info.expression = expression;
	info.file = trimmedFile;
	info.line = line;

	HINSTANCE instance = GetModuleHandleW( nullptr );

	bool shouldBreak = static_cast<bool>( DialogBoxParamW( instance, MAKEINTRESOURCEW( IDD_ASSERT_DIALOG ), nullptr, AssertDialogProc, reinterpret_cast<LPARAM>( &info ) ) );

	if ( shouldBreak && IsDebuggerPresent() )
	{
		__debugbreak();
	}
#endif
}

#ifdef _WIN32

//
// Override the CRT's assertion handler with our own
//
void _wassert( _In_z_ wchar_t const *message, _In_z_ wchar_t const *file, _In_ unsigned line )
{
	char narrowMessage[MAX_ASSERT_STRING_LENGTH];
	char narrowFile[MAX_ASSERT_STRING_LENGTH];

	Sys_UTF16toUTF8( message, static_cast<strlen_t>( wcslen( message ) ) + 1, narrowMessage, MAX_ASSERT_STRING_LENGTH );
	Sys_UTF16toUTF8( file,    static_cast<strlen_t>( wcslen( file ) ) + 1,    narrowFile,    MAX_ASSERT_STRING_LENGTH );

	AssertionFailed( nullptr, narrowMessage, narrowFile, line );
}

#endif
