/*
===================================================================================================

	Platform specific garbage functions

===================================================================================================
*/

#include "core.h"

#include <io.h>

#include "sys_includes.h"

/*
=======================================
	The hunk allocator, which is not
	long for this world
=======================================
*/

static int		hunkCount;

static void *	memBase;
static size_t	hunkMaxSize;
static size_t	curSize;

void *Hunk_Begin( size_t maxSize )
{
	// reserve a huge chunk of memory, but don't commit any yet
	curSize = 0;
	hunkMaxSize = maxSize;

	memBase = VirtualAlloc( nullptr, maxSize, MEM_RESERVE, PAGE_NOACCESS );

	if ( !memBase ) {
		Com_FatalError( "VirtualAlloc reserve failed" );
	}

	return memBase;
}

void *Hunk_Alloc( size_t size )
{
	void *buf;

	// round to cacheline
	size = ( size + 31 ) & ~31;

	// commit pages as needed
	buf = VirtualAlloc( memBase, curSize + size, MEM_COMMIT, PAGE_READWRITE );
	if ( !buf )
	{
		// leaks memory
		FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, GetLastError(), MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), (LPSTR)&buf, 0, nullptr );
		Com_FatalErrorf( "VirtualAlloc commit failed\n%s", (char *)buf );
	}

	curSize += size;
	if ( curSize > hunkMaxSize ) {
		Com_FatalError( "Hunk_Alloc overflow" );
	}

	return (void *)( (byte *)memBase + curSize - size );
}

size_t Hunk_End()
{
	// free the remaining unused virtual memory
#if 0
	void *buf;

	// write protect it
	buf = VirtualAlloc( membase, cursize, MEM_COMMIT, PAGE_READONLY );
	if ( !buf )
		Com_FatalErrorf( "VirtualAlloc commit failed" );
#endif

	++hunkCount;
	//Com_Printf( "hunkCount: %i\n", hunkCount );

	return curSize;
}

void Hunk_Free( void *base )
{
	if ( base ) {
		VirtualFree( base, 0, MEM_RELEASE );
	}

	--hunkCount;
}

/*
=======================================
	Miscellaneous

	TODO: Are there any performance concerns related
	to using the ANSI versions of these functions?
	Should we widen the inputs ourselves?
=======================================
*/

void Sys_CopyFile( const char *src, const char *dst )
{
	CopyFileA( src, dst, FALSE );
}

void Sys_DeleteFile( const char *filename )
{
	DeleteFileA( filename );
}

void Sys_CreateDirectory( const char *path )
{
	CreateDirectoryA( path, nullptr );
}

void Sys_GetWorkingDirectory( char *path, uint length )
{
	GetCurrentDirectoryA( length, path );
	Str_FixSlashes( path );
}

/*
=======================================
	Timing
=======================================
*/

static int64	startTime;
static double	toSeconds;
static double	toMilliseconds;
static double	toMicroseconds;

void Time_Init()
{
	QueryPerformanceFrequency( (LARGE_INTEGER *)&startTime );
	toSeconds = 1e0 / startTime;
	toMilliseconds = 1e3 / startTime;
	toMicroseconds = 1e6 / startTime;

	QueryPerformanceCounter( (LARGE_INTEGER *)&startTime );
}

double Time_FloatSeconds()
{
	int64 currentTime;

	QueryPerformanceCounter( (LARGE_INTEGER *)&currentTime );

	return ( currentTime - startTime ) * toSeconds;
}

double Time_FloatMilliseconds()
{
	int64 currentTime;

	QueryPerformanceCounter( (LARGE_INTEGER *)&currentTime );

	return ( currentTime - startTime ) * toMilliseconds;
}

double Time_FloatMicroseconds()
{
	int64 currentTime;

	QueryPerformanceCounter( (LARGE_INTEGER *)&currentTime );

	return ( currentTime - startTime ) * toMicroseconds;
}

// Practically useless
/*int64 Time_Seconds()
{
	int64 currentTime;

	QueryPerformanceCounter( (LARGE_INTEGER *)&currentTime );

	return llround( ( currentTime - startTime ) * toSeconds );
}*/

int64 Time_Milliseconds()
{
	int64 currentTime;

	QueryPerformanceCounter( (LARGE_INTEGER *)&currentTime );

	return llround( ( currentTime - startTime ) * toMilliseconds );
}

int64 Time_Microseconds()
{
	int64 currentTime;

	QueryPerformanceCounter( (LARGE_INTEGER *)&currentTime );

	return llround( ( currentTime - startTime ) * toMicroseconds );
}

