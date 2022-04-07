/*
===================================================================================================

	C string manipulation tools

===================================================================================================
*/

#include "core.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "../thirdparty/stb/stb_sprintf.h"

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

void Q_strcpy_s( _Post_z_ char *pDest, strlen_t nDestSize, const char *pSrc )
{
	Assert( ( pDest && pSrc ) && nDestSize != 0 );

	char *pLast = pDest + nDestSize - 1;
	while ( ( pDest < pLast ) && ( *pSrc != 0 ) )
	{
		*pDest = *pSrc;
		++pDest; ++pSrc;
	}
	*pDest = '\0';
}

// The implementation of Q_vsprintf_s *must* be capable of taking null for dest and 0 for destsize
// to support returning of the required buffer size, stb_sprintf supports this but it is non-standard!!!

int Q_vsprintf_s( _Post_z_ char *pDest, strlen_t nDestSize, _Printf_format_string_ const char *pFmt, va_list args )
{
	return stbsp_vsnprintf( pDest, static_cast<int>( nDestSize ), pFmt, args );
}

int Q_vsprintf( _Post_z_ char *pDest, _Printf_format_string_ const char *pFmt, va_list args )
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
	Assert( n > 0 );

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
	Assert( str && substr );

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
=======================================
	String to number
=======================================
*/

template< std::signed_integral intT >
intT Str_StringToSignedInt( const char *str )
{
	intT val;
	intT sign;
	intT c;

	Assert( str );

	switch ( str[0] )
	{
	case '-':
		sign = -1;
		++str;
		break;
	case '+':
		sign = 1;
		++str;
		break;
	default:
		sign = 1;
		break;
	}

	val = 0;

	// Hex?
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val<<4) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val<<4) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val<<4) + c - 'A' + 10;
			else
				return val*sign;
		}
	}

	// Escape character?
	if (str[0] == '\'')
	{
		return sign*str[1];
	}

	// Decimal?
	while (1)
	{
		c = *str++;
		if (c <'0' || c > '9')
			return val*sign;
		val = val*10 + c - '0';
	}

	// Not a number!
	return 0;
}

template< std::unsigned_integral intT >
intT Str_StringToUnsignedInt( const char *str )
{
	intT val;
	intT c;

	Assert( str );

	val = 0;

	// Hex?
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val<<4) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val<<4) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val<<4) + c - 'A' + 10;
			else
				return val;
		}
	}

	// Escape character?
	if (str[0] == '\'')
	{
		return str[1];
	}

	// Decimal?
	while (1)
	{
		c = *str++;
		if (c <'0' || c > '9')
			return val;
		val = val*10 + c - '0';
	}

	// Not a number!
	return 0;
}

template< std::floating_point floatT, std::signed_integral intT >
floatT Str_StringToFloat( const char *str )
{
	floatT val;
	intT sign;
	intT c;
	intT decimal, total;
	intT exponent;

	Assert( str );
	
	switch ( str[0] )
	{
	case '-':
		sign = -1;
		++str;
		break;
	case '+':
		sign = 1;
		++str;
		break;
	default:
		sign = 1;
		break;
	}

	val = 0;

	// Hex?
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val*16) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val*16) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val*16) + c - 'A' + 10;
			else
				return val*sign;
		}
	}
	
	// Escape character?
	if (str[0] == '\'')
	{
		return sign*str[1];
	}
	
	// Decimal!
	decimal = -1;
	total = 0;
	exponent = 0;
	while (1)
	{
		c = *str++;
		if (c == '.')
		{
			if (decimal != -1)
			{
				break;
			}

			decimal = total;
			continue;
		}
		if (c <'0' || c > '9')
		{
			if (c == 'e' || c == 'E')
			{
				exponent = Str_StringToSignedInt<intT>(str);
			}
			break;
		}
		val = val*10 + c - '0';
		++total;
	}

	if (exponent != 0)
	{
		val *= std::pow(static_cast<floatT>(10), static_cast<floatT>(exponent));
	}
	if (decimal == -1)
		return val*sign;
	while (total > decimal)
	{
		val /= 10;
		--total;
	}
	
	return val*sign;
}

int64	Q_atoi64( const char *str )		{ return Str_StringToSignedInt<int64>( str ); }
uint64	Q_atoui64( const char *str )	{ return Str_StringToUnsignedInt<uint64>( str ); }

int32	Q_atoi32( const char *str )		{ return Str_StringToSignedInt<int32>( str ); }
uint32	Q_atoui32( const char *str )	{ return Str_StringToUnsignedInt<uint32>( str ); }

double	Q_atod( const char *str )		{ return Str_StringToFloat<double, int>( str ); }
float	Q_atof( const char *str )		{ return static_cast<float>( Q_atod( str ) ); }
// My testing has decreed that casting Q_atod to float is faster than using actual floats

/*
===================================================================================================

	Our string functions

===================================================================================================
*/

void Str_Widen( const char *pSrc, wchar_t *pDest, strlen_t nDestSize )
{
	wchar_t *pLast = pDest + nDestSize - 1;
	while ( ( pDest < pLast ) && ( *pSrc != 0 ) )
	{
		*pDest = *pSrc;
		++pDest; ++pSrc;
	}
	*pDest = 0;
}
