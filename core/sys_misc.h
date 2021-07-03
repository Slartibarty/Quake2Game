/*
===================================================================================================

	System specific functionality

===================================================================================================
*/

#pragma once

/*
=======================================
	The soon to be dead hunk allocator
=======================================
*/

void *	Hunk_Begin( size_t maxsize );
void *	Hunk_Alloc( size_t size );
void	Hunk_Free( void *buf );
size_t	Hunk_End();

/*
=======================================
	Miscellaneous
=======================================
*/

void	Sys_CopyFile( const char *src, const char *dst );
void	Sys_DeleteFile( const char *filename );
void	Sys_CreateDirectory( const char *path );
void	Sys_GetWorkingDirectory( char *path, uint length );

/*
=======================================
	Timing
=======================================
*/

void	Time_Init();
double	Time_FloatSeconds();
double	Time_FloatMilliseconds();
double	Time_FloatMicroseconds();

/*int64	Time_Seconds();*/
int64	Time_Milliseconds();
int64	Time_Microseconds();

// Legacy
int		Sys_Milliseconds();

/*
=======================================
	Directory searching
=======================================
*/

// directory searching
#define SFF_ARCH	0x01
#define SFF_HIDDEN	0x02
#define SFF_RDONLY	0x04
#define SFF_SUBDIR	0x08
#define SFF_SYSTEM	0x10

// pass in an attribute mask of things you wish to REJECT
char *	Sys_FindFirst( const char *path, unsigned musthave, unsigned canthave );
char *	Sys_FindNext( unsigned musthave, unsigned canthave );
void	Sys_FindClose( void );
