// The q_shared of the tools

#pragma once

#include "q_shared.h"

extern bool verbose;
void Com_Printf( _Printf_format_string_ const char *fmt, ... );
void Com_DPrintf( _Printf_format_string_ const char *fmt, ... );
[[noreturn]]
void Com_Error( int code, _Printf_format_string_ const char *fmt, ... );

#define qprintf( ... ) Com_DPrintf( __VA_ARGS__ )
#define Error( ... ) Com_Error( ERR_FATAL, __VA_ARGS__ )

// Game directory, replace with filesystem
extern char qdir[1024];
extern char gamedir[1024];
void SetQdirFromPath( char *path );

FILE	*SafeOpenWrite (char *filename);
FILE	*SafeOpenRead (char *filename);
void	SafeRead (FILE *f, void *buffer, int count);
void	SafeWrite (FILE *f, void *buffer, int count);

int		LoadFile (char *filename, void **bufferptr);
int		TryLoadFile (char *filename, void **bufferptr);
void	SaveFile (char *filename, void *buffer, int count);

char	*ExpandArg( char *path );
char	*ExpandPath( char *path );
char	*copystring( const char *s );

void 	DefaultExtension (char *path, char *extension);
void 	DefaultPath (char *path, char *basepath);
void 	StripFilename (char *path);
void 	StripExtension (char *path);

void 	ExtractFilePath (char *path, char *dest);
void 	ExtractFileBase (char *path, char *dest);
void	ExtractFileExtension (char *path, char *dest);
