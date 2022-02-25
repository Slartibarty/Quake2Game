
#include "core.h"

#include "sys_includes.h"

#include "threading.h"

/*
===================================================================================================

	Thread

===================================================================================================
*/

static void Sys_SetThreadName( HANDLE thread, const platChar_t *name )
{
	SetThreadDescription( thread, name );
}

void Sys_SetCurrentThreadName( const platChar_t *name )
{
	Sys_SetThreadName( GetCurrentThread(), name );
}

threadHandle_t Sys_CreateThread( threadProc_t function, void *parms, threadPriority_t priority, const platChar_t *name, size_t stackSize, bool suspended )
{
	DWORD flags = ( suspended ? CREATE_SUSPENDED : 0 );

	// Without this flag the 'dwStackSize' parameter to CreateThread specifies the "Stack Commit Size"
	// and the "Stack Reserve Size" is set to the value specified at link-time.
	// With this flag the 'dwStackSize' parameter to CreateThread specifies the "Stack Reserve Size"
	// and the "Stack Commit Size" is set to the value specified at link-time.
	// For various reasons (some of which historic) we reserve a large amount of stack space in the
	// project settings. By setting this flag and by specifying 64 kB for the "Stack Commit Size" in
	// the project settings we can create new threads with a much smaller reserved (and committed)
	// stack space. It is very important that the "Stack Commit Size" is set to a small value in
	// the project settings. If it is set to a large value we may be both reserving and committing
	// a lot of memory by setting the STACK_SIZE_PARAM_IS_A_RESERVATION flag. There are some
	// 50 threads allocated for normal game play. If, for instance, the commit size is set to 16 MB
	// then by adding this flag we would be reserving and committing 50 x 16 = 800 MB of memory.
	// On the other hand, if this flag is not set and the "Stack Reserve Size" is set to 16 MB in the
	// project settings, then we would still be reserving 50 x 16 = 800 MB of virtual address space.
	flags |= STACK_SIZE_PARAM_IS_A_RESERVATION;

	HANDLE handle = CreateThread( nullptr, stackSize, (LPTHREAD_START_ROUTINE)function, parms, flags, nullptr );
	if ( !handle ) {
		Com_FatalErrorf( "Sys_CreateThread error: %u\n", GetLastError() );
	}

	Sys_SetThreadName( handle, name );

	// no case for normal, it's the default
	switch ( priority )
	{
	case THREAD_HIGHEST:
		// we better sleep enough to do this
		SetThreadPriority( handle, THREAD_PRIORITY_HIGHEST );
		break;
	case THREAD_ABOVE_NORMAL:
		SetThreadPriority( handle, THREAD_PRIORITY_ABOVE_NORMAL );
		break;
	case THREAD_BELOW_NORMAL:
		SetThreadPriority( handle, THREAD_PRIORITY_BELOW_NORMAL );
		break;
	case THREAD_LOWEST:
		SetThreadPriority( handle, THREAD_PRIORITY_LOWEST );
		break;
	}

	// Under Windows, we don't set the thread affinity and let the OS deal with scheduling

	return (threadHandle_t)handle;
}

threadID_t Sys_GetCurrentThreadID()
{
	return (threadID_t)GetCurrentThreadId();
}

void Sys_WaitForThread( threadHandle_t threadHandle )
{
	// TODO: Add some kind of security for runaway threads, INFINITE is not the solution
	WaitForSingleObject( (HANDLE)threadHandle, INFINITE );
}

void Sys_WaitForMultipleThreads( const threadHandle_t *threadHandles, uint32 numThreads )
{
	// TODO: Add some kind of security for runaway threads, INFINITE is not the solution
	WaitForMultipleObjects( numThreads, (const HANDLE *)threadHandles, TRUE, INFINITE );
}

void Sys_DestroyThread( threadHandle_t threadHandle )
{
	CloseHandle( (HANDLE)threadHandle );
}

void Sys_Yield()
{
	SwitchToThread();
}

/*
===================================================================================================

	Mutex

===================================================================================================
*/

void Sys_MutexCreate( mutex_t &mutex )
{
	mutex.ptr = nullptr;
}

void Sys_MutexDestroy( mutex_t &mutex )
{
	// Nothing to do
}

bool Sys_MutexTryLock( mutex_t &mutex )
{
	return TryAcquireSRWLockExclusive( (PSRWLOCK)&mutex );
}

void Sys_MutexLock( mutex_t &mutex )
{
	AcquireSRWLockExclusive( (PSRWLOCK)&mutex );
}

void Sys_MutexUnlock( mutex_t &mutex )
{
	ReleaseSRWLockExclusive( (PSRWLOCK)&mutex );
}
