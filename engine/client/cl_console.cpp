/*
===================================================================================================

	Consul hl2 beta

===================================================================================================
*/

#include "client.h"

static constexpr uint32 colorConsoleText = colorWhite;

#define	DEFAULT_CONSOLE_WIDTH	78

#define NUM_CON_TIMES	8

#define CON_TEXTSIZE	0x10000

static struct console_t
{
	char	text[CON_TEXTSIZE];	// the contents of the console window (not the edit line)
	int		current;			// line where next message will be printed
	int		x;					// offset in current line for next print
	int		display;			// bottom of console displays this line

	int 	linewidth;			// characters across screen
	int		totallines;			// total lines in console scrollback

	int		xadjust;			// for wide aspect screens

	int		vislines;

	float	times[NUM_CON_TIMES];	// cls.realtime time the line was generated
									// for transparent notify lines

	bool	initialized;
} con;

cvar_t *con_notifytime;

// duplicated in: cl_console.cpp, cl_keys.cpp
#define MAXCMDLINE 256
extern char		key_lines[32][MAXCMDLINE];
extern int		edit_line;
extern int		key_linepos;

/*
========================
DrawString

This shouldn't be here
========================
*/
void DrawString( int x, int y, const char *s )
{
	while ( *s ) {
		R_DrawChar( x, y, *s );
		x += CONCHAR_WIDTH;
		s++;
	}
}

/*
========================
DrawAltString

This shouldn't be here
========================
*/
void DrawAltString( int x, int y, const char *s )
{
	while ( *s ) {
		R_DrawCharColor( x, y, *s, colorGreen );
		x += CONCHAR_WIDTH;
		s++;
	}
}

/*
========================
Key_ClearTyping
========================
*/
void Key_ClearTyping()
{
	key_lines[edit_line][0] = 0;	// clear any typing
	key_linepos = 0;
}

/*
===================================================================================================

	Commands

===================================================================================================
*/

/*
========================
Con_ToggleConsole_f
========================
*/
void Con_ToggleConsole_f()
{
	SCR_EndLoadingPlaque(); // get rid of loading plaque

	// uncomment to kill the attract loop
	if ( cl.attractloop ) {
		Cbuf_AddText( "killserver\n" );
		return;
	}

	if ( cls.state == ca_disconnected ) {
		// start the demo loop again
		Cbuf_AddText( "d1\n" );
		return;
	}

	Key_ClearTyping();
	Con_ClearNotify();

	if ( cls.key_dest == key_console ) {
		M_ForceMenuOff();
		Cvar_Set( "paused", "0" );
	}
	else {
		M_ForceMenuOff();
		cls.key_dest = key_console;

		if ( Cvar_VariableValue( "maxclients" ) == 1 && Com_ServerState() ) {
			Cvar_Set( "paused", "1" );
		}
	}
}

/*
========================
Con_ToggleChat_f
========================
*/
static void Con_ToggleChat_f()
{
	Key_ClearTyping();

	if ( cls.key_dest == key_console ) {
		if ( cls.state == ca_active ) {
			M_ForceMenuOff();
			cls.key_dest = key_game;
		}
	}
	else {
		cls.key_dest = key_console;
	}

	Con_ClearNotify();
}

/*
========================
Con_Clear_f
========================
*/
static void Con_Clear_f()
{
	memset( con.text, ' ', CON_TEXTSIZE );

	Con_Bottom(); // go to end
}

