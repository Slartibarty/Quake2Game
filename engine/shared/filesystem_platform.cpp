/*
===================================================================================================

	Platform specific filesystem code

	This goes against the concept of having different files for platform specific stuff
	but I don't want to mess with the premake script again

	TODO: Is there any reason in the world why we wouldn't want to use stdio on all platforms?
	the buffered IO seems to be a big win.

===================================================================================================
*/

#include "engine.h"

// Undefine to force stdio on Windows
//#define FS_FORCE_STDIO

#if defined( _WIN32 ) && !defined( FS_FORCE_STDIO )
#include "../../core/sys_includes.h"
#endif

#if defined( _WIN32 ) && !defined( FS_FORCE_STDIO )

/*
=======================================
	WinAPI
=======================================
*/

namespace FileSystem
{

namespace Internal
{

template< DWORD desiredAccess, DWORD creationDisposition, bool append >
fsHandle_t OpenFileBase( const char *absPath )
{
	wchar_t wideAbsPath[MAX_OSPATH];
	Sys_UTF8ToUTF16( absPath, Q_strlen( absPath ) + 1, wideAbsPath, countof( wideAbsPath ) );

	HANDLE handle = CreateFileW( wideAbsPath, desiredAccess, FILE_SHARE_READ, nullptr, creationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr );
	if ( handle == INVALID_HANDLE_VALUE )
	{
		return nullptr;
	}

	if constexpr ( append )
	{
		SetEndOfFile( handle );
	}

	return reinterpret_cast<fsHandle_t>( handle );
}

fsHandle_t OpenFileRead( const char *absPath )
{
	return OpenFileBase< GENERIC_READ, OPEN_EXISTING, false >( absPath );
}

fsHandle_t OpenFileWrite( const char *absPath )
{
	return OpenFileBase< GENERIC_WRITE, CREATE_ALWAYS, false >( absPath );
}

fsHandle_t OpenFileAppend( const char *absPath )
{
	return OpenFileBase< FILE_APPEND_DATA, OPEN_ALWAYS, true >( absPath );
}

const char *GetAPIName()
{
	return "the WinAPI";
}

} // namespace Internal

int GetFileSize( fsHandle_t handle )
{
	LARGE_INTEGER fileSize;

	GetFileSizeEx( reinterpret_cast<HANDLE>( handle ), &fileSize );

	return static_cast<int>( fileSize.QuadPart );
}

void Seek( fsHandle_t handle, int offset, fsSeek_t seek )
{
	SetFilePointerEx( reinterpret_cast<HANDLE>( handle ), static_cast<LARGE_INTEGER>( offset ), nullptr, static_cast<DWORD>( seek ) );
}

int Tell( fsHandle_t handle )
{
	LARGE_INTEGER currentPosition;

	SetFilePointerEx( reinterpret_cast<HANDLE>( handle ), LARGE_INTEGER(), &currentPosition, FILE_CURRENT );

	return static_cast<int>( currentPosition.QuadPart );
}

void CloseFile( fsHandle_t handle )
{
	CloseHandle( reinterpret_cast<HANDLE>( handle ) );
}

int ReadFile( void *buffer, int length, fsHandle_t handle )
{
	assert( length > 0 );

	DWORD bytesRead;

	::ReadFile( reinterpret_cast<HANDLE>( handle ), buffer, static_cast<DWORD>( length ), &bytesRead, nullptr );

	return bytesRead;
}

void WriteFile( const void *buffer, int length, fsHandle_t handle )
{
	assert( length > 0 );

	DWORD bytesWritten;

	::WriteFile( reinterpret_cast<HANDLE>( handle ), buffer, static_cast<DWORD>( length ), &bytesWritten, nullptr );
}

void PrintFile( const char *string, fsHandle_t handle )
{
	WriteFile( string, Q_strlen( string ), handle );
}

void PrintFileVFmt( fsHandle_t handle, const char *fmt, va_list args )
{
	int strSize = Q_vsprintf_s( nullptr, 0, fmt, args ) + 1;
	assert( strSize <= MAX_PRINT_MSG );
	char *str = (char *)Mem_StackAlloc( strSize );
	Q_vsprintf( str, fmt, args );

	PrintFile( str, handle );
}

void PrintFileFmt( fsHandle_t handle, const char *fmt, ... )
{
	va_list args;
	va_start( args, fmt );
	PrintFileVFmt( handle, fmt, args );
	va_end( args );
}

void FlushFile( fsHandle_t handle )
{
	// Doesn't the winapi system io not have buffered IO? wtf does this function do?
	//FlushFileBuffers( (HANDLE)handle );
}

} // namespace FileSystem

#else

/*
=======================================
	stdio
=======================================
*/

namespace FileSystem
{

namespace Internal
{

fsHandle_t OpenFileRead( const char *absPath )
{
	return reinterpret_cast<fsHandle_t>( fopen( absPath, "rb" ) );
}

fsHandle_t OpenFileWrite( const char *absPath )
{
	return reinterpret_cast<fsHandle_t>( fopen( absPath, "wb" ) );
}

fsHandle_t OpenFileAppend( const char *absPath )
{
	return reinterpret_cast<fsHandle_t>( fopen( absPath, "ab" ) );
}

const char *GetAPIName()
{
	return "stdio";
}

} // namespace Internal

int GetFileSize( fsHandle_t handle )
{
	long pos;
	long end;

	pos = ftell( (FILE *)handle );
	fseek( (FILE *)handle, 0, SEEK_END );
	end = ftell( (FILE *)handle );
	fseek( (FILE *)handle, pos, SEEK_SET );

	return static_cast<int>( end );
}

void Seek( fsHandle_t handle, int offset, fsSeek_t seek )
{
	fseek( reinterpret_cast<FILE *>( handle ), static_cast<long>( offset ), static_cast<int>( seek ) );
}

int Tell( fsHandle_t handle )
{
	return static_cast<int>( ftell( reinterpret_cast<FILE *>( handle ) ) );
}

void CloseFile( fsHandle_t handle )
{
	fclose( (FILE *)handle );
}

int ReadFile( void *buffer, int length, fsHandle_t handle )
{
	assert( length > 0 );
	int read = static_cast<int>( fread( buffer, length, 1, (FILE*)handle ) );

	assert( read != 0 );

	return read;
}

void WriteFile( const void *buffer, int length, fsHandle_t handle )
{
	assert( length > 0 );
	[[maybe_unused]] int written = static_cast<int>( fwrite( buffer, length, 1, (FILE*)handle ) );

	assert( written != 0 );
}

void PrintFile( const char *string, fsHandle_t handle )
{
	fputs( string, (FILE *)handle );
}

void PrintFileVFmt( fsHandle_t handle, const char *fmt, va_list args )
{
	vfprintf( (FILE *)handle, fmt, args );
}

void PrintFileFmt( fsHandle_t handle, const char *fmt, ... )
{
	va_list args;
	va_start( args, fmt );
	PrintFileVFmt( handle, fmt, args );
	va_end( args );
}

void FlushFile( fsHandle_t handle )
{
	fflush( (FILE *)handle );
}

} // namespace FileSystem

#endif
