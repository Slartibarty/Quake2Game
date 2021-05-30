/*
===================================================================================================

	"Screen" operations. Master for refresh, status bar, console, chat, notify, etc.

	Handles updating the screen, calls down into the view functions to render the 3D world,
	then draws 2D imagery on top (HUD, crosshair, etc). Also parses HUD scripts from the game code

	Ideal candidate to be moved to cgame, alongside cl_view

===================================================================================================
*/

/*

  id developer's notes:

  full screen console
  put up loading plaque
  blanked background with loading plaque
  blanked background with menu
  cinematics
  full screen image for quit and victory

  end of unit intermissions

*/

#include "cl_local.h"

float		scr_con_current;	// aproaches scr_conlines at scr_conspeed
float		scr_conlines;		// 0.0 to 1.0 lines of console to display

bool		scr_initialized;		// ready to draw

int			scr_draw_loading;

vrect_t		scr_vrect;		// position of render window on screen

cvar_t		*scr_viewsize;
cvar_t		*scr_conspeed;
cvar_t		*scr_centertime;
cvar_t		*scr_showpause;
cvar_t		*scr_printspeed;

cvar_t		*scr_netgraph;
cvar_t		*scr_timegraph;
cvar_t		*scr_debuggraph;
cvar_t		*scr_graphheight;
cvar_t		*scr_graphscale;
cvar_t		*scr_graphshift;

char		crosshair_pic[MAX_QPATH];
int			crosshair_width, crosshair_height;

void SCR_TimeRefresh_f();
void SCR_Loading_f();

/*
===================================================================================================

	Bar graphs

===================================================================================================
*/

/*
========================
CL_AddNetgraph

A new packet was just parsed
========================
*/
void CL_AddNetgraph()
{
	int		i;
	int		in;
	int		ping;

	// if using the debuggraph for something else, don't
	// add the net lines
	if (scr_debuggraph->value || scr_timegraph->value)
		return;

	for (i=0 ; i<cls.netchan.dropped ; i++)
		SCR_DebugGraph( 30, PackColor( 167, 59, 43, 255 ) );

	for (i=0 ; i<cl.surpressCount ; i++)
		SCR_DebugGraph( 30, PackColor( 255, 191, 15, 255 ) );

	// see what the latency was on this packet
	in = cls.netchan.incoming_acknowledged & (CMD_BACKUP-1);
	ping = cls.realtime - cl.cmd_time[in];
	ping /= 30;
	if (ping > 30)
		ping = 30;
	SCR_DebugGraph( (float)ping, colors::green );
}

struct graphsamp_t
{
	float	value;
	uint32	color;
};

static int			current;
static graphsamp_t	values[1024];

/*
========================
SCR_DebugGraph
========================
*/
void SCR_DebugGraph( float value, uint32 color )
{
	values[current & 1023].value = value;
	values[current & 1023].color = color;
	current++;
}

/*
========================
SCR_DrawDebugGraph
========================
*/
void SCR_DrawDebugGraph()
{
	int		a, x, y, w, i, h;
	float	v;
	uint32	color;

	//
	// draw the graph
	//
	w = scr_vrect.width;

	x = scr_vrect.x;
	y = scr_vrect.y + scr_vrect.height;
	R_DrawFilled( x, y - scr_graphheight->GetInt32(), w, scr_graphheight->GetInt32(), colors::dkGray );

	for (a=0 ; a<w ; a++)
	{
		i = (current-1-a+1024) & 1023;
		v = values[i].value;
		color = values[i].color;
		v = v*scr_graphscale->value + scr_graphshift->value;
		
		if ( v < 0.0f ) {
			v += scr_graphheight->GetInt32() * (1+(int)(-v/scr_graphheight->value));
		}
		h = (int)v % scr_graphheight->GetInt32();
		R_DrawFilled(x+w-1-a, y - h, 1, h, color);
	}
}

/*
===================================================================================================

	Printing / center printing

===================================================================================================
*/

