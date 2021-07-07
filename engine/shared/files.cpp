/*
===================================================================================================

	The Virtual Filesystem 2.0

	TODO: gameinfo.txt

	MAYBE: changing the gameDir is unsupported right now, it's always the working directory

	Notes:
	The entire filesystem uses forward slashes to separate paths, any results returned from
	Windows system calls have their slashes fixed
	THE CODE ASSUMES THE WRITE DIR EXISTS!

	NO TRAILING SLASHES!!!

===================================================================================================
*/

#include "engine.h"

#ifdef _WIN32
#include "../../core/sys_includes.h"
#include <ShlObj.h>
#endif

#include "files.h"

#if 1

namespace FileSystem
{

// I wear a huge trollface upon my head
static constexpr auto SaveFolderName = "Freeze Team/Project Moon";

cvar_t *fs_debug;
cvar_t *fs_mod;

struct searchPath_t
{
	char			dirName[MAX_OSPATH]; // Absolute search path
	searchPath_t *	pNext;
};

static struct fileSystem_t
{
	// The root of the content filesystem, IE: "C:/Projects/Moon/content"
	//char contentDir[MAX_OSPATH];
	// The root of the game filesystem, IE: "C:/Projects/Moon/game"
	char gameDir[MAX_OSPATH];
	// Same concept as baseDir, but it's located elsewhere, like "My Games" or something
	char writeDir[MAX_OSPATH];
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

	PWSTR savedGames;
	if ( SUCCEEDED( SHGetKnownFolderPath( FOLDERID_SavedGames, KF_FLAG_CREATE | KF_FLAG_INIT, nullptr, &savedGames ) ) )
	{
		char fullPath[MAX_OSPATH];
		WideCharToMultiByte( CP_UTF8, 0, savedGames, static_cast<int>( wcslen( savedGames ) + 1 ), fullPath, sizeof( fullPath ), nullptr, nullptr );
		Str_FixSlashes( fullPath );
		Q_sprintf_s( fs.writeDir, "%s/%s", fullPath, SaveFolderName );
		CoTaskMemFree( savedGames );
	}
	else
	{
		// this will never happen
		assert( 0 );
		Q_strcpy_s( fs.writeDir, fs.gameDir );
	}

#else

	// lol
	Q_strcpy_s( fs.writeDir, fs.gameDir );

#endif
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
	fs_debug = Cvar_Get( "fs_debug", "0", 0, "Controls FS spew." );
	fs_mod = Cvar_Get( "fs_mod", BASEDIRNAME, CVAR_NOSET, "The primary mod dir." );

	// Set gameDir
	// Use the current working directory as the base dir.
	// Consider security implications of this in the future.
	Sys_GetWorkingDirectory( fs.gameDir, sizeof( fs.gameDir ) );

	// Set writeDir
	SetWriteDirectory();

	// Set modDir
	Q_strcpy_s( fs.modDir, fs_mod->GetString() );

	if ( fs_debug->GetBool() ) {
		Com_DPrintf( "[FileSystem] gameDir is %s\n", fs.gameDir );
		Com_DPrintf( "[FileSystem] writeDir is %s\n", fs.writeDir );
		Com_DPrintf( "[FileSystem] modDir is %s\n", fs.modDir );
	}

