//=================================================================================================
// Think Tank 2
// Copyright (c) Slartibarty. All Rights Reserved.
//=================================================================================================

#include "../../core/core.h"

#include "q_imconfig.h"
#include "../../framework/filesystem.h"

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

#ifdef IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS

//-------------------------------------------------------------------------------------------------
// Filesystem
//-------------------------------------------------------------------------------------------------

ImFileHandle ImFileOpen( const char *filename, const char *mode )
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
		return FileSystem::OpenFileAppend( filename );
	}
	if ( wantRead && wantWrite )
	{
		return nullptr;
	}
	if ( wantRead )
	{
		return FileSystem::OpenFileRead( filename );
	}
	if ( wantWrite )
	{
		return FileSystem::OpenFileWrite( filename );
	}

	return nullptr;
}

bool ImFileClose( ImFileHandle file )
{
	FileSystem::CloseFile( file );

	return true;
}

uint64 ImFileGetSize( ImFileHandle file )
{
	uint64 size = static_cast<uint64>( FileSystem::GetFileSize( file ) );

	if ( size == 0 ) {
		size = static_cast<uint64>( -1 );
	}

	return size;
}

uint64 ImFileRead( void *data, uint64 size, uint64 count, ImFileHandle file )
{
	assert( size * count != 0 );

	return static_cast<uint64>( FileSystem::ReadFile( data, static_cast<fsSize_t>( size * count ), file ) );
}

uint64 ImFileWrite( const void *data, uint64 size, uint64 count, ImFileHandle file )
{
	assert( size * count != 0 );

	FileSystem::WriteFile( data, static_cast<fsSize_t>( size * count ), file );

	return count;
}

#endif
