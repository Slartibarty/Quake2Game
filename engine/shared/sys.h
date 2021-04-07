//=================================================================================================
// Non-portable system services
//=================================================================================================

#pragma once

void	Sys_Init( void );

void	Sys_AppActivate( void );

void	Sys_UnloadGame( void );
void	Sys_UnloadCGame( void );
void	*Sys_GetGameAPI( void *parms );
void	*Sys_GetCGameAPI( void *parms );
// loads the game dll and calls the api init function

char	*Sys_ConsoleInput( void );
void	Sys_ConsoleOutput( const char *string );
void	Sys_SendKeyEvents( void );

int		Sys_GetNumVidModes();
bool	Sys_GetVidModeInfo( int &width, int &height, int mode );

void	Sys_OutputDebugString( const char *msg );
[[noreturn]]
void	Sys_Error( const char *msg );
[[noreturn]]
void	Sys_Quit( int code );

char	*Sys_GetClipboardData( void );
void	Sys_CopyProtect( void );
