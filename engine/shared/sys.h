//=================================================================================================
// Non-portable system services
//=================================================================================================

#pragma once

// frameTime in milliseconds
extern uint sys_frameTime;

void	Sys_Init( int argc, char **argv );
void	Sys_Shutdown();

void	Sys_AppActivate();

void	Sys_UnloadGame();
void	Sys_UnloadCGame();
void	*Sys_GetGameAPI( void *parms );
void	*Sys_GetCGameAPI( void *parms );
// loads the game dll and calls the api init function

char	*Sys_ConsoleInput();
void	Sys_ConsoleOutput( const char *string );

int		Sys_GetNumVidModes();
bool	Sys_GetVidModeInfo( int &width, int &height, int mode );

[[noreturn]]
void	Sys_Quit( int code );
[[noreturn]]
void	Sys_Error( const platChar_t *mainInstruction, const char *msg );

char *	Sys_GetClipboardData();
void	Sys_CopyProtect();

// common file dialog interface

struct filterSpec_t
{
	const platChar_t *pszName;
	const platChar_t *pszSpec;
};

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
