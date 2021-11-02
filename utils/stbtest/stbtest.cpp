
#define TEST_SPRINTF

#if 1

#define USE_STB 1

#if USE_STB
# define STB_SPRINTF_IMPLEMENTATION
# define STB_SPRINTF_WIDE
# define STB_SPRINTF_NOFLOAT
# include "stb_sprintf2.h"
# define SPRINTF stbsp_sprintf
# define SNPRINTF stbsp_snprintf
#else
# include <locale.h>
# define SPRINTF sprintf
# define SNPRINTF snprintf
#endif

#ifdef STB_SPRINTF_WIDE
#define TXT_(quote)		L##quote
#define TXT(quote)		TXT_(quote)
#else
#define TXT_(quote)		(quote)
#define TXT(quote)		(quote)
#endif

#include <assert.h>
#include <math.h>    // for INFINITY, NAN
#include <stddef.h>  // for ptrdiff_t
#include <stdio.h>   // for printf
#include <string.h>  // for strcmp, strncmp, strlen

#if _MSC_VER && _MSC_VER <= 1600 
typedef int intmax_t;
typedef ptrdiff_t ssize_t;
#else
#include <stdint.h>  // for intmax_t, ssize_t
#endif

using ssize_t = int64_t;

// stbsp_sprintf
#define CHECK_END(str) \
   if (wcscmp(buf, str) != 0 || (unsigned) ret != wcslen(str)) { \
      wprintf(L"< '%s'\n> '%s'\n", str, buf); \
      assert(!"Fail"); \
   }

#define CHECK(str, fmt, ...) { int ret = SPRINTF(buf, fmt, __VA_ARGS__); CHECK_END(str); }

