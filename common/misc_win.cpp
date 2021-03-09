
#include "q_shared.h"

#include <io.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//=================================================================================================

int		hunkcount;

void	*membase;
int		hunkmaxsize;
int		cursize;

#define	VIRTUAL_ALLOC

void *Hunk_Begin (int maxsize)
{
	// reserve a huge chunk of memory, but don't commit any yet
	cursize = 0;
	hunkmaxsize = maxsize;
#ifdef VIRTUAL_ALLOC
	membase = VirtualAlloc (NULL, maxsize, MEM_RESERVE, PAGE_NOACCESS);
#else
	membase = Z_Malloc(maxsize);
#endif

	if (!membase)
		Sys_Error ("VirtualAlloc reserve failed");

	return (void *)membase;
}

void *Hunk_Alloc (int size)
{
	void	*buf;

	// round to cacheline
	size = (size+31)&~31;

#ifdef VIRTUAL_ALLOC
	// commit pages as needed
	buf = VirtualAlloc (membase, cursize+size, MEM_COMMIT, PAGE_READWRITE);
	if (!buf)
	{
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &buf, 0, NULL);
		Sys_Error ("VirtualAlloc commit failed.\n%s", buf);
	}
#endif

	cursize += size;
	if (cursize > hunkmaxsize)
		Sys_Error ("Hunk_Alloc overflow");

	return (void *)((byte*)membase+cursize-size);
}

int Hunk_End (void)
{
	// free the remaining unused virtual memory
#if 0
	void	*buf;

	// write protect it
	buf = VirtualAlloc (membase, cursize, MEM_COMMIT, PAGE_READONLY);
	if (!buf)
		Sys_Error ("VirtualAlloc commit failed");
#endif

	++hunkcount;
	//Com_Printf ("hunkcount: %i\n", hunkcount);

	return cursize;
}

void Hunk_Free (void *base)
{
	if ( base )
#ifdef VIRTUAL_ALLOC
		VirtualFree (base, 0, MEM_RELEASE);
#else
		Z_Free (base);
#endif

	--hunkcount;
}

//=================================================================================================

static int64	startTime;
static double	toSeconds;
static double	toMilliseconds;
static double	toMicroseconds;

int curtime;

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

	return llround( ( currentTime - startTime ) * toMilliseconds );
}

// LEGACY
int Sys_Milliseconds()
{
	int64 currentTime;

	QueryPerformanceCounter( (LARGE_INTEGER *)&currentTime );

	// SlartTodo: Replace int with int64
	curtime = (int)( ( currentTime - startTime ) * toMilliseconds );

	return curtime;
}

void Sys_CopyFile (const char *src, const char *dst)
{
	CopyFileA (src, dst, FALSE);
}

void Sys_CreateDirectory (const char *path)
{
	CreateDirectoryA (path, NULL);
}

//=================================================================================================

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
		Sys_Error ("Sys_BeginFind without close");
	findhandle = 0;

	COM_FilePath (path, findbase);
	findhandle = _findfirst (path, &findinfo);
	if (findhandle == -1)
		return NULL;
	if ( !CompareAttributes( findinfo.attrib, musthave, canthave ) )
		return NULL;
	Q_sprintf_s (findpath, "%s/%s", findbase, findinfo.name);
	return findpath;
}

char *Sys_FindNext (unsigned musthave, unsigned canthave)
{
	struct _finddata_t findinfo;

	if (findhandle == -1)
		return NULL;
	if (_findnext (findhandle, &findinfo) == -1)
		return NULL;
	if ( !CompareAttributes( findinfo.attrib, musthave, canthave ) )
		return NULL;

	Q_sprintf_s (findpath, "%s/%s", findbase, findinfo.name);
	return findpath;
}

void Sys_FindClose (void)
{
	if (findhandle != -1)
		_findclose (findhandle);
	findhandle = 0;
}