	// Add the write dir as a search path, it takes top priority
	AddSearchPath( fs.writeDir, fs.modDir );
	// Add the mod dir as a search path
	AddSearchPath( fs.gameDir, fs.modDir );
	// Add our hardcoded base dir, so long as it's not the current mod dir
	if ( Q_stricmp( fs.modDir, BASEDIRNAME ) != 0 )
	{
		// Note that we disregard write directories of the base mod
		AddSearchPath( fs.writeDir, BASEDIRNAME );
	}
}

void Shutdown()
{
	// Clean up search paths
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

char *RelativePathToAbsolutePath( const char *relativePath )
{
	Com_FatalError( "FUCK!\n" );
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

int GetFileLength( fsHandle_t handle )
{
	long pos;
	long end;

	pos = ftell( (FILE *)handle );
	fseek( (FILE *)handle, 0, SEEK_END );
	end = ftell( (FILE *)handle );
	fseek( (FILE *)handle, pos, SEEK_SET );

	return static_cast<int>( end );
}

fsHandle_t OpenFileRead( const char *filename )
{
	char fullPath[MAX_OSPATH];

	// go through each search path until we find our file
	for ( searchPath_t *pSP = fs.searchPaths; pSP; pSP = pSP->pNext )
	{
		Q_sprintf_s( fullPath, "%s/%s", pSP->dirName, filename );

		fsHandle_t handle = (fsHandle_t)fopen( fullPath, "rb" );
		if ( handle == FS_INVALID_HANDLE ) {
			continue;
		}

		if ( fs_debug->GetBool() ) {
			Com_DPrintf( "[FileSystem] Reading %s\n", fullPath );
		}

		return handle;
	}

	if ( fs_debug->GetBool() ) {
		Com_DPrintf( S_COLOR_RED "[FileSystem] Can't find %s\n", filename );
	}

	return FS_INVALID_HANDLE;
}

fsHandle_t OpenFileWrite( const char *filename )
{
	char fullPath[MAX_OSPATH];
	Q_sprintf_s( fullPath, "%s/%s/%s", fs.writeDir, fs.modDir, filename );
	CreateAbsolutePath( fullPath, Q_strlen( fs.writeDir ) );

	fsHandle_t handle = (fsHandle_t)fopen( fullPath, "wb" );

	if ( fs_debug->GetBool() ) {
		if ( handle ) {
			Com_DPrintf( "[FileSystem] Writing %s\n", fullPath );
		} else {
			Com_DPrintf( S_COLOR_RED "[FileSystem] Failed to write %s\n", fullPath );
		}
	}

	return handle;
}

fsHandle_t OpenFileAppend( const char *filename )
{
	char fullPath[MAX_OSPATH];
	Q_sprintf_s( fullPath, "%s/%s/%s", fs.writeDir, fs.modDir, filename );
	CreateAbsolutePath( fullPath, Q_strlen( fs.writeDir ) );

	fsHandle_t handle = (fsHandle_t)fopen( fullPath, "ab" );

	if ( fs_debug->GetBool() ) {
		if ( handle ) {
			Com_DPrintf( "[FileSystem] Appending %s\n", fullPath );
		}
		else {
			Com_DPrintf( S_COLOR_RED "[FileSystem] Failed to append %s\n", fullPath );
		}
	}

	return handle;
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

void PrintFileFmt( fsHandle_t handle, const char *fmt, ... )
{
	va_list args;
	va_start( args, fmt );
	vfprintf( (FILE *)handle, fmt, args );
	va_end( args );
}

void FlushFile( fsHandle_t handle )
{
	fflush( (FILE *)handle );
}

bool FileExists( const char *filename, fsPath_t fsPath /*= FS_GAMEDIR*/ )
{
#if 0
	// using this implementation for now, it's slower but easier
	return LoadFile( filename, nullptr ) != -1;
#else
	// alternate implementation
	char fullPath[MAX_OSPATH];

	switch ( fsPath )
	{
	case FS_GAMEDIR:
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
	default:
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

int LoadFile( const char *filename, void **buffer, int extraData /*= 0*/ )
{
	assert( filename && filename[0] );
	assert( extraData >= 0 );

	fsHandle_t handle = OpenFileRead( filename );
	if ( handle == FS_INVALID_HANDLE )
	{
		if ( buffer ) {
			*buffer = nullptr;
		}
		return -1;
	}

	int length = GetFileLength( handle );

	if ( !buffer )
	{
		CloseFile( handle );
		return length;
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
	case FS_WRITEDIR:
	default:
	{
		Q_sprintf_s( buffer, bufferSize, "%s/%s/%s", fs.writeDir, fs.modDir, filename );

		if ( Sys_FileExists( buffer ) ) {
			return true;
		}

		return false;
	}
	}
}

void ExecAutoexec()
{
	Cbuf_AddText( "exec autoexec.cfg\n" );
}

}

// DLL interface
CFileSystem g_fileSystem;

#else

char	fs_gamedir[MAX_OSPATH];

cvar_t *fs_basedir;			// "." by default
cvar_t *fs_gamedirvar;		// 

//
// in memory
//

struct packfile_t
{
	char	name[MAX_QPATH];
	int		filepos, filelen;
};

struct pack_t
{
	char	filename[MAX_OSPATH];
	FILE	*handle;
	int		numfiles;
	packfile_t	*files;
};

struct searchpath_t
{
	char		filename[MAX_OSPATH];
	pack_t		*pack; // only one of filename / pack will be used
	searchpath_t	*next;
};

searchpath_t	*fs_searchpaths;
searchpath_t	*fs_base_searchpaths;	// without gamedirs

/*

All of Quake's data access is through a hierchal file system, but the contents of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all game directories.  The sys_* files pass this to host_init in quakeparms_t->basedir.  This can be overridden with the "-basedir" command line parm to allow code debugging in a different directory.  The base directory is
only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that all generated files (savegames, screenshots, demos, config files) will be saved to.  This can be overridden with the "-game" command line parameter.  The game directory can never be changed while quake is executing.  This is a precacution against having a malicious server instruct clients to write files over areas they shouldn't.

*/

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int FS_FileLength( FILE *f )
{
	long pos;
	long end;

	pos = ftell( f );
	fseek( f, 0, SEEK_END );
	end = ftell( f );
	fseek( f, pos, SEEK_SET );

	return end;
}

//-------------------------------------------------------------------------------------------------
// RAFAEL - Slart: All the mission pack code is shit
// zoid's download stuff is horrible too, damn
// This function is called somewhere
//-------------------------------------------------------------------------------------------------
int	Developer_searchpath()
{
	searchpath_t *search;

	for ( search = fs_searchpaths; search; search = search->next )
	{
		if ( strstr( search->filename, "xatrix" ) )
			return 1;

		if ( strstr( search->filename, "rogue" ) )
			return 2;
	}
	return 0;
}


//-------------------------------------------------------------------------------------------------
// Takes an explicit (not game tree related) path to a pak file.
//
// Loads the header and directory, adding the files at the beginning
// of the list so they override previous pack files.
//-------------------------------------------------------------------------------------------------
static pack_t *FS_LoadPackFile( const char *packfile )
{
	int				i;
	dpackheader_t	header;
	packfile_t		*newfiles;
	int				numpackfiles;
	pack_t			*pack;
	FILE			*packhandle;
	dpackfile_t		*info;

	packhandle = fopen( packfile, "rb" );
	if ( !packhandle )
		return nullptr;

	fread( &header, 1, sizeof( header ), packhandle );
	if ( LittleLong( header.ident ) != IDPAKHEADER )
		Com_FatalErrorf("%s is not a packfile", packfile );
	header.dirofs = LittleLong( header.dirofs );
	header.dirlen = LittleLong( header.dirlen );

	numpackfiles = header.dirlen / sizeof( dpackfile_t );

	if ( numpackfiles > MAX_FILES_IN_PACK )
		Com_FatalErrorf("%s has %d files, time for a new file format...", packfile, numpackfiles );

	fseek( packhandle, header.dirofs, SEEK_SET );

	info = (dpackfile_t *)Mem_Alloc( header.dirlen );

	fread( info, 1, header.dirlen, packhandle );

	newfiles = (packfile_t *)Mem_Alloc( numpackfiles * sizeof( packfile_t ) );

// parse the directory
	for ( i = 0; i < numpackfiles; i++ )
	{
		Q_strcpy_s( newfiles[i].name, info[i].name );
		newfiles[i].filepos = LittleLong( info[i].filepos );
		newfiles[i].filelen = LittleLong( info[i].filelen );
	}

	Mem_Free( info );

	pack = (pack_t *)Mem_Alloc( sizeof( pack_t ) );
	Q_strcpy_s( pack->filename, packfile );
	pack->handle = packhandle;
	pack->numfiles = numpackfiles;
	pack->files = newfiles;

	Com_Printf( "Added packfile %s (%i files)\n", packfile, numpackfiles );
	return pack;
}

//-------------------------------------------------------------------------------------------------
// Sets fs_gamedir, adds the directory to the head of the path,
// then loads and adds pak1.pak pak2.pak ... 
//-------------------------------------------------------------------------------------------------
static void FS_AddGameDirectory( const char *dir )
{
	int				i;
	searchpath_t	*search;
	pack_t			*pak;
	char			pakfile[MAX_OSPATH];

	Q_strcpy_s( fs_gamedir, dir );

	//
	// add the directory to the search path
	//
	search = (searchpath_t *)Mem_Alloc( sizeof( searchpath_t ) );
	Q_strcpy_s( search->filename, dir );
	search->pack = nullptr;
	search->next = fs_searchpaths;
	fs_searchpaths = search;

	//
	// add any pak files in the format pak0.pak pak1.pak, ...
	//
	for ( i = 0; i < 512; i++ )
	{
		Q_sprintf_s( pakfile, "%s/pak%i.pak", dir, i );
		pak = FS_LoadPackFile( pakfile );
		if ( !pak )
			break; // If a game is missing packs, then too bad
		search = (searchpath_t *)Mem_Alloc( sizeof( searchpath_t ) );
		search->filename[0] = '\0';
		search->pack = pak;
		search->next = fs_searchpaths;
		fs_searchpaths = search;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
char **FS_ListFiles( const char *findname, int *numfiles, unsigned musthave, unsigned canthave )
{
	char *s;
	int nfiles = 0;
	char **list = nullptr;

	s = Sys_FindFirst( findname, musthave, canthave );
	while ( s )
	{
		if ( s[strlen( s ) - 1] != '.' )
			nfiles++;
		s = Sys_FindNext( musthave, canthave );
	}
	Sys_FindClose();

	if ( !nfiles )
		return NULL;

	nfiles++; // add space for a guard
	*numfiles = nfiles;

	list = (char **)Mem_ClearedAlloc( sizeof( char * ) * nfiles );

	s = Sys_FindFirst( findname, musthave, canthave );
	nfiles = 0;
	while ( s )
	{
		if ( s[strlen( s ) - 1] != '.' )
		{
			list[nfiles] = Mem_CopyString( s );
#ifdef _WIN32
			// slart: is this needed?
			Q_strlwr( list[nfiles] );
#endif
			nfiles++;
		}
		s = Sys_FindNext( musthave, canthave );
	}
	Sys_FindClose();

	return list;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void FS_Dir_f()
{
	const char *path = NULL;
	char	findname[1024];
	char	wildcard[1024] = "*.*";
	char	**dirnames;
	int		ndirs;

	if ( Cmd_Argc() != 1 )
	{
		Q_strcpy_s( wildcard, Cmd_Argv( 1 ) );
	}

	while ( ( path = FS_NextPath( path ) ) != NULL )
	{
		char *tmp = findname;

		Q_sprintf_s( findname, "%s/%s", path, wildcard );

		while ( *tmp != 0 )
		{
			if ( *tmp == '\\' )
				*tmp = '/';
			tmp++;
		}
		Com_Printf( "Directory of %s\n", findname );
		Com_Printf( "----\n" );

		if ( ( dirnames = FS_ListFiles( findname, &ndirs, 0, 0 ) ) != 0 )
		{
			int i;

			for ( i = 0; i < ndirs - 1; i++ )
			{
				if ( strrchr( dirnames[i], '/' ) )
					Com_Printf( "%s\n", strrchr( dirnames[i], '/' ) + 1 );
				else
					Com_Printf( "%s\n", dirnames[i] );

				Mem_Free( dirnames[i] );
			}
			Mem_Free( dirnames );
		}
		Com_Printf( "\n" );
	};
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void FS_Path_f()
{
	searchpath_t *s;

	Com_Printf( "Current search path:\n" );
	for ( s = fs_searchpaths; s; s = s->next )
	{
		if ( s == fs_base_searchpaths )
			Com_Printf( "----------\n" );
		if ( s->pack )
			Com_Printf( "%s (%i files)\n", s->pack->filename, s->pack->numfiles );
		else
			Com_Printf( "%s\n", s->filename );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void FS_Init()
{
	Cmd_AddCommand( "path", FS_Path_f );
	Cmd_AddCommand( "dir", FS_Dir_f );

	//
	// basedir <path>
	// allows the game to run from outside the data tree
	//
	fs_basedir = Cvar_Get( "fs_basedir", ".", CVAR_NOSET );

	//
	// start up with base by default
	//
	FS_AddGameDirectory( va( "%s/" BASEDIRNAME, fs_basedir->GetString() ) );

	// any set gamedirs will be freed up to here
	fs_base_searchpaths = fs_searchpaths;

	// check for game override
	fs_gamedirvar = Cvar_Get( "fs_game", BASEDIRNAME, CVAR_LATCH | CVAR_SERVERINFO );
	if ( Q_strcmp( fs_gamedirvar->GetString(), BASEDIRNAME ) != 0 )
		FS_SetGamedir( fs_gamedirvar->GetString(), false );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void FS_Shutdown()
{
	searchpath_t *path = fs_searchpaths;
	searchpath_t *lastPath = nullptr;

	// clean up all search paths
	for ( searchpath_t *path = fs_searchpaths; path; path = path->next )
	{
		if ( lastPath ) {
			Mem_Free( path );
		}
	}
	Mem_Free( path );
}

//
// I/O
//

//-------------------------------------------------------------------------------------------------
// Finds the file in the search path.
// returns filesize and an open FILE *
// Used for streaming data out of either a pak file or a seperate file.
//-------------------------------------------------------------------------------------------------
int file_from_pak = 0; // download system hack
int FS_FOpenFile( const char *filename, FILE **file )
{
	int				i;
	searchpath_t	*search;
	char			netpath[MAX_OSPATH];
	pack_t			*pak;

	file_from_pak = 0;

//
// search through the path, one element at a time
//
	for ( search = fs_searchpaths; search; search = search->next )
	{
	// is the element a pak file?
		if ( search->pack )
		{
		// look through all the pak file elements
			pak = search->pack;
			for ( i = 0; i < pak->numfiles; i++ )
			{
			// It's better to emulate Windows here, with insensitive comparisons
				if ( Q_strcasecmp( pak->files[i].name, filename ) == 0 )
				{	// found it!
					file_from_pak = 1;
					Com_DPrintf( "PackFile: %s : %s\n", pak->filename, filename );
					// open a new file on the pakfile
					*file = fopen( pak->filename, "rb" );
					if ( !*file )
						Com_FatalErrorf("Couldn't reopen %s", pak->filename );
					fseek( *file, pak->files[i].filepos, SEEK_SET );
					return pak->files[i].filelen;
				}
			}
		}
		else
		{
		// check a file in the directory tree

			Q_sprintf_s( netpath, "%s/%s", search->filename, filename );

			*file = fopen( netpath, "rb" );
			if ( !*file )
				continue;

			Com_DPrintf( "FindFile: %s\n", netpath );

			return FS_FileLength( *file );
		}

	}

	Com_DPrintf( "FindFile: can't find %s\n", filename );

	*file = NULL;
	return -1;
}

//-------------------------------------------------------------------------------------------------
// For some reason, other dll's can't just cal fclose()
// on files returned by FS_FOpenFile...
//-------------------------------------------------------------------------------------------------
void FS_FCloseFile( FILE *f )
{
	fclose( f );
}

//-------------------------------------------------------------------------------------------------
// Properly handles partial reads
//-------------------------------------------------------------------------------------------------
void FileSystem::ReadFile( void *buffer, int len, FILE *f )
{
	size_t read;

	assert( len > 0 );

	read = fread( buffer, len, 1, f );

	if ( read == 0 ) {
		Com_FatalErrorf("FileSystem::ReadFile: 0 bytes read" );
	}
}

//-------------------------------------------------------------------------------------------------
// Filename are relative to the quake search path
// a null buffer will just return the file length without loading
//-------------------------------------------------------------------------------------------------
int FS_LoadFile( const char *path, void **buffer, int extradata )
{
	FILE *h;
	int len;

	assert( extradata >= 0 );

	// look for it in the filesystem or pack files
	len = FS_FOpenFile( path, &h );
	if ( len <= 0 )
	{
		if ( buffer )
			*buffer = NULL;
		return -1;
	}

	if ( !buffer )
	{
		FS_FCloseFile( h );
		assert( extradata == 0 ); // Shouldn't have extradata specified if we don't have a buffer
		return len;
	}

	*buffer = Mem_Alloc( len + extradata );

	FileSystem::ReadFile( *buffer, len, h );

	memset( (byte *)*buffer + len, 0, extradata );

	FS_FCloseFile( h );

	return len + extradata;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void FS_FreeFile( void *buffer )
{
	Mem_Free( buffer );
}

//-------------------------------------------------------------------------------------------------
// Allows enumerating all of the directories in the search path
//-------------------------------------------------------------------------------------------------
const char *FS_NextPath( const char *prevpath )
{
	searchpath_t *s;
	const char *prev;

	if ( !prevpath )
		return fs_gamedir;

	prev = fs_gamedir;
	for ( s = fs_searchpaths; s; s = s->next )
	{
		if ( s->pack )
			continue;
		if ( prevpath == prev )
			return s->filename;
		prev = s->filename;
	}

	return nullptr;
}

//=================================================================================================
// Utilities
//=================================================================================================

//-------------------------------------------------------------------------------------------------
// Sets the gamedir and path to a different directory
//-------------------------------------------------------------------------------------------------
void FS_SetGamedir( const char *dir, bool flush )
{
	searchpath_t *next;

	if ( strstr( dir, ".." ) || strstr( dir, "/" )
		|| strstr( dir, "\\" ) || strstr( dir, ":" ) )
	{
		Com_Printf( "Gamedir should be a single filename, not a path\n" );
		return;
	}

	//
	// free up any current game dir info
	//
	while ( fs_searchpaths != fs_base_searchpaths )
	{
		if ( fs_searchpaths->pack )
		{
			fclose( fs_searchpaths->pack->handle );
			Mem_Free( fs_searchpaths->pack->files );
			Mem_Free( fs_searchpaths->pack );
		}
		next = fs_searchpaths->next;
		Mem_Free( fs_searchpaths );
		fs_searchpaths = next;
	}

	//
	// optionally flush all data, so it will be forced to reload
	//
	if ( flush && ( dedicated && !dedicated->GetString() ) )
		Cbuf_AddText( "vid_restart\nsnd_restart\n" );

	Q_sprintf_s( fs_gamedir, "%s/%s", fs_basedir->GetString(), dir );

	if ( Q_strcmp( dir, BASEDIRNAME ) == 0 || *dir == '\0' )
	{
		Cvar_FullSet( "gamedir", "", CVAR_SERVERINFO | CVAR_NOSET );
		Cvar_FullSet( "fs_game", "", CVAR_LATCH | CVAR_SERVERINFO );
	}
	else
	{
		Cvar_FullSet( "gamedir", dir, CVAR_SERVERINFO | CVAR_NOSET );
		FS_AddGameDirectory( va( "%s/%s", fs_basedir->GetString(), dir ) );
	}
}

//-------------------------------------------------------------------------------------------------
// Called to find where to write a file (demos, savegames, etc)
//-------------------------------------------------------------------------------------------------
const char *FS_Gamedir()
{
	return fs_gamedir[0] ? fs_gamedir : BASEDIRNAME;
}

//-------------------------------------------------------------------------------------------------
// Creates any directories needed to store the given filename
//-------------------------------------------------------------------------------------------------
void FS_CreatePath( char *path )
{
	char *ofs;

	for ( ofs = path + 1; *ofs; ofs++ )
	{
		if ( *ofs == '/' )
		{	// create the directory
			*ofs = 0;
			Sys_CreateDirectory( path );
			*ofs = '/';
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void FS_ExecAutoexec()
{
	char name[MAX_QPATH];

	const char *dir = FS_Gamedir();
	if ( *dir ) {
		Q_sprintf_s( name, "%s/%s/autoexec.cfg", fs_basedir->GetString(), dir );
	} else {
		Q_sprintf_s( name, "%s/%s/autoexec.cfg", fs_basedir->GetString(), BASEDIRNAME );
	}
	if ( Sys_FindFirst( name, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM ) ) {
		Cbuf_AddText( "exec autoexec.cfg\n" );
	}
	Sys_FindClose();
}

#endif
