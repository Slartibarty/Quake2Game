/*
===================================================================================================

	Colour

===================================================================================================
*/

#pragma once

#include "sys_types.h"

inline constexpr uint32 PackColor( uint32 r, uint32 g, uint32 b, uint32 a )
{
	return ( ( r ) | ( g << 8 ) | ( b << 16 ) | ( a << 24 ) );
}

inline constexpr uint32 PackColorFromFloats( float fr, float fg, float fb, float fa )
{
	uint32 r = static_cast<uint32>( fr * 255.0f + 0.5f );
	uint32 g = static_cast<uint32>( fg * 255.0f + 0.5f );
	uint32 b = static_cast<uint32>( fb * 255.0f + 0.5f );
	uint32 a = static_cast<uint32>( fa * 255.0f + 0.5f );
	return PackColor( r, g, b, a );
}

namespace colors
{
	inline constexpr uint32 black		= PackColor( 0, 0, 0, 255 );
	inline constexpr uint32 white		= PackColor( 255, 255, 255, 255 );
	inline constexpr uint32 red			= PackColor( 255, 0, 0, 255 );
	inline constexpr uint32 green		= PackColor( 0, 255, 0, 255 );
	inline constexpr uint32 blue		= PackColor( 0, 0, 255, 255 );
	inline constexpr uint32 yellow		= PackColor( 255, 255, 0, 255 );
	inline constexpr uint32 magenta		= PackColor( 255, 0, 255, 255 );
	inline constexpr uint32 cyan		= PackColor( 0, 255, 255, 255 );
	inline constexpr uint32 orange		= PackColor( 255, 128, 0, 255 );
	inline constexpr uint32 purple		= PackColor( 153, 0, 153, 255 );
	inline constexpr uint32 pink		= PackColor( 186, 102, 122, 255 );
	inline constexpr uint32 brown		= PackColor( 102, 89, 20, 255 );
	inline constexpr uint32 ltGray		= PackColor( 192, 192, 192, 255 );
	inline constexpr uint32 mdGray		= PackColor( 128, 128, 128, 255 );
	inline constexpr uint32 dkGray		= PackColor( 64, 64, 64, 255 );

	inline constexpr uint32 defaultText	= white;
}

// colour escape characters
#define C_COLOR_ESCAPE			'^'
#define C_COLOR_DEFAULT			'0'
#define C_COLOR_RED				'1'
#define C_COLOR_GREEN			'2'
#define C_COLOR_YELLOW			'3'
#define C_COLOR_BLUE			'4'
#define C_COLOR_CYAN			'5'
#define C_COLOR_ORANGE			'6'
#define C_COLOR_WHITE			'7'
#define C_COLOR_GRAY			'8'
#define C_COLOR_BLACK			'9'

// colour escape strings
#ifndef Q_CONSOLE_APP
#define S_COLOR_DEFAULT			"^0"
#define S_COLOR_RED				"^1"
#define S_COLOR_GREEN			"^2"
#define S_COLOR_YELLOW			"^3"
#define S_COLOR_BLUE			"^4"
#define S_COLOR_CYAN			"^5"
#define S_COLOR_ORANGE			"^6"
#define S_COLOR_WHITE			"^7"
#define S_COLOR_GRAY			"^8"
#define S_COLOR_BLACK			"^9"
#define S_COLOR_RESET
#else
#define S_COLOR_DEFAULT			"\u001b[0m"
#define S_COLOR_RED				"\u001b[31;1m"
#define S_COLOR_GREEN			"\u001b[32;1m"
#define S_COLOR_YELLOW			"\u001b[33;1m"
#define S_COLOR_BLUE			"\u001b[34;1m"
#define S_COLOR_CYAN			"\u001b[36;1m"
#define S_COLOR_ORANGE			S_COLOR_RED
#define S_COLOR_WHITE			"\u001b[37;1m"
#define S_COLOR_GRAY			S_COLOR_WHITE
#define S_COLOR_BLACK			S_COLOR_WHITE
#define S_COLOR_RESET			S_COLOR_DEFAULT
#endif

// returns true if a char is a valid colour index
inline constexpr bool IsColorIndex( int ch )
{
	return ch >= '0' && ch <= '9';
}

// returns a colour for a given colour index ( 0 - 9 )
inline constexpr uint32 ColorForIndex( int ch )
{
	switch ( ch )
	{
	default: // C_COLOR_DEFAULT
		return colors::defaultText;
	case C_COLOR_RED:
		return colors::red;
	case C_COLOR_GREEN:
		return colors::green;
	case C_COLOR_YELLOW:
		return colors::yellow;
	case C_COLOR_BLUE:
		return colors::blue;
	case C_COLOR_CYAN:
		return colors::cyan;
	case C_COLOR_ORANGE:
		return colors::orange;
	case C_COLOR_WHITE:
		return colors::white;
	case C_COLOR_GRAY:
		return colors::mdGray;
	case C_COLOR_BLACK:
		return colors::black;
	}
}
