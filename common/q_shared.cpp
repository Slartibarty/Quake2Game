//-------------------------------------------------------------------------------------------------
// Shared stuff available to all program modules
//-------------------------------------------------------------------------------------------------

#include "q_shared.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "../external/stb/stb_sprintf.h"

//-------------------------------------------------------------------------------------------------
//
// Misc
//
//-------------------------------------------------------------------------------------------------

char null_string[1] = "";

//-------------------------------------------------------------------------------------------------
// Extract the filename from a path
//-------------------------------------------------------------------------------------------------
void COM_FileBase( const char *in, char *out )
{
	strlen_t len, start, end;

	len = Q_strlen( in );

	// Scan backwards for the extension
	end = len - 1;
	for ( ; end >= 0 && in[end] != '.' && !Q_IsPathSeparator( in[end] ); --end )
		;

	// No extension?
	if ( in[end] != '.' )
		end = len - 1;
	else
		--end;

	// Scan backwards for the last slash
	start = len - 1;
	for ( ; start >= 0 && !Q_IsPathSeparator( in[start] ); --start )
		;

	if ( start < 0 || !Q_IsPathSeparator( in[start] ) )
		start = 0;
	else
		++start;

	//memcpy( out, in + start, end - start );
	strncpy( out, in + start, end - start + 1 );
	*( out + end - start + 1 ) = '\0';
}

//-------------------------------------------------------------------------------------------------
// Returns the path up to, but not including the last /
//-------------------------------------------------------------------------------------------------
void COM_FilePath (const char *in, char *out)
{
	const char *s;
	
	s = in + strlen(in) - 1;
	
	while (s != in && *s != '/')
		s--;

	strncpy (out,in, s-in);
	out[s-in] = 0;
}

//-------------------------------------------------------------------------------------------------
// Parse a token out of a string
//-------------------------------------------------------------------------------------------------
char *COM_Parse (char **data_p)
{
	static char	com_token[MAX_TOKEN_CHARS];
	int		c;
	int		len;
	char	*data;

	data = *data_p;
	len = 0;
	com_token[0] = 0;
	
	if (!data)
	{
		*data_p = NULL;
		return null_string;
	}
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
		{
			*data_p = NULL;
			return null_string;
		}
		data++;
	}
	
// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32);

	if (len == MAX_TOKEN_CHARS)
	{
//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = data;
	return com_token;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Com_PageInMemory (byte *buffer, int size)
{
	static int paged_total;
	int i;

	for (i=size-1 ; i>0 ; i-=4096)
		paged_total += buffer[i];
}

//-------------------------------------------------------------------------------------------------
// does a varargs printf into a temp buffer, so I don't need to have
// varargs versions of all text functions.
//-------------------------------------------------------------------------------------------------
char *va(const char *format, ...)
{
	static char		string[8192];
	va_list			argptr;

	va_start(argptr, format);
	Q_vsprintf_s(string, format, argptr);
	va_end(argptr);

	return string;
}

//-------------------------------------------------------------------------------------------------
//
// Library replacement functions
//
//-------------------------------------------------------------------------------------------------

#define AssertString(str) assert( str && str[0] )

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Q_strcpy_s(char *pDest, strlen_t nDestSize, const char *pSrc)
{
	AssertString(pSrc);

	char *pLast = pDest + nDestSize - 1;
	while ((pDest < pLast) && (*pSrc != 0))
	{
		*pDest = *pSrc;
		++pDest; ++pSrc;
	}
	*pDest = 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int Q_strcasecmp(const char *s1, const char *s2)
{
#ifdef _WIN32
	return _stricmp(s1, s2);
#else
	return strcasecmp(s1, s2);
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int Q_strncasecmp(const char *s1, const char *s2, strlen_t n)
{
#ifdef _WIN32
	return _strnicmp(s1, s2, n);
#else
	return strcasecmp(s1, s2);
#endif
}

//-------------------------------------------------------------------------------------------------
// FIXME: replace all Q_stricmp with Q_strcasecmp
//-------------------------------------------------------------------------------------------------
int Q_stricmp(const char *s1, const char *s2)
{
	return Q_strcasecmp(s1, s2);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Q_vsprintf_s(char *pDest, strlen_t nDestSize, const char *pFmt, va_list args)
{
#if 0
#ifdef _WIN32
	vsprintf_s(pDest, nDestSize, pFmt, args);
#else
	vsprintf(pDest, pFmt, args);
#endif
#else
	stbsp_vsnprintf(pDest, nDestSize, pFmt, args);
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Q_vsprintf(char *pDest, const char *pFmt, va_list args)
{
#if 0
	vsprintf(pDest, pFmt, args);
#else
	stbsp_vsprintf(pDest, pFmt, args);
#endif
}

//-------------------------------------------------------------------------------------------------
//
// Byte order functions
//
//-------------------------------------------------------------------------------------------------

short ShortSwap (short s)
{
	byte    b1,b2;

	b1 = s&255;
	b2 = (s>>8)&255;

	return (b1<<8) + b2;
}

int LongSwap (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

float FloatSwap (float f)
{
	union
	{
		float	f;
		byte	b[4];
	} dat1, dat2;
	
	
	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

//-------------------------------------------------------------------------------------------------
//
// Info strings
//
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Searches the string for the given
// key and returns the associated value, or an empty string.
//-------------------------------------------------------------------------------------------------
char *Info_ValueForKey (const char *s, const char *key)
{
	char	pkey[512];
	static	char value[2][512];	// use two buffers so compares
								// work without stomping on each other
	static	int	valueindex;
	char	*o;
	
	valueindex ^= 1;
	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return null_string;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			if (!*s)
				return null_string;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey) )
			return value[valueindex];

		if (!*s)
			return null_string;
		s++;
	}
}

void Info_RemoveKey (char *s, const char *key)
{
	char	*start;
	char	pkey[512];
	char	value[512];
	char	*o;

	if (strstr (key, "\\"))
	{
//		Com_Printf ("Can't use a key with a \\\n");
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey) )
		{
			strcpy (start, s);	// remove this part
			return;
		}

		if (!*s)
			return;
	}

}

//-------------------------------------------------------------------------------------------------
// Some characters are illegal in info strings because they
// can mess up the server's parsing
//-------------------------------------------------------------------------------------------------
qboolean Info_Validate (const char *s)
{
	if (strstr (s, "\""))
		return false;
	if (strstr (s, ";"))
		return false;
	return true;
}

void Info_SetValueForKey (char *s, const char *key, const char *value)
{
	char	newi[MAX_INFO_STRING], *v;
	int		c;

	if (strstr (key, "\\") || strstr (value, "\\") )
	{
		Com_Printf ("Can't use keys or values with a \\\n");
		return;
	}

	if (strstr (key, ";") )
	{
		Com_Printf ("Can't use keys or values with a semicolon\n");
		return;
	}

	if (strstr (key, "\"") || strstr (value, "\"") )
	{
		Com_Printf ("Can't use keys or values with a \"\n");
		return;
	}

	if (strlen(key) > MAX_INFO_KEY-1 || strlen(value) > MAX_INFO_KEY-1)
	{
		Com_Printf ("Keys and values must be < 64 characters.\n");
		return;
	}
	Info_RemoveKey (s, key);
	if (!value || !strlen(value))
		return;

	Q_sprintf_s (newi, "\\%s\\%s", key, value);

	if (strlen(newi) + strlen(s) > MAX_INFO_STRING)
	{
		Com_Printf ("Info string length exceeded\n");
		return;
	}

	// only copy ascii values
	s += strlen(s);
	v = newi;
	while (*v)
	{
		c = *v++;
		c &= 127;		// strip high bits
		if (c >= 32 && c < 127)
			*s++ = c;
	}
	*s = 0;
}
