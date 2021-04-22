/*
===================================================================================================

	Basic utilities

===================================================================================================
*/

#pragma once

#include "sys_types.h"

template< typename T >
inline constexpr T Min( const T valMin, const T valMax ) {
	return valMin < valMax ? valMin : valMax;
}

template< typename T >
inline constexpr T Max( const T valMin, const T valMax ) {
	return valMin > valMax ? valMin : valMax;
}

template< typename T >
inline constexpr T Clamp( const T val, const T valMin, const T valMax ) {
	return Min( Max( val, valMin ), valMax );
}

inline consteval int32 MakeFourCC( int32 d, int32 c, int32 b, int32 a ) {
	return ( ( a << 24 ) | ( b << 16 ) | ( c << 8 ) | ( d ) );
}
