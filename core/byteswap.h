/*
===================================================================================================

	Byteswapping

	GCC and Clang boil these two swap functions down into three instructions
	however MSVC isn't smart enough to do that

===================================================================================================
*/

#pragma once

#define Q_BIG_ENDIAN 0

inline constexpr uint16 Swap16( uint16 d )
{
	byte b1, b2;
	b1 = d & 255;
	b2 = ( d >> 8 ) & 255;
	return ( b1 << 8 ) | b2;
}

inline constexpr uint32 Swap32( uint32 d )
{
	byte b1, b2, b3, b4;

	b1 = d & 255;
	b2 = ( d >> 8 ) & 255;
	b3 = ( d >> 16 ) & 255;
	b4 = ( d >> 24 ) & 255;

	return ( b1 << 24 ) | ( b2 << 16 ) | ( b3 << 8 ) | b4;
}

#if Q_BIG_ENDIAN

#define BigShort(s)		(s)
#define BigLong(l)		(l)
#define BigFloat(f)		(f)

#define LittleShort(s)	Swap16(s)
#define LittleLong(l)	Swap32(l)
#define LittleFloat(f)	std::bit_cast<float>(Swap32(std::bit_cast<uint32>(f)))

#else // Little endian

#define BigShort(s)		Swap16(s)
#define BigLong(l)		Swap32(l)
#define BigFloat(f)		std::bit_cast<float>(Swap32(std::bit_cast<uint32>(f)))

#define LittleShort(s)	(s)
#define LittleLong(l)	(l)
#define LittleFloat(f)	(f)

#endif
