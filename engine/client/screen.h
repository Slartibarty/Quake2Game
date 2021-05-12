// screen.h

#pragma once

void	SCR_Init (void);

void	SCR_UpdateScreen (void);

void	SCR_CenterPrint (const char *str);
void	SCR_BeginLoadingPlaque (void);
void	SCR_EndLoadingPlaque (void);

void	SCR_DebugGraph (float value, uint32 color);

void	SCR_TouchPics (void);

void	SCR_RunConsole (void);

extern	cvar_t		*scr_viewsize;
extern	cvar_t		*crosshair;

extern	vrect_t		scr_vrect;		// position of render window

extern	char		crosshair_pic[MAX_QPATH];
extern	int			crosshair_width, crosshair_height;

//
// scr_cin.c
//
void SCR_PlayCinematic (const char *name);
qboolean SCR_DrawCinematic (void);
void SCR_RunCinematic (void);
void SCR_StopCinematic (void);
void SCR_FinishCinematic (void);

