/*
===================================================================================================

	Platform specific filesystem code

	This goes against the concept of having different files for platform specific stuff
	but I don't want to mess with the premake script again

	TODO: Is there any reason in the world why we wouldn't want to use stdio on all platforms?
	the buffered IO seems to be a big win.

	NOTE: There are a number of gotchas in this code, the winapi uses DWORD and LONGLONG
	inconsistently to denote the size of a file. ideally we want to use unsigned 32-bit ints for
	representing file sizes, as we don't ever expect to have to touch files larger than 4 gigabytes
	so this shouldn't really be a problem

===================================================================================================
*/

#include "framework_local.h"

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

fsSize_t GetFileSize( fsHandle_t handle )
{
	LONGLONG fileSize;

	GetFileSizeEx( reinterpret_cast<HANDLE>( handle ), reinterpret_cast<LARGE_INTEGER *>( &fileSize ) );
	assert( fileSize <= UINT32_MAX );

	return static_cast<fsSize_t>( fileSize );
}

void Seek( fsHandle_t handle, fsSize_t offset, fsSeek_t seek )
{
	SetFilePointerEx( reinterpret_cast<HANDLE>( handle ), static_cast<LARGE_INTEGER>( offset ), nullptr, static_cast<DWORD>( seek ) );
}

fsSize_t Tell( fsHandle_t handle )
{
	LONGLONG currentPosition;

	SetFilePointerEx( reinterpret_cast<HANDLE>( handle ), static_cast<LARGE_INTEGER>( 0 ), reinterpret_cast<LARGE_INTEGER *>( &currentPosition ), FILE_CURRENT );

	return static_cast<fsSize_t>( currentPosition );
}

void CloseFile( fsHandle_t handle )
{
	CloseHandle( reinterpret_cast<HANDLE>( handle ) );
}

fsSize_t ReadFile( void *buffer, fsSize_t length, fsHandle_t handle )
{
	assert( length > 0 );

	DWORD bytesRead;

	::ReadFile( reinterpret_cast<HANDLE>( handle ), buffer, static_cast<DWORD>( length ), &bytesRead, nullptr );

	return static_cast<fsSize_t>( bytesRead );
}

fsSize_t WriteFile( const void *buffer, fsSize_t length, fsHandle_t handle )
{
	assert( length > 0 );

	DWORD bytesWritten;

	::WriteFile( reinterpret_cast<HANDLE>( handle ), buffer, static_cast<DWORD>( length ), &bytesWritten, nullptr );

	return static_cast<fsSize_t>( bytesWritten );
}

void PrintFile( const char *string, fsHandle_t handle )
{
	WriteFile( string, static_cast<fsSize_t>( strlen( string ) ), handle );
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
	// Doesn't the winapi system io not have buffered IO? What does this function do?
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

fsSize_t GetFileSize( fsHandle_t handle )
{
	long pos;
	long end;

	// longs are always 32-bit on Windows for backwards compatibility
	// longs are 64-bit on Linux64 (and 32-bit on Linux32)
	// soo... this code is unoptimal for Windows
	// _ftell64 is a microsoft extension we can leverage if we ever
	// *have* to use stdio on Windows

	pos = ftell( (FILE *)handle );
	fseek( (FILE *)handle, 0, SEEK_END );
	end = ftell( (FILE *)handle );
	fseek( (FILE *)handle, pos, SEEK_SET );

	return static_cast<fsSize_t>( end );
}

void Seek( fsHandle_t handle, fsSize_t offset, fsSeek_t seek )
{
	fseek( (FILE*)handle, (long)offset, (int)seek );
}

fsSize_t Tell( fsHandle_t handle )
{
	return static_cast<fsSize_t>( ftell( (FILE*)handle ) );
}

void CloseFile( fsHandle_t handle )
{
	fclose( (FILE *)handle );
}

fsSize_t ReadFile( void *buffer, fsSize_t length, fsHandle_t handle )
{
	assert( length > 0 );

	fsSize_t read = static_cast<fsSize_t>( fread( buffer, (size_t)length, 1, (FILE*)handle ) );

	assert( read != 0 );

	return read;
}

fsSize_t WriteFile( const void *buffer, fsSize_t length, fsHandle_t handle )
{
	assert( length > 0 );

	fsSize_t written = static_cast<fsSize_t>( fwrite( buffer, (size_t)length, 1, (FILE*)handle ) );

	assert( written != 0 );

	return written;
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
