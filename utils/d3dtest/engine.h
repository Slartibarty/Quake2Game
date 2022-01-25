/*
===================================================================================================

	Base engine header for Engine 3

	Third time's a charm!

===================================================================================================
*/

#pragma once

// Core
#include "../../core/core.h"

// C++ Headers

// Framework
#include "../../framework/filesystem.h"
#include "../../framework/cmdsystem.h"
#include "../../framework/cvarsystem.h"

/*=================================================================================================
	Build information
=================================================================================================*/

// The name of the engine, idTech uses this for engine variations (IE: "D3BFG", "RAGE")
#define	ENGINE_VERSION "Engine 3"

// The default argument for fs_mod
#define	BASE_MODDIR "base"

#ifdef _WIN32

#ifdef _WIN64
#define BLD_PLATFORM "Win64"
#define BLD_ARCHITECTURE "x64"
#else
#define BLD_PLATFORM "Win32"
#define BLD_ARCHITECTURE "x86"
#endif

#elif defined __linux__

#define BLD_PLATFORM "Linux64"
#define BLD_ARCHITECTURE "x64"

#else

#error

#endif

#if defined Q_RETAIL
#define BLD_CONFIG "Retail"
#elif defined Q_RELEASE
#define BLD_CONFIG "Release"
#elif defined Q_DEBUG
#define BLD_CONFIG "Debug"
#else
#error No config defined
#endif

#define BLD_STRING ( ENGINE_VERSION " - " BLD_PLATFORM " " BLD_CONFIG )

/*=================================================================================================
	System Specific
=================================================================================================*/

// sys_main.cpp

// Use this instead of exit()
[[nodiscard]]
void Sys_Quit( int code );

// sys_console.cpp

void SysConsole_Init();
void SysConsole_Shutdown();
void SysConsole_Print( const char *msg );

/*=================================================================================================
	Common
=================================================================================================*/

// Frame delta time in seconds
extern float com_deltaTime;

void Com_Init( int argc, char **argv );
void Com_Shutdown();
void Com_Quit( int code );					// Graceful quit
void Com_Frame( float deltaTime );

/*=================================================================================================
	Physics
=================================================================================================*/

namespace PhysicsSystem
{
	void Init();
	void Shutdown();
	void Frame( float deltaTime );

	void AddBodyForOBJ( void *buffer, fsSize_t bufferLength );
}

/*=================================================================================================
	Physical Objects
=================================================================================================*/

namespace ObjectManager
{
	using objectIndex_t = uint32;

	objectIndex_t ObjectForName( const char *filename );
}
