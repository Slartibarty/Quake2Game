/*
===================================================================================================

	The Virtual Filesystem 2.0

	Fact: We come *very* close to having Windows.h steal our function names

===================================================================================================
*/

#pragma once

#include "../../common/filesystem_interface.h"

// Local engine interface

namespace FileSystem
{
	void			Init();
	void			Shutdown();

					// Returns true if a path is absolute.
	bool			IsAbsolutePath( const char *path );
					// Translates a relative path to an absolute path. UNTESTED AS OF NOW!
	const char *	RelativePathToAbsolutePath( const char *relativePath, fsPath_t fsPath = FS_GAMEDIR );
					// Translates an absolute path to a relative path, Returns nullptr on failure.
					// Returns absolutePath trimmed to the relative part.
	const char *	AbsolutePathToRelativePath( const char *absolutePath, fsPath_t fsPath = FS_GAMEDIR );
					// Creates the given directory tree in the write dir. Add gamedir support when needed.
	void			CreatePath( const char *path );
					// Returns the length of a file.
	int				GetFileLength( fsHandle_t handle );

					// Opens an existing file for reading.
	fsHandle_t		OpenFileRead( const char *filename );
					// Opens a new file for writing, will create any needed subdirectories.
	fsHandle_t		OpenFileWrite( const char *filename, fsPath_t fsPath = FS_WRITEDIR );
					// Opens a file for writing, at the end. If it doesn't exist, it is created. (creates subdirectories).
	fsHandle_t		OpenFileAppend( const char *filename, fsPath_t fsPath = FS_WRITEDIR );
					// Closes a file.
	void			CloseFile( fsHandle_t handle );
					// Reads raw data from a file. Returns bytes read.
	int				ReadFile( void *buffer, int length, fsHandle_t handle );
					// Writes raw data to a file.
	void			WriteFile( const void *buffer, int length, fsHandle_t handle );
					// Writes a string to a file.
	void			PrintFile( const char *string, fsHandle_t handle );
					// Writes a string to a file. Funkily!
	void			PrintFileFmt( fsHandle_t handle, const char *fmt, ... );
					// Force the file to be flushed to disc. If underlying implementation allows (win32 sys api is unbuffered).
	void			FlushFile( fsHandle_t handle );
					// Checks to see if a file exists in any search paths.
	bool			FileExists( const char *filename, fsPath_t fsPath = FS_GAMEDIR );
					// Deletes a file from the write directory. Only named Remove because win32 steals DeleteFile.
	void			RemoveFile( const char *filename );

					// Loads a complete file.
					// Returns the length of the file, or -1 on failure.
					// A null buffer will just return the file length without loading.
					// extraData specifies the amount of additional zeroed memory to
					// add to the end of the file, typical usage is extraData = 1 to null
					// terminate text files.
	int				LoadFile( const char *filename, void **buffer, int extraData = 0 );
					// Frees the memory allocated by LoadFile.
	void			FreeFile( void *buffer );

					// Finds a file without considering packfiles (for dlls), returns the absolute path.
	bool			FindPhysicalFile( const char *name, char *buffer, strlen_t bufferSize, fsPath_t fsPath = FS_GAMEDIR );
}

inline consteval int Developer_searchpath()
{
	return 0;
}

// DLL interface

class CFileSystem final : public IFileSystem
{
public:
	fsHandle_t OpenFileRead( const char *filename ) override
	{ return FileSystem::OpenFileRead( filename ); }
	fsHandle_t OpenFileWrite( const char *filename ) override
	{ return FileSystem::OpenFileWrite( filename ); }
	fsHandle_t OpenFileAppend( const char *filename ) override
	{ return FileSystem::OpenFileAppend( filename ); }
	void CloseFile( fsHandle_t handle ) override
	{ FileSystem::CloseFile( handle ); }
	int ReadFile( void *buffer, int length, fsHandle_t handle ) override
	{ return FileSystem::ReadFile( buffer, length, handle ); }
	void WriteFile( const void *buffer, int length, fsHandle_t handle ) override
	{ FileSystem::WriteFile( buffer, length, handle ); }
	void PrintFile( const char *string, fsHandle_t handle ) override
	{ FileSystem::PrintFile( string, handle ); }
	void PrintFileFmt( fsHandle_t handle, const char *fmt, ... ) override
	{
		// HACK
		va_list args;
		va_start( args, fmt );
		vfprintf( (FILE *)handle, fmt, args );
		va_end( args );
	}
	void FlushFile( fsHandle_t handle ) override
	{ FileSystem::FlushFile( handle ); }
	bool FileExists( const char *filename, fsPath_t fsPath = FS_GAMEDIR ) override
	{ return FileSystem::FileExists( filename, fsPath ); }
};

extern CFileSystem g_fileSystem;
