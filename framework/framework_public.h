/*
===================================================================================================

	The Framework

	Generic functionality you'd usually find in the engine, sectioned out for elsewhere
	IE: Utilities, Worldcraft, a new level editor, etc

===================================================================================================
*/

#pragma once

#include "filesystem.h"
#include "cmdsystem.h"
#include "cvarsystem.h"

// The engine doesn't use these
#ifndef Q_ENGINE

inline void Framework_Init( int argc, char **argv )
{
	Cbuf_Init();
	Cmd_Init();
	Cvar_Init();

	// apply +set commands
	Cvar_AddEarlyCommands( argc, argv );

	FileSystem::Init();
}

inline void Framework_Shutdown()
{
	FileSystem::Shutdown();
	Cvar_Shutdown();
	Cmd_Shutdown();
	Cbuf_Shutdown();
}

#endif
