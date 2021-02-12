
extern int numthreads;

typedef void ( *threadworker_f )( int thread );

void ThreadSetDefault();
int	GetThreadWork();
void RunThreadsOnIndividual( int workcnt, bool showpacifier, void( *func )( int ) );
void RunThreadsOn( int workcnt, bool showpacifier, void( *func )( int ) );
void ThreadLock();
void ThreadUnlock();

