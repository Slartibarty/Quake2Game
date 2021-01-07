// Main windowed and fullscreen graphics interface module. This module
// is used for both the software and OpenGL rendering versions of the
// Quake refresh engine.

#include "client.h"
#include "winquake.h"

// Structure containing functions exported from refresh DLL
refexport_t	re;

// Console variables that we need to access from this module
cvar_t		*vid_gamma;
cvar_t		*vid_ref;			// Name of Refresh DLL loaded
cvar_t		*vid_fullscreen;

// Global variables used internally by this module
viddef_t	viddef;				// global video state; used by other modules
HINSTANCE	reflib_library;		// Handle to refresh DLL 
bool		reflib_active;

HWND        cl_hwnd;            // Main window handle for life of program

extern	unsigned	sys_msg_time;

/*
==========================================================================

DLL GLUE

==========================================================================
*/

static void VID_Printf (int print_level, const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];
	
	va_start (argptr,fmt);
	Com_vsprintf (msg,fmt,argptr);
	va_end (argptr);

	if (print_level == PRINT_ALL)
	{
		Com_Printf ("%s", msg);
	}
	else // if ( print_level == PRINT_DEVELOPER )
	{
		Com_DPrintf ("%s", msg);
	}
}

static void VID_Error (int err_level, const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAX_PRINT_MSG];
	
	va_start (argptr,fmt);
	Com_vsprintf (msg,fmt,argptr);
	va_end (argptr);

	Com_Error (err_level,"%s", msg);
}

//==========================================================================

/*
============
VID_Restart_f

Console command to re-start the video mode and refresh DLL. We do this
simply by setting the modified flag for the vid_ref variable, which will
cause the entire video mode and refresh DLL to be reset on the next frame.
============
*/
static void VID_Restart_f (void)
{
	vid_ref->modified = true;
}

static void VID_Front_f( void )
{
	SetWindowLongW( cl_hwnd, GWL_EXSTYLE, WS_EX_TOPMOST );
	SetForegroundWindow( cl_hwnd );
}

/*
** VID_GetModeInfo
*/
struct vidmode_t
{
	char	description[32];
	int		width, height, mode;
};

static vidmode_t	*s_vid_modes;
static int			s_num_modes;

// Allocate space for 16 modes by default, 32, 64, etc
static constexpr int DefaultNumModes = 16;
static constexpr int DefaultAlloc = DefaultNumModes * sizeof(vidmode_t);

static void VID_InitModes()
{
	DEVMODEW dm;
	dm.dmSize = sizeof(dm);
	dm.dmDriverExtra = 0;

	DWORD lastWidth = 0, lastHeight = 0;

	int allocMultiplier = 2;			// +2 each time we realloc
	int internalNum = 0;				// For EnumDisplaySettings
	int numModes = 0;					// Same as s_num_modes
	int lastAlloc = DefaultNumModes;	// The last count we allocated

	s_vid_modes = (vidmode_t*)Z_Malloc(DefaultAlloc);

	while (EnumDisplaySettingsW(nullptr, internalNum, &dm) != FALSE)
	{
		if (dm.dmPelsWidth == lastWidth && dm.dmPelsHeight == lastHeight)
		{
			++internalNum;
			continue;
		}

		lastWidth = dm.dmPelsWidth;
		lastHeight = dm.dmPelsHeight;

		if (numModes+1 > lastAlloc)
		{
			s_vid_modes = (vidmode_t*)realloc(s_vid_modes, DefaultAlloc * allocMultiplier);
			lastAlloc = DefaultNumModes * allocMultiplier;
			allocMultiplier += 2;
		}

		vidmode_t &mode = s_vid_modes[numModes];

		Com_sprintf(mode.description, "Mode %d:\t%dx%d", numModes, dm.dmPelsWidth, dm.dmPelsHeight);
		mode.width = dm.dmPelsWidth;
		mode.height = dm.dmPelsHeight;
		mode.mode = numModes;

		++numModes;
		++internalNum;
	}

	s_num_modes = numModes;
}

static void VID_DeleteModes()
{
	Z_Free(s_vid_modes);
	s_vid_modes = nullptr;
	s_num_modes = 0;
}

qboolean VID_GetModeInfo( int *width, int *height, int mode )
{
	if ( mode < 0 || mode >= s_num_modes)
		return false;

	*width  = s_vid_modes[mode].width;
	*height = s_vid_modes[mode].height;

	return true;
}

int VID_GetNumModes()
{
	return s_num_modes;
}

/*
** VID_NewWindow
*/
static void VID_NewWindow ( int width, int height)
{
	viddef.width  = width;
	viddef.height = height;

	cl.force_refdef = true;		// can't use a paused refdef
}

