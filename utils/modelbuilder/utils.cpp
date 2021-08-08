
#include "modelbuilder_local.h"

static size_t GetFileSize( FILE *handle )
{
	long oldpos, newpos;

	oldpos = ftell( handle );
	fseek( handle, 0, SEEK_END );
	newpos = ftell( handle );
	fseek( handle, oldpos, SEEK_SET );

	return static_cast<size_t>( newpos );
}

// Load a file into a buffer
size_t Local_LoadFile( const char *filename, byte **buffer, size_t extradata /*= 0*/ )
{
	FILE *handle = fopen( filename, "rb" );
	if ( !handle ) {
		*buffer = nullptr;
		return 0;
	}

	size_t bufsize = GetFileSize( handle ) + extradata;

	*buffer = reinterpret_cast<byte *>( malloc( bufsize ) );
	fread( *buffer, 1, bufsize, handle );
	fclose( handle );

	if ( extradata > 0 )
	{
		memset( *buffer + bufsize - extradata, 0, extradata );
	}

	return bufsize;
}

// By the time this function is called we know several things:
//  1. The string equates to a valid file that has been opened and parsed
//  2. The string almost certainly has no slash as the last character
void Local_StripFilename( char *str )
{
	char *lastSlash = nullptr;
	for ( char *blah = str; *str; ++str )
	{
		if ( *blah == '/' )
		{
			lastSlash = blah;
		}
	}
	if ( lastSlash )
	{
		lastSlash = '\0';
	}
}
