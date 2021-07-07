/*
===================================================================================================

	The interface to the engine's filesystem
	Go there for more detailed information

===================================================================================================
*/

#pragma once

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
	FS_WRITEDIR,
	FS_CONTENTDIR
};

class IFileSystem
{
public:
	virtual fsHandle_t OpenFileRead( const char *filename ) = 0;
	virtual fsHandle_t OpenFileWrite( const char *filename ) = 0;
	virtual fsHandle_t OpenFileAppend( const char *filename ) = 0;
	virtual void CloseFile( fsHandle_t handle ) = 0;
	virtual int ReadFile( void *buffer, int length, fsHandle_t handle ) = 0;
	virtual void WriteFile( const void *buffer, int length, fsHandle_t handle ) = 0;
	virtual void PrintFile( const char *string, fsHandle_t handle ) = 0;
	virtual void PrintFileFmt( fsHandle_t handle, const char *fmt, ... ) = 0;
	virtual void FlushFile( fsHandle_t handle ) = 0;
	virtual bool FileExists( const char *filename, fsPath_t fsPath = FS_GAMEDIR ) = 0;
};