static char		scr_centerstring[MAX_PRINT_MSG];
static float	scr_centertime_start;	// for slow victory printing
static float	scr_centertime_off;
static int		scr_center_lines;
static int		scr_erase_center;

/*
========================
SCR_DrawString
========================
*/
void SCR_DrawStringColor( int x, int y, const char *s, uint32 color )
{
	while ( *s ) {
		R_DrawCharColor( x, y, *s, color );
		x += CONCHAR_WIDTH;
		s++;
	}
}

/*
========================
SCR_DrawString
========================
*/
void SCR_DrawString( int x, int y, const char *s )
{
	SCR_DrawStringColor( x, y, s, colors::defaultText );
}

/*
========================
SCR_DrawString
========================
*/
void SCR_DrawAltString( int x, int y, const char *s )
{
	SCR_DrawStringColor( x, y, s, colors::green );
}

/*
========================
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
========================
*/
void SCR_CenterPrint( const char *str )
{
	const char	*s;
	char		line[64];
	int			i, j, l;

	Q_strcpy_s( scr_centerstring, str );
	scr_centertime_off = scr_centertime->value;
	scr_centertime_start = static_cast<float>( cl.time );

	// count the number of lines for centering
	scr_center_lines = 1;
	s = str;
	while ( *s )
	{
		if ( *s == '\n' ) {
			scr_center_lines++;
		}
		s++;
	}

	// echo it to the console
	Com_Print( "\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n" );

	s = str;
	while ( true )
	{
		// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (s[l] == '\n' || !s[l])
				break;
		for (i=0 ; i<(40-l)/2 ; i++)
			line[i] = ' ';

		for (j=0 ; j<l ; j++)
		{
			line[i++] = s[j];
		}

		line[i] = '\n';
		line[i+1] = 0;

		Com_Print (line);

		while (*s && *s != '\n')
			s++;

		if (!*s)
			break;
		s++;		// skip the \n
	}

	Com_Print( "\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n" );
	Con_ClearNotify();
}

static void SCR_DrawCenterString()
{
	char	*start;
	int		l;
	int		j;
	int		x, y;
	int		remaining;

	// the finale prints the characters one at a time
	remaining = 9999;

	scr_erase_center = 0;
	start = scr_centerstring;

	if ( scr_center_lines <= 4 ) {
		y = static_cast<int>( viddef.height * 0.35f );
	} else {
		y = 48;
	}

	while ( true )
	{
		// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (viddef.width - l*8)/2;

		for (j=0 ; j<l ; j++, x+=8)
		{
			R_DrawChar (x, y, start[j]);
			if (!remaining--)
				return;
		}
			
		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	}
}

static void SCR_CheckDrawCenterString()
{
	scr_centertime_off -= cls.frametime;

	if ( scr_centertime_off <= 0.0f ) {
		return;
	}

	SCR_DrawCenterString();
}

//=================================================================================================

/*
=================
SCR_CalcVrect

Sets scr_vrect, the coordinates of the rendered window
=================
*/
static void SCR_CalcVrect()
{
	scr_vrect.width = viddef.width;
	scr_vrect.height = viddef.height;

	scr_vrect.x = 0;
	scr_vrect.y = 0;
}

//=================================================================================================

/*
========================
SCR_Init
========================
*/
void SCR_Init()
{
	scr_viewsize = Cvar_Get( "viewsize", "100", CVAR_ARCHIVE );
	scr_conspeed = Cvar_Get( "scr_conspeed", "3", 0 );
	scr_showpause = Cvar_Get( "scr_showpause", "1", 0 );
	scr_centertime = Cvar_Get( "scr_centertime", "2.5", 0 );
	scr_printspeed = Cvar_Get( "scr_printspeed", "8", 0 );
	scr_netgraph = Cvar_Get( "netgraph", "0", 0 );
	scr_timegraph = Cvar_Get( "timegraph", "0", 0 );
	scr_debuggraph = Cvar_Get( "debuggraph", "0", 0 );
	scr_graphheight = Cvar_Get( "graphheight", "32", 0 );
	scr_graphscale = Cvar_Get( "graphscale", "1", 0 );
	scr_graphshift = Cvar_Get( "graphshift", "0", 0 );

	Cmd_AddCommand( "timerefresh", SCR_TimeRefresh_f );
	Cmd_AddCommand( "loading", SCR_Loading_f );

	scr_initialized = true;
}

