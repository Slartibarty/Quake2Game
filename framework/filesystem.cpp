/*
===================================================================================================

	The Virtual Filesystem 2.0

	MAYBE: changing the gameDir is unsupported right now, it's always the working directory

	Notes:
	The entire filesystem uses forward slashes to separate paths, any results returned from
	Windows system calls have their slashes fixed
	THE CODE ASSUMES THE WRITE DIR EXISTS!
	RelativePathToAbsolutePath should return a std::string or something, not a static char *

	NO TRAILING SLASHES!!!

===================================================================================================
*/

#include "framework_local.h"

#include "rapidjson/document.h"

#ifdef _WIN32
#include "../../core/sys_includes.h"
#include <ShlObj.h>

#ifdef Q_ENGINE
// COM is initialised for the main thread by Sys_Init already
#define FS_INITCOM
#define FS_KILLCOM
#else
// Must use COINIT_MULTITHREADED because we typically don't have a message pump in utils
#define FS_INITCOM CoInitializeEx( nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE );
#define FS_KILLCOM CoUninitialize();
#endif
#endif

#include "filesystem.h"

#define FS_VERIFYPATH(a) ASSUME( a == FS_GAMEDIR || a == FS_WRITEDIR || a == FS_CONTENTDIR )

// DLL interface
CFileSystem g_fileSystem;

namespace FileSystem
{

namespace Internal
{
fsHandle_t OpenFileRead( const char *absPath );
fsHandle_t OpenFileWrite( const char *absPath );
fsHandle_t OpenFileAppend( const char *absPath );
const char *GetAPIName();
}

namespace ModInfo
{
static void ParseModInfo();
static void Shutdown();
}

#define MODINFO_NAME		"modinfo.json"
#define SAVEFOLDER_NAME		"Freeze Team/Project Moon"

static cvar_t *fs_production;
static cvar_t *fs_debug;
static cvar_t *fs_mod;

struct searchPath_t
{
	char			dirName[MAX_OSPATH]; // Absolute search path
	searchPath_t *	pNext;
};

static struct fileSystem_t
{
	// The root of the game filesystem, IE: "C:/Projects/Moon/game"
	char gameDir[MAX_OSPATH];
	// Same concept as baseDir, but it's located elsewhere, like "My Games" or something
	char writeDir[MAX_OSPATH];
	// The root of the content filesystem, IE: "C:/Projects/Moon/content".
	char contentDir[MAX_OSPATH];
	// The primary mod we're working with, IE: "moon". Multiple mod folders can be mounted. But this is our main one.
	char modDir[MAX_QPATH];

