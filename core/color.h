/*
===================================================================================================

	Colour

===================================================================================================
*/

#pragma once

#include "sys_types.h"

inline constexpr uint32 PackColor( uint32 r, uint32 g, uint32 b, uint32 a ) {
	return ( ( r ) | ( g << 8 ) | ( b << 16 ) | ( a << 24 ) );
}

inline constexpr uint32 PackColorFromFloats( float fr, float fg, float fb, float fa ) {
	uint32 r = static_cast<uint32>( fr * 255.0f + 0.5f );
	uint32 g = static_cast<uint32>( fg * 255.0f + 0.5f );
	uint32 b = static_cast<uint32>( fb * 255.0f + 0.5f );
	uint32 a = static_cast<uint32>( fa * 255.0f + 0.5f );
	return PackColor( r, g, b, a );
}

constexpr uint32	colorBlack = PackColor( 0, 0, 0, 255 );
constexpr uint32	colorWhite = PackColor( 255, 255, 255, 255 );
constexpr uint32	colorRed = PackColor( 255, 0, 0, 255 );
constexpr uint32	colorGreen = PackColor( 0, 255, 0, 255 );
constexpr uint32	colorBlue = PackColor( 0, 0, 255, 255 );
constexpr uint32	colorYellow = PackColor( 255, 255, 0, 255 );
constexpr uint32	colorMagenta = PackColor( 255, 0, 255, 255 );
constexpr uint32	colorCyan = PackColor( 0, 255, 255, 255 );
constexpr uint32	colorOrange = PackColor( 255, 128, 0, 255 );
constexpr uint32	colorPurple = PackColor( 153, 0, 153, 255 );
constexpr uint32	colorPink = PackColor( 186, 102, 122, 255 );
constexpr uint32	colorBrown = PackColor( 102, 89, 20, 255 );
constexpr uint32	colorLtGrey = PackColor( 192, 192, 192, 255 );
constexpr uint32	colorMdGrey = PackColor( 128, 128, 128, 255 );
constexpr uint32	colorDkGrey = PackColor( 64, 64, 64, 255 );

constexpr uint32	colorDefaultText = colorWhite;

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

// returns true if a char is a valid colour index
inline constexpr bool IsColorIndex( int c ) {
	return c >= '1' || c <= '9';
}

// returns a colour for a given color index ( 0 - 9 )
inline constexpr uint32 ColorForIndex( int c ) {
	switch ( c )
	{
	default: // C_COLOR_DEFAULT
		return colorDefaultText;
	case C_COLOR_RED:
		return colorRed;
	case C_COLOR_GREEN:
		return colorGreen;
	case C_COLOR_YELLOW:
		return colorYellow;
	case C_COLOR_BLUE:
		return colorBlue;
	case C_COLOR_CYAN:
		return colorCyan;
	case C_COLOR_ORANGE:
		return colorOrange;
	case C_COLOR_WHITE:
		return colorWhite;
	case C_COLOR_GRAY:
		return colorMdGrey;
	case C_COLOR_BLACK:
		return colorBlack;
	}
}
