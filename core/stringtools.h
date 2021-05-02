/*
===================================================================================================

	C string manipulation tools

===================================================================================================
*/

#pragma once

#include <cstdarg>
#include <cstring>

#include "sys_types.h"

/*
===================================================================================================

	Our string functions

===================================================================================================
*/

inline void Str_Substitute( char *pStr, int a, int b )
{
	for ( ; *pStr; ++pStr )
	{
		if ( *pStr == a )
		{
			*pStr = b;
		}
	}
}

inline void Str_FixSlashes( char *pStr ) {
	Str_Substitute( pStr, '\\', '/' );
}

/*
===================================================================================================

	Standard library string function replacements

===================================================================================================
*/

using strlen_t = uint32;

inline strlen_t Q_strlen( const char *str ) {
	return static_cast<strlen_t>( strlen( str ) );
}

/*
=======================================
	String modification
=======================================
*/

void Q_strcpy_s( char *pDest, strlen_t nDestSize, const char *pSrc );

template< strlen_t nDestSize >
inline void Q_strcpy_s( char( &pDest )[nDestSize], const char *pSrc ) {
	Q_strcpy_s( pDest, nDestSize, pSrc );
}

void Q_vsprintf_s( char *pDest, strlen_t nDestSize, _Printf_format_string_ const char *pFmt, va_list args );

template< strlen_t nDestSize >
inline void Q_vsprintf_s( char( &pDest )[nDestSize], _Printf_format_string_ const char *pFmt, va_list args )
{
	Q_vsprintf_s( pDest, nDestSize, pFmt, args );
}

inline void Q_sprintf_s( char *pDest, strlen_t nDestSize, _Printf_format_string_ const char *pFmt, ... )
{
	va_list args;
	va_start( args, pFmt );
	Q_vsprintf_s( pDest, nDestSize, pFmt, args );
	va_end( args );
}

template< strlen_t nDestSize >
inline void Q_sprintf_s( char( &pDest )[nDestSize], _Printf_format_string_ const char *pFmt, ... )
{
	va_list args;
	va_start( args, pFmt );
	Q_vsprintf_s( pDest, nDestSize, pFmt, args );
	va_end( args );
}

void Q_vsprintf( char *pDest, _Printf_format_string_ const char *pFmt, va_list args );

inline void Q_sprintf( char *pDest, _Printf_format_string_ const char *pFmt, ... )
{
	va_list args;
	va_start( args, pFmt );
	Q_vsprintf( pDest, pFmt, args );
	va_end( args );
}

/*
=======================================
	String comparisons
=======================================
*/

inline int Q_strcmp( const char *s1, const char *s2 ) {
	return s1 == s2 ? 0 : strcmp( s1, s2 );
}

inline int Q_strncmp( const char *s1, const char *s2, strlen_t maxcount ) {
	return s1 == s2 ? 0 : strncmp( s1, s2, maxcount );
}

int Q_strcasecmp( const char *s1, const char *s2 );
int Q_strncasecmp( const char *s1, const char *s2, strlen_t n );

inline int Q_stricmp( const char *s1, const char *s2 ) {
	return Q_strcasecmp( s1, s2 );
}

/*
=======================================
	String to upper / lower case
=======================================
*/

inline int Q_tolower( int ch ) {
	return ( ch <= 'Z' && ch >= 'A' ) ? ( ch + ( 'a' - 'A' ) ) : ch;
}

inline int Q_toupper( int ch ) {
	return ( ch >= 'a' && ch <= 'z' ) ? ( ch - ( 'a' - 'A' ) ) : ch;
}

inline void Q_strlwr( char *dest ) {
	while ( *dest )
	{
		*dest = Q_tolower( *dest );
		++dest;
	}
}

inline void Q_strupr( char *dest ) {
	while ( *dest )
	{
		*dest = Q_toupper( *dest );
		++dest;
	}
}