	// Singly-linked list of search paths
	searchPath_t *searchPaths;

} fs;

// On Windows this sets the write directory to "C:/Users/Name/Saved Games/Game Name".
// Games regularly use Documents/My Games, but this is non-standard. That is a better place however...
// On Linux it's the game dir (sucks a little).
static void SetWriteDirectory()
{
#ifdef _WIN32

	FS_INITCOM

	PWSTR savedGames;
	if ( SUCCEEDED( SHGetKnownFolderPath( FOLDERID_SavedGames, KF_FLAG_CREATE | KF_FLAG_INIT, nullptr, &savedGames ) ) )
	{
		char fullPath[MAX_OSPATH];
		Sys_UTF16toUTF8( savedGames, static_cast<int>( wcslen( savedGames ) + 1 ), fullPath, sizeof( fullPath ) );
		CoTaskMemFree( savedGames );
		Str_FixSlashes( fullPath );
		Q_sprintf_s( fs.writeDir, "%s/%s", fullPath, SAVEFOLDER_NAME );
	}
	else
	{
		// this will never happen
		assert( 0 );
		Q_strcpy_s( fs.writeDir, fs.gameDir );
	}

	FS_KILLCOM

#else

	// lol
	Q_strcpy_s( fs.writeDir, fs.gameDir );

#endif
}

static void SetContentDirectory()
{
	// The content directory is by default, the game dir but the last directory is replaced with "content".
	// This is because in our dev setups, we have "content", "game" and "src" all next to each other.
	// Don't have to worry about production code using this because if the directory doesn't exist, the op will fail.
	Q_strcpy_s( fs.contentDir, fs.gameDir );

	// find the last slash
	char *copyZone = strrchr( fs.contentDir, '/' );
	if ( !copyZone ) {
		// wtf! bail! this should never happen, make the content dir the write dir
		Q_strcpy_s( fs.contentDir, fs.writeDir );
		return;
	}
	++copyZone; // skip slash

	strlen_t sizeAfter = static_cast<strlen_t>( sizeof( fs.contentDir ) - ( copyZone - fs.contentDir ) );

	Q_strcpy_s( copyZone, sizeAfter, "content" );
}

// Does all the grunt work related to adding a new game directory (finding packs, etc)
static void AddSearchPath( const char *baseDir, const char *dirName )
{
	searchPath_t *searchPath = new searchPath_t;
	//Q_strcpy_s( searchPath->dirName, dirName );
	Q_sprintf_s( searchPath->dirName, "%s/%s", baseDir, dirName );
	searchPath->pNext = fs.searchPaths;
	fs.searchPaths = searchPath;
}

void Init()
{
	Com_Printf(
		"-------- Initializing FileSystem --------\n"
		"Using %s for IO operations\n", Internal::GetAPIName()
	);

	fs_production = Cvar_Get( "fs_production", "0", 0, "If true, file writes are forced to writeDir." );
	fs_debug = Cvar_Get( "fs_debug", "0", 0, "Controls FS spew." );
	fs_mod = Cvar_Get( "fs_mod", BASE_MODDIR, CVAR_INIT, "The primary mod dir." );

	// Set gameDir
	// Use the current working directory as the base dir.
	// Consider security implications of this in the future.
	Sys_GetWorkingDirectory( fs.gameDir, sizeof( fs.gameDir ) );

	// Set writeDir
	SetWriteDirectory();

	// set contentDir
	SetContentDirectory();

	// Set modDir
	Q_strcpy_s( fs.modDir, fs_mod->GetString() );

	Com_Printf( "gameDir   : %s\n", fs.gameDir );
	Com_Printf( "writeDir  : %s\n", fs.writeDir );
	Com_Printf( "contentDir: %s\n", fs.contentDir );
	Com_Printf( "modDir    : %s\n", fs.modDir );

	// Search paths are added backwards due to the nature of singly linked lists.
	// The write directory takes presedence for searches

	// Third priority, additional game dirs specified by the modinfo
	ModInfo::ParseModInfo();

	// Second priority, Add the game/mod directory
	AddSearchPath( fs.gameDir, fs.modDir );

	// First priority, Add the write/mod directory
	AddSearchPath( fs.writeDir, fs.modDir );

	Com_Print( "Search paths:\n" );
	for ( searchPath_t *pSP = fs.searchPaths; pSP; pSP = pSP->pNext )
	{
		Com_Printf( "  %s\n", pSP->dirName );
	}

	Com_Print( "FileSystem initialized\n" "-----------------------------------------\n\n" );
}

void Shutdown()
{
	ModInfo::Shutdown();

	// Clean up search paths
	searchPath_t *pLastSP = nullptr;
	for ( searchPath_t *pSP = fs.searchPaths; pSP; pSP = pSP->pNext )
	{
		if ( pLastSP ) {
			delete pLastSP;
		}
		pLastSP = pSP;
	}
	delete pLastSP;
}

//=============================================================================

bool IsAbsolutePath( const char *path )
{
	assert( strlen( path ) > 2 );
#ifdef _WIN32
	// Under win32, an absolute path always begins with "<letter>:/"
	// This doesn't consider network directories
	return path[1] == ':';
#else
	// Unix absolute paths always begin with a slash
	return path[0] == '/';
#endif
}

const char *RelativePathToAbsolutePath( const char *relativePath, fsPath_t fsPath /*= FS_GAMEDIR*/ )
{
	static char absolutePath[MAX_OSPATH];

	if ( IsAbsolutePath( relativePath ) )
	{
		return relativePath;
	}

	FS_VERIFYPATH( fsPath );
	switch ( fsPath )
	{
	default: // FS_GAMEDIR
	{
		// go through each search path until we find our file
		for ( searchPath_t *pSP = fs.searchPaths; pSP; pSP = pSP->pNext )
		{
			Q_sprintf_s( absolutePath, "%s/%s", pSP->dirName, relativePath );

			// this checks directory existence too
			if ( !Sys_FileExists( absolutePath ) ) {
				continue;
			}

			return absolutePath;
		}

		absolutePath[0] = '\0';
		return absolutePath;
	}
	case FS_WRITEDIR:
	{
		Q_sprintf_s( absolutePath, "%s/%s/%s", fs.writeDir, fs.modDir, relativePath );

		if ( !Sys_FileExists( absolutePath ) ) {
			absolutePath[0] = '\0';
			return absolutePath;
		}

		return absolutePath;
	}
	}
}

const char *AbsolutePathToRelativePath( const char *absolutePath, fsPath_t fsPath /*= FS_GAMEDIR*/ )
{
	if ( !IsAbsolutePath( absolutePath ) )
	{
		return absolutePath;
	}

	FS_VERIFYPATH( fsPath );
	switch ( fsPath )
	{
	default: // FS_GAMEDIR
	{
		// go through each search path until we find our file
		for ( searchPath_t *pSP = fs.searchPaths; pSP; pSP = pSP->pNext )
		{
			strlen_t length = Q_strlen( pSP->dirName );
			if ( Q_strnicmp( pSP->dirName, absolutePath, length ) == 0 )
			{
				// if the beginning of this filename matches the absolute path, then it's what we want
				return absolutePath + length + 1;
			}
		}

		return nullptr;
	}
	case FS_WRITEDIR:
	{
		char fullPath[MAX_OSPATH];
		Q_sprintf_s( fullPath, "%s/%s", fs.writeDir, fs.modDir );

		strlen_t length = Q_strlen( fullPath );
		if ( Q_strnicmp( fullPath, absolutePath, length ) == 0 )
		{
			// if the beginning of this filename matches the absolute path, then it's what we want
			return absolutePath + length + 1;
		}

		return nullptr;
	}
	}
}

static void CreateAbsolutePath( char *path, strlen_t skipDist )
{
	for ( char *ofs = path + skipDist + 1; *ofs; ++ofs )
	{
		if ( *ofs == '/' )
		{
			// create the directory
			*ofs = '\0';
			Sys_CreateDirectory( path );
			*ofs = '/';
		}
	}
}

void CreatePath( const char *path )
{
	char fullPath[MAX_OSPATH];

	strlen_t skipDist = Q_strlen( path );
	Q_sprintf_s( fullPath, "%s/%s", fs.writeDir, path );

	CreateAbsolutePath( fullPath, skipDist );
}

fsHandle_t OpenFileRead( const char *filename )
{
	char fullPath[MAX_OSPATH];

	// go through each search path until we find our file
	for ( searchPath_t *pSP = fs.searchPaths; pSP; pSP = pSP->pNext )
	{
		Q_sprintf_s( fullPath, "%s/%s", pSP->dirName, filename );

		fsHandle_t handle = Internal::OpenFileRead( fullPath );
		if ( handle == FS_INVALID_HANDLE ) {
			continue;
		}

		if ( fs_debug->GetBool() ) {
			Com_Printf( S_COLOR_YELLOW "[FileSystem] Reading %s\n", fullPath );
		}

		return handle;
	}

	if ( fs_debug->GetBool() ) {
		Com_Printf( S_COLOR_RED "[FileSystem] Can't find %s\n", filename );
	}

	return FS_INVALID_HANDLE;
}

fsHandle_t OpenFileWrite( const char *filename, fsPath_t fsPath /*= FS_WRITEDIR*/ )
{
	char fullPath[MAX_OSPATH];
	const char *directory;

	if ( fs_production->GetBool() ) {
		fsPath = FS_WRITEDIR;
	}

	FS_VERIFYPATH( fsPath );
	switch ( fsPath )
	{
	case FS_GAMEDIR:
		directory = fs.gameDir;
		break;
	default: // FS_WRITEDIR
		directory = fs.writeDir;
		break;
	}

	Q_sprintf_s( fullPath, "%s/%s/%s", directory, fs.modDir, filename );
	CreateAbsolutePath( fullPath, Q_strlen( directory ) );

	fsHandle_t handle = Internal::OpenFileWrite( fullPath );

	if ( fs_debug->GetBool() ) {
		if ( handle ) {
			Com_Printf( S_COLOR_YELLOW "[FileSystem] Writing %s\n", fullPath );
		} else {
			Com_Printf( S_COLOR_RED "[FileSystem] Failed to write %s\n", fullPath );
		}
	}

	return handle;
}

fsHandle_t OpenFileAppend( const char *filename, fsPath_t fsPath /*= FS_WRITEDIR*/ )
{
	char fullPath[MAX_OSPATH];
	const char *directory;

	if ( fs_production->GetBool() ) {
		fsPath = FS_WRITEDIR;
	}

	FS_VERIFYPATH( fsPath );
	switch ( fsPath )
	{
	case FS_GAMEDIR:
		directory = fs.gameDir;
		break;
	default: // FS_WRITEDIR
		directory = fs.writeDir;
		break;
	}

	Q_sprintf_s( fullPath, "%s/%s/%s", directory, fs.modDir, filename );
	CreateAbsolutePath( fullPath, Q_strlen( directory ) );

	fsHandle_t handle = Internal::OpenFileAppend( fullPath );

	if ( fs_debug->GetBool() ) {
		if ( handle ) {
			Com_Printf( S_COLOR_YELLOW "[FileSystem] Appending %s\n", fullPath );
		} else {
			Com_Printf( S_COLOR_RED "[FileSystem] Failed to append %s\n", fullPath );
		}
	}

	return handle;
}

bool FileExists( const char *filename, fsPath_t fsPath /*= FS_GAMEDIR*/ )
{
#if 0
	// using this implementation for now, it's slower but easier
	return LoadFile( filename, nullptr ) != -1;
#else
	// alternate implementation
	char fullPath[MAX_OSPATH];

	FS_VERIFYPATH( fsPath );
	switch ( fsPath )
	{
	default: // FS_GAMEDIR
	{
		// go through each search path until we find our file
		for ( searchPath_t *pSP = fs.searchPaths; pSP; pSP = pSP->pNext )
		{
			Q_sprintf_s( fullPath, "%s/%s", pSP->dirName, filename );

			if ( !Sys_FileExists( fullPath ) ) {
				continue;
			}

			return true;
		}

		return false;
	}
	case FS_WRITEDIR:
	{
		Q_sprintf_s( fullPath, "%s/%s/%s", fs.writeDir, fs.modDir, filename );

		return Sys_FileExists( fullPath );
	}
	}
#endif
}

void RemoveFile( const char *filename )
{
	char fullPath[MAX_OSPATH];
	Q_sprintf_s( fullPath, "%s/%s/%s", fs.writeDir, fs.modDir, filename );

	// FIXME: Doesn't consider pak files (when we re-introduce them)
	if ( !Sys_FileExists( fullPath ) ) {
		return;
	}

	Sys_DeleteFile( fullPath );
}

//=============================================================================

fsSize_t LoadFile( const char *filename, void **buffer, fsSize_t extraData /*= 0*/ )
{
	assert( filename && filename[0] );
	assert( extraData >= 0 );

	fsHandle_t handle = OpenFileRead( filename );
	if ( handle == FS_INVALID_HANDLE )
	{
		if ( buffer ) {
			*buffer = nullptr;
		}
		return 0;
	}

	fsSize_t length = GetFileSize( handle );

	if ( !buffer )
	{
		CloseFile( handle );
		return length;
	}

	if ( length == 0 )
	{
		// If it's 0kb we can't garner any information from this
		// if you need to test if a file exists, use FileExists
		CloseFile( handle );
		*buffer = nullptr;
		return 0;
	}

	void *buf = Mem_Alloc( length + extraData );
	*buffer = buf;

	ReadFile( buf, length, handle );
	CloseFile( handle );

	memset( (byte *)buf + length, 0, extraData );

	return length + extraData;
}

void FreeFile( void *buffer )
{
	Mem_Free( buffer );
}

//=============================================================================

bool FindPhysicalFile( const char *filename, char *buffer, strlen_t bufferSize, fsPath_t fsPath /*= FS_GAMEDIR*/ )
{
	FS_VERIFYPATH( fsPath );
	switch ( fsPath )
	{
	case FS_GAMEDIR:
	{
		// go through each search path until we find our file
		for ( searchPath_t *pSP = fs.searchPaths; pSP; pSP = pSP->pNext )
		{
			Q_sprintf_s( buffer, bufferSize, "%s/%s", pSP->dirName, filename );

			if ( Sys_FileExists( buffer ) ) {
				return true;
			}
		}

		return false;
	}
	default: // FS_WRITEDIR
	{
		Q_sprintf_s( buffer, bufferSize, "%s/%s/%s", fs.writeDir, fs.modDir, filename );

		if ( Sys_FileExists( buffer ) ) {
			return true;
		}

		return false;
	}
	}
}

// Modinfo is a subsystem of the filesystem, since they're inherently closely related
namespace ModInfo
{

static struct modInfo_t
{
	char gameTitle[64];				// The game title
	platChar_t windowTitle[64];		// The title created windows will have

