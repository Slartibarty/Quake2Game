//=================================================================================================
// Video driver defs
//=================================================================================================

#pragma once

struct vrect_t
{
	int x, y, width, height;
};

struct viddef_t
{
	int width, height;		// coordinates from main game
};

extern viddef_t viddef;			// global video state

// Video module initialisation etc
void	VID_Init();
void	VID_Shutdown();
