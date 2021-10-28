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

#include "memory_impl.h"

// mem debugging, must be above the CRT headers
#if defined Q_DEBUG && defined _WIN32 && !defined Q_MEM_USE_MIMALLOC
#define Q_MEM_DEBUG
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
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

#ifndef _WIN32 // Linux
// Include our bogus sal.h
// special thanks to the Wine team!
#include "sal.h"
#endif

// entirely inline
#include "sys_defines.h"
#include "sys_types.h"
#include "utilities.h"
#include "color.h"

// partially inline
#include "memory.h"
#include "math.h"			// blahhh, nasty filename?
#include "byteswap.h"
#include "stringtools.h"
#include "sys_misc.h"

// classes
#include "labstring.h"

// optional, since it includes windows.h 
//#include "threading.h"
// optional, not everyone wants system headers
//#include "sys_includes.h"

/*
===================================================================================================

	Generic logging

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
