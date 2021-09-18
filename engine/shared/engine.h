//=================================================================================================
// The main header for the engine
// This file glues the client and server together
// It's like q_shared, but for the engine level
//=================================================================================================

#pragma once

#include "../../common/q_shared.h"
#include "../../common/q_formats.h" // File formats

// c++ headers
#include <vector>
#include <string>

// framework
#include "../../framework/framework_public.h"

#include "cmodel.h"
#include "conproc.h"
#include "crc.h"
#include "msg.h"
#include "net.h"
#include "protocol.h"
#include "sizebuf.h"
#include "sys.h"

/*
===================================================================================================

	Build information

===================================================================================================
*/

// The name of the engine, idTech uses this for engine variations (IE: "D3BFG", "RAGE")
#define	ENGINE_VERSION "JaffaQuake"

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

unsigned	Com_BlockChecksum( void *buffer, uint length ); // md4.c
byte		COM_BlockSequenceCRCByte( byte *base, int length, int sequence );

// Compressed vertex normals
#define NUMVERTEXNORMALS 162
extern vec3_t bytedirs[NUMVERTEXNORMALS];

extern cvar_t *		com_developer;
extern cvar_t *		dedicated;
extern cvar_t *		com_speeds;
extern cvar_t *		com_logStats;

extern fsHandle_t	log_stats_file;

extern int		curtime;

// com_speeds times
extern int		time_before_game;
extern int		time_after_game;
extern int		time_before_ref;
extern int		time_after_ref;

void Com_Init( int argc, char **argv );
void Com_Shutdown();
void Com_Frame( int frameTime );
bool Com_IsMainThread();

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
void SCR_BeginLoadingPlaque();

namespace UI::Console
{
	void Print( const char *text );
}

// this is in the client code, but can be used for debugging from server
void SCR_DebugGraph( float value, uint32 color );

// server

void SV_Init( void );
void SV_Shutdown( const char *finalmsg, bool reconnect );
void SV_Frame( int msec );
