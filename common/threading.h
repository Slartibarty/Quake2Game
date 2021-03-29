
#pragma once

/*
===================================================================================================

	Low level threading interface from Doom 3 BFG Edition

===================================================================================================
*/

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <string>
#include <vector>

#include "q_types.h"

constexpr int MAX_THREAD_NAME = 32;

/*
===================================================================================================

	Platform specific mutex, signal, atomic integer and memory barrier

===================================================================================================
*/

using mutexHandle_t				= CRITICAL_SECTION;
using signalHandle_t			= HANDLE;
using interlockedInt_t			= LONG;

// _ReadWriteBarrier() does not translate to any instructions but keeps the compiler
// from reordering read and write instructions across the barrier.
// MemoryBarrier() inserts and CPU instruction that keeps the CPU from reordering reads and writes.
#define SYS_MEMORYBARRIER		_ReadWriteBarrier(); MemoryBarrier()

/*
===================================================================================================

	Platform independent threading functions

===================================================================================================
*/

using threadHandle_t			= uintptr_t;
using threadID_t				= uintptr_t;

constexpr uint WAIT_INFINITE = -1;

enum core_t {
	CORE_ANY = -1,
	CORE_0A,
	CORE_0B,
	CORE_1A,
	CORE_1B,
	CORE_2A,
	CORE_2B
};

typedef uint ( *xthread_t )( void * );

enum xthreadPriority {
	THREAD_LOWEST,
	THREAD_BELOW_NORMAL,
	THREAD_NORMAL,
	THREAD_ABOVE_NORMAL,
	THREAD_HIGHEST
};

constexpr size_t	DEFAULT_THREAD_STACK_SIZE		= ( 256 * 1024 );

// on win32, the threadID is NOT the same as the threadHandle
threadID_t			Sys_GetCurrentThreadID();

// returns a threadHandle
threadHandle_t		Sys_CreateThread( xthread_t function, void *parms, xthreadPriority priority,
									  const char *name, core_t core, size_t stackSize = DEFAULT_THREAD_STACK_SIZE, 
									  bool suspended = false );

void				Sys_WaitForThread( threadHandle_t threadHandle );
void				Sys_DestroyThread( threadHandle_t threadHandle );
void				Sys_SetCurrentThreadName( const char *name );

void				Sys_SignalCreate( signalHandle_t &handle, bool manualReset );
void				Sys_SignalDestroy( signalHandle_t &handle );
void				Sys_SignalRaise( signalHandle_t &handle );
void				Sys_SignalClear( signalHandle_t &handle );
bool				Sys_SignalWait( signalHandle_t &handle, uint timeout );

void				Sys_MutexCreate( mutexHandle_t &handle );
void				Sys_MutexDestroy( mutexHandle_t &handle );
bool				Sys_MutexLock( mutexHandle_t &handle, bool blocking );
void				Sys_MutexUnlock( mutexHandle_t &handle );

interlockedInt_t	Sys_InterlockedIncrement( interlockedInt_t &value );
interlockedInt_t	Sys_InterlockedDecrement( interlockedInt_t &value );

interlockedInt_t	Sys_InterlockedAdd( interlockedInt_t &value, interlockedInt_t i );
interlockedInt_t	Sys_InterlockedSub( interlockedInt_t &value, interlockedInt_t i );

interlockedInt_t	Sys_InterlockedExchange( interlockedInt_t &value, interlockedInt_t exchange );
interlockedInt_t	Sys_InterlockedCompareExchange( interlockedInt_t &value, interlockedInt_t comparand, interlockedInt_t exchange );

void *				Sys_InterlockedExchangePointer( void *&ptr, void *exchange );
void *				Sys_InterlockedCompareExchangePointer( void *&ptr, void * comparand, void *exchange );

void				Sys_Yield();

/*
===================================================================================================

	Wrapper classes

===================================================================================================
*/

/*
================================================
idSysMutex provides a C++ wrapper to the low level system mutex functions.  A mutex is an
object that can only be locked by one thread at a time.  It's used to prevent two threads
from accessing the same piece of data simultaneously.
================================================
*/
class idSysMutex
{
public:
					idSysMutex() { Sys_MutexCreate( handle ); }
					~idSysMutex() { Sys_MutexDestroy( handle ); }

