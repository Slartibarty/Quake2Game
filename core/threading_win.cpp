
#include "core.h"

#include <process.h>

#include "threading.h"

/*
===================================================================================================

	Low level threading interface from Doom 3 BFG Edition

===================================================================================================
*/

/*
========================
Sys_SetThreadName
========================
*/
static void Sys_SetThreadName( HANDLE thread, const platChar_t *name )
{
	SetThreadDescription( thread, name );
}

/*
========================
Sys_SetCurrentThreadName
========================
*/
void Sys_SetCurrentThreadName( const platChar_t *name )
{
	Sys_SetThreadName( GetCurrentThread(), name );
}

/*
========================
Sys_Createthread
========================
*/
threadHandle_t Sys_CreateThread( xthread_t function, void *parms, xthreadPriority priority, const platChar_t *name, core_t core, size_t stackSize, bool suspended ) {

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
		Com_FatalErrorf( "Sys_CreateThread error: %u", GetLastError() );
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

/*
========================
Sys_GetCurrentThreadID
========================
*/
threadID_t Sys_GetCurrentThreadID()
{
	return (threadID_t)GetCurrentThreadId();
}

/*
========================
Sys_WaitForThread
========================
*/
void Sys_WaitForThread( threadHandle_t threadHandle )
{
	WaitForSingleObject( (HANDLE)threadHandle, INFINITE );
}

/*
========================
Sys_DestroyThread
========================
*/
void Sys_DestroyThread( threadHandle_t threadHandle )
{
	CloseHandle( (HANDLE)threadHandle );
}

/*
========================
Sys_Yield
========================
*/
void Sys_Yield()
{
	SwitchToThread();
}

/*
===================================================================================================

	Signal

===================================================================================================
*/

/*
========================
Sys_SignalCreate
========================
*/
void Sys_SignalCreate( signalHandle_t &handle, bool manualReset )
{
	handle = CreateEventW( nullptr, (BOOL)manualReset, FALSE, nullptr );
}

/*
========================
Sys_SignalDestroy
========================
*/
void Sys_SignalDestroy( signalHandle_t &handle )
{
	CloseHandle( handle );
}

/*
========================
Sys_SignalRaise
========================
*/
void Sys_SignalRaise( signalHandle_t &handle )
{
	SetEvent( handle );
}

/*
========================
Sys_SignalClear
========================
*/
void Sys_SignalClear( signalHandle_t &handle )
{
	// events are created as auto-reset so this should never be needed
	ResetEvent( handle );
}

/*
========================
Sys_SignalWait
========================
*/
bool Sys_SignalWait( signalHandle_t &handle, uint timeout )
{
	DWORD result = WaitForSingleObject( handle, timeout == WAIT_INFINITE ? INFINITE : timeout );
	assert( result == WAIT_OBJECT_0 || ( timeout != WAIT_INFINITE && result == WAIT_TIMEOUT ) );
	return ( result == WAIT_OBJECT_0 );
}

/*
===================================================================================================

	Mutex

===================================================================================================
*/

/*
========================
Sys_MutexCreate
========================
*/
void Sys_MutexCreate( mutexHandle_t &handle )
{
	InitializeCriticalSection( &handle );
}

/*
========================
Sys_MutexDestroy
========================
*/
void Sys_MutexDestroy( mutexHandle_t &handle )
{
	DeleteCriticalSection( &handle );
}

/*
========================
Sys_MutexLock
========================
*/
bool Sys_MutexLock( mutexHandle_t &handle, bool blocking )
{
	if ( TryEnterCriticalSection( &handle ) == FALSE ) {
		if ( !blocking ) {
			return false;
		}
		EnterCriticalSection( &handle );
	}
	return true;
}

/*
========================
Sys_MutexUnlock
========================
*/
void Sys_MutexUnlock( mutexHandle_t &handle )
{
	LeaveCriticalSection( &handle );
}

/*
===================================================================================================

	Interlocked Integer

===================================================================================================
*/

/*
========================
Sys_InterlockedIncrement
========================
*/
interlockedInt_t Sys_InterlockedIncrement( interlockedInt_t &value ) {
	return InterlockedIncrementAcquire( &value );
}

/*
========================
Sys_InterlockedDecrement
========================
*/
interlockedInt_t Sys_InterlockedDecrement( interlockedInt_t &value ) {
	return InterlockedDecrementRelease( &value );
}

/*
========================
Sys_InterlockedAdd
========================
*/
interlockedInt_t Sys_InterlockedAdd( interlockedInt_t &value, interlockedInt_t i ) {
	return InterlockedExchangeAdd( &value, i ) + i;
}

/*
========================
Sys_InterlockedSub
========================
*/
interlockedInt_t Sys_InterlockedSub( interlockedInt_t &value, interlockedInt_t i ) {
	return InterlockedExchangeAdd( &value, -i ) - i;
}

/*
========================
Sys_InterlockedExchange
========================
*/
interlockedInt_t Sys_InterlockedExchange( interlockedInt_t &value, interlockedInt_t exchange ) {
	return InterlockedExchange( &value, exchange );
}

/*
========================
Sys_InterlockedCompareExchange
========================
*/
interlockedInt_t Sys_InterlockedCompareExchange( interlockedInt_t &value, interlockedInt_t comparand, interlockedInt_t exchange ) {
	return InterlockedCompareExchange( &value, exchange, comparand );
}

/*
===================================================================================================

	Interlocked Pointer

===================================================================================================
*/

/*
========================
Sys_InterlockedExchangePointer
========================
*/
void *Sys_InterlockedExchangePointer( void *&ptr, void *exchange )
{
	return InterlockedExchangePointer( &ptr, exchange );
}

/*
========================
Sys_InterlockedCompareExchangePointer
========================
*/
void *Sys_InterlockedCompareExchangePointer( void *&ptr, void *comparand, void *exchange )
{
	return InterlockedCompareExchangePointer( &ptr, exchange, comparand );
}
