
#include "q_shared.h"

#include "threading.h"

/*
===================================================================================================
Contains the vartious ThreadingClass implementations.
===================================================================================================
*/

/*
===================================================================================================

	idSysThread

===================================================================================================
*/

/*
========================
idSysThread::idSysThread
========================
*/
idSysThread::idSysThread() :
		threadHandle( 0 ),
		isWorker( false ),
		isRunning( false ),
		isTerminating( false ),
		moreWorkToDo( false ),
		signalWorkerDone( true ) {
}

/*
========================
idSysThread::~idSysThread
========================
*/
idSysThread::~idSysThread() {
	StopThread( true );
	if ( threadHandle ) {
		Sys_DestroyThread( threadHandle );
	}
}

/*
========================
idSysThread::StartThread
========================
*/
bool idSysThread::StartThread( const char *name_, core_t core, xthreadPriority priority, size_t stackSize ) {
	if ( isRunning ) {
		return false;
	}

	Q_strcpy_s( name, name_ );

	isTerminating = false;

	if ( threadHandle ) {
		Sys_DestroyThread( threadHandle );
	}

	threadHandle = Sys_CreateThread( (xthread_t)ThreadProc, this, priority, name_, core, stackSize, false );

	isRunning = true;
	return true;
}

/*
========================
idSysThread::StartWorkerThread
========================
*/
bool idSysThread::StartWorkerThread( const char *name_, core_t core, xthreadPriority priority, size_t stackSize ) {
	if ( isRunning ) {
		return false;
	}

	isWorker = true;

	bool result = StartThread( name_, core, priority, stackSize );

	signalWorkerDone.Wait( WAIT_INFINITE );

	return result;
}

/*
========================
idSysThread::StopThread
========================
*/
void idSysThread::StopThread( bool wait ) {
	if ( !isRunning ) {
		return;
	}
	if ( isWorker ) {
		signalMutex.Lock();
		moreWorkToDo = true;
		signalWorkerDone.Clear();
		isTerminating = true;
		signalMoreWorkToDo.Raise();
		signalMutex.Unlock();
	} else {
		isTerminating = true;
	}
	if ( wait ) {
		WaitForThread();
	}
}

/*
========================
idSysThread::WaitForThread
========================
*/
void idSysThread::WaitForThread() {
	if ( isWorker ) {
		signalWorkerDone.Wait( WAIT_INFINITE );
	} else if ( isRunning ) {
		Sys_DestroyThread( threadHandle );
		threadHandle = 0;
	}
}

/*
========================
idSysThread::SignalWork
========================
*/
void idSysThread::SignalWork() {
	if ( isWorker ) {
		signalMutex.Lock();
		moreWorkToDo = true;
		signalWorkerDone.Clear();
		signalMoreWorkToDo.Raise();
		signalMutex.Unlock();
	}
}

/*
========================
idSysThread::IsWorkDone
========================
*/
bool idSysThread::IsWorkDone() {
	if ( isWorker ) {
		// a timeout of 0 will return immediately with true if signaled
		if ( signalWorkerDone.Wait( 0 ) ) {
			return true;
		}
	}
	return false;
}

/*
========================
idSysThread::ThreadProc
========================
*/
int idSysThread::ThreadProc( idSysThread * thread ) {
	int retVal = 0;

	// slart: there used to be try-catch code here, what do we do?
	if ( thread->isWorker ) {
		for( ; ; ) {
			thread->signalMutex.Lock();
			if ( thread->moreWorkToDo ) {
				thread->moreWorkToDo = false;
				thread->signalMoreWorkToDo.Clear();
				thread->signalMutex.Unlock();
			} else {
				thread->signalWorkerDone.Raise();
				thread->signalMutex.Unlock();
				thread->signalMoreWorkToDo.Wait( WAIT_INFINITE );
				continue;
			}

			if ( thread->isTerminating ) {
				break;
			}

			retVal = thread->Run();
		}
		thread->signalWorkerDone.Raise();
	} else {
		retVal = thread->Run();
	}

	thread->isRunning = false;

	return retVal;
}

/*
========================
idSysThread::Run
========================
*/
int idSysThread::Run() {
	// The Run() is not pure virtual because on destruction of a derived class
	// the virtual function pointer will be set to NULL before the idSysThread
	// destructor actually stops the thread.
	return 0;
}

/*
================================================================================================

	test

================================================================================================
*/

/*
================================================
idMyThread test class.
================================================
*/
class idMyThread : public idSysThread {
public:
	int Run() override {

		Com_Print( "Printing a thousand hellos in another thread\n" );

		for ( int i = 0; i < 1000; ++i ) {
			Com_Print( "Hello\n" );
		}

		int64 start = Time_Milliseconds();

		int64 now;

		while ( ( ( now = Time_Milliseconds() ) - start ) < 5000 )
			;

		Com_Print( "Thread done\n" );

		return 0;
	}
	// specify thread data here
};

/*
========================
TestThread
========================
*/
void TestThread() {
	idMyThread *thread = new idMyThread;
	thread->StartThread( "myThread", CORE_ANY );
}

/*
========================
TestWorkers
========================
*/
void TestWorkers() {
	idSysWorkerThreadGroup<idMyThread> workers( "myWorkers", 4 );
	for ( ; ; ) {
		for ( int i = 0; i < workers.GetNumThreads(); i++ ) {
			// workers.GetThread( i )-> // setup work for this thread
		}
		workers.SignalWorkAndWait();
	}
}