	rapidjson::Document *pDoc;
} modInfo;

static void ParseModInfo()
{
	using namespace rapidjson;

	char fullPath[MAX_OSPATH];
	Q_sprintf_s( fullPath, "%s/%s/%s", fs.gameDir, fs.modDir, MODINFO_NAME );

	// Search paths aren't initialised yet, so we need to use low-level functions
	fsHandle_t handle = Internal::OpenFileRead( fullPath );
	if ( !handle ) {
		Com_FatalErrorf( "The primary mod (\"%s\") does not contain a " MODINFO_NAME "\n", fs.modDir);
	}
	fsSize_t length = GetFileSize( handle );
	if ( length == 0 ) {
		CloseFile( handle );
		Com_FatalErrorf( "The primary mod (\"%s\") contains an empty " MODINFO_NAME "\n", fs.modDir);
	}

	void *pData = Mem_Alloc( length );
	ReadFile( pData, length, handle );
	CloseFile( handle );

	Com_Printf( "Parsing %s/%s\n", fs.modDir, MODINFO_NAME );

	modInfo.pDoc = new Document;

	modInfo.pDoc->Parse( (char *)pData, length );
	FileSystem::FreeFile( pData );
	if ( modInfo.pDoc->HasParseError() || !modInfo.pDoc->IsObject() ) {
		Com_FatalErrorf( "Failed to parse %s/%s\n", fs.modDir, MODINFO_NAME );
	}

	Value::ConstMemberIterator member;

	// More than one subsystem needs the information here, so just store it in the filesystem

	member = modInfo.pDoc->FindMember( "GameTitle" );
	if ( member != modInfo.pDoc->MemberEnd() && member->value.IsString() )
	{
		const char *str = member->value.GetString();
		Q_strcpy_s( modInfo.gameTitle, str );
	}

	member = modInfo.pDoc->FindMember( "WindowTitle" );
	if ( member != modInfo.pDoc->MemberEnd() && member->value.IsString() )
	{
#ifdef _WIN32
		Sys_UTF8ToUTF16( member->value.GetString(), member->value.GetStringLength() + 1, modInfo.windowTitle, countof( modInfo.windowTitle ) );
#else
		Q_strcpy_s( modInfo.gameTitle, member->value.GetString() );
#endif
	}

	member = modInfo.pDoc->FindMember( "FileSystem" );
	if ( member != modInfo.pDoc->MemberEnd() && member->value.IsObject() )
	{
		Value::ConstMemberIterator searchPaths = member->value.FindMember( "SearchPaths" );
		if ( searchPaths != modInfo.pDoc->MemberEnd() && searchPaths->value.IsArray() )
		{
			Value::ConstArray arr = searchPaths->value.GetArray();

			for ( SizeType i = 0; i < arr.Size(); ++i )
			{
				const char *str = arr[i].GetString();

				AddSearchPath( fs.gameDir, str );
			}
		}
	}

}

static void Shutdown()
{
	delete modInfo.pDoc;
}

const char *GetGameTitle()
{
	if ( modInfo.gameTitle[0] == 0 )
	{
		return ENGINE_VERSION;
	}

	return modInfo.gameTitle;
}

const platChar_t *GetWindowTitle()
{
	if ( modInfo.windowTitle[0] == 0 )
	{
		// Filesystem didn't get a chance to init, so return this instead
		return PLATTEXT( ENGINE_VERSION );
	}

	return modInfo.windowTitle;
}

void *GetModInfoDocument()
{
	return reinterpret_cast<void *>( modInfo.pDoc );
}

} // namespace ModInfo

} // namespace FileSystem
