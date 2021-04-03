//=================================================================================================
// The console
//=================================================================================================

#pragma once

#define NUM_CON_TIMES	8

#define CON_TEXTSIZE	0x10000

struct console_t
{
	bool	initialized;

	char	text[CON_TEXTSIZE];
	int		current;		// line where next message will be printed
	int		x;				// offset in current line for next print
	int		display;		// bottom of console displays this line

	int 	linewidth;		// characters across screen
	int		totallines;		// total lines in console scrollback

	int		xadjust;		// for wide aspect screens

	int		vislines;

	float	times[NUM_CON_TIMES];	// cls.realtime time the line was generated
									// for transparent notify lines
};

extern console_t con;

void Con_CheckResize (void);
void Con_Init (void);
void Con_DrawConsole (float frac);
void Con_Print (const char *txt);
void Con_Clear_f (void);
void Con_DrawNotify (void);
void Con_ClearNotify (void);
void Con_ToggleConsole_f (void);

// Q3 added
void Con_PageUp( void );
void Con_PageDown( void );
void Con_Top( void );
void Con_Bottom( void );