	bool			Lock( bool blocking = true ) { return Sys_MutexLock( handle, blocking ); }
	void			Unlock() { Sys_MutexUnlock( handle ); }

private:
	mutexHandle_t	handle;

					idSysMutex( const idSysMutex &s ) = delete;
	void			operator=( const idSysMutex &s ) = delete;
};

/*
================================================
idScopedMutex is a helper class that automagically locks a mutex when it's created
and unlocks it when it goes out of scope.
================================================
*/
class idScopedMutex {
public:
	idScopedMutex( idSysMutex &m ) : mutex(m) { mutex.Lock(); }
	~idScopedMutex() { mutex.Unlock(); }

private:
	idSysMutex &	mutex;
};

/*
================================================
idSysSignal is a C++ wrapper for the low level system signal functions.  A signal is an object
that a thread can wait on for it to be raised.  It's used to indicate data is available or that
a thread has reached a specific point.
================================================
*/
class idSysSignal {
public:
			idSysSignal( bool manualReset = false )	{ Sys_SignalCreate( handle, manualReset ); }
			~idSysSignal() { Sys_SignalDestroy( handle ); }

	void	Raise() { Sys_SignalRaise( handle ); }
	void	Clear() { Sys_SignalClear( handle ); }

	// Wait returns true if the object is in a signalled state and
	// returns false if the wait timed out. Wait also clears the signalled
	// state when the signalled state is reached within the time out period.
	bool	Wait( uint timeout = WAIT_INFINITE ) { return Sys_SignalWait( handle, timeout ); }

private:
	signalHandle_t		handle;

						idSysSignal( const idSysSignal & s ) {}
	void				operator=( const idSysSignal & s ) {}
};

/*
================================================
idSysInterlockedInteger is a C++ wrapper for the low level system interlocked integer
routines to atomically increment or decrement an integer.
================================================
*/
class idSysInterlockedInteger {
public:
	// atomically increments the integer and returns the new value
	int					Increment() { return Sys_InterlockedIncrement( value ); }

	// atomically decrements the integer and returns the new value
	int					Decrement() { return Sys_InterlockedDecrement( value ); }

	// atomically adds a value to the integer and returns the new value
	int					Add( int v ) { return Sys_InterlockedAdd( value, (interlockedInt_t) v ); }

	// atomically subtracts a value from the integer and returns the new value
	int					Sub( int v ) { return Sys_InterlockedSub( value, (interlockedInt_t) v ); }

	// returns the current value of the integer
	int					GetValue() const { return value; }

	// sets a new value, Note: this operation is not atomic
	void				SetValue( int v ) { value = v; }

private:
	interlockedInt_t	value = 0;
};

/*
================================================
idSysInterlockedPointer is a C++ wrapper around the low level system interlocked pointer
routine to atomically set a pointer while retrieving the previous value of the pointer.
================================================
*/
template< typename T >
class idSysInterlockedPointer {
public:
			idSysInterlockedPointer() : ptr( NULL ) {}

	// atomically sets the pointer and returns the previous pointer value
	T *		Set( T * newPtr ) { 
				return (T *) Sys_InterlockedExchangePointer( (void * &) ptr, newPtr ); 
			}

	// atomically sets the pointer to 'newPtr' only if the previous pointer is equal to 'comparePtr'
	// ptr = ( ptr == comparePtr ) ? newPtr : ptr
	T *		CompareExchange( T * comparePtr, T * newPtr ) {
				return (T *) Sys_InterlockedCompareExchangePointer( (void * &) ptr, comparePtr, newPtr );
	}

	// returns the current value of the pointer
	T *		Get() const { return ptr; }

private:
	T *		ptr;
};

