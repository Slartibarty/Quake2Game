/*
===================================================================================================

	The Virtual Filesystem 2.0

	Fact: We come *very* close to having Windows.h steal our function names

===================================================================================================
*/

#pragma once

#if 1

// Local engine interface

#define DECLARE_HANDLE(name) struct name##__{int unused;}; typedef struct name##__ *name

#ifdef Q_DEBUG
// to disallow implicit conversions
struct fsHandle_t__ { void *unused; }; using fsHandle_t = fsHandle_t__ *;
#else
using fsHandle_t = void *;
#endif

#define FS_INVALID_HANDLE nullptr

enum fsPath_t
{
	FS_GAMEDIR,
	FS_WRITEDIR
};

namespace FileSystem
{
	void			Init();
	void			Shutdown();

					// Returns true if a path is absolute.
	bool			IsAbsolutePath( const char *path );
					// Translates a relative path to an absolute path.
	char *			RelativePathToAbsolutePath( const char *relativePath );
					// Creates the given directory tree in the write dir.
	void			CreatePath( const char *path );
					// Returns the length of a file.
	int				GetFileLength( fsHandle_t handle );

					// Opens an existing file for reading.
	fsHandle_t		OpenFileRead( const char *filename );
					// Opens a new file for writing, will create any needed subdirectories.
	fsHandle_t		OpenFileWrite( const char *filename );
					// Opens a file for writing, at the end. If it doesn't exist, it is created. (creates subdirectories).
	fsHandle_t		OpenFileAppend( const char *filename );
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
	void			ExecAutoexec();
}

inline consteval int Developer_searchpath()
{
	return 0;
}

// DLL interface

#if 0

class __declspec( novtable ) IFileSystem
{
	virtual void OpenFileRead( const char *filename, fsHandle_t &handle ) = 0;
	virtual void CloseFile( fsHandle_t handle ) = 0;
	virtual void ReadFile( void *buffer, int length, fsHandle_t handle ) = 0;

	virtual int LoadFile( const char *filename, void **buffer, int extraData = 0 ) = 0;
	virtual void FreeFile( void *buffer ) = 0;
};

class CFileSystem final : public IFileSystem
{
	void OpenFileRead( const char *filename, fsHandle_t &handle ) override
	{ FileSystem::OpenFileRead( filename, handle ); }
	void CloseFile( fsHandle_t handle ) override
	{ FileSystem::CloseFile( handle ); }
	void ReadFile( void *buffer, int length, fsHandle_t handle ) override
	{ FileSystem::ReadFile( buffer, length, handle ); }

	int LoadFile( const char *filename, void **buffer, int extraData = 0 ) override
	{ FileSystem::LoadFile( filename, buffer, extraData ); }
	void FreeFile( void *buffer ) override
	{ FileSystem::FreeFile( buffer ); }
};

#endif

#else

void		FS_Init();
void		FS_Shutdown();

// I/O

int			FS_FOpenFile( const char *filename, FILE **file );
void		FS_FCloseFile( FILE *f );
// properly handles partial reads
void		FileSystem::ReadFile( void *buffer, int len, FILE *f );

// a null buffer will just return the file length without loading
// a -1 length is not present
int			FS_LoadFile( const char *path, void **buffer, int extradata = 0 );
void		FS_FreeFile( void *buffer );

const char	*FS_NextPath( const char *prevpath );

// Utilities

void		FS_SetGamedir( const char *dir, bool flush );
const char	*FS_Gamedir();
void		FS_CreatePath( char *path );
void		FS_ExecAutoexec();

#endif
