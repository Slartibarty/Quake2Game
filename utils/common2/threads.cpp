
#include "cmdlib.h"

#include "threads.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#define	MAX_THREADS	64

static int dispatch;
static int workcount;
static int oldf;
static bool pacifier;

static bool threaded;

/*
=============
GetThreadWork

=============
*/
int	GetThreadWork()
{
	int	r;
	int	f;

	ThreadLock();

	if ( dispatch == workcount )
	{
		ThreadUnlock();
		return -1;
	}

	f = 10 * dispatch / workcount;
	if ( f != oldf )
	{
		oldf = f;
		if ( pacifier )
			Com_Printf( "%i...", f );
	}

	r = dispatch;
	dispatch++;
	ThreadUnlock();

	return r;
}


static void ( *workfunction )( int );

void ThreadWorkerFunction( int threadnum )
{
	int work;

	while ( true )
	{
		work = GetThreadWork();
		if ( work == -1 )
			break;

		workfunction( work );
	}
}

void RunThreadsOnIndividual( int workcnt, bool showpacifier, threadworker_f func )
{
	if ( numthreads == -1 )
		ThreadSetDefault();

	workfunction = func;
	RunThreadsOn( workcnt, showpacifier, ThreadWorkerFunction );
}

/*
===============================================================================

	WINDOWS

===============================================================================
*/

#ifdef _WIN32

int numthreads = -1;

static CRITICAL_SECTION crit;
static int enter;

void ThreadSetDefault()
{
	SYSTEM_INFO info;

	if ( numthreads == -1 )	// not set manually
	{
		GetSystemInfo( &info );
		numthreads = info.dwNumberOfProcessors;
		if ( numthreads < 1 || numthreads > MAX_THREADS )
			numthreads = 1;
	}

	Com_DPrintf( "%d threads\n", numthreads );
}

void ThreadLock()
{
	if ( !threaded )
		return;
	EnterCriticalSection( &crit );
	if ( enter )
		Com_Error( ERR_FATAL, "Recursive ThreadLock\n" );
	enter = 1;
}

void ThreadUnlock()
{
	if ( !threaded )
		return;
	if ( !enter )
		Com_Error( ERR_FATAL, "ThreadUnlock without lock\n" );
	enter = 0;
	LeaveCriticalSection( &crit );
}

/*
=============
RunThreadsOn
=============
*/
void RunThreadsOn( int workcnt, bool showpacifier, threadworker_f func )
{
	HANDLE	threadhandles[MAX_THREADS];
	int		i;
	double	start, end;

	start = Time_FloatSeconds();
	dispatch = 0;
	workcount = workcnt;
	oldf = -1;
	pacifier = showpacifier;
	threaded = true;

	//
	// run threads in parallel
	//
	InitializeCriticalSection( &crit );

	if ( numthreads == 1 )
	{	// use same thread
		func( 0 );
	}
	else
	{
		// Create threads
		for ( i = 0; i < numthreads; i++ )
		{
			threadhandles[i] = CreateThread(
				NULL,
				0,
				(LPTHREAD_START_ROUTINE)func,
				(LPVOID)i,
				0,
				nullptr
			);
		}

		// Run them!
		WaitForMultipleObjects( numthreads, threadhandles, TRUE, INFINITE );

		// Delete them
		for ( i = 0; i < numthreads; i++ )
		{
			CloseHandle( threadhandles[i] );
		}
	}

	DeleteCriticalSection( &crit );

	threaded = false;
	end = Time_FloatSeconds();
	if ( pacifier )
		Com_Printf( " (%f)\n", end - start );
}

#endif
