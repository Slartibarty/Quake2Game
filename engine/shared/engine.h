//=================================================================================================
// The main header for the engine
// This file glues the client and server together
// It's like q_shared, but for the engine level
//=================================================================================================

#pragma once

#include "../../common/q_shared.h"

#include "cmd.h"
#include "cmodel.h"
#include "conproc.h"
#include "crc.h"
#include "cvar.h"
#include "files.h"
#include "msg.h"
#include "net.h"
#include "protocol.h"
#include "sizebuf.h"
#include "sys.h"
#include "memory.h"

#include "../../common/q_formats.h" // File formats

/*
===================================================================================================

	Build information

===================================================================================================
*/

#define	ENGINE_VERSION "JaffaQuake"

#define	BASEDIRNAME "base"

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
#else
#define BLD_CONFIG "Debug"
#endif

constexpr const char BLD_STRING[] = ( ENGINE_VERSION " - " BLD_PLATFORM " " BLD_CONFIG );

/*
===================================================================================================

	Miscellaneous

===================================================================================================
*/

typedef void ( *rd_flush_t )( int target, char *buffer );

void		Com_BeginRedirect( int target, char *buffer, int buffersize, rd_flush_t flush );
void		Com_EndRedirect();
[[noreturn]]
void		Com_Disconnect();
[[noreturn]]
void		Com_Quit( int code );

void		Info_Print( const char *s );

int			Com_ServerState();	// this should have just been a cvar...
void		Com_SetServerState( int state );

unsigned	Com_BlockChecksum( void *buffer, int length ); // md4.c
byte		COM_BlockSequenceCRCByte( byte *base, int length, int sequence );

// Compressed vertex normals
#define NUMVERTEXNORMALS 162
extern vec3_t bytedirs[NUMVERTEXNORMALS];

extern cvar_t *		developer;
extern cvar_t *		dedicated;
extern cvar_t *		host_speeds;
extern cvar_t *		log_stats;

extern FILE *		log_stats_file;

extern int		curtime;

// host_speeds times
extern int		time_before_game;
extern int		time_after_game;
extern int		time_before_ref;
extern int		time_after_ref;

void Engine_Init( int argc, char **argv );
void Engine_Shutdown();
void Engine_Frame( int msec );
bool Engine_IsMainThread();

/*
===================================================================================================

	Client / Server entry points

===================================================================================================
*/

// client

void CL_Init();
void CL_Drop();
void CL_Shutdown();
void CL_Frame( int msec );
void Con_Print( const char *text );
void SCR_BeginLoadingPlaque();

// this is in the client code, but can be used for debugging from server
void SCR_DebugGraph( float value, uint32 color );

// server

void SV_Init( void );
void SV_Shutdown( const char *finalmsg, bool reconnect );
void SV_Frame( int msec );
