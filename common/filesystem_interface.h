/*
===================================================================================================

	The interface to the engine's filesystem
	Go there for more detailed information

===================================================================================================
*/

#pragma once

#ifdef Q_DEBUG
// To disallow implicit conversions
struct fsHandle_t__ { void *unused; }; using fsHandle_t = fsHandle_t__ *;
#else
using fsHandle_t = void *;
#endif

// Should be at least uint32 later on
using fsSize_t = uint32;

#define FS_INVALID_HANDLE nullptr

enum fsPath_t
{
	FS_GAMEDIR,
	FS_WRITEDIR,
	FS_CONTENTDIR
};

enum fsSeek_t
{
	FS_SEEK_SET,		// SEEK_SET or FILE_BEGIN
	FS_SEEK_CUR,		// SEEK_CUR or FILE_CURRENT
	FS_SEEK_END			// SEEK_END or FILE_END
};

class IFileSystem
{
public:
	virtual fsHandle_t		OpenFileRead( const char *filename ) = 0;
	virtual fsHandle_t		OpenFileWrite( const char *filename ) = 0;
	virtual fsHandle_t		OpenFileAppend( const char *filename ) = 0;
	virtual void			CloseFile( fsHandle_t handle ) = 0;
	virtual fsSize_t		ReadFile( void *buffer, fsSize_t length, fsHandle_t handle ) = 0;
	virtual fsSize_t		WriteFile( const void *buffer, fsSize_t length, fsHandle_t handle ) = 0;
	virtual void			PrintFile( const char *string, fsHandle_t handle ) = 0;
	virtual void			PrintFileFmt( fsHandle_t handle, const char *fmt, ... ) = 0;
	virtual void			FlushFile( fsHandle_t handle ) = 0;
	virtual bool			FileExists( const char *filename, fsPath_t fsPath = FS_GAMEDIR ) = 0;
};