/*
========================
Con_Dump_f

Save the console contents out to a file
========================
*/
static void Con_Dump_f()
{
	int		l, x;
	char *	line;
	FILE *	f;
	char	name[MAX_OSPATH];

	if ( Cmd_Argc() != 2 ) {
		Com_Print( "usage: condump <filename>\n" );
		return;
	}

	Q_sprintf_s( name, "%s/%s.txt", FS_Gamedir(), Cmd_Argv( 1 ) );

	Com_Printf( "Dumping console text to %s.\n", name );
	FS_CreatePath( name );
	f = fopen( name, "w" );
	if ( !f ) {
		Com_Print( "ERROR: couldn't open.\n" );
		return;
	}

	// skip empty lines
	for ( l = con.current - con.totallines + 1; l <= con.current; l++ )
	{
		line = con.text + ( l % con.totallines ) * con.linewidth;
		for ( x = 0; x < con.linewidth; x++ ) {
			if ( line[x] != ' ' ) {
				break;
			}
		}
		if ( x != con.linewidth ) {
			break;
		}
	}

	char *buffer = (char *)Z_Malloc( con.linewidth + 1 );

	// write the remaining lines
	buffer[con.linewidth] = 0;
	for ( ; l <= con.current; l++ )
	{
		line = con.text + ( l % con.totallines ) * con.linewidth;
		Q_strcpy_s( buffer, con.linewidth, line );
		for ( x = con.linewidth - 1; x >= 0; x-- )
		{
			if ( buffer[x] == ' ' )
				buffer[x] = 0;
			else
				break;
		}

		fputs( buffer, f );
		fputc( '\n', f );
	}

	free( buffer );

	fclose( f );
}

/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify (void)
{
	int		i;
	
	for (i=0 ; i<NUM_CON_TIMES ; i++)
		con.times[i] = 0;
}
						
