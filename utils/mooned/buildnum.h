
#pragma once

// Haha okay, yeah.

consteval int CE_StringToSignedInt( const char *str )
{
	int val;
	int sign;
	int c;

	assert( str );

	switch ( str[0] )
	{
	case '-':
		sign = -1;
		++str;
		break;
	case '+':
		sign = 1;
		++str;
		break;
	default:
		sign = 1;
		break;
	}

	val = 0;

	// Hex?
	if ( str[0] == '0' && ( str[1] == 'x' || str[1] == 'X' ) )
	{
		str += 2;
		while ( 1 )
		{
			c = *str++;
			if ( c >= '0' && c <= '9' )
				val = ( val << 4 ) + c - '0';
			else if ( c >= 'a' && c <= 'f' )
				val = ( val << 4 ) + c - 'a' + 10;
			else if ( c >= 'A' && c <= 'F' )
				val = ( val << 4 ) + c - 'A' + 10;
			else
				return val * sign;
		}
	}

	// Escape character?
	if ( str[0] == '\'' )
	{
		return sign * str[1];
	}

	// Decimal?
	while ( 1 )
	{
		c = *str++;
		if ( c < '0' || c > '9' )
			return val * sign;
		val = val * 10 + c - '0';
	}

	// Not a number!
	return 0;
}

consteval int CE_strncasecmp( const char *s1, const char *s2, strlen_t n )
{
	assert( n > 0 );

	if ( s1 == s2 )
	{
		return 0;
	}

	// TODO: might need to cast to uint8 to preserve multibyte chars?

	while ( n-- > 0 )
	{
		int c1 = *( s1++ );
		int c2 = *( s2++ );
		if ( c1 == c2 )
		{
			if ( !c1 )
			{
				return 0;
			}
		}
		else
		{
			if ( !c2 )
			{
				return c1 - c2;
			}
			c1 = Q_tolower_fast( c1 );
			c2 = Q_tolower_fast( c2 );
			if ( c1 != c2 )
			{
				return c1 - c2;
			}
		}
	}

	return 0;
}

consteval int GetBuildNum()
{
	int b = 0;

	constexpr const char *date = __DATE__;
	constexpr const char *mon[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	constexpr int mond[12] = { 31,    28,    31,    30,    31,    30,    31,    31,    30,    31,    30,    31 };

	int m = 0;
	int d = 0;
	int y = 0;

	for ( m = 0; m < 11; ++m )
	{
		if ( CE_strncasecmp( date, mon[m], 3 ) == 0 ) {
			break;
		}
		d += mond[m];
	}

	d += CE_StringToSignedInt( date + 4 ) - 1;

	y = CE_StringToSignedInt( date + 7 ) - 1900;

	b = d + static_cast<int>( static_cast<double>( y - 1 ) * 365.25 );

	if ( ( ( y % 4 ) == 0 ) && m > 1 )
	{
		b += 1;
	}

	b -= 44132; // 30/10/2021, no relevance

	return b;
}
