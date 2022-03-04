
#include "core.h"

/*
===================================================================================================

	Common string functions and miscellania

	These all suck, get rid of them

===================================================================================================
*/

static char core_null_string[1];

void COM_FileBase( const char *in, char *out )
{
	int len, start, end;

	len = static_cast<int>( Q_strlen( in ) );

	// Scan backwards for the extension
	end = len - 1;
	for ( ; end >= 0 && in[end] != '.' && !Str_IsPathSeparator( in[end] ); --end )
		;

	// No extension?
	if ( in[end] != '.' )
		end = len - 1;
	else
		--end;

	// Scan backwards for the last slash
	start = len - 1;
	for ( ; start >= 0 && !Str_IsPathSeparator( in[start] ); --start )
		;

	if ( start < 0 || !Str_IsPathSeparator( in[start] ) )
		start = 0;
	else
		++start;

	//memcpy( out, in + start, end - start );
	Q_strcpy_s( out, end - start + 1, in + start );
}

void COM_FilePath( const char *in, char *out )
{
	const char *s;

	s = in + strlen( in ) - 1;

	while ( s != in && *s != '/' )
		s--;

	strncpy( out, in, s - in );
	out[s - in] = 0;
}

void Com_FileSetExtension( const char *in, char *out, const char *extension )
{
	const char *ext;
	size_t difference;

	ext = strrchr( in, '.' );
	if ( !ext )
	{
		// no extension, just strcpy and strcat
		difference = strlen( in );
		memcpy( out, in, difference );
		strcpy( out + difference, extension );
		return;
	}

	difference = ext - in;

	memcpy( out, in, difference );

	strcpy( out + difference, extension );
}

char *COM_Parse( char **data_p )
{
	static char	com_token[MAX_TOKEN_CHARS];
	int			c;
	int			len;
	char *		data;

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	if ( !data )
	{
		*data_p = NULL;
		return core_null_string;
	}

// skip whitespace
skipwhite:
	while ( ( c = *data ) <= ' ' )
	{
		if ( c == 0 )
		{
			*data_p = NULL;
			return core_null_string;
		}
		data++;
	}

// skip // comments
	if ( c == '/' && data[1] == '/' )
	{
		while ( *data && *data != '\n' )
			data++;
		goto skipwhite;
	}

// handle quoted strings specially
	if ( c == '\"' )
	{
		data++;
		while ( 1 )
		{
			c = *data++;
			if ( c == '\"' || !c )
			{
				com_token[len] = 0;
				*data_p = data;
				return com_token;
			}
			if ( len < MAX_TOKEN_CHARS )
			{
				com_token[len] = c;
				len++;
			}
		}
	}

// parse a regular word
	do
	{
		if ( len < MAX_TOKEN_CHARS )
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while ( c > 32 );

	if ( len == MAX_TOKEN_CHARS )
	{
		//Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = data;
	return com_token;
}

void COM_Parse2( char **data_p, char **token_p, int tokenlen )
{
	int		c;
	int		len;
	char	*data;
	char	*token;

	data = *data_p;
	token = *token_p;
	len = 0;

	if ( !data )
	{
		*data_p = NULL;
		token[0] = '\0';
		return;
	}

// skip whitespace
skipwhite:
	while ( ( c = *data ) <= ' ' )
	{
		if ( c == 0 )
		{
			*data_p = NULL;
			token[0] = '\0';
			return;
		}
		data++;
	}

// skip // comments
	if ( c == '/' && data[1] == '/' )
	{
		while ( *data && *data != '\n' )
			data++;
		goto skipwhite;
	}

// handle quoted strings specially
	if ( c == '\"' )
	{
		data++;
		while ( 1 )
		{
			c = *data++;
			if ( c == '\"' || !c )
			{
				token[len] = 0;
				*data_p = data;
				return;
			}
			if ( len < tokenlen )
			{
				token[len] = c;
				len++;
			}
		}
	}

// parse a regular word
	do
	{
		if ( len < tokenlen )
		{
			token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while ( c > 32 );

	if ( len == tokenlen )
	{
		//Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	token[len] = 0;

	*data_p = data;
}

float frand()
{
	return (rand()&32767) * (1.0f/32767.0f);
}

float crand()
{
	return (rand()&32767) * (2.0f/32767.0f) - 1.0f;
}

char *va( _Printf_format_string_ const char *fmt, ... )
{
	static char string[MAX_PRINT_MSG];
	va_list argptr;

	va_start( argptr, fmt );
	Q_vsprintf_s( string, fmt, argptr );
	va_end( argptr );

	return string;
}
