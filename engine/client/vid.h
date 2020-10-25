// vid.h -- video driver defs

#pragma once

struct vrect_t
{
	int				x, y, width, height;
};

struct viddef_t
{
	unsigned		width, height;		// coordinates from main game
};

extern viddef_t viddef;					// global video state

// Video module initialisation etc
void	VID_Init (void);
void	VID_Shutdown (void);
void	VID_CheckChanges (void);

qboolean VID_GetModeInfo(int *width, int *height, int mode);
int VID_GetNumModes();

void	VID_MenuInit( void );
void	VID_MenuDraw( void );
const char *VID_MenuKey( int );