// LEGACY
int Sys_Milliseconds()
{
	int64 currentTime;

	QueryPerformanceCounter( (LARGE_INTEGER *)&currentTime );

	return lround( ( currentTime - startTime ) * toMilliseconds );
}

/*
=======================================
	Directory searching
=======================================
*/

static char			findbase[MAX_OSPATH];
static char			findpath[MAX_OSPATH];
static intptr_t		findhandle;

static bool CompareAttributes( unsigned found, unsigned musthave, unsigned canthave )
{
	if ( ( found & _A_RDONLY ) && ( canthave & SFF_RDONLY ) )
		return false;
	if ( ( found & _A_HIDDEN ) && ( canthave & SFF_HIDDEN ) )
		return false;
	if ( ( found & _A_SYSTEM ) && ( canthave & SFF_SYSTEM ) )
		return false;
	if ( ( found & _A_SUBDIR ) && ( canthave & SFF_SUBDIR ) )
		return false;
	if ( ( found & _A_ARCH ) && ( canthave & SFF_ARCH ) )
		return false;

	if ( ( musthave & SFF_RDONLY ) && !( found & _A_RDONLY ) )
		return false;
	if ( ( musthave & SFF_HIDDEN ) && !( found & _A_HIDDEN ) )
		return false;
	if ( ( musthave & SFF_SYSTEM ) && !( found & _A_SYSTEM ) )
		return false;
	if ( ( musthave & SFF_SUBDIR ) && !( found & _A_SUBDIR ) )
		return false;
	if ( ( musthave & SFF_ARCH ) && !( found & _A_ARCH ) )
		return false;

	return true;
}

char *Sys_FindFirst (const char *path, unsigned musthave, unsigned canthave )
{
	struct _finddata_t findinfo;

	if (findhandle)
		Com_FatalErrorf("Sys_BeginFind without close");
	findhandle = 0;

	COM_FilePath (path, findbase);
	findhandle = _findfirst (path, &findinfo);
	if (findhandle == -1)
		return nullptr;
	if ( !CompareAttributes( findinfo.attrib, musthave, canthave ) )
		return nullptr;
	Q_sprintf_s (findpath, "%s/%s", findbase, findinfo.name);
	return findpath;
}

char *Sys_FindNext (unsigned musthave, unsigned canthave)
{
	struct _finddata_t findinfo;

	if (findhandle == -1)
		return nullptr;
	if (_findnext (findhandle, &findinfo) == -1)
		return nullptr;
	if ( !CompareAttributes( findinfo.attrib, musthave, canthave ) )
		return nullptr;

	Q_sprintf_s (findpath, "%s/%s", findbase, findinfo.name);
	return findpath;
}

void Sys_FindClose (void)
{
	if (findhandle != -1)
		_findclose (findhandle);
	findhandle = 0;
}

/*
=======================================
	File dialog
=======================================
*/

void Sys_FileDialog()
{
#if 0
	IFileOpenDialog *pDialog;
	HRESULT hr = CoCreateInstance(
		CLSID_FileOpenDialog,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS( &pDialog ) );

	const COMDLG_FILTERSPEC fileDialogSaveTypes
	{
		L"JaffaQuake Material (*.mat)", L"*.mat"
	};

	if ( SUCCEEDED( hr ) )
	{
		DWORD flags;
		hr = pDialog->GetOptions( &flags );
		flags |= FOS_FORCEFILESYSTEM;
		hr = pDialog->SetOptions( flags );
		hr = pDialog->SetFileTypes( 1, &fileDialogSaveTypes );

		//hr = pDialog->SetFileTypeIndex( INDEX_WORDDOC );

		//hr = pDialog->SetDefaultExtension( L"mat" );

		hr = pDialog->Show( nullptr );
		if ( SUCCEEDED( hr ) )
		{
			IShellItem *pResult;
			hr = pDialog->GetResult( &pResult );
			if ( SUCCEEDED( hr ) )
			{
				LPWSTR pFilePath = nullptr;
				hr = pResult->GetDisplayName( SIGDN_FILESYSPATH, &pFilePath );
				if ( SUCCEEDED( hr ) )
				{
					TaskDialog( nullptr,
						nullptr,
						L"CommonFileDialogApp",
						pFilePath,
						nullptr,
						TDCBF_OK_BUTTON,
						TD_INFORMATION_ICON,
						nullptr );
					CoTaskMemFree( pFilePath );
				}
				pResult->Release();
			}
		}
		pDialog->Release();
	}
#endif
}