static void VID_FreeReflib (void)
{
	if ( !FreeLibrary( reflib_library ) )
		Com_Error( ERR_FATAL, "Reflib FreeLibrary failed" );
	memset (&re, 0, sizeof(re));
	reflib_library = NULL;
	reflib_active  = false;
}

/*
==============
VID_LoadRefresh
==============
*/
static qboolean VID_LoadRefresh( const char *name )
{
	refimport_t	ri;
	GetRefAPI_t	GetRefAPI;
	
	if ( reflib_active )
	{
		re.Shutdown();
		VID_FreeReflib ();
	}

	Com_Printf( "------- Loading %s -------\n", name );

	if ( ( reflib_library = LoadLibrary( name ) ) == 0 )
	{
		Com_Printf( "LoadLibrary(\"%s\") failed\n", name );

		return false;
	}

	ri.Cmd_AddCommand = Cmd_AddCommand;
	ri.Cmd_RemoveCommand = Cmd_RemoveCommand;
	ri.Cmd_Argc = Cmd_Argc;
	ri.Cmd_Argv = Cmd_Argv;
	ri.Cmd_ExecuteText = Cbuf_ExecuteText;
	ri.Con_Printf = VID_Printf;
	ri.Sys_Error = VID_Error;
	ri.FS_LoadFile = FS_LoadFile;
	ri.FS_FreeFile = FS_FreeFile;
	ri.FS_Gamedir = FS_Gamedir;
	ri.Cvar_Get = Cvar_Get;
	ri.Cvar_Set = Cvar_Set;
	ri.Cvar_SetValue = Cvar_SetValue;
	ri.Vid_GetModeInfo = VID_GetModeInfo;
	ri.Vid_NewWindow = VID_NewWindow;

	if ( ( GetRefAPI = (GetRefAPI_t) GetProcAddress( reflib_library, "GetRefAPI" ) ) == 0 )
		Com_Error( ERR_FATAL, "GetProcAddress failed on %s", name );

	re = GetRefAPI( ri );

	if (re.api_version != API_VERSION)
	{
		VID_FreeReflib ();
		Com_Error (ERR_FATAL, "%s has incompatible api_version", name);
	}

	extern LRESULT CALLBACK MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	if ( re.Init( g_hInstance, MainWndProc ) == -1 )
	{
		re.Shutdown();
		VID_FreeReflib ();
		return false;
	}

	Com_Printf( "------------------------------------\n");
	reflib_active = true;

//======
//PGM
	vidref_val = VIDREF_OTHER;
	if(vid_ref)
	{
		if(!strcmp (vid_ref->string, "gl"))
			vidref_val = VIDREF_GL;
		else if(!strcmp(vid_ref->string, "soft"))
			vidref_val = VIDREF_SOFT;
	}
//PGM
//======

	return true;
}

/*
============
VID_CheckChanges

This function gets called once just before drawing each frame, and it's sole purpose in life
is to check to see if any of the video mode parameters have changed, and if they have to 
update the rendering DLL and/or video mode to match.
============
*/
void VID_CheckChanges (void)
{
	char name[64];

	if ( vid_ref->modified )
	{
		cl.force_refdef = true;		// can't use a paused refdef
		S_StopAllSounds();
	}
	while (vid_ref->modified)
	{
		/*
		** refresh has changed
		*/
		vid_ref->modified = false;
		vid_fullscreen->modified = true;
		cl.refresh_prepped = false;
		cls.disable_screen = true;

		Com_sprintf( name, "ref_%s.dll", vid_ref->string );
		if ( !VID_LoadRefresh( name ) )
		{
			if ( strcmp (vid_ref->string, "soft") == 0 )
				Com_Error (ERR_FATAL, "Couldn't fall back to software refresh!");
			Cvar_Set( "vid_ref", "soft" );

			/*
			** drop the console if we fail to load a refresh
			*/
			if ( cls.key_dest != key_console )
			{
				Con_ToggleConsole_f();
			}
		}
		cls.disable_screen = false;
	}
}

/*
============
VID_Init
============
*/
void VID_Init (void)
{
	/* Create the video variables so we know how to start the graphics drivers */
	vid_ref = Cvar_Get ("vid_ref", "gl", CVAR_ARCHIVE);
	vid_fullscreen = Cvar_Get ("vid_fullscreen", "0", CVAR_ARCHIVE);
	vid_gamma = Cvar_Get( "vid_gamma", "1", CVAR_ARCHIVE );

	/* Add some console commands that we want to handle */
	Cmd_AddCommand ("vid_restart", VID_Restart_f);
	Cmd_AddCommand ("vid_front", VID_Front_f);

	VID_InitModes();
		
	/* Start the graphics mode and load refresh DLL */
	VID_CheckChanges();
}

/*
============
VID_Shutdown
============
*/
void VID_Shutdown (void)
{
	VID_DeleteModes();
	if ( reflib_active )
	{
		re.Shutdown ();
		VID_FreeReflib ();
	}
}


