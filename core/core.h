/*
===================================================================================================

	Core

	A statically linked support library for all of JaffaQuake.
	This header will be included globally from all translation units.

	The entire library should use unity builds to build real fast, but Premake doesn't support
	them yet.

===================================================================================================
*/

#pragma once

// mem debugging, must be above the CRT headers
#if defined Q_DEBUG && defined _WIN32
#define Q_MEM_DEBUG
#define _CRTDBG_MAP_ALLOC
#endif

#include <cstddef>
#include <cstdint>
#include <climits>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cctype>
#include <ctime>
#include <cassert>

#include <concepts>
#include <numbers>

#ifdef Q_MEM_DEBUG
#include <crtdbg.h>
#endif

#ifndef _WIN32
// Include our bogus sal.h
// special thanks to the Wine team!
#include "sal.h"
#endif

#include "sys_defines.h"
#include "sys_types.h"
#include "utilities.h"
#include "color.h"

#include "math.h"			// blahhh, nasty filename?
#include "byteswap.h"
#include "stringtools.h"

// optional, since it includes windows.h 
//#include "threading.h"

/*
===================================================================================================

	Logging

	These functions must be implemented by the client application

===================================================================================================
*/

void Com_Print( const char *msg );
void Com_Printf( _Printf_format_string_ const char *fmt, ... );

void Com_DPrint( const char *msg );
void Com_DPrintf( _Printf_format_string_ const char *fmt, ... );

[[noreturn]] void Com_Error( const char *msg );
[[noreturn]] void Com_Errorf( _Printf_format_string_ const char *fmt, ... );
[[noreturn]] void Com_FatalError( const char *msg );
[[noreturn]] void Com_FatalErrorf( _Printf_format_string_ const char *fmt, ... );

/*
===================================================================================================

	Platform specific functionality - misc_xxx.cpp

===================================================================================================
*/

/*
=======================================
	The soon to be dead hunk allocator
=======================================
*/

void *	Hunk_Begin( size_t maxsize );
void *	Hunk_Alloc( size_t size );
void	Hunk_Free( void *buf );
size_t	Hunk_End();

/*
=======================================
	Miscellaneous
=======================================
*/

void	Sys_CopyFile( const char *src, const char *dst );
void	Sys_CreateDirectory( const char *path );

/*
=======================================
	Timing
=======================================
*/

extern int curtime; // time returned by the last Sys_Milliseconds

void	Time_Init();
double	Time_FloatSeconds();
double	Time_FloatMilliseconds();
double	Time_FloatMicroseconds();

/*int64	Time_Seconds();*/
int64	Time_Milliseconds();
int64	Time_Microseconds();

// Legacy
int		Sys_Milliseconds();

/*
=======================================
	Directory searching
=======================================
*/

// directory searching
#define SFF_ARCH	0x01
#define SFF_HIDDEN	0x02
#define SFF_RDONLY	0x04
#define SFF_SUBDIR	0x08
#define SFF_SYSTEM	0x10

// pass in an attribute mask of things you wish to REJECT
char *	Sys_FindFirst( const char *path, unsigned musthave, unsigned canthave );
char *	Sys_FindNext( unsigned musthave, unsigned canthave );
void	Sys_FindClose( void );

/*
===================================================================================================

	Common string functions and miscellania

	Please make these functions go away

===================================================================================================
*/

inline bool Str_IsPathSeparator( int a ) {
	return ( a == '/' || a == '\\' );
}

// this sucks, deprecated
inline void COM_StripExtension( const char *in, char *out )
{
	while ( *in && *in != '.' )
		*out++ = *in++;
	*out = '\0';
}

// extract the filename from a path
void COM_FileBase( const char *in, char *out );

// returns the path up to, but not including the last /
void COM_FilePath( const char *in, char *out );

// Set a filename's extension
// extension should have the period
void Com_FileSetExtension( const char *in, char *out, const char *extension );

// Parse a token out of a string
// data is an in/out parm, returns a parsed out token
char *COM_Parse( char **data_p );
// version 2, takes a local token
void COM_Parse2( char **data_p, char **token_p, int tokenlen );

float frand();	// deprecated, 0 to 1
float crand();	// deprecated, -1 to 1

char *va( _Printf_format_string_ const char *fmt, ... );		// avoid
