/*
===================================================================================================

	C string manipulation tools

===================================================================================================
*/

#include "core.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

/*
===================================================================================================

	Standard library string function replacements

===================================================================================================
*/

/*
=======================================
	String modification
=======================================
*/

void Q_strcpy_s( char *pDest, strlen_t nDestSize, const char *pSrc )
{
	assert( pDest && pSrc );

	char *pLast = pDest + nDestSize - 1;
	while ( ( pDest < pLast ) && ( *pSrc != 0 ) )
	{
		*pDest = *pSrc;
		++pDest; ++pSrc;
	}
	*pDest = '\0';
}

int Q_vsprintf_s( char *pDest, strlen_t nDestSize, _Printf_format_string_ const char *pFmt, va_list args )
{
	return stbsp_vsnprintf( pDest, static_cast<int>( nDestSize ), pFmt, args );
}

int Q_vsprintf( char *pDest, _Printf_format_string_ const char *pFmt, va_list args )
{
	return stbsp_vsprintf( pDest, pFmt, args );
}

/*
=======================================
	String comparisons
=======================================
*/

int Q_strcasecmp( const char *s1, const char *s2 )
{
	if ( s1 == s2 )
	{
		return 0;
	}

	// TODO: might need to cast to uint8 to preserve multibyte chars?

	// inner loop is duplicated twice in an attempt to improve loop unrolling
	for ( ;; )
	{
		int c1 = *( s1++ );
		int c2 = *( s2++ );
		if ( c1 == c2 )
		{
			if ( !c1 )
			{
				return 0;
			}
		}
		else
		{
			if ( !c2 )
			{
				return c1 - c2;
			}
			c1 = Q_tolower_fast( c1 );
			c2 = Q_tolower_fast( c2 );
			if ( c1 != c2 )
			{
				return c1 - c2;
			}
		}
		c1 = *( s1++ );
		c2 = *( s2++ );
		if ( c1 == c2 )
		{
			if ( !c1 )
			{
				return 0;
			}
		}
		else
		{
			if ( !c2 )
			{
				return c1 - c2;
			}
			c1 = Q_tolower_fast( c1 );
			c2 = Q_tolower_fast( c2 );
			if ( c1 != c2 )
			{
				return c1 - c2;
			}
		}
	}
}

int Q_strncasecmp( const char *s1, const char *s2, strlen_t n )
{
	assert( n > 0 );

	if ( s1 == s2 )
	{
		return 0;
	}

	// TODO: might need to cast to uint8 to preserve multibyte chars?

	while ( n-- > 0 )
	{
		int c1 = *( s1++ );
		int c2 = *( s2++ );
		if ( c1 == c2 )
		{
			if ( !c1 )
			{
				return 0;
			}
		}
		else
		{
			if ( !c2 )
			{
				return c1 - c2;
			}
			c1 = Q_tolower_fast( c1 );
			c2 = Q_tolower_fast( c2 );
			if ( c1 != c2 )
			{
				return c1 - c2;
			}
		}
	}

	return 0;
}

const char *Q_stristr( const char *str, const char *substr )
{
	assert( str );
	assert( substr );

	const char *letter = str;

	while ( *letter != '\0' )
	{
		if ( Q_tolower_fast( *letter ) == Q_tolower_fast( *substr ) )
		{
			const char *match = letter + 1;
			const char *test = substr + 1;
			while ( *test != '\0' )
			{
				if ( *match == 0 ) {
					return nullptr;
				}

				if ( Q_tolower_fast( *match ) != Q_tolower_fast( *test ) ) {
					break;
				}

				++match;
				++test;
			}

			if ( *test == '\0' ) {
				return letter;
			}
		}

		++letter;
	}

	return nullptr;
}

/*
===================================================================================================

	Our string functions

===================================================================================================
*/

void Str_NarrowToWide( const char *pNarrowString, wchar_t *pWideString )
{
	strlen_t i;
	strlen_t narrowLength = Q_strlen( pNarrowString );

	for ( i = 0; i < narrowLength; ++i )
	{
		// blind expansion
		pWideString[i] = static_cast<wchar_t>( pNarrowString[i] );
	}

	pWideString[i] = 0;
}