/*
========================
SCR_DrawNet
========================
*/
static void SCR_DrawNet()
{
	if ( ( cls.netchan.outgoing_sequence - cls.netchan.incoming_acknowledged ) < CMD_BACKUP - 1 ) {
		return;
	}

	R_DrawPic( scr_vrect.x + 64, scr_vrect.y, "net" );
}

/*
========================
SCR_DrawPause
========================
*/
static void SCR_DrawPause()
{
	int w, h;

	if ( !cl_paused->GetBool() ) {
		return;
	}

	if ( !scr_showpause->GetBool() ) {
		// turn off for screenshots
		return;
	}

	R_DrawGetPicSize( &w, &h, "pause" );
	R_DrawPic( ( viddef.width - w ) / 2, viddef.height / 2 + 8, "pause" );
}

/*
========================
SCR_DrawLoading
========================
*/
static void SCR_DrawLoading()
{
	int w, h;

	if ( !scr_draw_loading ) {
		return;
	}
	scr_draw_loading = false;

	R_DrawGetPicSize( &w, &h, "loading" );
	R_DrawPic( ( viddef.width - w ) / 2, ( viddef.height - h ) / 2, "loading" );
}

/*
========================
SCR_DrawCrosshair
========================
*/
static void SCR_DrawCrosshair()
{
	if ( !cl_crosshair->GetBool() ) {
		return;
	}

	if ( cl_crosshair->IsModified() ) {
		cl_crosshair->ClearModified();
		SCR_TouchPics();
	}

	if ( !crosshair_pic[0] ) {
		return;
	}

	R_DrawPic( scr_vrect.x + ( ( scr_vrect.width - crosshair_width ) >> 1 )
		, scr_vrect.y + ( ( scr_vrect.height - crosshair_height ) >> 1 ), crosshair_pic );
}

//=================================================================================================

/*
========================
SCR_RunConsole

Scroll it up or down
========================
*/
void SCR_RunConsole()
{
	// decide on the height of the console
	if ( cls.key_dest == key_console ) {
		// half screen
		scr_conlines = 0.5f;
	} else {
		// none visible
		scr_conlines = 0.0f;
	}

	if ( scr_conlines < scr_con_current ) {
		scr_con_current -= scr_conspeed->GetFloat() * cls.frametime;
		if ( scr_conlines > scr_con_current ) {
			scr_con_current = scr_conlines;
		}
	} else if ( scr_conlines > scr_con_current ) {
		scr_con_current += scr_conspeed->GetFloat() * cls.frametime;
		if ( scr_conlines < scr_con_current ) {
			scr_con_current = scr_conlines;
		}
	}
}

/*
========================
SCR_DrawConsole
========================
*/
static void SCR_DrawConsole()
{
	Con_CheckResize();

	if ( cls.state == ca_disconnected || cls.state == ca_connecting ) {
		// forced full screen console
		Con_DrawConsole( 1.0f );
		return;
	}

	if ( cls.state != ca_active || !cl.refresh_prepped ) {
		// connected, but can't render
		Con_DrawConsole( 0.5f );
		return;
	}

	if ( scr_con_current ) {
		Con_DrawConsole( scr_con_current );
	} else {
		if ( cls.key_dest == key_game || cls.key_dest == key_message ) {
			// only draw notify in game
			Con_DrawNotify();
		}
	}
}

//=================================================================================================

