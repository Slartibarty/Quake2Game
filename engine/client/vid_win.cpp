//=================================================================================================
// Main windowed and fullscreen graphics interface module. This module
// is used for both the software and OpenGL rendering versions of the
// Quake refresh engine.
//=================================================================================================

#include "client.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include "winquake.h"

#include <vector>

// Console variables that we need to access from this module
cvar_t		*vid_gamma;
cvar_t		*vid_ref;			// Name of Refresh DLL loaded
cvar_t		*vid_fullscreen;

// Global variables used internally by this module
viddef_t	viddef;				// global video state; used by other modules
bool		reflib_active;

//=================================================================================================

//-------------------------------------------------------------------------------------------------
// Console command to re-start the video mode and refresh DLL. We do this
// simply by setting the modified flag for the vid_ref variable, which will
// cause the entire video mode and refresh DLL to be reset on the next frame.
//-------------------------------------------------------------------------------------------------
static void VID_Restart_f()
{
	vid_ref->modified = true;
}

//=================================================================================================
// Vidmodes
//=================================================================================================

struct alignas( 16 ) vidmode_t
{
	/*char	description[32];*/
	int		width, height, mode;
};

// allocate space for 32 modes by default, 64, 128, etc
static constexpr size_t DefaultNumModes = 32;

std::vector<vidmode_t> s_vidModes;

static void VID_InitModes()
{
	DEVMODEW dm;
	dm.dmSize = sizeof( dm );
	dm.dmDriverExtra = 0;

	DWORD lastWidth = 0, lastHeight = 0;

	int internalNum = 0;				// for EnumDisplaySettings
	int numModes = 0;					// same as s_num_modes

	s_vidModes.reserve( DefaultNumModes );

	while ( EnumDisplaySettingsW( nullptr, internalNum, &dm ) != FALSE )
	{
		if ( dm.dmPelsWidth == lastWidth && dm.dmPelsHeight == lastHeight )
		{
			++internalNum;
			continue;
		}

		lastWidth = dm.dmPelsWidth;
		lastHeight = dm.dmPelsHeight;

		vidmode_t &mode = s_vidModes.emplace_back();

		/*Q_sprintf_s( mode.description, "Mode %d:\t%dx%d", numModes, dm.dmPelsWidth, dm.dmPelsHeight );*/
		mode.width = static_cast<int>( dm.dmPelsWidth );
		mode.height = static_cast<int>( dm.dmPelsHeight );
		mode.mode = numModes;

		++internalNum;
		++numModes;
	}

#if 0
	qsort( s_vidModes.data(), s_vidModes.size(), sizeof( vidmode_t ), []( const void *p1, const void *p2 )
		{
			const vidmode_t *m1 = reinterpret_cast<const vidmode_t *>( p1 );
			const vidmode_t *m2 = reinterpret_cast<const vidmode_t *>( p2 );

			if ( m1-> )

			return 1;
		} );
#endif
}

static void VID_FreeModes()
{
	s_vidModes.clear();
	s_vidModes.shrink_to_fit();
}

int VID_GetNumModes()
{
	return static_cast<int>( s_vidModes.size() );
}

bool VID_GetModeInfo( int &width, int &height, int mode )
{
	if ( mode < 0 || mode >= VID_GetNumModes() ) {
		return false;
	}

	width = s_vidModes[mode].width;
	height = s_vidModes[mode].height;

	return true;
}

//=================================================================================================

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void VID_NewWindow ( int width, int height)
{
	viddef.width  = width;
	viddef.height = height;

	cl.force_refdef = true;		// can't use a paused refdef
}

/*
========================
VID_LoadRefresh
========================
*/
static bool VID_LoadRefresh()
{
	if ( reflib_active ) {
		R_Shutdown();
	}

	extern LRESULT CALLBACK MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	if ( R_Init( g_hInstance, MainWndProc ) == false ) {
		R_Shutdown();
		return false;
	}

	reflib_active = true;

	return true;
}

/*
========================
VID_CheckChanges

This function gets called once just before drawing each frame, and it's sole purpose in life
is to check to see if any of the video mode parameters have changed, and if they have to 
update the rendering DLL and/or video mode to match.
========================
*/
void VID_CheckChanges()
{
	if ( vid_ref->IsModified() )
	{
		cl.force_refdef = true;		// can't use a paused refdef
		S_StopAllSounds();
	}
	while ( vid_ref->IsModified() )
	{
		vid_ref->modified = false;
		vid_fullscreen->modified = true;
		cl.refresh_prepped = false;

		cls.disable_screen = true;

		if ( !VID_LoadRefresh() )
		{
			Com_FatalError( "Couldn't load renderer!" );
		}

		cls.disable_screen = false;
	}
}

/*
========================
VID_Init
========================
*/
void VID_Init()
{
	vid_ref = Cvar_Get( "vid_ref", "gl", CVAR_ARCHIVE );
	vid_fullscreen = Cvar_Get( "vid_fullscreen", "0", CVAR_ARCHIVE );
	vid_gamma = Cvar_Get( "vid_gamma", "1", CVAR_ARCHIVE );

	Cmd_AddCommand( "vid_restart", VID_Restart_f );

	VID_InitModes();

	VID_CheckChanges();
}

/*
========================
VID_Shutdown
========================
*/
void VID_Shutdown()
{
	VID_FreeModes();

	if ( reflib_active )
	{
		R_Shutdown();
		reflib_active = false;
	}
}
