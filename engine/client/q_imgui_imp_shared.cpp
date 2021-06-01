//=================================================================================================
// Think Tank 2
// Copyright (c) Slartibarty. All Rights Reserved.
//=================================================================================================

#include "../../core/stringtools.h"

#include "q_imconfig.h"

//-------------------------------------------------------------------------------------------------
// Implements some imgui functions that we disable
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// String formatting
//-------------------------------------------------------------------------------------------------

int ImFormatString( char *buf, size_t buf_size, const char *fmt, ... )
{
	va_list args;
	int val;

	va_start( args, fmt );
	val = Q_vsprintf_s( buf, static_cast<strlen_t>( buf_size ), fmt, args );
	va_end( args );

	return val;
}

int ImFormatStringV( char *buf, size_t buf_size, const char *fmt, va_list args )
{
	return Q_vsprintf_s( buf, static_cast<strlen_t>( buf_size ), fmt, args );
}

#if IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS

//-------------------------------------------------------------------------------------------------
// Filesystem
//-------------------------------------------------------------------------------------------------

using ImFileHandle = fsHandle;

static fsMode StdioModeToFS( const char *mode )
{
	bool wantRead = false;
	bool wantWrite = false;
	bool wantAppend = false;

	for ( ; *mode; ++mode )
	{
		if ( *mode == 'r' )
		{
			wantRead = true;
		}
		if ( *mode == 'a' )
		{
			wantAppend = true;
			continue;
		}
		if ( *mode == 'w' )
		{
			wantWrite = true;
			continue;
		}
		if ( *mode == '+' && wantRead )
		{
			wantRead = true;
			wantWrite = true;
		}
	}

	if ( wantAppend )
	{
		return fsMode::Append;
	}
	if ( wantRead && wantWrite )
	{
		return fsMode::ReadWrite;
	}
	if ( wantRead )
	{
		return fsMode::Read;
	}
	if ( wantWrite )
	{
		return fsMode::Write;
	}

	return fsMode::None;
}

ImFileHandle ImFileOpen( const char *filename, const char *mode )
{
	return filesystem::Open( filename, StdioModeToFS( mode ) );
}

bool ImFileClose( ImFileHandle file )
{
	filesystem::Close( file );

	return true;
}

uint64 ImFileGetSize( ImFileHandle file )
{
	size_t size = filesystem::Size( file );

	if ( size == 0 ) {
		size = (size_t)-1;
	}

	return size;
}

uint64 ImFileRead( void *data, uint64 size, uint64 count, ImFileHandle file )
{
	assert( size * count != 0 );

	return filesystem::Read( (byte *)data, 0, size * count, file ) ? count : 0;
}

uint64 ImFileWrite( const void *data, uint64 size, uint64 count, ImFileHandle file )
{
	return filesystem::Write( (const byte *)data, size * count, file ) ? count : 0;
}

#endif
