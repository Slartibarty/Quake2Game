/*
===================================================================================================

	Byteswapping

===================================================================================================
*/

#pragma once

#define BIG_ENDIAN 0

short ShortSwap( short s );
int LongSwap( int l );
float FloatSwap( float f );

#if BIG_ENDIAN

inline short BigShort( short s ) { return s; }
inline short LittleShort( short s ) { return ShortSwap( s ); }

inline int BigLong( int l ) { return l; }
inline int LittleLong( int l ) { return LongSwap( l ); }

inline float BigFloat( float f ) { return f; }
inline float LittleFloat( float f ) { return FloatSwap( f ); }

#else // Little endian

inline short BigShort( short s ) { return ShortSwap( s ); }
inline short LittleShort( short s ) { return s; }

inline int BigLong( int l ) { return LongSwap( l ); }
inline int LittleLong( int l ) { return l; }

inline float BigFloat( float f ) { return FloatSwap( f ); }
inline float LittleFloat( float f ) { return f; }

#endif