/*
========================
SCR_BeginLoadingPlaque
========================
*/
void SCR_BeginLoadingPlaque()
{
	S_StopAllSounds();
	cl.sound_prepped = false;		// don't play ambients
	CDAudio_Stop();

	if ( cls.disable_screen ) {
		return;
	}
	if ( developer->GetBool() ) {
		return;
	}
	if ( cls.state == ca_disconnected ) {
		// if at console, don't bring up the plaque
		return;
	}
	if ( cls.key_dest == key_console ) {
		return;
	}

	if ( cl.cinematictime > 0 ) {
		// clear to black first
		scr_draw_loading = 2;
	} else {
		scr_draw_loading = 1;
	}

	SCR_UpdateScreen();

	cls.disable_screen = Time_FloatMilliseconds();
	cls.disable_servercount = cl.servercount;
}

/*
========================
SCR_EndLoadingPlaque
========================
*/
void SCR_EndLoadingPlaque()
{
	cls.disable_screen = 0.0f;
	Con_ClearNotify();
}

/*
========================
SCR_Loading_f
========================
*/
void SCR_Loading_f()
{
	SCR_BeginLoadingPlaque();
}

/*
========================
SCR_TimeRefresh_f
========================
*/
void SCR_TimeRefresh_f()
{
	int i;

	if ( cls.state != ca_active ) {
		return;
	}

	double start = Time_FloatSeconds();

	if ( Cmd_Argc() == 2 )
	{
		// run without page flipping
		R_BeginFrame();
		for ( i = 0; i < 128; i++ )
		{
			cl.refdef.viewangles[1] = i / 128.0f * 360.0f;
			R_RenderFrame( &cl.refdef );
		}
		R_EndFrame();
	}
	else
	{
		for ( i = 0; i < 128; i++ )
		{
			cl.refdef.viewangles[1] = i / 128.0f * 360.0f;

			R_BeginFrame();
			R_RenderFrame( &cl.refdef );
			R_EndFrame();
		}
	}

	double end = Time_FloatSeconds();
	double time = end - start;

	Com_Printf( "%g seconds (%g fps)\n", time, 128.0 / time );
}

//=================================================================================================

#define STAT_MINUS 10	// num frame for '-' stats digit
static const char *sb_nums[2][11]
{
	{"num_0", "num_1", "num_2", "num_3", "num_4", "num_5",
	"num_6", "num_7", "num_8", "num_9", "num_minus"},
	{"anum_0", "anum_1", "anum_2", "anum_3", "anum_4", "anum_5",
	"anum_6", "anum_7", "anum_8", "anum_9", "anum_minus"}
};

// GNU libc defines this in limits.h
#undef CHAR_WIDTH

#define	ICON_WIDTH	24
#define	ICON_HEIGHT	24
#define	CHAR_WIDTH	16
#define	ICON_SPACE	8

/*
========================
SizeHUDString

Allow embedded \n in the string
========================
*/
static void SizeHUDString( const char *string, int *w, int *h )
{
	int lines, width, current;

	lines = 1;
	width = 0;

	current = 0;
	while ( *string )
	{
		if ( *string == '\n' )
		{
			lines++;
			current = 0;
		}
		else
		{
			current++;
			if ( current > width )
				width = current;
		}
		++string;
	}

	*w = width * 8;
	*h = lines * 8;
}

/*
========================
DrawHUDString
========================
*/
static void DrawHUDString( const char *string, int x, int y, int centerwidth, uint32 color )
{
	int		margin;
	char	line[1024];
	int		width;
	int		i;

	margin = x;

	while ( *string )
	{
		// scan out one line of text from the string
		width = 0;
		while ( *string && *string != '\n' )
			line[width++] = *string++;
		line[width] = 0;

		if ( centerwidth )
			x = margin + ( centerwidth - width * 8 ) / 2;
		else
			x = margin;
		for ( i = 0; i < width; i++ )
		{
			R_DrawCharColor( x, y, line[i], color );
			x += 8;
		}
		if ( *string )
		{
			++string;	// skip the \n
			x = margin;
			y += 8;
		}
	}
}