#ifdef TEST_SPRINTF
int main()
{
	wchar_t buf[1024];
	int n = 0;
	const double pow_2_75 = 37778931862957161709568.0;
	const double pow_2_85 = 38685626227668133590597632.0;

	// integers
	CHECK( L"a b     1", L"%c %s     %d", 'a', L"b", 1 );
	CHECK( L"abc     ", L"%-8.3s", L"abcdefgh" );
	CHECK( L"+5", L"%+2d", 5 );
	CHECK( L"  6", L"% 3i", 6 );
	CHECK( L"-7  ", L"%-4d", -7 );
	CHECK( L"+0", L"%+d", 0 );
	CHECK( L"     00003:     00004", L"%10.5d:%10.5d", 3, 4 );
	CHECK( L"-100006789", L"%d", -100006789 );
	CHECK( L"20 0020", L"%u %04u", 20u, 20u );
	CHECK( L"12 1e 3C", L"%o %x %X", 10u, 30u, 60u );
	CHECK( L" 12 1e 3C ", L"%3o %2x %-3X", 10u, 30u, 60u );
	CHECK( L"012 0x1e 0X3C", L"%#o %#x %#X", 10u, 30u, 60u );
	CHECK( L"", L"%.0x", 0 );
#if USE_STB
	CHECK( L"0", L"%.0d", 0 );  // stb_sprintf gives "0"
#else
	CHECK( L"", L"%.0d", 0 );  // glibc gives "" as specified by C99(?)
#endif
	CHECK( L"33 555", L"%hi %ld", (short)33, 555l );
#if !defined(_MSC_VER) || _MSC_VER >= 1600
	CHECK( L"9888777666", L"%llu", 9888777666llu );
#endif
	CHECK( L"-1 2 -3", L"%ji %zi %ti", (intmax_t)-1, (ssize_t)2, (ptrdiff_t)-3 );

	// floating-point numbers
	CHECK( L"-3.000000", L"%f", -3.0 );
	CHECK( L"-8.8888888800", L"%.10f", -8.88888888 );
	CHECK( L"880.0888888800", L"%.10f", 880.08888888 );
	CHECK( L"4.1", L"%.1f", 4.1 );
	CHECK( L" 0", L"% .0f", 0.1 );
	CHECK( L"0.00", L"%.2f", 1e-4 );
	CHECK( L"-5.20", L"%+4.2f", -5.2 );
	CHECK( L"0.0       ", L"%-10.1f", 0. );
	CHECK( L"-0.000000", L"%f", -0. );
	CHECK( L"0.000001", L"%f", 9.09834e-07 );
#if USE_STB  // rounding differences
	CHECK( L"38685626227668133600000000.0", L"%.1f", pow_2_85 );
	CHECK( L"0.000000499999999999999978", L"%.24f", 5e-7 );
#else
	CHECK( L"38685626227668133590597632.0", L"%.1f", pow_2_85 ); // exact
	CHECK( L"0.000000499999999999999977", L"%.24f", 5e-7 );
#endif
	CHECK( L"0.000000000000000020000000", L"%.24f", 2e-17 );
	CHECK( L"0.0000000100 100000000", L"%.10f %.0f", 1e-8, 1e+8 );
	CHECK( L"100056789.0", L"%.1f", 100056789.0 );
	CHECK( L" 1.23 %", L"%*.*f %%", 5, 2, 1.23 );
	CHECK( L"-3.000000e+00", L"%e", -3.0 );
	CHECK( L"4.1E+00", L"%.1E", 4.1 );
	CHECK( L"-5.20e+00", L"%+4.2e", -5.2 );
	CHECK( L"+0.3 -3", L"%+g %+g", 0.3, -3.0 );
	CHECK( L"4", L"%.1G", 4.1 );
	CHECK( L"-5.2", L"%+4.2g", -5.2 );
	CHECK( L"3e-300", L"%g", 3e-300 );
	CHECK( L"1", L"%.0g", 1.2 );
	CHECK( L" 3.7 3.71", L"% .3g %.3g", 3.704, 3.706 );
	CHECK( L"2e-315:1e+308", L"%g:%g", 2e-315, 1e+308 );

#if USE_STB
	CHECK( L"Inf Inf NaN", L"%g %G %f", INFINITY, INFINITY, NAN );
	CHECK( L"N", L"%.1g", NAN );
#else
	CHECK( L"inf INF nan", L"%g %G %f", INFINITY, INFINITY, NAN );
	CHECK( L"nan", L"%.1g", NAN );
#endif

	// %n
	CHECK( L"aaa ", L"%.3s %n", L"aaaaaaaaaaaaa", &n );
	assert( n == 4 );

	// hex floats
	CHECK( L"0x1.fedcbap+98", L"%a", 0x1.fedcbap+98 );
	CHECK( L"0x1.999999999999a0p-4", L"%.14a", 0.1 );
	CHECK( L"0x1.0p-1022", L"%.1a", 0x1.ffp-1023 );
#if USE_STB  // difference in default precision and x vs X for %A
	CHECK( L"0x1.009117p-1022", L"%a", 2.23e-308 );
	CHECK( L"-0x1.AB0P-5", L"%.3A", -0x1.abp-5 );
#else
	CHECK( L"0x1.0091177587f83p-1022", L"%a", 2.23e-308 );
	CHECK( L"-0X1.AB0P-5", L"%.3A", -0X1.abp-5 );
#endif

	// %p
#if USE_STB
	CHECK( L"0000000000000000", L"%p", (void*)NULL );
#else
	CHECK( L"(nil)", L"%p", (void*)NULL );
#endif

	// snprintf
	assert( SNPRINTF( buf, 100, L" %s     %d", L"b", 123 ) == 10 );
	assert( wcscmp( buf, L" b     123" ) == 0 );
	assert( SNPRINTF( buf, 100, L"%f", pow_2_75 ) == 30 );
	assert( wcsncmp( buf, L"37778931862957161709568.000000", 17 ) == 0 );
	n = SNPRINTF( buf, 10, L"number %f", 123.456789 );
	assert( wcscmp( buf, L"number 12" ) == 0 );
	assert( n == 17 );  // written vs would-be written bytes
	n = SNPRINTF( buf, 0, L"7 chars" );
	assert( n == 7 );
	// stb_sprintf uses internal buffer of 512 chars - test longer string
	assert( SPRINTF( buf, L"%d  %600s", 3, L"abc" ) == 603 );
	assert( wcslen( buf ) == 603 );
	SNPRINTF( buf, 550, L"%d  %600s", 3, L"abc" );
	assert( wcslen( buf ) == 549 );
	assert( SNPRINTF( buf, 600, L"%510s     %c", L"a", 'b' ) == 516 );

	// length check
	assert( SNPRINTF( NULL, 0, L" %s     %d", L"b", 123 ) == 10 );

	// ' modifier. Non-standard, but supported by glibc.
#if !USE_STB
	setlocale( LC_NUMERIC, L"" );  // C locale does not group digits
#endif
	CHECK( L"1,200,000", L"%'d", 1200000 );
	CHECK( L"-100,006,789", L"%'d", -100006789 );
#if !defined(_MSC_VER) || _MSC_VER >= 1600
	CHECK( L"9,888,777,666", L"%'lld", 9888777666ll );
#endif
	CHECK( L"200,000,000.000000", L"%'18f", 2e8 );
	CHECK( L"100,056,789", L"%'.0f", 100056789.0 );
	CHECK( L"100,056,789.0", L"%'.1f", 100056789.0 );
#if USE_STB  // difference in leading zeros
	CHECK( L"000,001,200,000", L"%'015d", 1200000 );
#else
	CHECK( L"0000001,200,000", L"%'015d", 1200000 );
#endif

	// things not supported by glibc
#if USE_STB
	CHECK( L"null", L"%s", (char*)NULL );
	CHECK( L"123,4abc:", L"%'x:", 0x1234ABC );
	CHECK( L"100000000", L"%b", 256 );
	CHECK( L"0b10 0B11", L"%#b %#B", 2, 3 );
#if !defined(_MSC_VER) || _MSC_VER >= 1600
	CHECK( L"2 3 4", L"%I64d %I32d %Id", 2ll, 3, 4ll );
#endif
	CHECK( L"1k 2.54 M", L"%$_d %$.2d", 1000, 2536000 );
	CHECK( L"2.42 Mi 2.4 M", L"%$$.2d %$$$d", 2536000, 2536000 );

	// different separators
	stbsp_set_separators( ' ', ',' );
	CHECK( L"12 345,678900", L"%'f", 12345.6789 );
#endif

	return 0;
}
#endif

#else

#include "../../core/core.h"

#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_WIDE
#define STB_SPRINTF_NOFLOAT
#include "stb_sprintf2.h"

#ifdef STB_SPRINTF_WIDE
#define TXT_(quote)		L##quote
#define TXT(quote)		TXT_(quote)
#else
#define TXT_(quote)		(quote)
#define TXT(quote)		(quote)
#endif

#define countof(a) (sizeof(a)/sizeof(*a))

int main()
{
	wchar_t buf[128];
	stbsp_snprintf( buf, countof( buf ), TXT( "%s is %d and %f\n" ), TXT( "Blargers" ), 24, 19.0 );

	return 0;
}

#endif
