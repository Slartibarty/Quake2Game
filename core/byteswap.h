/*
===================================================================================================

	Byteswapping

===================================================================================================
*/

#pragma once

#include "sys_types.h"

#define Q_BIG_ENDIAN 0

inline constexpr short ShortSwap( short s )
{
	byte b1, b2;

	b1 = s & 255;
	b2 = ( s >> 8 ) & 255;

	return ( b1 << 8 ) + b2;
}

inline constexpr int LongSwap( int l )
{
	byte b1, b2, b3, b4;

	b1 = l & 255;
	b2 = ( l >> 8 ) & 255;
	b3 = ( l >> 16 ) & 255;
	b4 = ( l >> 24 ) & 255;

	return ( (int)b1 << 24 ) + ( (int)b2 << 16 ) + ( (int)b3 << 8 ) + b4;
}

inline constexpr float FloatSwap( float f )
{
	union
	{
		float	f;
		byte	b[4];
	} dat1, dat2;

	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

#if Q_BIG_ENDIAN

inline constexpr short BigShort( short s ) { return s; }
inline constexpr short LittleShort( short s ) { return ShortSwap( s ); }

inline constexpr int BigLong( int l ) { return l; }
inline constexpr int LittleLong( int l ) { return LongSwap( l ); }

inline constexpr float BigFloat( float f ) { return f; }
inline constexpr float LittleFloat( float f ) { return FloatSwap( f ); }

#else // Little endian

inline constexpr short BigShort( short s ) { return ShortSwap( s ); }
inline constexpr short LittleShort( short s ) { return s; }

inline constexpr int BigLong( int l ) { return LongSwap( l ); }
inline constexpr int LittleLong( int l ) { return l; }

inline constexpr float BigFloat( float f ) { return FloatSwap( f ); }
inline constexpr float LittleFloat( float f ) { return f; }

#endif
