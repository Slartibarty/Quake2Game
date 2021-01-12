//=================================================================================================
// The virtual filesystem
//
// Handles loading of data from pack files among other things
//=================================================================================================

#pragma once

#include <cstdio>

void		FS_InitFilesystem( void );
void		FS_SetGamedir( const char *dir );
const char	*FS_Gamedir( void );
char		*FS_NextPath( const char *prevpath );
void		FS_ExecAutoexec( void );

int			FS_FOpenFile( const char *filename, FILE **file );
void		FS_FCloseFile( FILE *f );

int			FS_LoadFile( const char *path, void **buffer );
// a null buffer will just return the file length without loading
// a -1 length is not present

void		FS_Read( void *buffer, int len, FILE *f );
// properly handles partial reads

void		FS_FreeFile( void *buffer );

void		FS_CreatePath( char *path );
