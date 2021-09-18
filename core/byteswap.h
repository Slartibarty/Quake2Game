/*
===================================================================================================

	Byteswapping

	MSVC isn't smart enough to detect that these are byteswap functions, so we have special logic
	for it here

===================================================================================================
*/

#pragma once

// This was Q_BIG_ENDIAN, but Qt ate that macro...
#define SYS_BIG_ENDIAN 0

inline constexpr uint16 Swap16_C( uint16 d )
{
	byte b1, b2;
	b1 = d & 255;
	b2 = ( d >> 8 ) & 255;
	return ( b1 << 8 ) | b2;
}

inline constexpr uint32 Swap32_C( uint32 d )
{
	byte b1, b2, b3, b4;
	b1 = d & 255;
	b2 = ( d >> 8 ) & 255;
	b3 = ( d >> 16 ) & 255;
	b4 = ( d >> 24 ) & 255;
	return ( b1 << 24 ) | ( b2 << 16 ) | ( b3 << 8 ) | b4;
}

#if defined _MSC_VER		// MSVC (What about MinGW and Clang for Windows)

#define Swap16(d) _byteswap_ushort(d)
#define Swap32(d) _byteswap_ulong(d)
#define Swap64(d) _byteswap_uint64(d)

#elif defined __GNUC__		// GCC or Clang

#define Swap16(d) __builtin_bswap16(d)
#define Swap32(d) __builtin_bswap32(d)
#define Swap64(d) __builtin_bswap64(d)

#else						// N/A, native code

#pragma message( "TODO: Using non-intrinsic byteswap functions..." )

#define Swap16 Swap16_C
#define Swap32 Swap32_C
#define Swap64 BYTESWAP_FIXME

#endif

#if SYS_BIG_ENDIAN

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