/*
========================
SCR_DrawField
========================
*/
static void SCR_DrawField( int x, int y, int color, int width, int value )
{
	char	num[16], *ptr;
	int		l;
	int		frame;

	if ( width < 1 ) {
		return;
	}

	// draw number string
	if ( width > 5 ) {
		width = 5;
	}

	Q_sprintf_s( num, "%i", value );
	l = (int)strlen( num );
	if ( l > width ) {
		l = width;
	}
	x += 2 + CHAR_WIDTH * ( width - l );

	ptr = num;
	while ( *ptr && l )
	{
		if ( *ptr == '-' )
			frame = STAT_MINUS;
		else
			frame = *ptr - '0';

		R_DrawPic( x, y, sb_nums[color][frame] );
		x += CHAR_WIDTH;
		ptr++;
		l--;
	}
}

/*
========================
SCR_TouchPics

Allows rendering code to cache all needed sbar graphics
========================
*/
void SCR_TouchPics()
{
	int i, j;

	for ( i = 0; i < 2; i++ ) {
		for ( j = 0; j < 11; j++ ) {
			R_RegisterPic( sb_nums[i][j] );
		}
	}

	if ( cl_crosshair->GetBool() )
	{
		if ( cl_crosshair->GetInt32() > 3 || cl_crosshair->GetInt32() < 0 ) {
			cl_crosshair->value = 3;
		}

		Q_sprintf_s( crosshair_pic, "ch%i", cl_crosshair->GetInt32() );
		R_DrawGetPicSize( &crosshair_width, &crosshair_height, crosshair_pic );
		if ( !crosshair_width ) {
			crosshair_pic[0] = '\0';
		}
	}
}