/*
================================================
idSysThread is an abstract base class, to be extended by classes implementing the
idSysThread::Run() method. 

	class idMyThread : public idSysThread {
	public:
		virtual int Run() {
			// run thread code here
			return 0;
		}
		// specify thread data here
	};

	idMyThread thread;
	thread.Start( "myThread" );

A worker thread is a thread that waits in place (without consuming CPU)
until work is available. A worker thread is implemented as normal, except that, instead of
calling the Start() method, the StartWorker() method is called to start the thread.
Note that the Sys_CreateThread function does not support the concept of worker threads.

	class idMyWorkerThread : public idSysThread {
	public:
		virtual int Run() {
			// run thread code here
			return 0;
		}
		// specify thread data here
	};

	idMyWorkerThread thread;
	thread.StartThread( "myWorkerThread" );
 
	// main thread loop
	for ( ; ; ) {
		// setup work for the thread here (by modifying class data on the thread)
		thread.SignalWork();           // kick in the worker thread
		// run other code in the main thread here (in parallel with the worker thread)
		thread.WaitForThread();        // wait for the worker thread to finish
		// use results from worker thread here
	}

In the above example, the thread does not continuously run in parallel with the main Thread,
but only for a certain period of time in a very controlled manner. Work is set up for the
Thread and then the thread is signalled to process that work while the main thread continues.
After doing other work, the main thread can wait for the worker thread to finish, if it has not
finished already. When the worker thread is done, the main thread can safely use the results
from the worker thread.

Note that worker threads are useful on all platforms but they do not map to the SPUs on the PS3.
================================================
*/
class idSysThread {
public:
					idSysThread();
	virtual			~idSysThread();

	const char *	GetName() const { return name; }
	threadHandle_t	GetThreadHandle() const { return threadHandle; }
	bool			IsRunning() const { return isRunning; }
	bool			IsTerminating() const { return isTerminating; }

	//------------------------
	// Thread Start/Stop/Wait
	//------------------------

	bool			StartThread( const char * name, core_t core,
								 xthreadPriority priority = THREAD_NORMAL,
								 size_t stackSize = DEFAULT_THREAD_STACK_SIZE );

	bool			StartWorkerThread( const char * name, core_t core,
									   xthreadPriority priority = THREAD_NORMAL,
									   size_t stackSize = DEFAULT_THREAD_STACK_SIZE );

	void			StopThread( bool wait = true );

	// This can be called from multiple other threads. However, in the case
	// of a worker thread, the work being "done" has little meaning if other
	// threads are continuously signalling more work.
	void			WaitForThread();

	//------------------------
	// Worker Thread
	//------------------------

	// Signals the thread to notify work is available.
	// This can be called from multiple other threads.
	void			SignalWork();

	// Returns true if the work is done without waiting.
	// This can be called from multiple other threads. However, the work
	// being "done" has little meaning if other threads are continuously
	// signalling more work.
	bool			IsWorkDone();

protected:
	// The routine that performs the work.
	virtual int		Run();

private:
	char			name[MAX_THREAD_NAME];
	threadHandle_t	threadHandle;
	bool			isWorker;
	bool			isRunning;
	volatile bool	isTerminating;
	volatile bool	moreWorkToDo;
	idSysSignal		signalWorkerDone;
	idSysSignal		signalMoreWorkToDo;
	idSysMutex		signalMutex;

	static int		ThreadProc( idSysThread * thread );

					idSysThread( const idSysThread & s ) = delete;
	void			operator=( const idSysThread & s ) = delete;
};

/*
================================================
idSysWorkerThreadGroup implements a group of worker threads that 
typically crunch through a collection of similar tasks.

	class idMyWorkerThread : public idSysThread {
	public:
		virtual int Run() {
			// run thread code here
			return 0;
		}
		// specify thread data here
	};

	idSysWorkerThreadGroup<idMyWorkerThread> workers( "myWorkers", 4 );
	for ( ; ; ) {
		for ( int i = 0; i < workers.GetNumThreads(); i++ ) {
			// workers.GetThread( i )-> // setup work for this thread
		}
		workers.SignalWorkAndWait();
		// use results from the worker threads here
	}

The concept of worker thread Groups is probably most useful for tools and compilers.
For instance, the AAS Compiler is using a worker thread group. Although worker threads
will work well on the PC, Mac and the 360, they do not directly map to the PS3,
in that the worker threads won't automatically run on the SPUs.
================================================
*/
template<class threadType>
class idSysWorkerThreadGroup {
public:
					idSysWorkerThreadGroup( const char * name, int numThreads,
											xthreadPriority priority = THREAD_NORMAL,
											size_t stackSize = DEFAULT_THREAD_STACK_SIZE );

	virtual			~idSysWorkerThreadGroup();

	int				GetNumThreads() const { return (int)threadList.size(); }
	threadType &	GetThread( int i ) { return *threadList[i]; }

