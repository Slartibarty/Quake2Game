//=================================================================================================
// The main header for the engine
// This file glues the client and server together
// It's like q_shared, but for the engine level
//=================================================================================================

#pragma once

// Grab q_shared
#include "../../common/q_shared.h"

// Include all our sub headers, since this is the main header for the entire engine
// We'll eventually want to use precompiled headers or a unity build, so these big includes make sense
// In all these headers we include the headers they need if they were ever to function on their own,
// this goes against the classic-quake way where you don't ever do that, but in a world of magical pragmas
// this is perfectly fine
#include "cmd.h"
#include "cmodel.h"
#include "conproc.h"
#include "crc.h"
#include "cvar.h"
#include "files.h"
#include "msg.h"
#include "net.h"
#include "pmove.h"
#include "protocol.h"
#include "sizebuf.h"
#include "sys.h"
#include "zone.h"

#include "../../common/q_formats.h" // File formats

//-------------------------------------------------------------------------------------------------

#define	VERSION "4.00"

#define	BASEDIRNAME "baseq2"

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

#ifdef NDEBUG
#define BLD_CONFIG "Release"
#else
#define BLD_CONFIG "Debug"
#endif

#define BLD_STRING BLD_PLATFORM " " BLD_CONFIG

//-------------------------------------------------------------------------------------------------
// Misc
//-------------------------------------------------------------------------------------------------

#define	EXEC_NOW	0		// don't return until completed
#define	EXEC_INSERT	1		// insert at current position, but don't run yet
#define	EXEC_APPEND	2		// add to end of the command buffer

#define	PRINT_ALL		0
#define PRINT_DEVELOPER	1	// only print when "developer 1"

typedef void (*rd_flush_t)(int target, char *buffer);

void		Com_BeginRedirect (int target, char *buffer, int buffersize, rd_flush_t flush);
void		Com_EndRedirect (void);
void		Com_Print( const char *msg );
void		Com_Printf( _Printf_format_string_ const char *fmt, ... );
void		Com_DPrint( const char *fmt );
void		Com_DPrintf( _Printf_format_string_ const char *fmt, ... );
[[noreturn]]
void		Com_Error( int code, _Printf_format_string_ const char *fmt, ... );
[[noreturn]]
void		Com_Quit( int code );

// Our copy of the command line parameters
int			COM_Argc( void );
char		*COM_Argv( int arg );	// range and null checked
void		COM_ClearArgv( int arg );
int			COM_CheckParm( char *parm );
void		COM_AddParm( char *parm );

void		COM_InitArgv( int argc, char **argv );

// Todo: Description
void		Info_Print( const char *s );

int			Com_ServerState (void);		// this should have just been a cvar...
void		Com_SetServerState (int state);

unsigned	Com_BlockChecksum (void *buffer, int length); // md4.c
byte		COM_BlockSequenceCRCByte (byte *base, int length, int sequence);

// Compressed vertex normals
#define NUMVERTEXNORMALS 162
extern vec3_t bytedirs[NUMVERTEXNORMALS];

extern	cvar_t	*developer;
extern	cvar_t	*dedicated;
extern	cvar_t	*host_speeds;
extern	cvar_t	*log_stats;

extern	FILE *log_stats_file;

// host_speeds times
extern	int		time_before_game;
extern	int		time_after_game;
extern	int		time_before_ref;
extern	int		time_after_ref;

void Engine_Init (int argc, char **argv);
void Engine_Frame (int msec);
void Engine_Shutdown (void);

//-------------------------------------------------------------------------------------------------
// client / server systems
//-------------------------------------------------------------------------------------------------

void CL_Init (void);
void CL_Drop (void);
void CL_Shutdown (void);
void CL_Frame (int msec);
void Con_Print (const char *text);
void SCR_BeginLoadingPlaque (void);

// this is in the client code, but can be used for debugging from server
void SCR_DebugGraph( float value, int color );

void SV_Init (void);
void SV_Shutdown (const char *finalmsg, qboolean reconnect);
void SV_Frame (int msec);
