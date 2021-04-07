//=================================================================================================
// Video driver defs
//=================================================================================================

#pragma once

struct vrect_t
{
	int		x, y, width, height;
};

struct viddef_t
{
	int		width, height;		// coordinates from main game
};

extern viddef_t viddef;			// global video state

// Video module initialisation etc
void	VID_Init (void);
void	VID_Shutdown (void);
void	VID_CheckChanges (void);

int		VID_GetNumModes();
bool	VID_GetModeInfo(int &width, int &height, int mode);

void	VID_MenuInit( void );
void	VID_MenuDraw( void );
const char *VID_MenuKey( int );
