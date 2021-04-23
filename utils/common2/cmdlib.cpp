// The q_shared of the tools

#include "../../core/core.h"

#ifdef _WIN32
#include <direct.h>
#endif

#include "cmdlib.h"

#define	BASEDIRNAME		"game"
#define PATHSEPERATOR	'/'

bool verbose;

void Com_Print( const char *msg )
{
	fputs( msg, stdout );
}

void Com_Printf( _Printf_format_string_ const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_Print( msg );
}

void Com_DPrint( const char *msg )
{
	if ( !verbose ) {
		return;
	}

	Com_Print( msg );
}

void Com_DPrintf( _Printf_format_string_ const char *fmt, ... )
{
	if ( !verbose ) {
		return;
	}

	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_Print( msg );
}

[[noreturn]] void Com_Error( const char *msg )
{
	Com_Print( "\n************ ERROR ************\n" );

	Com_Print( msg );
	Com_Print( "\n" );

	exit( 1 );
}

[[noreturn]] void Com_Errorf( _Printf_format_string_ const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_Error( msg );
}

[[noreturn]] void Com_FatalError( const char *msg )
{
	Com_Error( msg );
}

[[noreturn]] void Com_FatalErrorf( _Printf_format_string_ const char *fmt, ... )
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];

	va_start( argptr, fmt );
	Q_vsprintf_s( msg, fmt, argptr );
	va_end( argptr );

	Com_FatalError( msg );
}

//=============================================================================

/*

qdir will hold the path up to the quake directory, including the slash

  f:\quake\
  /raid/quake/

gamedir will hold qdir + the game directory (id1, id2, etc)

*/

char		qdir[1024];
char		gamedir[1024];

static void Q_getcwd( char *buf, int buflen )
{
#ifdef _WIN32
	_getcwd( buf, buflen );
	strcat( buf, "\\" );
#else
	getcwd( buf, buflen );
	strcat( buf, "/" );
#endif
}

void SetQdirFromPath (char *path)
{
	char		temp[1024];
	char		*c;
	strlen_t	len;

	if (!(path[0] == '/' || path[0] == '\\' || path[1] == ':'))
	{	// path is partial
		Q_getcwd (temp, sizeof(temp));
		strcat (temp, path);
		path = temp;
	}

	// search for "game" in path

	len = Q_strlen(BASEDIRNAME);
	for (c=path+strlen(path)-1 ; c != path ; c--)
		if (!Q_strncasecmp (c, BASEDIRNAME, len))
		{
			strncpy (qdir, path, c+len+1-path);
			Com_DPrintf ("qdir: %s\n", qdir);
			c += len+1;
			while (*c)
			{
				if (*c == '/' || *c == '\\')
				{
					strncpy (gamedir, path, c+1-path);
					Com_DPrintf ("gamedir: %s\n", gamedir);
					return;
				}
				c++;
			}
			Com_FatalErrorf("No gamedir in %s", path);
			return;
		}
	Com_FatalErrorf("SetQdirFromPath: no '%s' in %s", BASEDIRNAME, path);
}

/*
=============================================================================

						MISC FUNCTIONS

=============================================================================
*/

/*
================
Q_filelength
================
*/
static int Q_filelength( FILE *f )
{
	int		pos;
	int		end;

	pos = ftell( f );
	fseek( f, 0, SEEK_END );
	end = ftell( f );
	fseek( f, pos, SEEK_SET );

	return end;
}


FILE *SafeOpenWrite (char *filename)
{
	FILE	*f;

	f = fopen(filename, "wb");

	if (!f)
		Com_FatalErrorf("Error opening %s: %s",filename,strerror(errno));

	return f;
}

FILE *SafeOpenRead (char *filename)
{
	FILE	*f;

	f = fopen(filename, "rb");

	if (!f)
		Com_FatalErrorf("Error opening %s: %s",filename,strerror(errno));

	return f;
}


void SafeRead (FILE *f, void *buffer, int count)
{
	if ( fread (buffer, 1, count, f) != (size_t)count)
		Com_FatalErrorf("File read failure");
}


