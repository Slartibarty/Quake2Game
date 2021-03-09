
extern int numthreads;

typedef void ( *threadworker_f )( int thread );

void ThreadSetDefault();
int	GetThreadWork();
void RunThreadsOnIndividual( int workcnt, bool showpacifier, threadworker_f func );
void RunThreadsOn( int workcnt, bool showpacifier, threadworker_f func );
void ThreadLock();
void ThreadUnlock();

