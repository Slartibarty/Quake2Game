//=================================================================================================
// The virtual filesystem
//
// Handles loading of data from pack files among other things
//=================================================================================================

#pragma once

#include <cstdio>

void		FS_Init();

// I/O

int			FS_FOpenFile( const char *filename, FILE **file );
void		FS_FCloseFile( FILE *f );
// properly handles partial reads
void		FS_Read( void *buffer, int len, FILE *f );

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
