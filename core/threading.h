/*
===================================================================================================

	Low level threading interface from Doom 3 BFG Edition

	Should probably just get rid of this and write my own implementation based on the knowledge
	gained from this library

===================================================================================================
*/

#pragma once

// Not Linux-capable yet
#ifdef _WIN32

/*
===================================================================================================

	Platform specific mutex, signal, atomic integer and memory barrier

===================================================================================================
*/

#ifdef _WIN32

struct mutex_t
{
	void *ptr;
};

#else

#error determine this

#endif

/*
===================================================================================================

	Platform independent threading functions

===================================================================================================
*/

using threadHandle_t = uintptr_t;
using threadID_t = uintptr_t;

inline constexpr uint WAIT_INFINITE = UINT_MAX;

typedef uint32 ( *threadProc_t )( void * );

enum threadPriority_t
{
	THREAD_LOWEST,
	THREAD_BELOW_NORMAL,
	THREAD_NORMAL,
	THREAD_ABOVE_NORMAL,
	THREAD_HIGHEST
};

inline constexpr size_t DEFAULT_THREAD_STACK_SIZE = ( 256 * 1024 );

// On win32, the threadID is NOT the same as the threadHandle
threadID_t			Sys_GetCurrentThreadID();

// Returns a threadHandle
threadHandle_t		Sys_CreateThread( threadProc_t function, void *parms, threadPriority_t priority,
									  const platChar_t *name, size_t stackSize = DEFAULT_THREAD_STACK_SIZE,
									  bool suspended = false );

void				Sys_WaitForThread( threadHandle_t threadHandle );
void				Sys_WaitForMultipleThreads( const threadHandle_t *threadHandles, uint32 numThreads );
void				Sys_DestroyThread( threadHandle_t threadHandle );
void				Sys_SetCurrentThreadName( const platChar_t *name );

void				Sys_Yield();

void				Sys_MutexCreate( mutex_t &mutex );
void				Sys_MutexDestroy( mutex_t &mutex );
bool				Sys_MutexTryLock( mutex_t &mutex );
void				Sys_MutexLock( mutex_t &mutex );
void				Sys_MutexUnlock( mutex_t &mutex );

#endif