void SafeWrite (FILE *f, void *buffer, int count)
{
	if (fwrite (buffer, 1, count, f) != (size_t)count)
		Com_FatalErrorf("File write failure");
}


/*
==============
LoadFile
==============
*/
int    LoadFile (char *filename, void **bufferptr)
{
	FILE	*f;
	int    length;
	void    *buffer;

	f = SafeOpenRead (filename);
	length = Q_filelength (f);
	buffer = malloc (length+1);
	((char *)buffer)[length] = 0;
	SafeRead (f, buffer, length);
	fclose (f);

	*bufferptr = buffer;
	return length;
}


/*
==============
TryLoadFile

Allows failure
==============
*/
int    TryLoadFile (char *filename, void **bufferptr)
{
	FILE	*f;
	int    length;
	void    *buffer;

	*bufferptr = NULL;

	f = fopen (filename, "rb");
	if (!f)
		return -1;
	length = Q_filelength (f);
	buffer = malloc (length+1);
	((char *)buffer)[length] = 0;
	SafeRead (f, buffer, length);
	fclose (f);

	*bufferptr = buffer;
	return length;
}


/*
==============
SaveFile
==============
*/
void    SaveFile (char *filename, void *buffer, int count)
{
	FILE	*f;

	f = SafeOpenWrite (filename);
	SafeWrite (f, buffer, count);
	fclose (f);
}


char *ExpandArg( char *path )
{
	static char full[1024];

	if ( path[0] != '/' && path[0] != '\\' && path[1] != ':' )
	{
		Q_getcwd( full, sizeof( full ) );
		strcat( full, path );
	}
	else
		strcpy( full, path );
	return full;
}

char *ExpandPath( char *path )
{
	static char full[1024];
	if ( !qdir )
		Com_FatalErrorf("ExpandPath called without qdir set" );
	if ( path[0] == '/' || path[0] == '\\' || path[1] == ':' )
		return path;
	Q_sprintf_s( full, "%s%s", qdir, path );
	return full;
}

char *copystring( const char *s )
{
	char *b = (char *)malloc( strlen( s ) + 1 );
	strcpy( b, s );
	return b;
}

void DefaultExtension (char *path, char *extension)
{
	char    *src;
//
// if path doesnt have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while (*src != PATHSEPERATOR && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	strcat (path, extension);
}


void DefaultPath (char *path, char *basepath)
{
	char    temp[128];

	if (path[0] == PATHSEPERATOR)
		return;                   // absolute path location
	strcpy (temp,path);
	strcpy (path,basepath);
	strcat (path,temp);
}


void    StripFilename (char *path)
{
	int             length;

	length = int(strlen(path)-1);
	while (length > 0 && path[length] != PATHSEPERATOR)
		length--;
	path[length] = 0;
}

void    StripExtension (char *path)
{
	int             length;

	length = int(strlen(path)-1);
	while (length > 0 && path[length] != '.')
	{
		length--;
		if (path[length] == '/')
			return;		// no extension
	}
	if (length)
		path[length] = 0;
}


/*
====================
Extract file parts
====================
*/
// FIXME: should include the slash, otherwise
// backing to an empty path will be wrong when appending a slash
void ExtractFilePath (char *path, char *dest)
{
	char    *src;

	src = path + strlen(path) - 1;

//
// back up until a \ or the start
//
	while (src != path && *(src-1) != '\\' && *(src-1) != '/')
		src--;

	memcpy (dest, path, src-path);
	dest[src-path] = 0;
}

void ExtractFileBase (char *path, char *dest)
{
	char    *src;

	src = path + strlen(path) - 1;

//
// back up until a \ or the start
//
	while (src != path && *(src-1) != PATHSEPERATOR)
		src--;

	while (*src && *src != '.')
	{
		*dest++ = *src++;
	}
	*dest = 0;
}

void ExtractFileExtension (char *path, char *dest)
{
	char    *src;

	src = path + strlen(path) - 1;

//
// back up until a . or the start
//
	while (src != path && *(src-1) != '.')
		src--;
	if (src == path)
	{
		*dest = 0;	// no extension
		return;
	}

	strcpy (dest,src);
}