/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f (void)
{
	chat_team = false;
	cls.key_dest = key_message;
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f (void)
{
	chat_team = true;
	cls.key_dest = key_message;
}

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize (void)
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;
	static char	tbuf[CON_TEXTSIZE];

	width = (viddef.width/CONCHAR_WIDTH) - 2;

	if (width == con.linewidth)
		return;

	if (width < 1)			// video hasn't been initialized yet
	{
		width = DEFAULT_CONSOLE_WIDTH;
		con.linewidth = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		memset (con.text, ' ', CON_TEXTSIZE);
	}
	else
	{
		oldwidth = con.linewidth;
		con.linewidth = width;
		oldtotallines = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		numlines = oldtotallines;

		if (con.totallines < numlines)
			numlines = con.totallines;

		numchars = oldwidth;
	
		if (con.linewidth < numchars)
			numchars = con.linewidth;

		memcpy (tbuf, con.text, CON_TEXTSIZE);
		memset (con.text, ' ', CON_TEXTSIZE);

		for (i=0 ; i<numlines ; i++)
		{
			for (j=0 ; j<numchars ; j++)
			{
				con.text[(con.totallines - 1 - i) * con.linewidth + j] =
						tbuf[((con.current - i + oldtotallines) %
							  oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

	con.current = con.totallines - 1;
	con.display = con.current;
}

/*
========================
Con_Init
========================
*/
void Con_Init()
{
	con.linewidth = -1;

	Con_CheckResize();

	con_notifytime = Cvar_Get( "con_notifytime", "3", 0 );

	Cmd_AddCommand( "toggleconsole", Con_ToggleConsole_f );
	Cmd_AddCommand( "togglechat", Con_ToggleChat_f );
	Cmd_AddCommand( "messagemode", Con_MessageMode_f );
	Cmd_AddCommand( "messagemode2", Con_MessageMode2_f );
	Cmd_AddCommand( "clear", Con_Clear_f );
	Cmd_AddCommand( "condump", Con_Dump_f );

	con.initialized = true;

	Com_Print( "Console initialized.\n" );
}

/*
========================
Con_IsInitialized
========================
*/
bool Con_IsInitialized()
{
	return con.initialized;
}

/*
========================
Con_Linefeed
========================
*/
void Con_Linefeed()
{
	con.x = 0;
	if ( con.display == con.current ) {
		con.display++;
	}
	con.current++;
	memset( &con.text[( con.current % con.totallines ) * con.linewidth], ' ', con.linewidth );
}

/*
========================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
========================
*/
void Con_Print (const char *txt)
{
	int		y;
	int		c, l;
	static bool	cr;

	if ( !con.initialized ) {
		return;
	}

	while ( (c = *txt) )
	{
		// count word length
		for ( l = 0; l < con.linewidth; l++ ) {
			if ( txt[l] <= ' ' ) {
				break;
			}
		}

		// word wrap
		if ( l != con.linewidth && ( con.x + l > con.linewidth ) ) {
			con.x = 0;
		}

		txt++;

		if (cr)
		{
			con.current--;
			cr = false;
		}

		if (!con.x) {
			Con_Linefeed ();
			// mark time for transparent overlay
			if ( con.current >= 0 ) {
				con.times[con.current % NUM_CON_TIMES] = (float)cls.realtime;
			}
		}

		switch (c)
		{
		case '\n':
			con.x = 0;
			break;

		case '\r':
			con.x = 0;
			cr = true;
			break;

		default:	// display character and advance
			y = con.current % con.totallines;
			con.text[y*con.linewidth+con.x] = c;
			con.x++;
			if (con.x >= con.linewidth)
				con.x = 0;
			break;
		}
		
	}
}

/*
===================================================================================================

	Drawing

===================================================================================================
*/

/*
================
Con_DrawInput

The input line scrolls horizontally if typing goes beyond the right edge
========================
*/
#if 1
static void Con_DrawInput()
{
	int i;

	if ( cls.key_dest == key_menu ) {
		return;
	}
	if ( cls.key_dest != key_console && cls.state == ca_active ) {
		// don't draw anything (always draw if not active)
		return;
	}

	char *text = key_lines[edit_line];

	// fill out remainder with spaces
	for ( i = key_linepos; i < con.linewidth; i++ ) {
		text[i] = ' ';
	}

	// prestep if horizontally scrolling
	if ( key_linepos >= con.linewidth ) {
		text += key_linepos - con.linewidth;
	}

	// draw it
	int y = con.vislines - ( CONCHAR_HEIGHT * 2 );

	R_DrawCharColor( con.xadjust + CONCHAR_WIDTH,
		y, ']', colorConsoleText );

	for ( i = 0; i < con.linewidth; i++ ) {
		R_DrawCharColor( con.xadjust + ( i + 2 ) * CONCHAR_WIDTH,
			y, text[i], colorWhite );
	}

	// add the cursor frame
	if ( (int)( cls.realtime >> 8 ) & 1 ) {
		R_DrawCharColor( con.xadjust + ( key_linepos + 2 ) * CONCHAR_WIDTH,
			y, 11, colorWhite );
	}
}
#else
void Con_DrawInput (void)
{
	int		y;
	int		i;
	char	*text;

	if (cls.key_dest == key_menu)
		return;
	if (cls.key_dest != key_console && cls.state == ca_active)
		return;		// don't draw anything (always draw if not active)

	text = key_lines[edit_line];
	
// add the cursor frame
	text[key_linepos] = 10+((int)(cls.realtime>>8)&1);
	
// fill out remainder with spaces
	for (i=key_linepos+1 ; i< con.linewidth ; i++)
		text[i] = ' ';
		
//	prestep if horizontally scrolling
	if (key_linepos >= con.linewidth)
		text += 1 + key_linepos - con.linewidth;
		
// draw it
	y = con.vislines-16;

	for (i=0 ; i<con.linewidth ; i++)
		R_DrawChar ( con.xadjust + (i+1)*CONCHAR_WIDTH, con.vislines - (CONCHAR_HEIGHT*2), text[i]);

// remove cursor
	key_lines[edit_line][key_linepos] = 0;
}
#endif

/*
========================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
========================
*/
void Con_DrawNotify()
{
	int		x, v;
	char *	text;
	int		i;
	int		time;
	char *	s;
	int		skip;

	v = 0;
	for (i= con.current-NUM_CON_TIMES+1 ; i<=con.current ; i++)
	{
		if ( i < 0 ) {
			continue;
		}
		time = con.times[i % NUM_CON_TIMES];
		if ( time == 0 ) {
			continue;
		}
		time = cls.realtime - time;
		if ( time > static_cast<int>( SEC2MS( con_notifytime->value ) ) ) {
			continue;
		}
		text = con.text + (i % con.totallines)*con.linewidth;

		uint32 color = colorConsoleText;
		int localLineWidth = con.linewidth;
		
		for ( x = 0; x < localLineWidth; x++ ) {
			if ( text[x] == C_COLOR_ESCAPE && IsColorIndex( text[x + 1] ) ) {
				color = ColorForIndex( text[x + 1] );
				localLineWidth -= 2;
				text += 2;
			}
			R_DrawCharColor( con.xadjust + ( x + 1 ) * CONCHAR_WIDTH, v, text[x], color );
		}

		v += CONCHAR_HEIGHT;
	}

	if (cls.key_dest == key_message)
	{
		if (chat_team)
		{
			DrawString (8, v, "say_team:");
			skip = 11;
		}
		else
		{
			DrawString (8, v, "say:");
			skip = 5;
		}

		s = chat_buffer;
		if (chat_bufferlen > (viddef.width/CONCHAR_WIDTH)-(skip+1))
			s += chat_bufferlen - ((viddef.width/CONCHAR_WIDTH)-(skip+1));
		x = 0;
		while(s[x])
		{
			R_DrawChar ( con.xadjust + (x+skip)*CONCHAR_WIDTH, v, s[x]);
			x++;
		}
		R_DrawChar ( con.xadjust + (x+skip)*CONCHAR_WIDTH, v, 10+((cls.realtime>>8)&1));
		v += CONCHAR_HEIGHT;
	}
	
	if (v)
	{
		SCR_AddDirtyPoint (0,0);
		SCR_AddDirtyPoint (viddef.width-1, v);
	}
}

/*
========================
Con_DrawConsole

Draws the console with the solid background
========================
*/
void Con_DrawConsole( float frac )
{
	int		i, x, y;
	int		rows;
	char *	text;
	int		row;
	int		lines;

	assert( frac > 0.0f );

	lines = static_cast<int>( viddef.height * frac );
	if ( lines <= 0 ) {
		return;
	}

	if ( lines > viddef.height ) {
		lines = viddef.height;
	}

	con.xadjust = 0;

	// draw the background
	R_DrawFilled( 0, -viddef.height + lines, viddef.width, viddef.height, PackColor( 0, 0, 0, 192 ) );
	R_DrawFilled( 0, lines - 2, viddef.width, 2, colorConsoleText );
	SCR_AddDirtyPoint( 0, 0 );
	SCR_AddDirtyPoint( viddef.width - 1, lines - 1 );

	// draw the version
	constexpr int verLength = sizeof( BLD_STRING ) - 1;
	for ( x = 0; x < verLength; x++ ) {
		R_DrawCharColor( viddef.width - ( verLength - x + 1 ) * CONCHAR_WIDTH,
			lines - ( CONCHAR_HEIGHT * 2 ), BLD_STRING[x], colorConsoleText );
	}

	// draw the text
	con.vislines = lines;
	rows = ( lines - CONCHAR_WIDTH ) / CONCHAR_WIDTH;	// rows of text to draw

	y = lines - ( CONCHAR_HEIGHT * 3 );

	// draw from the bottom up
	if (con.display != con.current) {
		// draw arrows to show the buffer is backscrolled
		for ( x = 0; x < con.linewidth; x += 4 ) {
			R_DrawCharColor( con.xadjust + ( x + 1 ) * CONCHAR_WIDTH, y, '^', colorConsoleText );
		}
		y -= CONCHAR_HEIGHT;
		rows--;
	}
	
	row = con.display;

	//assert( x != 0 );
	// Doom 3 BFG code:
	/*if ( x == 0 ) {
		row--;
	}*/

	for ( i = 0; i < rows; i++, y -= CONCHAR_HEIGHT, row-- )
	{
		if ( row < 0 ) {
			break;
		}
		if ( con.current - row >= con.totallines ) {
			// past scrollback wrap point
			continue;
		}

		text = con.text + ( row % con.totallines ) * con.linewidth;

		uint32 color = colorConsoleText;
		int localLineWidth = con.linewidth;

		for ( x = 0; x < localLineWidth; x++ ) {
			if ( text[x] == C_COLOR_ESCAPE && IsColorIndex( text[x + 1] ) ) {
				color = ColorForIndex( text[x + 1] );
				localLineWidth -= 2;
				text += 2;
			}
			R_DrawCharColor( con.xadjust + ( x + 1 ) * CONCHAR_WIDTH, y, text[x], color );
		}
	}

	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput ();
}

/*
===================================================================================================

	Initialization

===================================================================================================
*/


/*
===================================================================================================

	Navigation

===================================================================================================
*/

void Con_PageUp()
{
	con.display -= 2;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}
}

void Con_PageDown()
{
	con.display += 2;
	if ( con.display > con.current ) {
		con.display = con.current;
	}
}

void Con_Top()
{
	con.display = con.totallines;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}
}

void Con_Bottom()
{
	con.display = con.current;
}