/*
========================
SCR_ExecuteLayoutString 
========================
*/
static void SCR_ExecuteLayoutString( char *s )
{
	int		x, y;
	int		value;
	char	*token;
	int		width;
	int		index;
	clientinfo_t	*ci;

	if ( cls.state != ca_active || !cl.refresh_prepped ) {
		return;
	}

	if ( !s[0] ) {
		return;
	}

	x = 0;
	y = 0;
	width = 3;

	while (s)
	{
		token = COM_Parse (&s);
		if (!Q_strcmp(token, "xl"))
		{
			token = COM_Parse (&s);
			x = atoi(token);
			continue;
		}
		if (!Q_strcmp(token, "xr"))
		{
			token = COM_Parse (&s);
			x = viddef.width + atoi(token);
			continue;
		}
		if (!Q_strcmp(token, "xv"))
		{
			token = COM_Parse (&s);
			x = viddef.width/2 - 160 + atoi(token);
			continue;
		}

		if (!Q_strcmp(token, "yt"))
		{
			token = COM_Parse (&s);
			y = atoi(token);
			continue;
		}
		if (!Q_strcmp(token, "yb"))
		{
			token = COM_Parse (&s);
			y = viddef.height + atoi(token);
			continue;
		}
		if (!Q_strcmp(token, "yv"))
		{
			token = COM_Parse (&s);
			y = viddef.height/2 - 120 + atoi(token);
			continue;
		}

		if (!Q_strcmp(token, "pic"))
		{	// draw a pic from a stat number
			token = COM_Parse (&s);
			value = cl.frame.playerstate.stats[atoi(token)];
			if (value >= MAX_IMAGES)
				Com_Errorf ("Pic >= MAX_IMAGES");
			if (cl.configstrings[CS_IMAGES+value])
			{
				R_DrawPic (x, y, cl.configstrings[CS_IMAGES+value]);
			}
			continue;
		}

		if (!Q_strcmp(token, "client"))
		{	// draw a deathmatch client block
			int		score, ping, time;

			token = COM_Parse (&s);
			x = viddef.width/2 - 160 + atoi(token);
			token = COM_Parse (&s);
			y = viddef.height/2 - 120 + atoi(token);

			token = COM_Parse (&s);
			value = atoi(token);
			if (value >= MAX_CLIENTS || value < 0)
				Com_Errorf ("client >= MAX_CLIENTS");
			ci = &cl.clientinfo[value];

			token = COM_Parse (&s);
			score = atoi(token);

			token = COM_Parse (&s);
			ping = atoi(token);

			token = COM_Parse (&s);
			time = atoi(token);

			SCR_DrawAltString (x+32, y, ci->name);
			SCR_DrawString (x+32, y+8,  "Score: ");
			SCR_DrawAltString (x+32+7*8, y+8,  va("%i", score));
			SCR_DrawString (x+32, y+16, va("Ping:  %i", ping));
			SCR_DrawString (x+32, y+24, va("Time:  %i", time));

			if (!ci->icon)
				ci = &cl.baseclientinfo;
			R_DrawPic (x, y, ci->iconname);
			continue;
		}

		if (!Q_strcmp(token, "ctf"))
		{	// draw a ctf client block
			int		score, ping;
			char	block[80];

			token = COM_Parse (&s);
			x = viddef.width/2 - 160 + atoi(token);
			token = COM_Parse (&s);
			y = viddef.height/2 - 120 + atoi(token);

			token = COM_Parse (&s);
			value = atoi(token);
			if (value >= MAX_CLIENTS || value < 0)
				Com_Errorf ("client >= MAX_CLIENTS");
			ci = &cl.clientinfo[value];

			token = COM_Parse (&s);
			score = atoi(token);

			token = COM_Parse (&s);
			ping = atoi(token);
			if (ping > 999)
				ping = 999;

			Q_sprintf_s(block, "%3d %3d %-12.12s", score, ping, ci->name);

			if (value == cl.playernum)
				SCR_DrawAltString (x, y, block);
			else
				SCR_DrawString (x, y, block);
			continue;
		}

		if (!Q_strcmp(token, "picn"))
		{	// draw a pic from a name
			token = COM_Parse (&s);
			R_DrawPic (x, y, token);
			continue;
		}

		if (!Q_strcmp(token, "num"))
		{	// draw a number
			token = COM_Parse (&s);
			width = atoi(token);
			token = COM_Parse (&s);
			value = cl.frame.playerstate.stats[atoi(token)];
			SCR_DrawField (x, y, 0, width, value);
			continue;
		}

		if (!Q_strcmp(token, "hnum"))
		{	// health number
			int		color;

			width = 3;
			value = cl.frame.playerstate.stats[STAT_HEALTH];
			if (value > 25)
				color = 0;	// green
			else if (value > 0)
				color = (cl.frame.serverframe>>2) & 1;		// flash
			else
				color = 1;

			if (cl.frame.playerstate.stats[STAT_FLASHES] & 1)
				R_DrawPic (x, y, "field_3");

			SCR_DrawField (x, y, color, width, value);
			continue;
		}

		if (!Q_strcmp(token, "anum"))
		{	// ammo number
			int		color;

			width = 3;
			value = cl.frame.playerstate.stats[STAT_AMMO];
			if (value > 5)
				color = 0;	// green
			else if (value >= 0)
				color = (cl.frame.serverframe>>2) & 1;		// flash
			else
				continue;	// negative number = don't show

			if (cl.frame.playerstate.stats[STAT_FLASHES] & 4)
				R_DrawPic (x, y, "field_3");

			SCR_DrawField (x, y, color, width, value);
			continue;
		}

		if (!Q_strcmp(token, "rnum"))
		{	// armor number
			int		color;

			width = 3;
			value = cl.frame.playerstate.stats[STAT_ARMOR];
			if (value < 1)
				continue;

			color = 0;	// green

			if (cl.frame.playerstate.stats[STAT_FLASHES] & 2)
				R_DrawPic (x, y, "field_3");

			SCR_DrawField (x, y, color, width, value);
			continue;
		}


		if (!Q_strcmp(token, "stat_string"))
		{
			token = COM_Parse (&s);
			index = atoi(token);
			if (index < 0 || index >= MAX_CONFIGSTRINGS)
				Com_Errorf ("Bad stat_string index");
			index = cl.frame.playerstate.stats[index];
			if (index < 0 || index >= MAX_CONFIGSTRINGS)
				Com_Errorf ("Bad stat_string index");
			SCR_DrawString (x, y, cl.configstrings[index]);
			continue;
		}

		if (!Q_strcmp(token, "cstring"))
		{
			token = COM_Parse (&s);
			DrawHUDString (token, x, y, 320, colors::defaultText);
			continue;
		}

		if (!Q_strcmp(token, "string"))
		{
			token = COM_Parse (&s);
			SCR_DrawString (x, y, token);
			continue;
		}

		if (!Q_strcmp(token, "cstring2"))
		{
			token = COM_Parse (&s);
			DrawHUDString (token, x, y, 320, colors::green);
			continue;
		}

		if (!Q_strcmp(token, "string2"))
		{
			token = COM_Parse (&s);
			SCR_DrawAltString (x, y, token);
			continue;
		}

		if (!Q_strcmp(token, "if"))
		{	// draw a number
			token = COM_Parse (&s);
			value = cl.frame.playerstate.stats[atoi(token)];
			if (!value)
			{	// skip to endif
				while (s && Q_strcmp(token, "endif") )
				{
					token = COM_Parse (&s);
				}
			}
			continue;
		}
	}
}