	void			SignalWorkAndWait();

private:
	std::vector<threadType *>	threadList;
	bool					runOneThreadInline;	// use the signalling thread as one of the threads
	bool					singleThreaded;		// set to true for debugging
};

/*
========================
idSysWorkerThreadGroup<threadType>::idSysWorkerThreadGroup
========================
*/
template<class threadType>
inline idSysWorkerThreadGroup<threadType>::idSysWorkerThreadGroup( const char * name,
			int numThreads, xthreadPriority priority, size_t stackSize ) {
	runOneThreadInline = ( numThreads < 0 );
	singleThreaded = false;
	numThreads = abs( numThreads );
	for( int i = 0; i < numThreads; i++ ) {
		threadType *thread = new threadType;
		thread->StartWorkerThread( va( "%s_worker%i", name, i ), (core_t) i, priority, stackSize );
		threadList.push_back( thread );
	}
}

/*
========================
idSysWorkerThreadGroup<threadType>::~idSysWorkerThreadGroup
========================
*/
template<class threadType>
inline idSysWorkerThreadGroup<threadType>::~idSysWorkerThreadGroup() {
	threadList.clear();
}

/*
========================
idSysWorkerThreadGroup<threadType>::SignalWorkAndWait
========================
*/
template<class threadType>
inline void idSysWorkerThreadGroup<threadType>::SignalWorkAndWait() {
	if ( singleThreaded ) {
		for( int i = 0; i < threadList.size(); i++ ) {
			threadList[ i ]->Run();
		}
		return;
	}
	for( int i = 0; i < threadList.size() - runOneThreadInline; i++ ) {
		threadList[ i ]->SignalWork();
	}
	if ( runOneThreadInline ) {
		threadList[ threadList.size() - 1 ]->Run();
	}
	for ( int i = 0; i < threadList.size() - runOneThreadInline; i++ ) {
		threadList[ i ]->WaitForThread();
	}
}

/*
================================================
idSysThreadSynchronizer, allows a group of threads to 
synchronize with each other half-way through execution.

	idSysThreadSynchronizer sync;

	class idMyWorkerThread : public idSysThread {
	public:
		virtual int Run() {
			// perform first part of the work here
			sync.Synchronize( threadNum );	// synchronize all threads
			// perform second part of the work here
			return 0;
		}
		// specify thread data here
		unsigned int threadNum;
	};

	idSysWorkerThreadGroup<idMyWorkerThread> workers( "myWorkers", 4 );
	for ( int i = 0; i < workers.GetNumThreads(); i++ ) {
		workers.GetThread( i )->threadNum = i;
	}

	for ( ; ; ) {
		for ( int i = 0; i < workers.GetNumThreads(); i++ ) {
			// workers.GetThread( i )-> // setup work for this thread
		}
		workers.SignalWorkAndWait();
		// use results from the worker threads here
	}

================================================
*/
class idSysThreadSynchronizer {
public:
	inline void			SetNumThreads( uint num );
	inline void			Signal();
	inline bool			Synchronize( uint threadNum, uint timeout = WAIT_INFINITE );

private:
	std::vector<idSysSignal *>	signals;
	idSysInterlockedInteger		busyCount;
};

/*
========================
idSysThreadSynchronizer::SetNumThreads
========================
*/
inline void idSysThreadSynchronizer::SetNumThreads( uint num ) {
	assert( busyCount.GetValue() == signals.size() );
	if ( num != signals.size() ) {
		signals.clear();
		signals.resize( num );
		for ( uint i = 0; i < num; i++ ) {
			signals[i] = new idSysSignal();
		}
		busyCount.SetValue( num );
		SYS_MEMORYBARRIER;
	}
}

/*
========================
idSysThreadSynchronizer::Signal
========================
*/
inline void idSysThreadSynchronizer::Signal() {
	if ( busyCount.Decrement() == 0 ) {
		busyCount.SetValue( (int)signals.size() );
		SYS_MEMORYBARRIER;
		for ( uint i = 0; i < (uint)signals.size(); i++ ) {
			signals[i]->Raise();
		}
	}
}

/*
========================
idSysThreadSynchronizer::Synchronize
========================
*/
inline bool idSysThreadSynchronizer::Synchronize( uint threadNum, uint timeout ) {
	return signals[threadNum]->Wait( timeout );
}
