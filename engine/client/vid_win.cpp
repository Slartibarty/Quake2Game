// Main windowed and fullscreen graphics interface module. This module
// is used for both the software and OpenGL rendering versions of the
// Quake refresh engine.

#include "client.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "winquake.h"

// Console variables that we need to access from this module
cvar_t		*vid_gamma;
cvar_t		*vid_ref;			// Name of Refresh DLL loaded
cvar_t		*vid_fullscreen;

// Global variables used internally by this module
viddef_t	viddef;				// global video state; used by other modules
HINSTANCE	reflib_library;		// Handle to refresh DLL 
bool		reflib_active;

HWND        cl_hwnd;            // Main window handle for life of program

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
			s_vid_modes = (vidmode_t*)Z_Realloc(s_vid_modes, DefaultAlloc * allocMultiplier);
			lastAlloc = DefaultNumModes * allocMultiplier;
			allocMultiplier += 2;
		}

		vidmode_t &mode = s_vid_modes[numModes];

		Q_sprintf_s(mode.description, "Mode %d:\t%dx%d", numModes, dm.dmPelsWidth, dm.dmPelsHeight);
		mode.width = dm.dmPelsWidth;
		mode.height = dm.dmPelsHeight;
		mode.mode = numModes;

		++numModes;
		++internalNum;
	}

	s_num_modes = numModes;
}

static void VID_FreeModes()
{
	Z_Free(s_vid_modes);
	s_vid_modes = nullptr;
	s_num_modes = 0;
}

bool VID_GetModeInfo( int *width, int *height, int mode )
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
void VID_NewWindow ( int width, int height)
{
	viddef.width  = width;
	viddef.height = height;

	cl.force_refdef = true;		// can't use a paused refdef
}

static bool VID_LoadRefresh()
{
	if ( reflib_active )
	{
		R_Shutdown();
	}

	Com_Printf( "------- Loading renderer -------\n" );

	extern LRESULT CALLBACK MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	if ( R_Init( g_hInstance, MainWndProc ) == false )
	{
		R_Shutdown();
		return false;
	}

	Com_Printf( "------------------------------------\n" );
	reflib_active = true;

	vidref_val = VIDREF_GL; // PGM

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
void VID_CheckChanges( void )
{
	if ( vid_ref->modified )
	{
		cl.force_refdef = true;		// can't use a paused refdef
		S_StopAllSounds();
	}
	while ( vid_ref->modified )
	{
		/*
		** refresh has changed
		*/
		vid_ref->modified = false;
		vid_fullscreen->modified = true;
		cl.refresh_prepped = false;
		cls.disable_screen = true;

		if ( !VID_LoadRefresh() )
		{
			Com_Error( ERR_FATAL, "Couldn't load renderer!" );
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
	// vid_menu
	VID_FreeModes();
	if ( reflib_active )
	{
		R_Shutdown ();
		reflib_active = false;
	}
}