/*
========================
SCR_DrawStats

The status bar is a small layout program that
is based on the stats array
========================
*/
static void SCR_DrawStats()
{
	SCR_ExecuteLayoutString( cl.configstrings[CS_STATUSBAR] );
}

/*
========================
SCR_DrawLayout
========================
*/
#define	STAT_LAYOUTS		13

static void SCR_DrawLayout()
{
	if ( !cl.frame.playerstate.stats[STAT_LAYOUTS] ) {
		return;
	}
	SCR_ExecuteLayoutString( cl.layout );
}

//=================================================================================================

/*
========================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
========================
*/
void SCR_UpdateScreen()
{
	// if the screen is disabled (loading plaque is up, or vid mode changing)
	// do nothing at all
	if ( cls.disable_screen )
	{
		if ( Time_FloatMilliseconds() - cls.disable_screen > 120000.0f ) {
			cls.disable_screen = 0.0f;
			Com_Print( "Loading plaque timed out.\n" );
		}
		return;
	}

	if ( !scr_initialized || !Con_IsInitialized() ) {
		// not initialized yet
		return;
	}

	R_BeginFrame();

	if ( scr_draw_loading == 2 )
	{
		// loading plaque over black screen
		int w, h;

		scr_draw_loading = false;

		R_DrawGetPicSize( &w, &h, "loading" );
		R_DrawPic( ( viddef.width - w ) / 2, ( viddef.height - h ) / 2, "loading" );
	}
	else if ( cl.cinematictime > 0 )
	{
		// if a cinematic is supposed to be running, handle menus and console specially

		if ( cls.key_dest == key_menu )
		{
			M_Draw();
		}
		else if ( cls.key_dest == key_console )
		{
			SCR_DrawConsole();
		}
		else
		{
			SCR_DrawCinematic();
		}
	}
	else
	{
		SCR_CalcVrect();

		// draw the game view

		V_RenderView();

		// draw 2d elements on top

		SCR_DrawCrosshair();

		SCR_DrawStats();
		if ( cl.frame.playerstate.stats[STAT_LAYOUTS] & 1 ) {
			SCR_DrawLayout();
		}
		if ( cl.frame.playerstate.stats[STAT_LAYOUTS] & 2 ) {
			CL_DrawInventory();
		}

		SCR_DrawNet();
		SCR_CheckDrawCenterString();

		if ( scr_timegraph->value ) {
			SCR_DebugGraph( cls.frametime * 300, colors::black );
		}

		if ( scr_debuggraph->value || scr_timegraph->value || scr_netgraph->value ) {
			SCR_DrawDebugGraph();
		}

		SCR_DrawPause();

		SCR_DrawConsole();

		M_Draw();

		SCR_DrawLoading();
	}

	R_EndFrame();
}
