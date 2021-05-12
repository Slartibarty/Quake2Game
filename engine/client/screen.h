
#pragma once

void	SCR_Init();

void	SCR_UpdateScreen();

void	SCR_CenterPrint( const char *str );
void	SCR_BeginLoadingPlaque();
void	SCR_EndLoadingPlaque();

void	SCR_DebugGraph( float value, uint32 color );

void	SCR_TouchPics();

void	SCR_RunConsole();

extern	cvar_t		*scr_viewsize;
extern	cvar_t		*cl_crosshair;

extern	vrect_t		scr_vrect;		// position of render window

extern	char		crosshair_pic[MAX_QPATH];
extern	int			crosshair_width, crosshair_height;

//
// cl_cin
//
void	SCR_PlayCinematic( const char *name );
bool	SCR_DrawCinematic();
void	SCR_RunCinematic();
void	SCR_StopCinematic();
void	SCR_FinishCinematic();

