//=================================================================================================
// Non-portable system services
//=================================================================================================

#pragma once

struct filterSpec_t
{
	const platChar_t *pszName;
	const platChar_t *pszSpec;
};

void	Sys_Init( void );

void	Sys_AppActivate( void );

void	Sys_UnloadGame( void );
void	Sys_UnloadCGame( void );
void	*Sys_GetGameAPI( void *parms );
void	*Sys_GetCGameAPI( void *parms );
// loads the game dll and calls the api init function

char	*Sys_ConsoleInput( void );
void	Sys_ConsoleOutput( const char *string );

int		Sys_GetNumVidModes();
bool	Sys_GetVidModeInfo( int &width, int &height, int mode );

void	Sys_OutputDebugString( const char *msg );
[[noreturn]]
void	Sys_Error( const char *msg );
[[noreturn]]
void	Sys_Quit( int code );

char	*Sys_GetClipboardData( void );
void	Sys_CopyProtect( void );

// common file dialog interface

void Sys_FileOpenDialog(
	std::string &filename,
	const platChar_t *title,
	const filterSpec_t *supportedTypes,
	const uint numTypes,
	const uint defaultIndex );

void Sys_FileOpenDialogMultiple(
	std::vector<std::string> &filenames,
	const platChar_t *title,
	const filterSpec_t *supportedTypes,
	const uint numTypes,
	const uint defaultIndex );
