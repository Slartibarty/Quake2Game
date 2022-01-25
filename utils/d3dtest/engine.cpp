
#include "engine.h"

#include "sv_public.h"
#include "cl_public.h"

float com_deltaTime;

static StaticCvar com_timeScale( "com_timeScale", "1", 0, "Amount to scale deltaTime by." );

CON_COMMAND( quit, "Exits the application.", 0 )
{
	Com_Quit( EXIT_SUCCESS );
}

CON_COMMAND( version, "Displays application version.", 0 )
{
	Com_Printf( "%s - %s, %s\n", BLD_STRING, __DATE__, __TIME__ );
}

void Com_Init( int argc, char **argv )
{
	// Core / Framework

	Time_Init();

	Cbuf_Init();
	Cmd_Init();
	Cvar_Init();

	Cvar_AddEarlyCommands( argc, argv );

	FileSystem::Init();

	Cbuf_AddLateCommands( argc, argv );

	// Common

	SysConsole_Init();
	PhysicsSystem::Init();

	// Server / Client

	SV_Init();
	CL_Init();
}

void Com_Shutdown()
{
	// Server / Client

	CL_Shutdown();
	SV_Shutdown();

	// Common

	PhysicsSystem::Shutdown();
	SysConsole_Shutdown();

	// Core / Framework

	FileSystem::Shutdown();
	Cvar_Shutdown();
	Cmd_Shutdown();
	Cbuf_Shutdown();
}

void Com_Quit( int code )
{
	Com_Shutdown();

	Sys_Quit( code );
}

void Com_Frame( float deltaTime )
{
	if ( com_timeScale.GetInt() != 1 )
	{
		deltaTime *= com_timeScale.GetFloat();
	}
	com_deltaTime = deltaTime;

	// Execute buffered commands
	Cbuf_Execute();

	// Let the fizzicks code move things
	PhysicsSystem::Frame( deltaTime );

	// Server!
	SV_Frame( deltaTime );

	// Client!
	CL_Frame( deltaTime );
}
