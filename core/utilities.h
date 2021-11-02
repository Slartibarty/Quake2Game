/*
===================================================================================================

	Basic utilities

===================================================================================================
*/

#pragma once

#include "sys_types.h"

//
// Macros
//

#define BIT(num)		(1<<(num))

// This only works with arrays, this isn't a consteval template because
// we need to be convertible to any integer type
#define countof(a)		(sizeof(a)/sizeof(*a))

//
// Templates
//

template< typename T >
inline constexpr T Min( const T valMin, const T valMax ) {
	static_assert( sizeof( T ) <= sizeof( int64 ), "Min should only be used with basic types!" );
	return valMin < valMax ? valMin : valMax;
}

template< typename T >
inline constexpr T Max( const T valMin, const T valMax ) {
	static_assert( sizeof( T ) <= sizeof( int64 ), "Max should only be used with basic types!" );
	return valMin > valMax ? valMin : valMax;
}

template< typename T >
inline constexpr T Clamp( const T val, const T valMin, const T valMax ) {
	static_assert( sizeof( T ) <= sizeof( int64 ), "Clamp should only be used with basic types!" );
	return Min( Max( val, valMin ), valMax );
}

inline consteval int32 MakeFourCC( int32 d, int32 c, int32 b, int32 a ) {
	return ( ( a << 24 ) | ( b << 16 ) | ( c << 8 ) | ( d ) );
}
