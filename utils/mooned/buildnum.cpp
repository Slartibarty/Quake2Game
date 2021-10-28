
#include "mooned_local.h"

static constexpr const char *mon[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static constexpr int mond[12]        = { 31,    28,    31,    30,    31,    30,    31,    31,    30,    31,    30,    31 };

int GetBuildNum()
{
	static int b = 0;

	if ( b != 0 ) {
		return b;
	}

	constexpr const char *date = __DATE__;

	int m = 0;
	int d = 0;
	int y = 0;

	for ( m = 0; m < 11; ++m )
	{
		if ( Q_strnicmp( date, mon[m], 3 ) == 0 ) {
			break;
		}
		d += mond[m];
	}

	d += Q_atoi( date + 4 ) - 1;

	y = Q_atoi( date + 7 ) - 1900;

	b = d + static_cast<int>( static_cast<double>( y - 1 ) * 365.25 );

	if ( ( ( y % 4 ) == 0 ) && m > 1 )
	{
		b += 1;
	}

	b -= 44117; // 34995

	return b;
}
